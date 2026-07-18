#ifndef __RECORDINGMOVEEXECUTOR_H
#define __RECORDINGMOVEEXECUTOR_H

#include "recordingmoveexecution.h"

#include <string>

enum class RecordingMoveExecutorStatus
{
  Moved,
  AlreadyMoved,
  Conflict,
  Blocked,
  NotFound,
  Failed
};

struct RecordingMoveExecutorResult
{
  RecordingMoveExecutorStatus status = RecordingMoveExecutorStatus::Failed;
  std::string recordingFile;
  std::string targetFile;
  std::string message;
  RecordingMoveExecutionGateResult gate;
};

class RecordingMoveExecutor
{
public:
  explicit RecordingMoveExecutor(const RecordingMoveExecutionGate& gate);

  RecordingMoveExecutorResult executeNormalCase(
    const std::string& recordingFile,
    const std::string& targetFile,
    const RecordingMutationRevision& expectedRevision,
    const RecordingMutationPolicy& policy) const;

private:
  const RecordingMoveExecutionGate& gate;
};

const char* RecordingMoveExecutorStatusName(RecordingMoveExecutorStatus status);

#endif
