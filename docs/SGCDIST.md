# sgcdist

## Overview

sgcdist is a local task execution agent for the stargate build system.
It runs as a daemon on any machine reachable via SSH in a cluster and accepts
task submissions through a JSON API over a Unix domain socket.

Stargate dispatches build tasks (shell commands) to remote machines by
connecting to the sgcdist instance running on each machine. sgcdist
owns the lifecycle of every process it spawns: it tracks them, logs their
output, and ensures no orphans survive when it shuts down.


## Subcommands

### sgcdist start

Start the daemon in the background on the current machine.

```
sgcdist start [-socket <path>] [-log_dir <path>] [-max_tasks <n>]
```

- `-socket`    : path to the Unix domain socket
  (default: `$HOME/.sgcdist/sgcdist.sock`)
- `-log_dir`   : base directory for task logs
  (default: `$HOME/.sgcdist/sgcdist.out`)
- `-max_tasks` : maximum number of concurrent tasks (default: 4)

Behavior:
- Creates `$HOME/.sgcdist/` if it does not exist.
- Writes a PID file at `$HOME/.sgcdist/sgcdist.pid`.
- Creates the Unix socket with permissions `0700` and begins listening
  for API requests. Only the owning user can connect.
- Refuses to start if another instance is already running (checks PID file
  and validates the process is alive).
- Logs daemon-level messages to `$HOME/.sgcdist/sgcdist.log`.

### sgcdist stop

Stop the running daemon gracefully.

```
sgcdist stop [-socket <path>]
```

Behavior:
1. Connects to the daemon socket and sends a `shutdown` request.
2. The daemon stops accepting new tasks and drains the submission queue.
3. The daemon sends SIGTERM to all running child process groups.
4. After a grace period (5 seconds), sends SIGKILL to any survivors.
5. The daemon exits and removes the PID file and socket.

If the socket is unreachable, falls back to reading the PID file and
sending SIGTERM directly to the daemon process.

### sgcdist run

Submit a task from the command line without writing JSON by hand.

```
sgcdist run [-socket <path>] [-name <name>] [-env KEY=VAL]...
               [-working_dir <path>] "<command>"
```

- `-name`        : optional human-readable task name (default: auto-generated)
- `-env`         : additional environment variables for the task (repeatable)
- `-working_dir` : working directory for the command

Behavior:
- Connects to the daemon socket and sends a `submit` request.
- Prints the assigned task ID and exits.
- Exit code 0 on successful submission, 1 on error.

### sgcdist status

Query the status of tasks.

```
sgcdist status [-socket <path>] [-task <id>] [-all]
```

- No flags: show summary of running and queued tasks.
- `-task <id>` : show detailed status of a specific task.
- `-all` : include all finished tasks.


## Task identification

Each task is assigned a monotonically increasing integer ID by the daemon.
The counter starts at 1 when the daemon starts and is never reset during
the daemon's lifetime. On restart the daemon scans the log directory for
existing task directories and resumes the counter from the highest ID found
plus one.

Task IDs are used as directory names under the log directory
(e.g. `1/`, `2/`, `37/`).


## Unix socket JSON API

All communication happens over a Unix domain socket using newline-delimited
JSON. Each request is a single JSON object terminated by `\n`. Each response
is a single JSON object terminated by `\n`.

Every request contains a `"method"` field. Every response contains a
`"status"` field (`"ok"` or `"error"`) and, on error, an `"error"` field
with a human-readable message.

### submit

Submit a task for execution.

Request:
```json
{
    "method": "submit",
    "command": "vivado -mode batch -source synth.tcl",
    "name": "synth",
    "env": {
        "XILINX_VIVADO": "/opt/Xilinx/Vivado/2023.2"
    },
    "working_dir": "/home/user/project/build"
}
```

- `method` (string, required): `"submit"`
- `command` (string, required): shell command to execute (passed to
  `/bin/sh -c`)
- `name` (string, optional): human-readable name; auto-generated as
  `"task_<id>"` if absent
- `env` (object, optional): additional environment variables; merged on
  top of the daemon's inherited environment
- `working_dir` (string, optional): working directory for the command;
  defaults to the daemon's working directory

