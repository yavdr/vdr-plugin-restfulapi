#include "recordingrenameplan.h"

#include <algorithm>
#include <cctype>

namespace {

std::string trim(const std::string& value)
{
  const auto first = std::find_if_not(
    value.begin(),
    value.end(),
    [](unsigned char character) {
      return std::isspace(character) != 0;
    });

  if (first == value.end())
    return std::string();

  const auto last = std::find_if_not(
    value.rbegin(),
    value.rend(),
    [](unsigned char character) {
      return std::isspace(character) != 0;
    }).base();

  return std::string(first, last);
}

bool hasInvalidNameCharacter(const std::string& value)
{
  for (const unsigned char character : value) {
    if (character == '/' || character == '\\' || character < 0x20 ||
        character == 0x7f)
      return true;
  }

  return false;
}

bool endsWith(const std::string& value, const std::string& suffix)
{
  return value.size() >= suffix.size() &&
    value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}

RecordingRenamePlan RecordingRenamePlanner::build(
  const std::string& recordingFile,
  const std::string& requestedName) const
{
  RecordingRenamePlan plan;
  plan.recordingFile = recordingFile;
  plan.requestedName = trim(requestedName);

  if (recordingFile.empty() || recordingFile.front() != '/' ||
      !endsWith(recordingFile, ".rec")) {
    plan.status = RecordingRenamePlanStatus::SourceInvalid;
    return plan;
  }

  if (plan.requestedName.empty()) {
    plan.status = RecordingRenamePlanStatus::NameMissing;
    return plan;
  }

  if (plan.requestedName == "." || plan.requestedName == ".." ||
      hasInvalidNameCharacter(plan.requestedName)) {
    plan.status = RecordingRenamePlanStatus::NameInvalid;
    return plan;
  }

  const std::string::size_type timestampSeparator = recordingFile.rfind('/');
  if (timestampSeparator == std::string::npos || timestampSeparator == 0 ||
      timestampSeparator + 1 >= recordingFile.size()) {
    plan.status = RecordingRenamePlanStatus::SourceInvalid;
    return plan;
  }

  const std::string timestampDirectory =
    recordingFile.substr(timestampSeparator + 1);
  const std::string titlePath = recordingFile.substr(0, timestampSeparator);
  const std::string::size_type titleSeparator = titlePath.rfind('/');

  if (titleSeparator == std::string::npos) {
    plan.status = RecordingRenamePlanStatus::SourceInvalid;
    return plan;
  }

  const std::string parentPath = titlePath.substr(0, titleSeparator);
  if (parentPath.empty()) {
    plan.targetFile = "/" + plan.requestedName + "/" + timestampDirectory;
  }
  else {
    plan.targetFile =
      parentPath + "/" + plan.requestedName + "/" + timestampDirectory;
  }

  if (plan.targetFile == recordingFile) {
    plan.status = RecordingRenamePlanStatus::TargetSameAsSource;
    return plan;
  }

  plan.status = RecordingRenamePlanStatus::Ready;
  return plan;
}

const char* RecordingRenamePlanStatusName(
  RecordingRenamePlanStatus status)
{
  switch (status) {
    case RecordingRenamePlanStatus::Ready:
      return "ready";
    case RecordingRenamePlanStatus::SourceInvalid:
      return "source-invalid";
    case RecordingRenamePlanStatus::NameMissing:
      return "name-missing";
    case RecordingRenamePlanStatus::NameInvalid:
      return "name-invalid";
    case RecordingRenamePlanStatus::TargetSameAsSource:
      return "target-same-as-source";
  }

  return "unknown";
}
