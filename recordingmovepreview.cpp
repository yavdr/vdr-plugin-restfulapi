#include "recordingmovepreview.h"

#include "recordinganalysis.h"
#include "recordingmutation.h"
#include "recordingpreflight.h"
#include "tools.h"

#include <cxxtools/jsonserializer.h>

void RecordingMovePreviewResponder::reply(
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
    reply.httpReturn(501, "Only POST method is supported by the /recordings/move/preview service.");
    return;
  }

  QueryHandler query("/recordings/move/preview", request);
  const std::string recordingFile = query.getBodyAsString("file");
  const std::string targetFile = query.getBodyAsString("target_file");

  if (recordingFile.empty()) {
    reply.httpReturn(400, "Recording file is missing.");
    return;
  }

  if (targetFile.empty()) {
    reply.httpReturn(400, "Recording move target file is missing.");
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

  RecordingMoveAnalyzer analyzer(
    recordingLookup,
    replayLookup,
    recordingHandlerLookup,
    localTimerLookup,
    remoteTimerLookup,
    searchTimerLookup);

  RecordingMutationPlanner planner;
  RecordingMovePreflightService preflightService(analyzer, planner);
  const RecordingMovePreflightResult result =
    preflightService.preview(recordingFile, targetFile, policy);

  reply.addHeader("Content-Type", "application/json; charset=utf-8");

  cxxtools::JsonSerializer serializer(out);
  serializer.serialize(result.executable, "executable");
  serializer.serialize(result.recordingFile, "recording_file");
  serializer.serialize(result.targetFile, "target_file");
  serializer.serialize(result.constraints, "constraints");
  serializer.serialize(result.blockers, "blockers");
  serializer.serialize(result.warnings, "warnings");
  serializer.serialize(result.steps, "steps");
  serializer.serialize(result.revision.recordingFile, "revision_recording_file");
  serializer.serialize(result.revision.targetFile, "revision_target_file");
  serializer.serialize(result.revision.recordingsState, "revision_recordings_state");
  serializer.serialize(result.revision.timersState, "revision_timers_state");
  serializer.finish();
}
