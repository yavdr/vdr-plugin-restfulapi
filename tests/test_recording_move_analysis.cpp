#include "../recordinganalysis.h"

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

RecordingMoveAnalyzer makeAnalyzer(
  const FakeRecordingLookup& recordingLookup,
  const FakeReplayLookup& replayLookup,
  const FakeHandlerLookup& handlerLookup,
  const FakeLocalTimerLookup& localTimerLookup,
  const FakeRemoteTimerLookup& remoteTimerLookup,
  const FakeSearchTimerLookup& searchTimerLookup)
{
  return RecordingMoveAnalyzer(
    recordingLookup,
    replayLookup,
    handlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);
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

  RecordingMoveAnalyzer analyzer = makeAnalyzer(
    recordingLookup,
    replayLookup,
    handlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);

  const RecordingMutationAnalysis ready = analyzer.analyze(source, target);
  assert(ready.type == RecordingMutationType::Move);
  assert(ready.recordingFile == source);
  assert(ready.targetFile == target);
  assert(ready.revision.recordingFile == source);
  assert(ready.revision.targetFile == target);
  assert(ready.revision.recordingsState != 0);
  assert(ready.revision.timersState != 0);
  assert(ready.constraints.empty());

  const RecordingMutationAnalysis missingTarget = analyzer.analyze(source, "");
  assert(missingTarget.hasConstraint(RecordingConstraint::MoveTargetMissing));

  const RecordingMutationAnalysis invalidTarget = analyzer.analyze(source, "Target/relative.rec");
  assert(invalidTarget.hasConstraint(RecordingConstraint::MoveTargetInvalid));

  const RecordingMutationAnalysis sameTarget = analyzer.analyze(source, source);
  assert(sameTarget.hasConstraint(RecordingConstraint::MoveTargetSameAsSource));
  assert(!sameTarget.hasConstraint(RecordingConstraint::MoveTargetExists));

  recordingLookup.results[target] = {true, target};
  analyzer = makeAnalyzer(
    recordingLookup,
    replayLookup,
    handlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);
  const RecordingMutationAnalysis collision = analyzer.analyze(source, target);
  assert(collision.hasConstraint(RecordingConstraint::MoveTargetExists));
  assert(collision.revision.recordingsState != ready.revision.recordingsState);

  replayLookup.replaying = true;
  handlerLookup.result.busy = true;
  localTimerLookup.result.active = true;
  remoteTimerLookup.result.active = true;
  searchTimerLookup.result.searchTimerRecording = true;
  analyzer = makeAnalyzer(
    recordingLookup,
    replayLookup,
    handlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);
  const RecordingMutationAnalysis active = analyzer.analyze(source, "/srv/vdr/video/Other/2026.rec");
  assert(active.hasConstraint(RecordingConstraint::ReplayActive));
  assert(active.hasConstraint(RecordingConstraint::RecordingHandlerBusy));
  assert(active.hasConstraint(RecordingConstraint::LocalTimerActive));
  assert(active.hasConstraint(RecordingConstraint::RemoteTimerActive));
  assert(active.hasConstraint(RecordingConstraint::SearchTimerRecording));

  const RecordingMutationAnalysis missingSource = analyzer.analyze(
    "/srv/vdr/video/Missing/2026.rec",
    target);
  assert(missingSource.hasConstraint(RecordingConstraint::RecordingMissing));

  return 0;
}
