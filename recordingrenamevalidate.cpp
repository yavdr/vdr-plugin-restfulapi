#include "recordingrenamevalidate.h"

#include "recordinganalysis.h"
#include "recordingmoveexecution.h"
#include "recordingmutation.h"
#include "recordingrenameplan.h"
#include "tools.h"

#include <cxxtools/jsonserializer.h>

#include <string>
#include <vector>

namespace {

bool parseLongLong(const std::string& value, long long& result)
{
  if (value.empty())
    return false;

  try {
    std::size_t parsed = 0;
    result = std::stoll(value, &parsed, 10);
    return parsed == value.size();
  }
  catch (...) {
    return false;
  }
}

std::vector<std::string> constraintNames(
  const std::vector<RecordingConstraint>& constraints)
{
  std::vector<std::string> names;
  for (RecordingConstraint constraint : constraints)
    names.push_back(RecordingConstraintName(constraint));
  return names;
}

std::string renameConstraintName(RecordingRenamePlanStatus status)
{
  switch (status) {
    case RecordingRenamePlanStatus::SourceInvalid:
      return "rename-source-invalid";
    case RecordingRenamePlanStatus::NameMissing:
      return "rename-name-missing";
    case RecordingRenamePlanStatus::NameInvalid:
      return "rename-name-invalid";
    case RecordingRenamePlanStatus::TargetSameAsSource:
      return "rename-target-same-as-source";
    case RecordingRenamePlanStatus::Ready:
      return std::string();
  }

  return "rename-plan-invalid";
}

}

void RecordingRenameValidateResponder::reply(
  std::ostream& out,
  cxxtools::http::Request& request,
  cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);

  if (request.method() == "OPTIONS") {
    reply.addHeader("Allow", "POST");
    reply.httpReturn(200, "OK");
    return;
  }

  if (request.method() != "POST") {
    reply.httpReturn(
      501,
      "Only POST method is supported by the /recordings/rename/validate service.");
    return;
  }

  QueryHandler query("/recordings/rename/validate", request);
  const std::string recordingFile = query.getBodyAsString("file");
  const std::string requestedName = query.getBodyAsString("name");
  const std::string recordingsStateText =
    query.getBodyAsString("revision_recordings_state");
  const std::string timersStateText =
    query.getBodyAsString("revision_timers_state");

  long long recordingsState = 0;
  long long timersState = 0;

  if (recordingFile.empty()) {
    reply.httpReturn(400, "Recording file is missing.");
    return;
  }

  if (requestedName.empty()) {
    reply.httpReturn(400, "Recording rename name is missing.");
    return;
  }

  if (!parseLongLong(recordingsStateText, recordingsState) ||
      !parseLongLong(timersStateText, timersState)) {
    reply.httpReturn(400, "Recording rename revision is missing or invalid.");
    return;
  }

  RecordingRenamePlanner renamePlanner;
  const RecordingRenamePlan renamePlan = renamePlanner.build(
    recordingFile,
    requestedName);

  reply.addHeader("Content-Type", "application/json; charset=utf-8");
  cxxtools::JsonSerializer serializer(out);

  if (!renamePlan.executable()) {
    const std::string blocker = renameConstraintName(renamePlan.status);
    const std::vector<std::string> blockers = blocker.empty()
      ? std::vector<std::string>()
      : std::vector<std::string>(1, blocker);

    serializer.serialize(std::string("blocked"), "status");
    serializer.serialize(recordingFile, "recording_file");
    serializer.serialize(requestedName, "name");
    serializer.serialize(renamePlan.targetFile, "target_file");
    serializer.serialize(
      std::string(RecordingRenamePlanStatusName(renamePlan.status)),
      "rename_status");
    serializer.serialize(blockers, "blockers");
    serializer.serialize(std::vector<std::string>(), "warnings");
    serializer.serialize(recordingFile, "expected_revision_recording_file");
    serializer.serialize(renamePlan.targetFile, "expected_revision_target_file");
    serializer.serialize(recordingsState, "expected_revision_recordings_state");
    serializer.serialize(timersState, "expected_revision_timers_state");
    serializer.serialize(std::string(), "current_revision_recording_file");
    serializer.serialize(std::string(), "current_revision_target_file");
    serializer.serialize(0LL, "current_revision_recordings_state");
    serializer.serialize(0LL, "current_revision_timers_state");
    serializer.finish();
    return;
  }

  RecordingMutationPolicy policy;
  policy.allowRecordingHandlerStop =
    query.getBodyAsBool("allow_recording_handler_stop");
  policy.allowReplayStop = query.getBodyAsBool("allow_replay_stop");
  policy.allowLocalTimerStop = query.getBodyAsBool("allow_local_timer_stop");
  policy.allowRemoteTimerStop = query.getBodyAsBool("allow_remote_timer_stop");

  VdrRecordingLookup recordingLookup;
  VdrRecordingReplayLookup replayLookup;
  VdrRecordingHandlerLookup recordingHandlerLookup;
  VdrRecordingLocalTimerLookup localTimerLookup;
  VdrRecordingRemoteTimerLookup remoteTimerLookup;
  VdrRecordingSearchTimerLookup searchTimerLookup;

  RecordingMoveAnalyzer analyzer(
    recordingLookup,
    replayLookup,
    recordingHandlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);

  RecordingMutationPlanner planner;
  RecordingMoveExecutionGate gate(analyzer, planner);

  RecordingMutationRevision expectedRevision;
  expectedRevision.recordingFile = recordingFile;
  expectedRevision.targetFile = renamePlan.targetFile;
  expectedRevision.recordingsState = recordingsState;
  expectedRevision.timersState = timersState;

  const RecordingMoveExecutionGateResult result = gate.validate(
    recordingFile,
    renamePlan.targetFile,
    expectedRevision,
    policy);

  serializer.serialize(
    std::string(RecordingMoveExecutionGateStatusName(result.status)),
    "status");
  serializer.serialize(result.recordingFile, "recording_file");
  serializer.serialize(requestedName, "name");
  serializer.serialize(result.targetFile, "target_file");
  serializer.serialize(
    std::string(RecordingRenamePlanStatusName(renamePlan.status)),
    "rename_status");
  serializer.serialize(constraintNames(result.blockers), "blockers");
  serializer.serialize(result.warnings, "warnings");
  serializer.serialize(
    result.expectedRevision.recordingFile,
    "expected_revision_recording_file");
  serializer.serialize(
    result.expectedRevision.targetFile,
    "expected_revision_target_file");
  serializer.serialize(
    result.expectedRevision.recordingsState,
    "expected_revision_recordings_state");
  serializer.serialize(
    result.expectedRevision.timersState,
    "expected_revision_timers_state");
  serializer.serialize(
    result.currentRevision.recordingFile,
    "current_revision_recording_file");
  serializer.serialize(
    result.currentRevision.targetFile,
    "current_revision_target_file");
  serializer.serialize(
    result.currentRevision.recordingsState,
    "current_revision_recordings_state");
  serializer.serialize(
    result.currentRevision.timersState,
    "current_revision_timers_state");
  serializer.finish();
}
