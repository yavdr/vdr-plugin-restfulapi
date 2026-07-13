#ifndef __RECORDINGANALYSIS_H
#define __RECORDINGANALYSIS_H

#include "recordingmutation.h"

#include <string>

struct RecordingLookupResult
{
  bool found = false;
  std::string recordingFile;
};

struct RecordingHandlerLookupResult
{
  bool known = false;
  bool busy = false;
};

struct RecordingLocalTimerLookupResult
{
  bool known = false;
  bool active = false;
};

struct RecordingRemoteTimerLookupResult
{
  bool known = false;
  bool active = false;
  int timerId = 0;
  std::string remote;
};

struct RecordingSearchTimerLookupResult
{
  bool known = false;
  bool searchTimerRecording = false;
  int searchTimerId = -1;
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

class IRecordingHandlerLookup
{
public:
  virtual ~IRecordingHandlerLookup() = default;
  virtual RecordingHandlerLookupResult getUsage(const std::string& recordingFile) const = 0;
};

class IRecordingLocalTimerLookup
{
public:
  virtual ~IRecordingLocalTimerLookup() = default;
  virtual RecordingLocalTimerLookupResult findActive(const std::string& recordingFile) const = 0;
};

class IRecordingRemoteTimerLookup
{
public:
  virtual ~IRecordingRemoteTimerLookup() = default;
  virtual RecordingRemoteTimerLookupResult findActive(const std::string& recordingFile) const = 0;
};

class IRecordingSearchTimerLookup
{
public:
  virtual ~IRecordingSearchTimerLookup() = default;
  virtual RecordingSearchTimerLookupResult findOrigin(const std::string& recordingFile) const = 0;
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

class VdrRecordingHandlerLookup : public IRecordingHandlerLookup
{
public:
  RecordingHandlerLookupResult getUsage(const std::string& recordingFile) const override;
};

class VdrRecordingLocalTimerLookup : public IRecordingLocalTimerLookup
{
public:
  RecordingLocalTimerLookupResult findActive(const std::string& recordingFile) const override;
};

class VdrRecordingRemoteTimerLookup : public IRecordingRemoteTimerLookup
{
public:
  RecordingRemoteTimerLookupResult findActive(const std::string& recordingFile) const override;
};

class VdrRecordingSearchTimerLookup : public IRecordingSearchTimerLookup
{
public:
  RecordingSearchTimerLookupResult findOrigin(const std::string& recordingFile) const override;
};

class RecordingTrashAnalyzer
{
public:
  RecordingTrashAnalyzer(
    const IRecordingLookup& recordingLookup,
    const IRecordingReplayLookup& replayLookup,
    const IRecordingHandlerLookup& recordingHandlerLookup,
    const IRecordingLocalTimerLookup& localTimerLookup,
    const IRecordingRemoteTimerLookup& remoteTimerLookup,
    const IRecordingSearchTimerLookup& searchTimerLookup);

  RecordingMutationAnalysis analyze(const std::string& recordingFile) const;

private:
  const IRecordingLookup& recordingLookup;
  const IRecordingReplayLookup& replayLookup;
  const IRecordingHandlerLookup& recordingHandlerLookup;
  const IRecordingLocalTimerLookup& localTimerLookup;
  const IRecordingRemoteTimerLookup& remoteTimerLookup;
  const IRecordingSearchTimerLookup& searchTimerLookup;
};

#endif
