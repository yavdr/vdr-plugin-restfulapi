#ifndef __RECORDINGMUTATION_H
#define __RECORDINGMUTATION_H

#include <string>
#include <vector>

enum class RecordingMutationType
{
  Trash,
  Restore,
  Purge,
  Move,
  Rename
};

enum class RecordingConstraint
{
  RecordingMissing,
  RecordingHandlerBusy,
  ReplayActive,
  LocalTimerActive,
  RemoteTimerActive,
  SearchTimerRecording,
  UnknownRecordingHandlerState,
  UnknownLocalTimerState,
  UnknownRemoteTimerState,
  UnknownSearchTimerState
};

enum class RecordingMutationStep
{
  StopRecordingHandler,
  StopReplay,
  DeactivateLocalTimer,
  DeactivateRemoteTimer,
  TrashRecording,
  RestoreRecording,
  PurgeRecording,
  RefreshRecordings,
  NotifyChange
};

struct RecordingMutationRevision
{
  std::string recordingFile;
  long long recordingsState = 0;
  long long timersState = 0;
};

struct RecordingMutationAnalysis
{
  RecordingMutationType type = RecordingMutationType::Trash;
  std::string recordingFile;
  std::vector<RecordingConstraint> constraints;
  std::vector<std::string> warnings;
  RecordingMutationRevision revision;

  bool hasConstraint(RecordingConstraint constraint) const;
};

struct RecordingMutationPolicy
{
  bool allowRecordingHandlerStop = false;
  bool allowReplayStop = false;
  bool allowLocalTimerStop = false;
  bool allowRemoteTimerStop = false;
};

struct RecordingMutationPlan
{
  RecordingMutationType type = RecordingMutationType::Trash;
  bool executable = false;
  std::vector<RecordingMutationStep> steps;
  std::vector<RecordingConstraint> blockers;
  std::vector<std::string> warnings;
  RecordingMutationRevision expectedRevision;
};

class RecordingMutationPlanner
{
public:
  RecordingMutationPlan buildTrashPlan(
    const RecordingMutationAnalysis& analysis,
    const RecordingMutationPolicy& policy) const;
};

const char* RecordingConstraintName(RecordingConstraint constraint);
const char* RecordingMutationStepName(RecordingMutationStep step);

#endif
