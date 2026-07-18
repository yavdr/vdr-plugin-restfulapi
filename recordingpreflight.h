#ifndef __RECORDINGPREFLIGHT_H
#define __RECORDINGPREFLIGHT_H

#include "recordinganalysis.h"
#include "recordingmutation.h"

#include <string>
#include <vector>

struct RecordingTrashPreflightResult
{
  bool executable = false;
  std::string recordingFile;
  std::vector<std::string> constraints;
  std::vector<std::string> blockers;
  std::vector<std::string> warnings;
  std::vector<std::string> steps;
  RecordingMutationRevision revision;
};

class RecordingTrashPreflightService
{
public:
  RecordingTrashPreflightService(
    const RecordingTrashAnalyzer& analyzer,
    const RecordingMutationPlanner& planner);

  RecordingTrashPreflightResult preview(
    const std::string& recordingFile,
    const RecordingMutationPolicy& policy) const;

private:
  const RecordingTrashAnalyzer& analyzer;
  const RecordingMutationPlanner& planner;
};

#endif
