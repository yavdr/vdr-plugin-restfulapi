#include "../recordingpreflight.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <string>

namespace {

class FakeRecordingLookup : public IRecordingLookup
{
public:
  std::map<std::string, RecordingLookupResult> results;

  RecordingLookupResult find(const std::string& recordingFile) const override
  {
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
  const std::string source = "/srv/vdr/video/Source/2026-07-14.20.00.1-0.rec";
  const std::string target = "/srv/vdr/video/Target/2026-07-14.20.00.1-0.rec";

  FakeRecordingLookup recordingLookup;
  recordingLookup.results[source] = {true, source};
  FakeReplayLookup replayLookup;
  FakeHandlerLookup handlerLookup;
  FakeLocalTimerLookup localTimerLookup;
  FakeRemoteTimerLookup remoteTimerLookup;
  FakeSearchTimerLookup searchTimerLookup;

  RecordingMoveAnalyzer analyzer(
    recordingLookup,
    replayLookup,
    handlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);
  RecordingMutationPlanner planner;
  RecordingMovePreflightService service(analyzer, planner);
  RecordingMutationPolicy policy;

  const RecordingMovePreflightResult ready = service.preview(source, target, policy);
  assert(ready.executable);
  assert(ready.recordingFile == source);
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

  const RecordingMovePreflightResult missingTarget = service.preview(source, "", policy);
  assert(!missingTarget.executable);
  assert(contains(missingTarget.constraints, "move-target-missing"));
  assert(contains(missingTarget.blockers, "move-target-missing"));

  recordingLookup.results[target] = {true, target};
  const RecordingMovePreflightResult collision = service.preview(source, target, policy);
  assert(!collision.executable);
  assert(contains(collision.constraints, "move-target-exists"));
  assert(contains(collision.blockers, "move-target-exists"));
  assert(collision.revision.recordingsState != ready.revision.recordingsState);

  replayLookup.replaying = true;
  const RecordingMovePreflightResult replayBlocked = service.preview(
    source,
    "/srv/vdr/video/Other/2026-07-14.20.00.1-0.rec",
    policy);
  assert(!replayBlocked.executable);
  assert(contains(replayBlocked.constraints, "replay-active"));
  assert(contains(replayBlocked.blockers, "replay-active"));

  return 0;
}
