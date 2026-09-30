#ifndef __RECORDINGRENAMEPLAN_H
#define __RECORDINGRENAMEPLAN_H

#include <string>

enum class RecordingRenamePlanStatus
{
  Ready,
  SourceInvalid,
  NameMissing,
  NameInvalid,
  TargetSameAsSource
};

struct RecordingRenamePlan
{
  RecordingRenamePlanStatus status = RecordingRenamePlanStatus::SourceInvalid;
  std::string recordingFile;
  std::string requestedName;
  std::string targetFile;

  bool executable() const
  {
    return status == RecordingRenamePlanStatus::Ready;
  }
};

class RecordingRenamePlanner
{
public:
  RecordingRenamePlan build(
    const std::string& recordingFile,
    const std::string& requestedName) const;
};

const char* RecordingRenamePlanStatusName(
  RecordingRenamePlanStatus status);

#endif
