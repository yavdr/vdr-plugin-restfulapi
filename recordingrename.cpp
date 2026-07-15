#include "recordingrename.h"

#include "recordinganalysis.h"
#include "recordingmoveexecution.h"
#include "recordingmoveexecutor.h"
#include "recordingmutation.h"
#include "recordingrenameplan.h"
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

}

void RecordingRenameResponder::reply(
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
    reply.httpReturn(501, "Only POST method is supported by the /recordings/rename service.");
    return;
  }

  QueryHandler query("/recordings/rename", request);
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

  if (!renamePlan.executable()) {
    reply.httpReturn(
      400,
      std::string("Recording rename request is invalid: ") +
        RecordingRenamePlanStatusName(renamePlan.status) + ".");
    return;
  }

  RecordingMutationPolicy policy;
  policy.allowRecordingHandlerStop = false;
  policy.allowReplayStop = false;
  policy.allowLocalTimerStop = false;
  policy.allowRemoteTimerStop = false;

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

  RecordingMutationPlanner mutationPlanner;
  RecordingMoveExecutionGate gate(analyzer, mutationPlanner);
  RecordingMoveExecutor executor(gate);

  RecordingMutationRevision expectedRevision;
  expectedRevision.recordingFile = recordingFile;
  expectedRevision.targetFile = renamePlan.targetFile;
  expectedRevision.recordingsState = recordingsState;
  expectedRevision.timersState = timersState;

  const RecordingMoveExecutorResult result = executor.executeNormalCase(
    recordingFile,
    renamePlan.targetFile,
    expectedRevision,
    policy);

  switch (result.status) {
    case RecordingMoveExecutorStatus::Conflict:
      reply.httpReturn(409, result.message);
      return;
    case RecordingMoveExecutorStatus::Blocked:
      reply.httpReturn(423, result.message);
      return;
    case RecordingMoveExecutorStatus::NotFound:
      reply.httpReturn(404, result.message);
      return;
    case RecordingMoveExecutorStatus::Failed:
      reply.httpReturn(500, result.message);
      return;
    case RecordingMoveExecutorStatus::Moved:
    case RecordingMoveExecutorStatus::AlreadyMoved:
      break;
  }

  reply.addHeader("Content-Type", "application/json; charset=utf-8");

  cxxtools::JsonSerializer serializer(out);
  serializer.serialize(
    result.status == RecordingMoveExecutorStatus::AlreadyMoved
      ? std::string("already-renamed")
      : std::string("renamed"),
    "status");
  serializer.serialize(result.recordingFile, "recording_file");
  serializer.serialize(requestedName, "name");
  serializer.serialize(result.targetFile, "target_file");
  serializer.serialize(
    result.status == RecordingMoveExecutorStatus::AlreadyMoved
      ? std::string("Recording already has the requested name.")
      : std::string("Recording renamed to the requested name."),
    "message");
  serializer.finish();
}
