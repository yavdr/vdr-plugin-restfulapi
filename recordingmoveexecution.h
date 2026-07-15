#ifndef __RECORDINGMOVEEXECUTION_H
#define __RECORDINGMOVEEXECUTION_H

#include "recordinganalysis.h"
#include "recordingmutation.h"

#include <string>
#include <vector>

enum class RecordingMoveExecutionGateStatus
{
  Ready,
  Conflict,
  Blocked
};

struct RecordingMoveExecutionGateResult
{
  RecordingMoveExecutionGateStatus status = RecordingMoveExecutionGateStatus::Blocked;
  std::string recordingFile;
  std::string targetFile;
  RecordingMutationRevision expectedRevision;
  RecordingMutationRevision currentRevision;
  std::vector<RecordingConstraint> blockers;
  std::vector<std::string> warnings;
};

class RecordingMoveExecutionGate
{
public:
  RecordingMoveExecutionGate(
    const RecordingMoveAnalyzer& analyzer,
    const RecordingMutationPlanner& planner);

  RecordingMoveExecutionGateResult validate(
    const std::string& recordingFile,
    const std::string& targetFile,
    const RecordingMutationRevision& expectedRevision,
    const RecordingMutationPolicy& policy) const;

private:
  const RecordingMoveAnalyzer& analyzer;
  const RecordingMutationPlanner& planner;
};

const char* RecordingMoveExecutionGateStatusName(
  RecordingMoveExecutionGateStatus status);

#endif
