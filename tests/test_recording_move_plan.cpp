#include "../recordingmutation.h"

#include <cassert>

namespace {

RecordingMutationAnalysis baseMoveAnalysis()
{
  RecordingMutationAnalysis analysis;
  analysis.type = RecordingMutationType::Move;
  analysis.recordingFile = "/srv/vdr/video/Source/2026-07-14.20.15.1-0.rec";
  analysis.targetFile = "/srv/vdr/video/Target/2026-07-14.20.15.1-0.rec";
  analysis.revision.recordingFile = analysis.recordingFile;
  analysis.revision.targetFile = analysis.targetFile;
  analysis.revision.recordingsState = 100;
  analysis.revision.timersState = 200;
  return analysis;
}

bool hasBlocker(
  const RecordingMutationPlan& plan,
  RecordingConstraint constraint)
{
  for (const RecordingConstraint blocker : plan.blockers) {
    if (blocker == constraint)
      return true;
  }
  return false;
}

bool hasStep(
  const RecordingMutationPlan& plan,
  RecordingMutationStep step)
{
  for (const RecordingMutationStep candidate : plan.steps) {
    if (candidate == step)
      return true;
  }
  return false;
}

}

int main()
{
  RecordingMutationPlanner planner;
  RecordingMutationPolicy policy;

  const RecordingMutationPlan ready =
    planner.buildMovePlan(baseMoveAnalysis(), policy);

  assert(ready.executable);
  assert(ready.blockers.empty());
  assert(hasStep(ready, RecordingMutationStep::MoveRecording));
  assert(hasStep(ready, RecordingMutationStep::RefreshRecordings));
  assert(hasStep(ready, RecordingMutationStep::NotifyChange));
  assert(ready.expectedRevision.recordingsState == 100);
  assert(ready.expectedRevision.timersState == 200);
  assert(
    ready.expectedRevision.targetFile ==
    "/srv/vdr/video/Target/2026-07-14.20.15.1-0.rec");

  RecordingMutationAnalysis missingTarget = baseMoveAnalysis();
  missingTarget.targetFile.clear();
  const RecordingMutationPlan missingTargetPlan =
    planner.buildMovePlan(missingTarget, policy);
  assert(!missingTargetPlan.executable);
  assert(hasBlocker(
    missingTargetPlan,
    RecordingConstraint::MoveTargetMissing));

  RecordingMutationAnalysis sameTarget = baseMoveAnalysis();
  sameTarget.targetFile = sameTarget.recordingFile;
  const RecordingMutationPlan sameTargetPlan =
    planner.buildMovePlan(sameTarget, policy);
  assert(!sameTargetPlan.executable);
  assert(hasBlocker(
    sameTargetPlan,
    RecordingConstraint::MoveTargetSameAsSource));

  RecordingMutationAnalysis targetExists = baseMoveAnalysis();
  targetExists.constraints.push_back(
    RecordingConstraint::MoveTargetExists);
  const RecordingMutationPlan targetExistsPlan =
    planner.buildMovePlan(targetExists, policy);
  assert(!targetExistsPlan.executable);
  assert(hasBlocker(
    targetExistsPlan,
    RecordingConstraint::MoveTargetExists));

  RecordingMutationAnalysis activeRecording = baseMoveAnalysis();
  activeRecording.constraints.push_back(
    RecordingConstraint::LocalTimerActive);
  const RecordingMutationPlan activeRecordingPlan =
    planner.buildMovePlan(activeRecording, policy);
  assert(!activeRecordingPlan.executable);
  assert(hasBlocker(
    activeRecordingPlan,
    RecordingConstraint::LocalTimerActive));

  RecordingMutationAnalysis replaying = baseMoveAnalysis();
  replaying.constraints.push_back(
    RecordingConstraint::ReplayActive);
  const RecordingMutationPlan replayingPlan =
    planner.buildMovePlan(replaying, policy);
  assert(!replayingPlan.executable);
  assert(hasBlocker(
    replayingPlan,
    RecordingConstraint::ReplayActive));

  assert(
    std::string(RecordingConstraintName(
      RecordingConstraint::MoveTargetInvalid)) ==
    "move-target-invalid");
  assert(
    std::string(RecordingMutationStepName(
      RecordingMutationStep::MoveRecording)) ==
    "move-recording");

  return 0;
}
