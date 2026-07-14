#include "recordingtrash.h"

#include "recordinganalysis.h"
#include "recordingexecution.h"
#include "recordingmutation.h"
#include "recordingtrashexecutor.h"
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

void RecordingTrashResponder::reply(
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
    reply.httpReturn(501, "Only POST method is supported by the /recordings/trash service.");
    return;
  }

  QueryHandler query("/recordings/trash", request);
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

  RecordingTrashAnalyzer analyzer(
    recordingLookup,
    replayLookup,
    recordingHandlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);

  RecordingMutationPlanner planner;
  RecordingTrashExecutionGate gate(analyzer, planner);
  RecordingTrashExecutor executor(gate);

  RecordingMutationRevision expectedRevision;
  expectedRevision.recordingFile = recordingFile;
  expectedRevision.recordingsState = recordingsState;
  expectedRevision.timersState = timersState;

  const RecordingTrashExecutorResult result =
    executor.executeNormalCase(recordingFile, expectedRevision, policy);

  switch (result.status) {
    case RecordingTrashExecutorStatus::Conflict:
      reply.httpReturn(409, result.message);
      return;
    case RecordingTrashExecutorStatus::Blocked:
      reply.httpReturn(423, result.message);
      return;
    case RecordingTrashExecutorStatus::NotFound:
      reply.httpReturn(404, result.message);
      return;
    case RecordingTrashExecutorStatus::Failed:
      reply.httpReturn(500, result.message);
      return;
    case RecordingTrashExecutorStatus::Trashed:
    case RecordingTrashExecutorStatus::AlreadyTrashed:
      break;
  }

  reply.addHeader("Content-Type", "application/json; charset=utf-8");

  cxxtools::JsonSerializer serializer(out);
  serializer.serialize(
    std::string(RecordingTrashExecutorStatusName(result.status)),
    "status");
  serializer.serialize(result.recordingFile, "recording_file");
  serializer.serialize(result.deletedRecordingFile, "deleted_recording_file");
  serializer.serialize(result.message, "message");
  serializer.finish();
}
