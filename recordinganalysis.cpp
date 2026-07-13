#include "recordinganalysis.h"

#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>
#include <sys/stat.h>

#include <vdr/menu.h>
#include <vdr/recording.h>

namespace {

bool parsePositiveInteger(const std::string& value, int& result)
{
  try {
    std::size_t parsed = 0;
    const long number = std::stol(value, &parsed, 10);
    if (parsed != value.size() || number < 0 || number > std::numeric_limits<int>::max())
      return false;
    result = static_cast<int>(number);
    return true;
  }
  catch (...) {
    return false;
  }
}

bool parseSearchTimerId(const char* aux, bool& present, int& searchTimerId)
{
  present = false;
  searchTimerId = -1;

  if (!aux || !*aux)
    return true;

  const std::string value(aux);
  const std::string openTag = "<s-id>";
  const std::string closeTag = "</s-id>";
  const std::string::size_type begin = value.find(openTag);

  if (begin == std::string::npos)
    return true;

  const std::string::size_type contentBegin = begin + openTag.size();
  const std::string::size_type end = value.find(closeTag, contentBegin);
  if (end == std::string::npos || end == contentBegin)
    return false;

  const std::string idText = value.substr(contentBegin, end - contentBegin);
  if (!parsePositiveInteger(idText, searchTimerId))
    return false;

  present = true;
  return true;
}

bool parseRemoteTimerId(
  const std::string& value,
  int& timerId,
  std::string& remote)
{
  const std::string::size_type separator = value.find('@');
  if (separator == std::string::npos || separator == 0 || separator + 1 >= value.size())
    return false;

  if (!parsePositiveInteger(value.substr(0, separator), timerId) || timerId <= 0)
    return false;

  remote = value.substr(separator + 1);
  return !remote.empty();
}

long long fingerprint(const std::string& value)
{
  std::uint64_t hash = 1469598103934665603ULL;
  for (unsigned char character : value) {
    hash ^= character;
    hash *= 1099511628211ULL;
  }
  return static_cast<long long>(hash & 0x7FFFFFFFFFFFFFFFULL);
}

long long recordingFingerprint(const std::string& recordingFile, bool found)
{
  std::ostringstream state;
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

}

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

RecordingLocalTimerLookupResult VdrRecordingLocalTimerLookup::findActive(
  const std::string& recordingFile) const
{
  RecordingLocalTimerLookupResult result;

  if (recordingFile.empty())
    return result;

  result.known = true;
  result.active = cRecordControls::GetRecordControl(recordingFile.c_str()) != nullptr;
  return result;
}

RecordingRemoteTimerLookupResult VdrRecordingRemoteTimerLookup::findActive(
  const std::string& recordingFile) const
{
  RecordingRemoteTimerLookupResult result;

  if (recordingFile.empty())
    return result;

  const cString timerIdText = GetRecordingTimerId(recordingFile.c_str());
  const char* timerId = *timerIdText;

  if (!timerId || !*timerId) {
    result.known = true;
    return result;
  }

  if (!parseRemoteTimerId(timerId, result.timerId, result.remote))
    return result;

  LOCK_TIMERS_READ;
  const cTimer* timer = Timers->GetById(result.timerId, result.remote.c_str());
  if (!timer)
    return result;

  result.known = true;
  result.active = timer->HasFlags(tfActive) || timer->Recording();
  return result;
}

RecordingSearchTimerLookupResult VdrRecordingSearchTimerLookup::findOrigin(
  const std::string& recordingFile) const
{
  RecordingSearchTimerLookupResult result;

  if (recordingFile.empty())
    return result;

  LOCK_TIMERS_READ;

  const cTimer* timer = nullptr;
  if (cRecordControl* recordControl = cRecordControls::GetRecordControl(recordingFile.c_str()))
    timer = recordControl->Timer();

  if (!timer) {
    const cString timerIdText = GetRecordingTimerId(recordingFile.c_str());
    const char* timerId = *timerIdText;

    if (!timerId || !*timerId) {
      result.known = true;
      return result;
    }

    int id = 0;
    std::string remote;
    if (!parseRemoteTimerId(timerId, id, remote))
      return result;

    timer = Timers->GetById(id, remote.c_str());
    if (!timer)
      return result;
  }

  bool present = false;
  int searchTimerId = -1;
  if (!parseSearchTimerId(timer->Aux(), present, searchTimerId))
    return result;

  result.known = true;
  result.searchTimerRecording = present;
  result.searchTimerId = present ? searchTimerId : -1;
  return result;
}

RecordingTrashAnalyzer::RecordingTrashAnalyzer(
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

RecordingMutationAnalysis RecordingTrashAnalyzer::analyze(
  const std::string& recordingFile) const
{
  RecordingMutationAnalysis analysis;
  analysis.type = RecordingMutationType::Trash;
  analysis.recordingFile = recordingFile;
  analysis.revision.recordingFile = recordingFile;

  const RecordingLookupResult recording = recordingLookup.find(recordingFile);
  analysis.revision.recordingsState = recordingFingerprint(recordingFile, recording.found);

  if (!recording.found) {
    analysis.constraints.push_back(RecordingConstraint::RecordingMissing);
    return analysis;
  }

  analysis.recordingFile = recording.recordingFile;
  analysis.revision.recordingFile = recording.recordingFile;
  analysis.revision.recordingsState = recordingFingerprint(recording.recordingFile, true);

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

  analysis.revision.timersState = timerFingerprint(
    replaying,
    handlerUsage,
    localTimer,
    remoteTimer,
    searchTimer);

  return analysis;
}
