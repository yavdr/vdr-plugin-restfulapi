#include "../recordingmoveexecution.h"

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
  RecordingMoveExecutionGate gate(analyzer, planner);
  RecordingMutationPolicy policy;

  const RecordingMutationAnalysis initial = analyzer.analyze(source, target);
  const RecordingMoveExecutionGateResult ready = gate.validate(
    source,
    target,
    initial.revision,
    policy);
  assert(ready.status == RecordingMoveExecutionGateStatus::Ready);
  assert(ready.currentRevision.targetFile == target);

  RecordingMutationRevision wrongTargetRevision = initial.revision;
  wrongTargetRevision.targetFile = "/srv/vdr/video/Other/2026-07-14.20.00.1-0.rec";
  const RecordingMoveExecutionGateResult wrongTarget = gate.validate(
    source,
    target,
    wrongTargetRevision,
    policy);
  assert(wrongTarget.status == RecordingMoveExecutionGateStatus::Conflict);

  recordingLookup.results[target] = {true, target};
  const RecordingMoveExecutionGateResult collision = gate.validate(
    source,
    target,
    initial.revision,
    policy);
  assert(collision.status == RecordingMoveExecutionGateStatus::Conflict);

  recordingLookup.results.erase(target);
  replayLookup.replaying = true;
  const RecordingMutationAnalysis replayAnalysis = analyzer.analyze(source, target);
  const RecordingMoveExecutionGateResult blocked = gate.validate(
    source,
    target,
    replayAnalysis.revision,
    policy);
  assert(blocked.status == RecordingMoveExecutionGateStatus::Blocked);
  assert(!blocked.blockers.empty());

  return 0;
}
