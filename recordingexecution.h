#ifndef __RECORDINGEXECUTION_H
#define __RECORDINGEXECUTION_H

#include "recordinganalysis.h"
#include "recordingmutation.h"

#include <string>
#include <vector>

enum class RecordingTrashExecutionGateStatus
{
  Ready,
  Conflict,
  Blocked
};

struct RecordingTrashExecutionGateResult
{
  RecordingTrashExecutionGateStatus status = RecordingTrashExecutionGateStatus::Blocked;
  std::string recordingFile;
  RecordingMutationRevision expectedRevision;
  RecordingMutationRevision currentRevision;
  std::vector<RecordingConstraint> blockers;
  std::vector<std::string> warnings;
};

class RecordingTrashExecutionGate
{
public:
  RecordingTrashExecutionGate(
    const RecordingTrashAnalyzer& analyzer,
    const RecordingMutationPlanner& planner);

  RecordingTrashExecutionGateResult validate(
    const std::string& recordingFile,
    const RecordingMutationRevision& expectedRevision,
    const RecordingMutationPolicy& policy) const;

private:
  const RecordingTrashAnalyzer& analyzer;
  const RecordingMutationPlanner& planner;
};

const char* RecordingTrashExecutionGateStatusName(
  RecordingTrashExecutionGateStatus status);

#endif
