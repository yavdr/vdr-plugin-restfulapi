#include "recordingexecution.h"

namespace {

bool sameRevision(
  const RecordingMutationRevision& left,
  const RecordingMutationRevision& right)
{
  return left.recordingFile == right.recordingFile
    && left.recordingsState == right.recordingsState
    && left.timersState == right.timersState;
}

}

RecordingTrashExecutionGate::RecordingTrashExecutionGate(
  const RecordingTrashAnalyzer& analyzer,
  const RecordingMutationPlanner& planner)
  : analyzer(analyzer),
    planner(planner)
{
}

RecordingTrashExecutionGateResult RecordingTrashExecutionGate::validate(
  const std::string& recordingFile,
  const RecordingMutationRevision& expectedRevision,
  const RecordingMutationPolicy& policy) const
{
  RecordingTrashExecutionGateResult result;
  result.recordingFile = recordingFile;
  result.expectedRevision = expectedRevision;

  const RecordingMutationAnalysis analysis = analyzer.analyze(recordingFile);
  const RecordingMutationPlan plan = planner.buildTrashPlan(analysis, policy);

  result.currentRevision = analysis.revision;
  result.blockers = plan.blockers;
  result.warnings = plan.warnings;

  if (!sameRevision(expectedRevision, analysis.revision)) {
    result.status = RecordingTrashExecutionGateStatus::Conflict;
    return result;
  }

  if (!plan.executable) {
    result.status = RecordingTrashExecutionGateStatus::Blocked;
    return result;
  }

  result.status = RecordingTrashExecutionGateStatus::Ready;
  return result;
}

const char* RecordingTrashExecutionGateStatusName(
  RecordingTrashExecutionGateStatus status)
{
  switch (status) {
    case RecordingTrashExecutionGateStatus::Ready:
      return "ready";
    case RecordingTrashExecutionGateStatus::Conflict:
      return "conflict";
    case RecordingTrashExecutionGateStatus::Blocked:
      return "blocked";
  }

  return "blocked";
}
