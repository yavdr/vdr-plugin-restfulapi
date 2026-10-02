#include "recordingmoveexecutor.h"

#include "changestatetracker.h"
#include "tools.h"

#include <cstring>
#include <unistd.h>

#include <vdr/menu.h>
#include <vdr/recording.h>
#include <vdr/timers.h>
#include <vdr/videodir.h>

namespace {

bool pathExists(const std::string& path)
{
  return !path.empty() && access(path.c_str(), F_OK) == 0;
}

const int kMutationLockAttemptTimeoutMs = 100;
const int kMutationLockRetryDelayMs = 25;
const int kMutationLockAttempts = 20;

class RecordingMoveListLocks
{
public:
  RecordingMoveListLocks()
    : recordings(nullptr),
      recordingsModified(false)
  {
  }

  ~RecordingMoveListLocks()
  {
    release();
  }

  bool acquire()
  {
    for (int attempt = 0; attempt < kMutationLockAttempts; ++attempt) {
      if (acquireOnce())
        return true;

      if (attempt + 1 < kMutationLockAttempts)
        cCondWait::SleepMs(kMutationLockRetryDelayMs);
    }

    return false;
  }

  cRecordings* recordingsList() const
  {
    return recordings;
  }

  void markRecordingsModified()
  {
    recordingsModified = true;
  }

private:
  bool acquireOnce()
  {
    cTimers* timers = cTimers::GetTimersWrite(
      timersStateKey,
      kMutationLockAttemptTimeoutMs);

    if (!timers)
      return false;

    recordings = cRecordings::GetRecordingsWrite(
      recordingsStateKey,
      kMutationLockAttemptTimeoutMs);

    if (recordings)
      return true;

    timersStateKey.Remove(false);
    return false;
  }

  void release()
  {
    if (recordingsStateKey.InLock())
      recordingsStateKey.Remove(recordingsModified);

    if (timersStateKey.InLock())
      timersStateKey.Remove(false);

    recordings = nullptr;
  }

  cStateKey timersStateKey;
  cStateKey recordingsStateKey;
  cRecordings* recordings;
  bool recordingsModified;
};

}

RecordingMoveExecutor::RecordingMoveExecutor(const RecordingMoveExecutionGate& gate)
  : gate(gate)
{
}

RecordingMoveExecutorResult RecordingMoveExecutor::executeNormalCase(
  const std::string& recordingFile,
  const std::string& targetFile,
  const RecordingMutationRevision& expectedRevision,
  const RecordingMutationPolicy& policy) const
{
  RecordingMoveExecutorResult result;
  result.recordingFile = recordingFile;
  result.targetFile = targetFile;

  if (!pathExists(recordingFile) && pathExists(targetFile)) {
    result.status = RecordingMoveExecutorStatus::AlreadyMoved;
    result.message = "Recording is already present at the requested target.";
    return result;
  }

  result.gate = gate.validate(recordingFile, targetFile, expectedRevision, policy);

  if (result.gate.status == RecordingMoveExecutionGateStatus::Conflict) {
    result.status = RecordingMoveExecutorStatus::Conflict;
    result.message = "Recording state changed after preview.";
    return result;
  }

  if (result.gate.status != RecordingMoveExecutionGateStatus::Ready) {
    result.status = RecordingMoveExecutorStatus::Blocked;
    result.message = "Recording move execution is blocked by the current state or policy.";
    return result;
  }

  // VDR requires the global list lock order Timers -> Channels ->
  // Recordings -> Schedules. When more than one list is locked at the same
  // time, each lock attempt must be bounded so a contended later lock cannot
  // deadlock the earlier one indefinitely.
  RecordingMoveListLocks listLocks;
  if (!listLocks.acquire()) {
    result.status = RecordingMoveExecutorStatus::Conflict;
    result.message = "Recording state is busy; retry the move.";
    return result;
  }

  cRecordings* recordings = listLocks.recordingsList();
  cRecording* recording = recordings->GetByName(recordingFile.c_str());
  if (!recording) {
    if (!pathExists(recordingFile) && pathExists(targetFile)) {
      result.status = RecordingMoveExecutorStatus::AlreadyMoved;
      result.message = "Recording is already present at the requested target.";
    }
    else {
      result.status = RecordingMoveExecutorStatus::NotFound;
      result.message = "Recording disappeared before execution.";
    }
    return result;
  }

  if (pathExists(targetFile) || recordings->GetByName(targetFile.c_str())) {
    result.status = RecordingMoveExecutorStatus::Conflict;
    result.message = "The recording move target already exists.";
    return result;
  }

  const char* nowReplaying = cReplayControl::NowReplaying();
  if (nowReplaying && std::strcmp(nowReplaying, recordingFile.c_str()) == 0) {
    result.status = RecordingMoveExecutorStatus::Conflict;
    result.message = "Recording started replaying after validation.";
    return result;
  }

  if (RecordingsHandler.GetUsage(recordingFile.c_str()) != ruNone) {
    result.status = RecordingMoveExecutorStatus::Conflict;
    result.message = "Recording handler usage changed after validation.";
    return result;
  }

  if (cRecordControls::GetRecordControl(recordingFile.c_str()) != nullptr) {
    result.status = RecordingMoveExecutorStatus::Conflict;
    result.message = "A local recording control became active after validation.";
    return result;
  }

  const cString timerIdText = GetRecordingTimerId(recordingFile.c_str());
  const char* timerId = *timerIdText;
  if (timerId && *timerId) {
    result.status = RecordingMoveExecutorStatus::Conflict;
    result.message = "A timer association appeared after validation.";
    return result;
  }

  const std::string oldName = recording->FileName();
  if (!VdrExtension::MoveDirectory(oldName, targetFile, false)) {
    result.status = RecordingMoveExecutorStatus::Failed;
    result.message = "VDR failed to move the recording directory.";
    return result;
  }

  if (pathExists(oldName) || !pathExists(targetFile)) {
    result.status = RecordingMoveExecutorStatus::Failed;
    result.message = "Recording move postcondition verification failed.";
    return result;
  }

  listLocks.markRecordingsModified();
  recordings->Del(recording);
  recordings->AddByName(targetFile.c_str());

  const cRecording* movedRecording = recordings->GetByName(targetFile.c_str());
  if (!movedRecording) {
    result.status = RecordingMoveExecutorStatus::Failed;
    result.message = "Moved recording was not registered under its target identity.";
    return result;
  }

  cRecordingUserCommand::InvokeCommand(
    *cString::sprintf("rename \"%s\"", *strescape(oldName.c_str(), "\\\"$'")),
    targetFile.c_str());

  cVideoDiskUsage::ForceCheck();
  StateChangeTracker::UpdateRecordings();

  result.status = RecordingMoveExecutorStatus::Moved;
  result.message = "Recording moved to the requested target.";
  return result;
}

const char* RecordingMoveExecutorStatusName(RecordingMoveExecutorStatus status)
{
  switch (status) {
    case RecordingMoveExecutorStatus::Moved:
      return "moved";
    case RecordingMoveExecutorStatus::AlreadyMoved:
      return "already-moved";
    case RecordingMoveExecutorStatus::Conflict:
      return "conflict";
    case RecordingMoveExecutorStatus::Blocked:
      return "blocked";
    case RecordingMoveExecutorStatus::NotFound:
      return "not-found";
    case RecordingMoveExecutorStatus::Failed:
      return "failed";
  }

  return "failed";
}
