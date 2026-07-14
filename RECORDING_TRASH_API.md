# Safe Native Recording Trash API

This document describes the recording trash workflow implemented by the RESTfulAPI plugin.

The implementation deliberately follows native VDR behavior. It does not introduce a separate long-lived trash store and it does not automatically stop playback, stop recordings, delete timers, or cancel VDR handler operations.

## Native VDR semantics

A successful trash operation normally changes the recording directory from:

```text
/path/to/recording.rec
```

to:

```text
/path/to/recording.del
```

VDR may permanently remove the `.del` directory later during its regular cleanup. Therefore, `already-trashed` is only a temporary idempotent state while the native `.del` directory still exists. Once both `.rec` and `.del` are gone, the recording is no longer available.

## Workflow

Recording trash uses a three-step optimistic-locking workflow:

1. Preview the operation and obtain state fingerprints.
2. Optionally validate that the fingerprints are still current.
3. Execute the operation with the confirmed fingerprints.

Preview and validation are read-only. They do not modify the recording, timers, playback state, or VDR recording lists.

## Preview

```http
POST /recordings/trash/preview.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec"
}
```

Executable response:

```json
{
  "executable": true,
  "recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "constraints": [],
  "blockers": [],
  "warnings": [],
  "steps": [
    "trash-recording",
    "refresh-recordings",
    "notify-change"
  ],
  "revision_recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "revision_recordings_state": 123456789,
  "revision_timers_state": 987654321
}
```

Blocked response example:

```json
{
  "executable": false,
  "recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "constraints": [
    "local-timer-active"
  ],
  "blockers": [
    "local-timer-active"
  ],
  "warnings": [],
  "steps": [],
  "revision_recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "revision_recordings_state": 123456789,
  "revision_timers_state": 987654321
}
```

## Validate

```http
POST /recordings/trash/validate.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "revision_recordings_state": "123456789",
  "revision_timers_state": "987654321"
}
```

Validation status values:

- `ready`: the recording is still executable with this revision.
- `conflict`: recording, replay, handler, or timer state changed after preview.
- `blocked`: the current state or policy does not allow execution.

## Execute

```http
POST /recordings/trash.json
Content-Type: application/json
```

Request:

```json
{
  "file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "revision_recordings_state": "123456789",
  "revision_timers_state": "987654321"
}
```

Successful response:

```json
{
  "status": "trashed",
  "recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "deleted_recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.del",
  "message": "Recording moved to the VDR trash."
}
```

Immediate idempotent retry while `.del` still exists:

```json
{
  "status": "already-trashed",
  "recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.rec",
  "deleted_recording_file": "/srv/vdr/video/Example/2026-07-14.08.53.3-0.del",
  "message": "Recording is already present in the VDR trash."
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

The execution endpoint does not automatically resolve these states.

## HTTP status codes

| Status | Meaning |
|---|---|
| `200` | The recording was trashed or is already in VDR's native deleted state. |
| `400` | The recording file or required revision values are missing or invalid. |
| `404` | The recording is no longer present. |
| `409` | Recording or timer state changed after preview. |
| `423` | The operation is blocked by the current VDR state or policy. |
| `500` | The native VDR mutation or postcondition verification failed. |
| `501` | The requested HTTP method is not supported. |

## Verified integration behavior

The following cases were verified against a running VDR installation:

- normal completed recording: `200 trashed`
- immediate identical retry: `200 already-trashed`
- active local recording: `423`, recording and timer remain active
- active replay: `423`, recording remains present and replay is not stopped
- stale revision: `409`
- missing revision: `400`
- unsupported method: `501`
- preview: no filesystem mutation
- successful execution: `.rec` becomes `.del` and VDR remains active

## Design boundary

This API models native VDR deletion. It is not a separate VDR-Suite recycle bin and does not guarantee a restore window. VDR controls when native `.del` recordings are permanently cleaned up.
