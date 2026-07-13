#ifndef __RECORDINGANALYSIS_H
#define __RECORDINGANALYSIS_H

#include "recordingmutation.h"

#include <string>

struct RecordingLookupResult
{
  bool found = false;
  std::string recordingFile;
};

class IRecordingLookup
{
public:
  virtual ~IRecordingLookup() = default;
  virtual RecordingLookupResult find(const std::string& recordingFile) const = 0;
};

class IRecordingReplayLookup
{
public:
  virtual ~IRecordingReplayLookup() = default;
  virtual bool isReplaying(const std::string& recordingFile) const = 0;
};

class VdrRecordingLookup : public IRecordingLookup
{
public:
  RecordingLookupResult find(const std::string& recordingFile) const override;
};

class VdrRecordingReplayLookup : public IRecordingReplayLookup
{
public:
  bool isReplaying(const std::string& recordingFile) const override;
};

class RecordingTrashAnalyzer
{
public:
  RecordingTrashAnalyzer(
    const IRecordingLookup& recordingLookup,
    const IRecordingReplayLookup& replayLookup);

  RecordingMutationAnalysis analyze(const std::string& recordingFile) const;

private:
  const IRecordingLookup& recordingLookup;
  const IRecordingReplayLookup& replayLookup;
};

#endif
