#include "recordingvalidate.h"

#include "recordinganalysis.h"
#include "recordingexecution.h"
#include "recordingmutation.h"
#include "tools.h"

#include <cxxtools/jsonserializer.h>

#include <string>

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

}

void RecordingTrashValidateResponder::reply(
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
    reply.httpReturn(501, "Only POST method is supported by the /recordings/trash/validate service.");
    return;
  }

  QueryHandler query("/recordings/trash/validate", request);
  const std::string recordingFile = query.getBodyAsString("file");
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

  if (!parseLongLong(recordingsStateText, recordingsState) ||
      !parseLongLong(timersStateText, timersState)) {
    reply.httpReturn(400, "Recording revision is missing or invalid.");
    return;
  }

  RecordingMutationPolicy policy;
  policy.allowRecordingHandlerStop = query.getBodyAsBool("allow_recording_handler_stop");
  policy.allowReplayStop = query.getBodyAsBool("allow_replay_stop");
  policy.allowLocalTimerStop = query.getBodyAsBool("allow_local_timer_stop");
  policy.allowRemoteTimerStop = query.getBodyAsBool("allow_remote_timer_stop");

  VdrRecordingLookup recordingLookup;
  VdrRecordingReplayLookup replayLookup;
  VdrRecordingHandlerLookup recordingHandlerLookup;
  VdrRecordingLocalTimerLookup localTimerLookup;
  VdrRecordingRemoteTimerLookup remoteTimerLookup;
  VdrRecordingSearchTimerLookup searchTimerLookup;

  RecordingTrashAnalyzer analyzer(
    recordingLookup,
    replayLookup,
    recordingHandlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);

  RecordingMutationPlanner planner;
  RecordingTrashExecutionGate gate(analyzer, planner);

  RecordingMutationRevision expectedRevision;
  expectedRevision.recordingFile = recordingFile;
  expectedRevision.recordingsState = recordingsState;
  expectedRevision.timersState = timersState;

  const RecordingTrashExecutionGateResult result =
    gate.validate(recordingFile, expectedRevision, policy);

  reply.addHeader("Content-Type", "application/json; charset=utf-8");

  cxxtools::JsonSerializer serializer(out);
  serializer.serialize(
    std::string(RecordingTrashExecutionGateStatusName(result.status)),
    "status");
  serializer.serialize(result.recordingFile, "recording_file");
  serializer.serialize(constraintNames(result.blockers), "blockers");
  serializer.serialize(result.warnings, "warnings");
  serializer.serialize(
    result.expectedRevision.recordingFile,
    "expected_revision_recording_file");
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
    result.currentRevision.recordingsState,
    "current_revision_recordings_state");
  serializer.serialize(
    result.currentRevision.timersState,
    "current_revision_timers_state");
  serializer.finish();
}
