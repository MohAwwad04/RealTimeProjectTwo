# RealTimeProjectTwo
ENCS4330 Project #2 - Real-time 4-way traffic light controller in C using
  System V IPC (shared memory, semaphores, message queues) + mmap config +
  OpenGL/GLUT visualization. 10 cooperating Linux processes, FSM-driven,
  safety-invariant enforced.

  Suggested topics/tags:

  c  linux  ipc  shared-memory  semaphores  message-queues  mmap
  finite-state-machine  real-time  opengl  glut  multi-process
  traffic-light  encs4330  birzeit-university  systems-programming

  Longer README intro (if you want one above the existing README.md):

  # Real-Time Traffic Light Control System

  A multi-process Linux application that simulates and controls a 4-way
  (N/S/E/W)
  signalised intersection in real time. Built for **ENCS4330 - Real-Time
  Applications & Embedded Systems** at Birzeit University.

  ## What it demonstrates

  - **All four IPC techniques** taught in class, each where it naturally
  fits:
    - **Shared memory** (`shmget`) for the canonical `IntersectionState`.
    - **Semaphores** (`semget`, `SEM_UNDO`) for mutex + event signalling.
    - **Message queues** (`msgget`) with `mtype` routing for discrete
  events.
    - **mmap** (`MAP_PRIVATE`) for reading the configuration file.
  - A **finite-state machine** controller that switches phases under
    enforced safety invariants (no conflicting greens, no skipped yellow,
    pedestrian-vehicle isolation, emergency pre-emption).
  - **10 cooperating processes** launched and supervised by a single
    parent: controller, 4 lights, 3 sensors (vehicle / pedestrian /
    emergency), logger, GUI.
  - An **OpenGL / GLUT** visualization that reads the shared state
    read-only and never blocks the control path.
  - **Zero hard-coded values** - every timing constant, deadline, and
    arrival rate lives in `config.txt`.
  - **Clean shutdown** under SIGINT and under `kill -9` of any child:
    `SEM_UNDO` releases stuck mutexes, the launcher reaps all SysV IPC
    with `ipcrm`-equivalent calls.

  ## Stack
  C99 - GCC - Linux SysV IPC - POSIX signals - OpenGL/GLUT - make.

  ## Status
  Skeleton complete and running. Member-specific features (lights,
  sensors, GUI, QA harness) are being filled in on parallel branches per
  TEAM_DIVISION.pdf.

  ## Build & run
  ```bash
  make
  ./traffic_system ./config.txt

  Authors

  4-person team. Equal-effort split documented in TEAM_DIVISION.pdf.

  License

  Educational use - Birzeit University coursework.

  **For the GitHub "About" sidebar settings:**
  - Website: leave empty (or your Birzeit student page)
  - Topics: paste the tag list above
  - Include in the home page: ✅ Releases, ✅ Packages off, ✅ Deployments
  off
