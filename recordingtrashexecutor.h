#ifndef __RECORDINGTRASHEXECUTOR_H
#define __RECORDINGTRASHEXECUTOR_H

#include "recordingexecution.h"

#include <string>

enum class RecordingTrashExecutorStatus
{
  Trashed,
  Conflict,
  Blocked,
  NotFound,
  Failed
};

struct RecordingTrashExecutorResult
{
  RecordingTrashExecutorStatus status = RecordingTrashExecutorStatus::Failed;
  std::string recordingFile;
  std::string deletedRecordingFile;
  std::string message;
  RecordingTrashExecutionGateResult gate;
};

class RecordingTrashExecutor
{
public:
  explicit RecordingTrashExecutor(const RecordingTrashExecutionGate& gate);

  RecordingTrashExecutorResult executeNormalCase(
    const std::string& recordingFile,
    const RecordingMutationRevision& expectedRevision,
    const RecordingMutationPolicy& policy) const;

private:
  const RecordingTrashExecutionGate& gate;
};

const char* RecordingTrashExecutorStatusName(RecordingTrashExecutorStatus status);

#endif