Response:
```json
{
    "status": "ok",
    "task_id": 7,
    "message": "task submitted"
}
```

If `max_tasks` concurrent tasks are already running, the task is placed in
a FIFO submission queue and started automatically when a slot becomes
available. The response is immediate in both cases:
```json
{
    "status": "ok",
    "task_id": 8,
    "message": "task queued"
}
```

### query

Query the status of one or all tasks.

Request:
```json
{
    "method": "query",
    "task_id": 7
}
```

- `method` (string, required): `"query"`
- `task_id` (integer, optional): if omitted, returns all tasks

Response (single task):
```json
{
    "status": "ok",
    "task": {
        "task_id": 7,
        "name": "synth",
        "command": "vivado -mode batch -source synth.tcl",
        "status": "running",
        "pid": 48712,
        "start_time": "2026-04-15T14:30:22",
        "end_time": null,
        "exit_code": null
    }
}
```

Response (all tasks):
```json
{
    "status": "ok",
    "tasks": [ ... ]
}
```

Task statuses: `"queued"`, `"running"`, `"success"`, `"failed"`,
`"killed"`.

### cancel

Cancel a running or queued task.

Request:
```json
{
    "method": "cancel",
    "task_id": 7,
    "signal": "SIGTERM"
}
```

- `method` (string, required): `"cancel"`
- `task_id` (integer, required): task to cancel
- `signal` (string, optional): signal to send, default `"SIGTERM"`;
  accepted values: `"SIGTERM"`, `"SIGKILL"`

Response:
```json
{
    "status": "ok",
    "message": "signal sent"
}
```

The `cancel` method returns immediately after sending the signal. The
SIGTERM-to-SIGKILL escalation (5 second grace period) happens
asynchronously in the daemon's event loop. If the task is queued (not yet
running), it is removed from the queue and its status is set to `"killed"`.

### shutdown

Request the daemon to shut down gracefully.

Request:
```json
{
    "method": "shutdown"
}
```

Response:
```json
{
    "status": "ok",
    "message": "shutting down"
}
```

The daemon sends the response, then initiates the shutdown sequence
described in the `stop` subcommand section.


## Process management

### Environment inheritance

Each spawned task inherits the full environment of the daemon process.
The `env` field in a submit request provides additional variables that are
merged on top: existing variables are overridden, new variables are added.
The daemon's own environment is never modified.

### Process groups

Each spawned task runs in its own process group (`setpgid(pid, pid)` after
fork). This ensures that:
- Signals can be sent to the entire process tree of a task by signaling
  the process group (`kill(-pgid, signal)`).
- Child processes of a task do not escape cleanup.

### Orphan prevention

- The daemon sets itself as a subreaper (`prctl(PR_SET_CHILD_SUBREAPER)`
  on Linux) so that grandchild processes are reparented to sgcdist
  rather than to init/PID 1.
- On macOS, where subreaper is not available, the daemon relies on process
  group signaling for cleanup.
- On shutdown or task cancellation, the daemon always signals the entire
  process group, not just the direct child PID.

### Shutdown sequence

1. Stop accepting new connections on the socket.
2. Discard all queued (not yet running) tasks, marking them as `"killed"`.
3. For each running task, send SIGTERM to its process group.
4. Wait up to 5 seconds for all children to exit.
5. Send SIGKILL to any remaining process groups.
6. Wait for all children to be reaped (blocking `waitpid`).
7. Remove the PID file and socket file.
8. Exit.

### Child reaping

The daemon installs a SIGCHLD handler (or uses a signalfd/kqueue-based
event loop) to reap terminated children promptly via `waitpid(-1, ...,
WNOHANG)`. This prevents zombie accumulation and updates task status
immediately upon child termination. When a child is reaped and the
submission queue is non-empty, the daemon dequeues the next task and
starts it.


## Task logging

All task output is logged under the log directory (configurable via
`-log_dir`, default `$HOME/.sgcdist/sgcdist.out`).

### Directory structure

```
sgcdist.out/
    1/
        command.sh
        stdout.log
        stderr.log
        status.json
    2/
        command.sh
        stdout.log
        stderr.log
        status.json
```

### Files per task

