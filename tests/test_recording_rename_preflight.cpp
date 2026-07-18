#include "../recordingrenamepreflight.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <string>

namespace {

class FakeRecordingLookup : public IRecordingLookup
{
public:
  mutable int calls = 0;
  std::map<std::string, RecordingLookupResult> results;

  RecordingLookupResult find(const std::string& recordingFile) const override
  {
    ++calls;
    const auto result = results.find(recordingFile);
    return result == results.end() ? RecordingLookupResult() : result->second;
  }
};

class FakeReplayLookup : public IRecordingReplayLookup
{
public:
  bool replaying = false;

  bool isReplaying(const std::string&) const override
  {
    return replaying;
  }
};

class FakeHandlerLookup : public IRecordingHandlerLookup
{
public:
  RecordingHandlerLookupResult result{true, false};

  RecordingHandlerLookupResult getUsage(const std::string&) const override
  {
    return result;
  }
};

class FakeLocalTimerLookup : public IRecordingLocalTimerLookup
{
public:
  RecordingLocalTimerLookupResult result{true, false};

  RecordingLocalTimerLookupResult findActive(const std::string&) const override
  {
    return result;
  }
};

class FakeRemoteTimerLookup : public IRecordingRemoteTimerLookup
{
public:
  RecordingRemoteTimerLookupResult result{true, false, 0, ""};

  RecordingRemoteTimerLookupResult findActive(const std::string&) const override
  {
    return result;
  }
};

class FakeSearchTimerLookup : public IRecordingSearchTimerLookup
{
public:
  RecordingSearchTimerLookupResult result{true, false, -1};

  RecordingSearchTimerLookupResult findOrigin(const std::string&) const override
  {
    return result;
  }
};

bool contains(const std::vector<std::string>& values, const std::string& value)
{
  return std::find(values.begin(), values.end(), value) != values.end();
}

}

int main()
{
  const std::string source =
    "/srv/vdr/video/Hutehiermorgenda/Old title/2026-07-08.21.45.2-0.rec";
  const std::string target =
    "/srv/vdr/video/Hutehiermorgenda/New title/2026-07-08.21.45.2-0.rec";

  FakeRecordingLookup recordingLookup;
  recordingLookup.results[source] = {true, source};
  FakeReplayLookup replayLookup;
  FakeHandlerLookup handlerLookup;
  FakeLocalTimerLookup localTimerLookup;
  FakeRemoteTimerLookup remoteTimerLookup;
  FakeSearchTimerLookup searchTimerLookup;

  RecordingRenamePlanner renamePlanner;
  RecordingMoveAnalyzer moveAnalyzer(
    recordingLookup,
    replayLookup,
    handlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);
  RecordingMutationPlanner mutationPlanner;
  RecordingRenamePreflightService service(
    renamePlanner,
    moveAnalyzer,
    mutationPlanner);
  RecordingMutationPolicy policy;

  const RecordingRenamePreflightResult ready = service.preview(
    source,
    "  New title  ",
    policy);
  assert(ready.executable);
  assert(ready.renameStatus == RecordingRenamePlanStatus::Ready);
  assert(ready.recordingFile == source);
  assert(ready.requestedName == "New title");
  assert(ready.targetFile == target);
  assert(ready.constraints.empty());
  assert(ready.blockers.empty());
  assert(contains(ready.steps, "move-recording"));
  assert(contains(ready.steps, "refresh-recordings"));
  assert(contains(ready.steps, "notify-change"));
  assert(ready.revision.recordingFile == source);
  assert(ready.revision.targetFile == target);
  assert(ready.revision.recordingsState != 0);
  assert(ready.revision.timersState != 0);

  const int callsAfterReady = recordingLookup.calls;
  const RecordingRenamePreflightResult invalidName = service.preview(
    source,
    "../Other",
    policy);
  assert(!invalidName.executable);
  assert(invalidName.renameStatus == RecordingRenamePlanStatus::NameInvalid);
  assert(contains(invalidName.constraints, "rename-name-invalid"));
  assert(contains(invalidName.blockers, "rename-name-invalid"));
  assert(recordingLookup.calls == callsAfterReady);

  recordingLookup.results[target] = {true, target};
  const RecordingRenamePreflightResult collision = service.preview(
    source,
    "New title",
    policy);
  assert(!collision.executable);
  assert(contains(collision.constraints, "move-target-exists"));
  assert(contains(collision.blockers, "move-target-exists"));

  recordingLookup.results.erase(target);
  replayLookup.replaying = true;
  const RecordingRenamePreflightResult replayBlocked = service.preview(
    source,
    "Another title",
    policy);
  assert(!replayBlocked.executable);
  assert(contains(replayBlocked.constraints, "replay-active"));
  assert(contains(replayBlocked.blockers, "replay-active"));

  const RecordingRenamePreflightResult sameName = service.preview(
    source,
    "Old title",
    policy);
  assert(!sameName.executable);
  assert(sameName.renameStatus == RecordingRenamePlanStatus::TargetSameAsSource);
  assert(contains(sameName.blockers, "rename-target-same-as-source"));

  return 0;
}
