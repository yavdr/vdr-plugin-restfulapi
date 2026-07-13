#include "recordingmutation.h"

#include <algorithm>

bool RecordingMutationAnalysis::hasConstraint(RecordingConstraint constraint) const
{
  return std::find(constraints.begin(), constraints.end(), constraint) != constraints.end();
}

namespace {

void addBlocker(RecordingMutationPlan& plan, RecordingConstraint constraint)
{
  if (std::find(plan.blockers.begin(), plan.blockers.end(), constraint) == plan.blockers.end())
    plan.blockers.push_back(constraint);
}

void addStep(RecordingMutationPlan& plan, RecordingMutationStep step)
{
  if (std::find(plan.steps.begin(), plan.steps.end(), step) == plan.steps.end())
    plan.steps.push_back(step);
}

}

RecordingMutationPlan RecordingMutationPlanner::buildTrashPlan(
  const RecordingMutationAnalysis& analysis,
  const RecordingMutationPolicy& policy) const
{
  RecordingMutationPlan plan;
  plan.type = RecordingMutationType::Trash;
  plan.warnings = analysis.warnings;
  plan.expectedRevision = analysis.revision;

  if (analysis.type != RecordingMutationType::Trash)
    addBlocker(plan, RecordingConstraint::RecordingMissing);

  if (analysis.hasConstraint(RecordingConstraint::RecordingMissing))
    addBlocker(plan, RecordingConstraint::RecordingMissing);

  if (analysis.hasConstraint(RecordingConstraint::UnknownTimerState))
    addBlocker(plan, RecordingConstraint::UnknownTimerState);

  if (analysis.hasConstraint(RecordingConstraint::UnknownSearchTimerState))
    addBlocker(plan, RecordingConstraint::UnknownSearchTimerState);

  if (analysis.hasConstraint(RecordingConstraint::RecordingHandlerBusy)) {
    if (policy.allowRecordingHandlerStop)
      addStep(plan, RecordingMutationStep::StopRecordingHandler);
    else
      addBlocker(plan, RecordingConstraint::RecordingHandlerBusy);
  }

  if (analysis.hasConstraint(RecordingConstraint::ReplayActive)) {
    if (policy.allowReplayStop)
      addStep(plan, RecordingMutationStep::StopReplay);
    else
      addBlocker(plan, RecordingConstraint::ReplayActive);
  }

  if (analysis.hasConstraint(RecordingConstraint::LocalTimerActive)) {
    if (policy.allowLocalTimerStop)
      addStep(plan, RecordingMutationStep::DeactivateLocalTimer);
    else
      addBlocker(plan, RecordingConstraint::LocalTimerActive);
  }

  if (analysis.hasConstraint(RecordingConstraint::RemoteTimerActive)) {
    if (policy.allowRemoteTimerStop)
      addStep(plan, RecordingMutationStep::DeactivateRemoteTimer);
    else
      addBlocker(plan, RecordingConstraint::RemoteTimerActive);
  }

  if (analysis.hasConstraint(RecordingConstraint::SearchTimerRecording))
    plan.warnings.push_back("EPGSearch may classify an interrupted recording as incomplete.");

  if (plan.blockers.empty()) {
    addStep(plan, RecordingMutationStep::TrashRecording);
    addStep(plan, RecordingMutationStep::RefreshRecordings);
    addStep(plan, RecordingMutationStep::NotifyChange);
    plan.executable = true;
  }

  return plan;
}

const char* RecordingConstraintName(RecordingConstraint constraint)
{
  switch (constraint) {
    case RecordingConstraint::RecordingMissing: return "recording-missing";
    case RecordingConstraint::RecordingHandlerBusy: return "recording-handler-busy";
    case RecordingConstraint::ReplayActive: return "replay-active";
    case RecordingConstraint::LocalTimerActive: return "local-timer-active";
    case RecordingConstraint::RemoteTimerActive: return "remote-timer-active";
    case RecordingConstraint::SearchTimerRecording: return "searchtimer-recording";
    case RecordingConstraint::UnknownTimerState: return "unknown-timer-state";
    case RecordingConstraint::UnknownSearchTimerState: return "unknown-searchtimer-state";
  }
  return "unknown";
}

const char* RecordingMutationStepName(RecordingMutationStep step)
{
  switch (step) {
    case RecordingMutationStep::StopRecordingHandler: return "stop-recording-handler";
    case RecordingMutationStep::StopReplay: return "stop-replay";
    case RecordingMutationStep::DeactivateLocalTimer: return "deactivate-local-timer";
    case RecordingMutationStep::DeactivateRemoteTimer: return "deactivate-remote-timer";
    case RecordingMutationStep::TrashRecording: return "trash-recording";
    case RecordingMutationStep::RestoreRecording: return "restore-recording";
    case RecordingMutationStep::PurgeRecording: return "purge-recording";
    case RecordingMutationStep::RefreshRecordings: return "refresh-recordings";
    case RecordingMutationStep::NotifyChange: return "notify-change";
  }
  return "unknown";
}
