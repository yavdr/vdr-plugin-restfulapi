#include "../recordingrenameplan.h"

#include <cassert>
#include <string>

int main()
{
  RecordingRenamePlanner planner;

  const std::string source =
    "/srv/vdr/video/Hutehiermorgenda/"
    "VDR-SUITE-TEST_heute_journal4/"
    "2026-07-08.21.45.2-0.rec";

  const RecordingRenamePlan ready =
    planner.build(source, "heute journal Testaufnahme");

  assert(ready.executable());
  assert(ready.status == RecordingRenamePlanStatus::Ready);
  assert(ready.recordingFile == source);
  assert(ready.requestedName == "heute journal Testaufnahme");
  assert(
    ready.targetFile ==
    "/srv/vdr/video/Hutehiermorgenda/"
    "heute journal Testaufnahme/"
    "2026-07-08.21.45.2-0.rec");

  const RecordingRenamePlan trimmed =
    planner.build(source, "  Neuer Titel  ");
  assert(trimmed.executable());
  assert(trimmed.requestedName == "Neuer Titel");

  const RecordingRenamePlan missingName = planner.build(source, "   ");
  assert(!missingName.executable());
  assert(missingName.status == RecordingRenamePlanStatus::NameMissing);

  const RecordingRenamePlan slash = planner.build(source, "Serie/Folge");
  assert(!slash.executable());
  assert(slash.status == RecordingRenamePlanStatus::NameInvalid);

  const RecordingRenamePlan backslash = planner.build(source, "Serie\\Folge");
  assert(!backslash.executable());
  assert(backslash.status == RecordingRenamePlanStatus::NameInvalid);

  const RecordingRenamePlan dot = planner.build(source, ".");
  assert(!dot.executable());
  assert(dot.status == RecordingRenamePlanStatus::NameInvalid);

  const RecordingRenamePlan dotDot = planner.build(source, "..");
  assert(!dotDot.executable());
  assert(dotDot.status == RecordingRenamePlanStatus::NameInvalid);

  const RecordingRenamePlan same =
    planner.build(source, "VDR-SUITE-TEST_heute_journal4");
  assert(!same.executable());
  assert(same.status == RecordingRenamePlanStatus::TargetSameAsSource);

  const RecordingRenamePlan relativeSource =
    planner.build("relative/2026-07-08.21.45.2-0.rec", "Neuer Titel");
  assert(!relativeSource.executable());
  assert(relativeSource.status == RecordingRenamePlanStatus::SourceInvalid);

  const RecordingRenamePlan deletedSource =
    planner.build(
      "/srv/vdr/video/Alt/2026-07-08.21.45.2-0.del",
      "Neuer Titel");
  assert(!deletedSource.executable());
  assert(deletedSource.status == RecordingRenamePlanStatus::SourceInvalid);

  assert(
    std::string(RecordingRenamePlanStatusName(
      RecordingRenamePlanStatus::NameInvalid)) ==
    "name-invalid");

  return 0;
}
