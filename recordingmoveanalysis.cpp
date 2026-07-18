#include "recordinganalysis.h"

#include <cstdint>
#include <sstream>
#include <sys/stat.h>

namespace {

long long fingerprint(const std::string& value)
{
  std::uint64_t hash = 1469598103934665603ULL;
  for (unsigned char character : value) {
    hash ^= character;
    hash *= 1099511628211ULL;
  }
  return static_cast<long long>(hash & 0x7FFFFFFFFFFFFFFFULL);
}

void appendFileState(
  std::ostringstream& state,
  const std::string& recordingFile,
  bool found)
{
  state << recordingFile << '|';
  state << (found ? "found" : "missing");

  struct stat fileState;
  if (found && stat(recordingFile.c_str(), &fileState) == 0) {
    state << '|' << static_cast<unsigned long long>(fileState.st_dev);
    state << '|' << static_cast<unsigned long long>(fileState.st_ino);
    state << '|' << static_cast<long long>(fileState.st_mtime);
    state << '|' << static_cast<long long>(fileState.st_ctime);
  }
  else if (found) {
    state << "|stat-unavailable";
  }
}

long long moveRecordingFingerprint(
  const std::string& recordingFile,
  bool recordingFound,
  const std::string& targetFile,
  bool targetFound)
{
  std::ostringstream state;
  state << "source=";
  appendFileState(state, recordingFile, recordingFound);
  state << "|target=";
  appendFileState(state, targetFile, targetFound);
  return fingerprint(state.str());
}

long long timerFingerprint(
  bool replaying,
  const RecordingHandlerLookupResult& handlerUsage,
  const RecordingLocalTimerLookupResult& localTimer,
  const RecordingRemoteTimerLookupResult& remoteTimer,
  const RecordingSearchTimerLookupResult& searchTimer)
{
  std::ostringstream state;
  state << "replay=" << replaying;
  state << "|handler-known=" << handlerUsage.known;
  state << "|handler-busy=" << handlerUsage.busy;
  state << "|local-known=" << localTimer.known;
  state << "|local-active=" << localTimer.active;
  state << "|remote-known=" << remoteTimer.known;
  state << "|remote-active=" << remoteTimer.active;
  state << "|remote-id=" << remoteTimer.timerId;
  state << "|remote=" << remoteTimer.remote;
  state << "|search-known=" << searchTimer.known;
  state << "|search-recording=" << searchTimer.searchTimerRecording;
  state << "|search-id=" << searchTimer.searchTimerId;
  return fingerprint(state.str());
}

bool isAbsoluteRecordingTarget(const std::string& targetFile)
{
  return targetFile.size() > 1 &&
    targetFile.front() == '/' &&
    targetFile.back() != '/';
}

}

RecordingMoveAnalyzer::RecordingMoveAnalyzer(
  const IRecordingLookup& recordingLookup,
  const IRecordingReplayLookup& replayLookup,
  const IRecordingHandlerLookup& recordingHandlerLookup,
  const IRecordingLocalTimerLookup& localTimerLookup,
  const IRecordingRemoteTimerLookup& remoteTimerLookup,
  const IRecordingSearchTimerLookup& searchTimerLookup)
  : recordingLookup(recordingLookup),
    replayLookup(replayLookup),
    recordingHandlerLookup(recordingHandlerLookup),
    localTimerLookup(localTimerLookup),
    remoteTimerLookup(remoteTimerLookup),
    searchTimerLookup(searchTimerLookup)
{
}

RecordingMutationAnalysis RecordingMoveAnalyzer::analyze(
  const std::string& recordingFile,
  const std::string& targetFile) const
{
  RecordingMutationAnalysis analysis;
  analysis.type = RecordingMutationType::Move;
  analysis.recordingFile = recordingFile;
  analysis.targetFile = targetFile;
  analysis.revision.recordingFile = recordingFile;
  analysis.revision.targetFile = targetFile;

  const RecordingLookupResult recording = recordingLookup.find(recordingFile);
  const RecordingLookupResult target = targetFile.empty()
    ? RecordingLookupResult()
    : recordingLookup.find(targetFile);

  analysis.revision.recordingsState = moveRecordingFingerprint(
    recordingFile,
    recording.found,
    targetFile,
    target.found);

  if (!recording.found)
    analysis.constraints.push_back(RecordingConstraint::RecordingMissing);

  if (targetFile.empty())
    analysis.constraints.push_back(RecordingConstraint::MoveTargetMissing);
  else if (!isAbsoluteRecordingTarget(targetFile))
    analysis.constraints.push_back(RecordingConstraint::MoveTargetInvalid);

  if (!recordingFile.empty() && recordingFile == targetFile)
    analysis.constraints.push_back(RecordingConstraint::MoveTargetSameAsSource);

  if (target.found && target.recordingFile != recording.recordingFile)
    analysis.constraints.push_back(RecordingConstraint::MoveTargetExists);

  if (!recording.found)
    return analysis;

  analysis.recordingFile = recording.recordingFile;
  analysis.revision.recordingFile = recording.recordingFile;

  const bool replaying = replayLookup.isReplaying(recording.recordingFile);
  if (replaying)
    analysis.constraints.push_back(RecordingConstraint::ReplayActive);

  const RecordingHandlerLookupResult handlerUsage =
    recordingHandlerLookup.getUsage(recording.recordingFile);
  if (!handlerUsage.known)
    analysis.constraints.push_back(RecordingConstraint::UnknownRecordingHandlerState);
  else if (handlerUsage.busy)
    analysis.constraints.push_back(RecordingConstraint::RecordingHandlerBusy);

  const RecordingLocalTimerLookupResult localTimer =
    localTimerLookup.findActive(recording.recordingFile);
  if (!localTimer.known)
    analysis.constraints.push_back(RecordingConstraint::UnknownLocalTimerState);
  else if (localTimer.active)
    analysis.constraints.push_back(RecordingConstraint::LocalTimerActive);

  const RecordingRemoteTimerLookupResult remoteTimer =
    remoteTimerLookup.findActive(recording.recordingFile);
  if (!remoteTimer.known)
    analysis.constraints.push_back(RecordingConstraint::UnknownRemoteTimerState);
  else if (remoteTimer.active)
    analysis.constraints.push_back(RecordingConstraint::RemoteTimerActive);

  const RecordingSearchTimerLookupResult searchTimer =
    searchTimerLookup.findOrigin(recording.recordingFile);
  if (!searchTimer.known)
    analysis.constraints.push_back(RecordingConstraint::UnknownSearchTimerState);
  else if (searchTimer.searchTimerRecording)
    analysis.constraints.push_back(RecordingConstraint::SearchTimerRecording);

  analysis.revision.recordingsState = moveRecordingFingerprint(
    recording.recordingFile,
    true,
    targetFile,
    target.found);
  analysis.revision.timersState = timerFingerprint(
    replaying,
    handlerUsage,
    localTimer,
    remoteTimer,
    searchTimer);

  return analysis;
}
