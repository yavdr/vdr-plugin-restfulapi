#ifndef __RECORDINGRENAMEPREFLIGHT_H
#define __RECORDINGRENAMEPREFLIGHT_H

#include "recordingpreflight.h"
#include "recordingrenameplan.h"

#include <string>
#include <vector>

struct RecordingRenamePreflightResult
{
  bool executable = false;
  RecordingRenamePlanStatus renameStatus = RecordingRenamePlanStatus::SourceInvalid;
  std::string recordingFile;
  std::string requestedName;
  std::string targetFile;
  std::vector<std::string> constraints;
  std::vector<std::string> blockers;
  std::vector<std::string> warnings;
  std::vector<std::string> steps;
  RecordingMutationRevision revision;
};

class RecordingRenamePreflightService
{
public:
  RecordingRenamePreflightService(
    const RecordingRenamePlanner& renamePlanner,
    const RecordingMoveAnalyzer& moveAnalyzer,
    const RecordingMutationPlanner& mutationPlanner);

  RecordingRenamePreflightResult preview(
    const std::string& recordingFile,
    const std::string& requestedName,
    const RecordingMutationPolicy& policy) const;

private:
  const RecordingRenamePlanner& renamePlanner;
  const RecordingMoveAnalyzer& moveAnalyzer;
  const RecordingMutationPlanner& mutationPlanner;
};

#endif
