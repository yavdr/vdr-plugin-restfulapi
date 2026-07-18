#include "recordingrenamepreflight.h"

namespace {

void appendMovePreflight(
  RecordingRenamePreflightResult& result,
  const RecordingMutationAnalysis& analysis,
  const RecordingMutationPlan& plan)
{
  result.executable = plan.executable;
  result.targetFile = analysis.targetFile;
  result.warnings = plan.warnings;
  result.revision = plan.expectedRevision;

  for (const RecordingConstraint constraint : analysis.constraints)
    result.constraints.push_back(RecordingConstraintName(constraint));

  for (const RecordingConstraint blocker : plan.blockers)
    result.blockers.push_back(RecordingConstraintName(blocker));

  for (const RecordingMutationStep step : plan.steps)
    result.steps.push_back(RecordingMutationStepName(step));
}

std::string renameConstraintName(RecordingRenamePlanStatus status)
{
  return std::string("rename-") + RecordingRenamePlanStatusName(status);
}

}

RecordingRenamePreflightService::RecordingRenamePreflightService(
  const RecordingRenamePlanner& renamePlanner,
  const RecordingMoveAnalyzer& moveAnalyzer,
  const RecordingMutationPlanner& mutationPlanner)
  : renamePlanner(renamePlanner),
    moveAnalyzer(moveAnalyzer),
    mutationPlanner(mutationPlanner)
{
}

RecordingRenamePreflightResult RecordingRenamePreflightService::preview(
  const std::string& recordingFile,
  const std::string& requestedName,
  const RecordingMutationPolicy& policy) const
{
  RecordingRenamePreflightResult result;
  const RecordingRenamePlan renamePlan = renamePlanner.build(
    recordingFile,
    requestedName);

  result.renameStatus = renamePlan.status;
  result.recordingFile = renamePlan.recordingFile;
  result.requestedName = renamePlan.requestedName;
  result.targetFile = renamePlan.targetFile;

  if (!renamePlan.executable()) {
    const std::string constraint = renameConstraintName(renamePlan.status);
    result.constraints.push_back(constraint);
    result.blockers.push_back(constraint);
    return result;
  }

  const RecordingMutationAnalysis analysis = moveAnalyzer.analyze(
    renamePlan.recordingFile,
    renamePlan.targetFile);
  const RecordingMutationPlan mutationPlan = mutationPlanner.buildMovePlan(
    analysis,
    policy);
  appendMovePreflight(result, analysis, mutationPlan);
  return result;
}
