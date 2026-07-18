#include "recordingpreflight.h"

namespace {

void appendMoveAnalysisAndPlan(
  RecordingMovePreflightResult& result,
  const RecordingMutationAnalysis& analysis,
  const RecordingMutationPlan& plan)
{
  result.executable = plan.executable;
  result.recordingFile = analysis.recordingFile;
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

}

RecordingMovePreflightService::RecordingMovePreflightService(
  const RecordingMoveAnalyzer& analyzer,
  const RecordingMutationPlanner& planner)
  : analyzer(analyzer),
    planner(planner)
{
}

RecordingMovePreflightResult RecordingMovePreflightService::preview(
  const std::string& recordingFile,
  const std::string& targetFile,
  const RecordingMutationPolicy& policy) const
{
  RecordingMovePreflightResult result;
  const RecordingMutationAnalysis analysis = analyzer.analyze(recordingFile, targetFile);
  const RecordingMutationPlan plan = planner.buildMovePlan(analysis, policy);
  appendMoveAnalysisAndPlan(result, analysis, plan);
  return result;
}
