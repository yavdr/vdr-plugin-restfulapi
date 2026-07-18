# Safe Native Recording Move API

This document describes the safe recording move workflow implemented by the RESTfulAPI plugin.

The implementation follows native VDR behavior. It does not stop playback, stop recordings, modify timers, cancel recording handler operations, or bypass VDR recording list updates.

## Workflow

Recording move uses a three-step optimistic-locking workflow:

1. Preview the operation and obtain state fingerprints.
2. Optionally validate that the fingerprints are still current.
3. Execute the operation with the confirmed fingerprints.

Preview and validation are read-only. They do not modify the recording, timers, playback state, filesystem, or VDR recording lists.

## Path contract

Both `file` and `target_file` are complete native VDR filesystem identities.

Example:

```text
/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec
```

The target must be absolute, must differ from the source, and must not already exist in the filesystem or VDR recording list.

## Preview

```http
POST /recordings/move/preview.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "target_file": "/srv/vdr/video/Archive/Example/2026-07-14.08.53.3-0.rec"
}
```

Executable response:

```json
{
  "executable": true,
  "recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "target_file": "/srv/vdr/video/Archive/Example/2026-07-14.08.53.3-0.rec",
  "constraints": [],
  "blockers": [],
  "warnings": [],
  "steps": [
    "move-recording",
    "refresh-recordings",
    "notify-change"
  ],
  "revision_recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "revision_target_file": "/srv/vdr/video/Archive/Example/2026-07-14.08.53.3-0.rec",
  "revision_recordings_state": 123456789,
  "revision_timers_state": 987654321
}
```

## Validate

```http
POST /recordings/move/validate.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "target_file": "/srv/vdr/video/Archive/Example/2026-07-14.08.53.3-0.rec",
  "revision_recordings_state": "123456789",
  "revision_timers_state": "987654321"
}
```

Validation status values:

- `ready`: source, target, recording state, and timer state still match the preview revision.
- `conflict`: source, target, replay, handler, or timer state changed after preview.
- `blocked`: the current state or policy does not allow execution.

## Execute

```http
POST /recordings/move.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "target_file": "/srv/vdr/video/Archive/Example/2026-07-14.08.53.3-0.rec",
  "revision_recordings_state": "123456789",
  "revision_timers_state": "987654321"
}
```

Successful response:

```json
{
  "status": "moved",
  "recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "target_file": "/srv/vdr/video/Archive/Example/2026-07-14.08.53.3-0.rec",
  "message": "Recording moved to the requested target."
}
```

Idempotent retry after the source has already been moved to the requested target:

```json
{
  "status": "already-moved",
  "recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "target_file": "/srv/vdr/video/Archive/Example/2026-07-14.08.53.3-0.rec",
  "message": "Recording is already present at the requested target."
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

Move-specific blockers include:

- `move-target-missing`
- `move-target-invalid`
- `move-target-same-as-source`
- `move-target-exists`

The execution endpoint does not automatically resolve these states.

## Execution behavior

The executor:

1. Revalidates the optimistic-locking revision.
2. Acquires VDR timer and recording write locks.
3. Rechecks source, target, replay, handler, recording control, and timer association.
4. Uses the native VDR directory move helper.
5. Verifies that the source disappeared and the target exists.
6. Replaces the source identity with the target identity in the VDR recording list.
7. Runs the native `recordingaction rename` hook.
8. Forces video disk usage refresh and recording change notification.

## HTTP status codes

| Status | Meaning |
|---|---|
| `200` | The recording was moved or is already present at the requested target. |
| `400` | Source, target, or required revision values are missing or invalid. |
| `404` | The source recording disappeared before execution. |
| `409` | Source, target, recording, replay, handler, or timer state changed after preview. |
| `423` | The operation is blocked by the current VDR state or policy. |
| `500` | The native VDR mutation or postcondition verification failed. |
| `501` | The requested HTTP method is not supported. |

## Verified integration behavior

The following cases were verified against a running VDR installation on July 15, 2026:

- preview: `200`, executable, no filesystem mutation
- validate: `200 ready`, expected and current revisions identical
- missing revision: `400`
- stale revision: `409`
- unsupported method: `501`
- completed 285 MB recording: `200 moved`
- source directory removed and target directory present
- old VDR recording identity absent and new identity present
- native directory creation and rename logged by VDR
- native `vdr-recordingaction rename` hook executed
- immediate identical retry: `200 already-moved`
- reverse move through the same preview, validate, and execute workflow: `200 moved`
- final VDR recording list restored to the original identity

## Design boundary

This API moves a recording to one explicit native VDR filesystem identity. It is not a generic file manager, does not copy recordings, and does not silently stop active VDR operations.