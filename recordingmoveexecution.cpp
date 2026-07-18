#include "recordingmoveexecution.h"

namespace {

bool sameRevision(
  const RecordingMutationRevision& left,
  const RecordingMutationRevision& right)
{
  return left.recordingFile == right.recordingFile
    && left.targetFile == right.targetFile
    && left.recordingsState == right.recordingsState
    && left.timersState == right.timersState;
}

}

RecordingMoveExecutionGate::RecordingMoveExecutionGate(
  const RecordingMoveAnalyzer& analyzer,
  const RecordingMutationPlanner& planner)
  : analyzer(analyzer),
    planner(planner)
{
}

RecordingMoveExecutionGateResult RecordingMoveExecutionGate::validate(
  const std::string& recordingFile,
  const std::string& targetFile,
  const RecordingMutationRevision& expectedRevision,
  const RecordingMutationPolicy& policy) const
{
  RecordingMoveExecutionGateResult result;
  result.recordingFile = recordingFile;
  result.targetFile = targetFile;
  result.expectedRevision = expectedRevision;

  const RecordingMutationAnalysis analysis = analyzer.analyze(recordingFile, targetFile);
  const RecordingMutationPlan plan = planner.buildMovePlan(analysis, policy);

  result.currentRevision = analysis.revision;
  result.blockers = plan.blockers;
  result.warnings = plan.warnings;

  if (!sameRevision(expectedRevision, analysis.revision)) {
    result.status = RecordingMoveExecutionGateStatus::Conflict;
    return result;
  }

  if (!plan.executable) {
    result.status = RecordingMoveExecutionGateStatus::Blocked;
    return result;
  }

  result.status = RecordingMoveExecutionGateStatus::Ready;
  return result;
}

const char* RecordingMoveExecutionGateStatusName(
  RecordingMoveExecutionGateStatus status)
{
  switch (status) {
    case RecordingMoveExecutionGateStatus::Ready:
      return "ready";
    case RecordingMoveExecutionGateStatus::Conflict:
      return "conflict";
    case RecordingMoveExecutionGateStatus::Blocked:
      return "blocked";
  }

  return "blocked";
}
