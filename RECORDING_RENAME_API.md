# Safe Native Recording Rename API

This document describes the safe recording rename workflow implemented by the RESTfulAPI plugin.

The implementation follows native VDR behavior. Rename changes only the title-directory component of a recording identity, preserves the timestamp `.rec` directory, reuses the native recording move path internally, updates the VDR recording list, and runs the native `vdr-recordingaction rename` hook.

It does not stop playback, stop recordings, modify timers, cancel recording handler operations, or bypass VDR recording list updates.

## Workflow

Recording rename uses a three-step optimistic-locking workflow:

1. Preview the requested name and obtain state fingerprints.
2. Optionally validate that the fingerprints are still current.
3. Execute the rename with the confirmed fingerprints.

Preview and validation are read-only. They do not modify the recording, timers, playback state, filesystem, or VDR recording lists.

## Name and path contract

The request contains:

- `file`: complete native VDR recording identity ending in `.rec`
- `name`: one new title-directory name

Example source:

```text
/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec
```

Example requested name:

```text
New_Title
```

Calculated target:

```text
/srv/vdr/video/Folder/New_Title/2026-07-08.21.45.2-0.rec
```

The timestamp `.rec` directory remains unchanged. The rename planner only replaces the title-directory component directly above it.

The requested name:

- must not be empty
- must not be `.` or `..`
- must not contain `/` or `\`
- must not contain control characters
- must not resolve to the current source identity

The calculated target must not already exist in the filesystem or VDR recording list.

## Preview

```http
POST /recordings/rename/preview.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec",
  "name": "New_Title"
}
```

Executable response:

```json
{
  "executable": true,
  "recording_file": "/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec",
  "name": "New_Title",
  "target_file": "/srv/vdr/video/Folder/New_Title/2026-07-08.21.45.2-0.rec",
  "rename_status": "ready",
  "constraints": [],
  "blockers": [],
  "warnings": [],
  "steps": [
    "move-recording",
    "refresh-recordings",
    "notify-change"
  ],
  "revision_recording_file": "/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec",
  "revision_target_file": "/srv/vdr/video/Folder/New_Title/2026-07-08.21.45.2-0.rec",
  "revision_recordings_state": 123456789,
  "revision_timers_state": 987654321
}
```

Rename-specific constraints include:

- `rename-source-invalid`
- `rename-name-missing`
- `rename-name-invalid`
- `rename-target-same-as-source`

## Validate

```http
POST /recordings/rename/validate.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec",
  "name": "New_Title",
  "revision_recordings_state": "123456789",
  "revision_timers_state": "987654321"
}
```

Validation status values:

- `ready`: calculated target and current VDR state still match the preview revision
- `conflict`: source, target, recording state, timer state, replay state, or handler state changed after preview
- `blocked`: the current state or policy does not allow execution

## Execute

```http
POST /recordings/rename.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec",
  "name": "New_Title",
  "revision_recordings_state": "123456789",
  "revision_timers_state": "987654321"
}
```

Successful response:

```json
{
  "status": "renamed",
  "recording_file": "/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec",
  "name": "New_Title",
  "target_file": "/srv/vdr/video/Folder/New_Title/2026-07-08.21.45.2-0.rec",
  "message": "Recording renamed to the requested name."
}
```

Idempotent retry after the source has already been renamed to the requested target:

```json
{
  "status": "already-renamed",
  "recording_file": "/srv/vdr/video/Folder/Old_Title/2026-07-08.21.45.2-0.rec",
  "name": "New_Title",
  "target_file": "/srv/vdr/video/Folder/New_Title/2026-07-08.21.45.2-0.rec",
  "message": "Recording already has the requested name."
}
```

## Protected states

The operation is blocked when safety cannot be established or VDR is actively using the recording:

- `replay-active`
- `recording-handler-busy`
- `local-timer-active`
- `remote-timer-active`
- `unknown-recording-handler-state`
- `unknown-local-timer-state`
- `unknown-remote-timer-state`
- `unknown-search-timer-state`
- target already exists

The execution endpoint does not automatically resolve these states.

## Execution behavior

The executor:

1. Rebuilds the rename target from `file` and `name`.
2. Revalidates the optimistic-locking revision.
3. Acquires VDR timer and recording write locks.
4. Rechecks source, target, replay, handler, recording control, and timer association.
5. Uses the native VDR directory move helper.
6. Verifies that the source disappeared and the target exists.
7. Replaces the old identity with the new identity in the VDR recording list.
8. Runs the native `vdr-recordingaction rename` hook.
9. Forces video disk usage refresh and recording change notification.

## HTTP status codes

| Status | Meaning |
|---|---|
| `200` | The recording was renamed, is already renamed, or validation returned a structured status. |
| `400` | Source, name, or required revision values are missing or invalid. |
| `404` | The source recording disappeared before execution. |
| `409` | Source, target, recording, replay, handler, or timer state changed after preview. |
| `423` | The operation is blocked by the current VDR state or policy. |
| `500` | The native VDR mutation or postcondition verification failed. |
| `501` | The requested HTTP method is not supported. |

## Verified integration behavior

The following cases were verified against a running VDR installation on July 15, 2026:

- focused rename plan and preflight tests passed
- all move regression tests passed
- complete plugin build passed
- preview: `HTTP 200`, executable, no filesystem mutation
- invalid name: `HTTP 200`, blocked with `rename-name-invalid`
- unsupported method: `HTTP 501`
- validate: `HTTP 200`, `status: ready`
- missing revision: `HTTP 400`
- stale revision: `HTTP 200`, `status: conflict`
- completed 285 MB recording: `HTTP 200`, `status: renamed`
- source directory removed and target directory present with unchanged size
- old VDR recording identity absent and new identity present
- native directory creation and rename logged by VDR
- native `vdr-recordingaction rename` hook executed
- immediate identical retry: `HTTP 200`, `status: already-renamed`
- reverse rename through the same preview and execute workflow: `HTTP 200`, `status: renamed`
- final filesystem and VDR recording list restored to the original identity

## Design boundary

This API renames one recording title directory while preserving the native timestamp `.rec` directory. It is not a generic file manager, does not copy recordings, does not accept arbitrary target paths, and does not silently stop active VDR operations.
