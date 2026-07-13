#include "recordingpreflight.h"

RecordingTrashPreflightService::RecordingTrashPreflightService(
  const RecordingTrashAnalyzer& analyzer,
  const RecordingMutationPlanner& planner)
  : analyzer(analyzer),
    planner(planner)
{
}

RecordingTrashPreflightResult RecordingTrashPreflightService::preview(
  const std::string& recordingFile,
  const RecordingMutationPolicy& policy) const
{
  RecordingTrashPreflightResult result;
  const RecordingMutationAnalysis analysis = analyzer.analyze(recordingFile);
  const RecordingMutationPlan plan = planner.buildTrashPlan(analysis, policy);

  result.executable = plan.executable;
  result.recordingFile = analysis.recordingFile;
  result.warnings = plan.warnings;
  result.revision = plan.expectedRevision;

  for (const RecordingConstraint constraint : analysis.constraints)
    result.constraints.push_back(RecordingConstraintName(constraint));

  for (const RecordingConstraint blocker : plan.blockers)
    result.blockers.push_back(RecordingConstraintName(blocker));

  for (const RecordingMutationStep step : plan.steps)
    result.steps.push_back(RecordingMutationStepName(step));

  return result;
}
