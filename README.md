# Real-Time Traffic Light Control System

A multi-process Linux application that simulates and controls a 4-way (N/S/E/W) signalised intersection in real time.  
Built for **ENCS4330 - Real-Time Applications & Embedded Systems** at Birzeit University.

## Project Summary

ENCS4330 Project #2 - Real-time 4-way traffic light controller in C using System V IPC:

- Shared memory
- Semaphores
- Message queues
- `mmap` config loading
- OpenGL/GLUT visualization
- 10 cooperating Linux processes
- FSM-driven controller with safety invariants

## What it demonstrates

- **All four IPC techniques** taught in class, each where it naturally fits:
  - **Shared memory** (`shmget`) for the canonical `IntersectionState`.
  - **Semaphores** (`semget`, `SEM_UNDO`) for mutex + event signalling.
  - **Message queues** (`msgget`) with `mtype` routing for discrete events.
  - **mmap** (`MAP_PRIVATE`) for reading the configuration file.
- A **finite-state machine** controller that switches phases under enforced safety invariants:
  - No conflicting greens
  - No skipped yellow
  - Pedestrian-vehicle isolation
  - Emergency pre-emption
- **10 cooperating processes** launched and supervised by a single parent:
  - Controller
  - 4 lights
  - 3 sensors (vehicle / pedestrian / emergency)
  - Logger
  - GUI
- An **OpenGL / GLUT** visualization that reads shared state read-only and never blocks the control path.
- **Zero hard-coded values** — every timing constant, deadline, and arrival rate lives in `config.txt`.
- **Clean shutdown** under `SIGINT` and under `kill -9` of any child:
  - `SEM_UNDO` releases stuck mutexes.
  - Launcher reaps all SysV IPC objects via `ipcrm`-equivalent calls.

## Stack

C99 - GCC - Linux SysV IPC - POSIX signals - OpenGL/GLUT - make.

## Status

Skeleton complete and running. Member-specific features (lights, sensors, GUI, QA harness) are being filled in on parallel branches per `TEAM_DIVISION.pdf`.

## Build & run

```bash
make
./traffic_system ./config.txt
```

## Authors

4-person team. Equal-effort split documented in `TEAM_DIVISION.pdf`.

## License

Educational use - Birzeit University coursework.

## Suggested GitHub topics

```text
c linux ipc shared-memory semaphores message-queues mmap finite-state-machine real-time opengl glut multi-process traffic-light encs4330 birzeit-university systems-programming
```

## GitHub "About" sidebar settings

- Website: leave empty (or your Birzeit student page)
- Include in the home page:
  - ✅ Releases
  - ✅ Packages off
  - ✅ Deployments off
