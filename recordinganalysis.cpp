#include "recordinganalysis.h"

#include <cstring>

#include <vdr/menu.h>
#include <vdr/recording.h>

RecordingLookupResult VdrRecordingLookup::find(const std::string& recordingFile) const
{
  RecordingLookupResult result;

  if (recordingFile.empty())
    return result;

  LOCK_RECORDINGS_READ;
  const cRecording* recording = Recordings->GetByName(recordingFile.c_str());

  if (!recording)
    return result;

  result.found = true;
  result.recordingFile = recording->FileName();
  return result;
}

bool VdrRecordingReplayLookup::isReplaying(const std::string& recordingFile) const
{
  if (recordingFile.empty())
    return false;

  const char* nowReplaying = cReplayControl::NowReplaying();
  return nowReplaying && std::strcmp(nowReplaying, recordingFile.c_str()) == 0;
}

RecordingHandlerLookupResult VdrRecordingHandlerLookup::getUsage(
  const std::string& recordingFile) const
{
  RecordingHandlerLookupResult result;

  if (recordingFile.empty())
    return result;

  result.known = true;
  result.busy = RecordingsHandler.GetUsage(recordingFile.c_str()) != ruNone;
  return result;
}

RecordingTrashAnalyzer::RecordingTrashAnalyzer(
  const IRecordingLookup& recordingLookup,
  const IRecordingReplayLookup& replayLookup,
  const IRecordingHandlerLookup& recordingHandlerLookup)
  : recordingLookup(recordingLookup),
    replayLookup(replayLookup),
    recordingHandlerLookup(recordingHandlerLookup)
{
}

RecordingMutationAnalysis RecordingTrashAnalyzer::analyze(
  const std::string& recordingFile) const
{
  RecordingMutationAnalysis analysis;
  analysis.type = RecordingMutationType::Trash;
  analysis.recordingFile = recordingFile;
  analysis.revision.recordingFile = recordingFile;

  const RecordingLookupResult recording = recordingLookup.find(recordingFile);

  if (!recording.found) {
    analysis.constraints.push_back(RecordingConstraint::RecordingMissing);
    return analysis;
  }

  analysis.recordingFile = recording.recordingFile;
  analysis.revision.recordingFile = recording.recordingFile;

  if (replayLookup.isReplaying(recording.recordingFile))
    analysis.constraints.push_back(RecordingConstraint::ReplayActive);

  const RecordingHandlerLookupResult handlerUsage =
    recordingHandlerLookup.getUsage(recording.recordingFile);

  if (!handlerUsage.known)
    analysis.constraints.push_back(RecordingConstraint::UnknownRecordingHandlerState);
  else if (handlerUsage.busy)
    analysis.constraints.push_back(RecordingConstraint::RecordingHandlerBusy);

  analysis.constraints.push_back(RecordingConstraint::UnknownTimerState);
  analysis.constraints.push_back(RecordingConstraint::UnknownSearchTimerState);

  return analysis;
}
