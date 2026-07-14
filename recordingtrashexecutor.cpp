#include "recordingtrashexecutor.h"

#include "changestatetracker.h"

#include <cstring>

#include <vdr/menu.h>
#include <vdr/recording.h>
#include <vdr/videodir.h>

namespace {

std::string deletedFileName(const std::string& recordingFile)
{
  static const std::string recordingSuffix = ".rec";
  if (recordingFile.size() >= recordingSuffix.size() &&
      recordingFile.compare(
        recordingFile.size() - recordingSuffix.size(),
        recordingSuffix.size(),
        recordingSuffix) == 0)
    return recordingFile.substr(0, recordingFile.size() - recordingSuffix.size()) + ".del";

  return std::string();
}

}

RecordingTrashExecutor::RecordingTrashExecutor(const RecordingTrashExecutionGate& gate)
  : gate(gate)
{
}

RecordingTrashExecutorResult RecordingTrashExecutor::executeNormalCase(
  const std::string& recordingFile,
  const RecordingMutationRevision& expectedRevision,
  const RecordingMutationPolicy& policy) const
{
  RecordingTrashExecutorResult result;
  result.recordingFile = recordingFile;
  result.gate = gate.validate(recordingFile, expectedRevision, policy);

  if (result.gate.status == RecordingTrashExecutionGateStatus::Conflict) {
    result.status = RecordingTrashExecutorStatus::Conflict;
    result.message = "Recording state changed after preview.";
    return result;
  }

  if (result.gate.status != RecordingTrashExecutionGateStatus::Ready) {
    result.status = RecordingTrashExecutorStatus::Blocked;
    result.message = "Recording trash execution is blocked by the current state or policy.";
    return result;
  }

  const std::string targetFile = deletedFileName(recordingFile);
  if (targetFile.empty()) {
    result.status = RecordingTrashExecutorStatus::Blocked;
    result.message = "Only active .rec recordings can be moved to the VDR trash.";
    return result;
  }

  LOCK_TIMERS_WRITE;
  LOCK_RECORDINGS_WRITE;

  cRecording* recording = Recordings->GetByName(recordingFile.c_str());
  if (!recording) {
    result.status = RecordingTrashExecutorStatus::NotFound;
    result.message = "Recording disappeared before execution.";
    return result;
  }

  const char* nowReplaying = cReplayControl::NowReplaying();
  if (nowReplaying && std::strcmp(nowReplaying, recordingFile.c_str()) == 0) {
    result.status = RecordingTrashExecutorStatus::Conflict;
    result.message = "Recording started replaying after validation.";
    return result;
  }

  if (RecordingsHandler.GetUsage(recordingFile.c_str()) != ruNone) {
    result.status = RecordingTrashExecutorStatus::Conflict;
    result.message = "Recording handler usage changed after validation.";
    return result;
  }

  if (cRecordControls::GetRecordControl(recordingFile.c_str()) != nullptr) {
    result.status = RecordingTrashExecutorStatus::Conflict;
    result.message = "A local recording control became active after validation.";
    return result;
  }

  const cString timerIdText = GetRecordingTimerId(recordingFile.c_str());
  const char* timerId = *timerIdText;
  if (timerId && *timerId) {
    result.status = RecordingTrashExecutorStatus::Conflict;
    result.message = "A timer association appeared after validation.";
    return result;
  }

  if (!recording->Delete()) {
    result.status = RecordingTrashExecutorStatus::Failed;
    result.message = "VDR failed to rename the recording into its deleted state.";
    return result;
  }

  {
    LOCK_DELETEDRECORDINGS_WRITE;
    Recordings->Del(recording, false);
    DeletedRecordings->Add(recording);
  }

  cVideoDiskUsage::ForceCheck();
  StateChangeTracker::UpdateRecordings();

  result.status = RecordingTrashExecutorStatus::Trashed;
  result.deletedRecordingFile = targetFile;
  result.message = "Recording moved to the VDR trash.";
  return result;
}

const char* RecordingTrashExecutorStatusName(RecordingTrashExecutorStatus status)
{
  switch (status) {
    case RecordingTrashExecutorStatus::Trashed:
      return "trashed";
    case RecordingTrashExecutorStatus::Conflict:
      return "conflict";
    case RecordingTrashExecutorStatus::Blocked:
      return "blocked";
    case RecordingTrashExecutorStatus::NotFound:
      return "not-found";
    case RecordingTrashExecutorStatus::Failed:
      return "failed";
  }

  return "failed";
}