- `command.sh` : the exact command submitted, written before execution
  starts. Includes the environment variables and working directory as
  comments at the top:
  ```sh
  # task_id: 1
  # name: synth
  # working_dir: /home/user/project/build
  # env: XILINX_VIVADO=/opt/Xilinx/Vivado/2023.2
  vivado -mode batch -source synth.tcl
  ```

- `stdout.log` : standard output of the task. The child's stdout file
  descriptor is redirected directly to this file via `dup2` after fork,
  so the daemon is not in the I/O path.

- `stderr.log` : standard error of the task, same `dup2` mechanism.

- `status.json` : written at submission time with status `"queued"` or
  `"running"`, and updated when the task completes or is killed:
  ```json
  {
      "status": "success",
      "task_id": 1,
      "name": "synth",
      "command": "vivado -mode batch -source synth.tcl",
      "pid": 48712,
      "start_time": "2026-04-15T14:30:22",
      "end_time": "2026-04-15T14:45:03",
      "exit_code": 0,
      "signal": null,
      "error": ""
  }
  ```
  This format is compatible with stargate's existing `TaskStatus` JSON
  conventions. The `status` field uses the same vocabulary as the query
  API: `"queued"`, `"running"`, `"success"`, `"failed"`, `"killed"`.

### Daemon restart and state recovery

On startup the daemon scans the log directory. For each task directory that
contains a `status.json`, the daemon reads the file to recover historical
task state. Tasks whose `status.json` shows `"running"` are marked as
`"failed"` with an error of `"daemon restart"`, since the processes they
referred to no longer exist. The task ID counter resumes from the highest
existing task directory ID plus one.


## Integration with stargate

Stargate dispatches tasks to remote machines by SSH-ing into them and
communicating with the local sgcdist daemon over its Unix socket.

Typical workflow:
1. Stargate ensures sgcdist is running on the target machine
   (`ssh host sgcdist start`).
2. Stargate submits a task
   (`ssh host sgcdist run "make synth"`).
3. Stargate polls task status
   (`ssh host sgcdist status -task 7`).
4. Stargate retrieves logs from `sgcdist.out/<task_id>/` on the
   remote machine when needed.

The CLI subcommands (`run`, `status`) serve as the SSH-friendly interface
to the JSON API. Stargate does not need to construct raw JSON; it invokes
sgcdist subcommands over SSH.


## Security

The Unix domain socket is the sole entry point to the daemon. Access
control is enforced via filesystem permissions:
- The socket file is created with mode `0700`, restricting access to the
  owning user.
- The `$HOME/.sgcdist/` directory is created with mode `0700`.
- Any user who can connect to the socket can submit arbitrary shell
  commands that run as the daemon's user. There is no additional
  authentication layer; filesystem permissions are the access control
  mechanism.


## Configuration summary

| Parameter     | Flag           | Default                                |
|---------------|----------------|----------------------------------------|
| Socket path   | `-socket`      | `$HOME/.sgcdist/sgcdist.sock`    |
| Log directory | `-log_dir`     | `$HOME/.sgcdist/sgcdist.out`     |
| Max tasks     | `-max_tasks`   | 4                                      |
| PID file      | (not settable) | `$HOME/.sgcdist/sgcdist.pid`     |
| Daemon log    | (not settable) | `$HOME/.sgcdist/sgcdist.log`     |
| Grace period  | (not settable) | 5 seconds                              |


## Code architecture

sgcdist is a standalone C++ executable in `tools/sgcdist/`, linked
against `sgc_common_s` for shared utilities (FileUtils, exceptions).

```
tools/sgcdist/
    SgcDist.cpp        -- main(), argument parsing, subcommand dispatch
    Daemon.h / Daemon.cpp -- daemon lifecycle, socket listener, event loop
    TaskRunner.h / .cpp   -- fork/exec, process group management, reaping
    TaskLog.h / .cpp      -- log directory creation, stdout/stderr capture
    Protocol.h / .cpp     -- JSON request/response serialization
```

The daemon uses an event loop (epoll on Linux, kqueue on macOS) to
multiplex the listening socket, client connections, and signal events
(SIGCHLD, SIGTERM) in a single thread.
