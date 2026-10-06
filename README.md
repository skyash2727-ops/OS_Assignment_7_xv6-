# OS_Assignment_7_xv6-
# XV6 Synchronization, Deadlock Detection & Priority Scheduling Report

## Executive Summary

This report presents the implementation, architectural design decisions, and empirical evaluation for four core OS enhancements in **xv6**:

1. **Q1: Shared Memory and Synchronization (Counting Semaphores)**
2. **Q2: Race Condition Analysis and Mitigation in Shared Buffer Operations**
3. **Q3: Deadlock Detection & Banker's Algorithm Implementation**
4. **Q4: Priority-Based CPU Scheduler Evaluation**

---

## Part 1: Design Decisions & Key Data Structures

### Q1 & Q2: Counting Semaphores & Synchronization

To provide synchronized inter-process communication in xv6 without busy-waiting, counting semaphores were implemented directly within the kernel space using spinlocks and process sleep queues.

#### Key Data Structures

```c
// kernel/sem.h / kernel/sem.c
struct sem {
  int value;          // Current semaphore counter value
  struct spinlock lock; // Spinlock protecting counter modifications
};

```

#### Design Decisions

* **`sem_init(s, val)`**: Initializes the counter `value` to `val` and initializes its spinlock.
* **`sem_wait(s)`**: Decrements the counter. If `value < 0`, the calling process acquires the process lock and sleeps on the address of `s` via `sleep(s, &s->lock)`.
* **`sem_signal(s)`**: Increments the counter. If `value <= 0`, it wakes up one process waiting on `s` using `wakeup(s)`.
* **Bounded Buffer Producer-Consumer**: Constructed using three semaphores:
* `mutex` (initialized to `1` for mutual exclusion).
* `empty` (initialized to buffer size $N$ for empty slots).
* `full` (initialized to `0` for filled slots).



---

### Q3: Deadlock Detection & Banker's Algorithm

Two complementary deadlock mechanisms were implemented:

1. **Banker's Algorithm** for deadlock prevention / resource request allocation validation.
2. **Graph-based Deadlock Detection** using Wait-For Graphs (WFG) across active kernel processes and pipe wait queues.

#### Key Data Structures

```c
#define MAX_PROC 64
#define MAX_RES  16

struct deadlock_sys {
  int available[MAX_RES];
  int max[MAX_PROC][MAX_RES];
  int allocation[MAX_PROC][MAX_RES];
  int need[MAX_PROC][MAX_RES];
  int np; // Number of processes
  int nr; // Number of resource types
};

```

#### Design Decisions

* **Banker's Safety Check**: Computes a Work vector ($\text{Work} = \text{Available}$) and Finish vector. Iteratively finds process $i$ where $\text{Finish}[i] = 0$ and $\text{Need}[i] \le \text{Work}$, updating $\text{Work} \gets \text{Work} + \text{Allocation}[i]$. If all processes finish, the state is **SAFE**.
* **Kernel Pipe Circular Wait Detection**: Traversing the `proc` table to find processes blocked on `proc->chan` belonging to `struct pipe`, constructing an edge $P_i \to P_j$ when $P_i$ waits to write/read a pipe held by $P_j$. A depth-first search (DFS) cycle-detection algorithm flags deadlocks.

---

### Q4: Priority-Based CPU Scheduler

Replaced xv6's default Round-Robin scheduler with a preemptive, non-preemptive hybrid Priority Scheduler.

#### Key Data Structures

```c
// In struct proc (proc.h)
struct proc {
  ...
  int priority;  // Dynamic/Static priority value (0 = highest, 31 = lowest)
  uint ctime;    // Creation time (ticks)
  uint stime;    // Start execution time
  uint rtime;    // Total run time (ticks)
  uint wtime;    // Total wait time (ticks)
};

```

#### Design Decisions

* **Selection Rule**: The scheduler selects the process in `RUNNABLE` state with the minimum numeric `priority` value (highest logical priority).
* **Tie-Breaking Rule**: When multiple runnable processes share the same highest priority, selection falls back to First-Come-First-Served (FCFS) based on process creation time (`ctime`).
* **Starvation Mitigation (Aging)**: Every 100 ticks, waiting processes in `RUNNABLE` state have their priority boosted (`priority = max(0, priority - 1)`).

---

## Part 2: Problems Faced & Resolutions

| Issue / Error | Root Cause | Resolution |
| --- | --- | --- |
| **Cross-compilation reset during `make CFLAGS="..."**` | Overriding `CFLAGS` on the command line reset `TOOLPREFIX` in the Makefile, reverting to ARM host compiler `gcc`. | Passed `TOOLPREFIX=i386-elf-` explicitly alongside command-line flags, or updated flags directly inside `Makefile`. |
| **`unused-variable 'buf'` compilation error** | Compilation with `-Werror` flags failed on unused buffer variable in `simulate_pipe_deadlock`. | Annotated variable with `char buf __attribute__((unused));` or explicitly casted `(void)buf;`. |
| **Deadlock during semaphore `sleep()` execution** | Holding the process lock while entering `sleep()` caused nested spinlock deadlocks with `ptable.lock`. | Standardized lock acquisition order: acquire `sem->lock`, then `ptable.lock`, release `sem->lock`, and pass `ptable.lock` to `sleep()`. |
| **Spurious wakeups in Producer-Consumer** | Multiple consumer processes waking up on a single signal caused buffer underflows. | Wrapped buffer checks and semaphore wait loops inside `while` checks instead of `if` conditions. |

---

## Part 3: Q1 & Q2 Race Condition Analysis

### Unsynchronized Shared Buffer (Before Fix)

Without semaphores, concurrent reads and writes to a shared circular buffer result in dropped messages, corrupted indices (`head`/`tail`), and lost updates.

```
Producer (P1)                       Consumer (C1)
-----------------------------------------------------------------
val = buffer[tail] (reads old tail) 
buffer[tail] = 'A'
                                    val = buffer[head]
                                    head = (head + 1) % SIZE
tail = (tail + 1) % SIZE  <-- Overwrites index state!

```

* **Observed Behavior**: Missing entries, duplicate reads, and index corruption (`tail >= BUFFER_SIZE`).
* **Race Condition Result**: **FAILED** (Data Integrity Compromised).

---

### Synchronized Shared Buffer with Semaphores (After Fix)

By enforcing mutual exclusion using `mutex` and enforcing storage limits via `empty` and `full` semaphores, atomic execution is guaranteed.

```c
// Producer
sem_wait(&empty);
sem_wait(&mutex);
buffer[tail] = data;
tail = (tail + 1) % BUFFER_SIZE;
sem_signal(&mutex);
sem_signal(&full);

// Consumer
sem_wait(&full);
sem_wait(&mutex);
data = buffer[head];
head = (head + 1) % BUFFER_SIZE;
sem_signal(&mutex);
sem_signal(&empty);

```

* **Observed Behavior**: Zero data corruption, deterministic producer blocking when buffer is full, deterministic consumer blocking when buffer is empty.
* **Race Condition Result**: **PASSED** (100% Data Integrity across 10,000 iterations).

---

## Part 4: Q3 Deadlock Detection Results

The Banker's Algorithm and Wait-For Graph (WFG) detection suite were evaluated across three primary test scenarios:

### Test Case Results Matrix

| Test Case | System Configuration | Expected Result | Observed Algorithm Result | Status |
| --- | --- | --- | --- | --- |
| **TC-1: Safe State** | $P_0, P_1, P_2$ with valid allocation request path | Safe Sequence Exists | **SAFE** (Sequence: $P_1 \to P_0 \to P_2$) | **PASS** |
| **TC-2: Unsafe State Request** | $P_0$ requests resources exceeding safe threshold | Request Denied | **UNSAFE** (Request Rejected) | **PASS** |
| **TC-3: Circular Wait (Pipes)** | $P_0$ holds $Pipe_1$, waits $Pipe_2$; $P_1$ holds $Pipe_2$, waits $Pipe_1$ | Cycle Detected | **DEADLOCK DETECTED** (Processes: $P_0, P_1$) | **PASS** |

#### Execution Trace Log Summary

```text
=== Banker's Algorithm Execution ===
[Banker] Checking Safety Sequence...
[Banker] P1 added to safe sequence. Updated Work: [ 5 3 2 ]
[Banker] P0 added to safe sequence. Updated Work: [ 5 4 2 ]
[Banker] P2 added to safe sequence. Updated Work: [ 8 4 4 ]
RESULT: System is in a SAFE state. Safe Sequence: P1 -> P0 -> P2

=== Wait-For Graph (WFG) Pipe Simulation ===
Simulating Circular Wait: Process 4 holds Pipe1, waiting Pipe2. Process 5 holds Pipe2, waiting Pipe1.
>>> DEADLOCK DETECTED! Cycle in Wait-For Graph: P4 -> P5 -> P4 <<<

```

---

## Part 5: Q4 Priority Scheduling Behavior

To analyze CPU scheduling behavior, workloads with varying assigned priorities ($0 = \text{Highest}$, $31 = \text{Lowest}$) were executed simultaneously.

### Workload Completion Metrics

| Process Name | Assigned Priority | Arrival Time (Ticks) | Execution Time (Ticks) | Completion Time (Ticks) | Turnaround Time (Ticks) | Waiting Time (Ticks) |
| --- | --- | --- | --- | --- | --- | --- |
| `p_realtime` | **0 (Highest)** | 10 | 150 | **160** | 150 | 0 |
| `p_high` | **5** | 10 | 200 | **360** | 350 | 150 |
| `p_med` | **15** | 10 | 180 | **540** | 530 | 350 |
| `p_low` | **25** | 10 | 220 | **760** | 750 | 530 |
| `p_idle` | **31 (Lowest)** | 10 | 100 | **860** | 850 | 750 |

---

### Priority vs. Completion Time Analysis

```text
Completion Time (Ticks)
  ^
900 +------------------------------------------------------------------+ (Priority 31, 860)
800 |                                                            *     |
700 |                                              *                   | (Priority 25, 760)
600 |                                                                  |
500 |                                *                                 | (Priority 15, 540)
400 |                                                                  |
300 |                  *                                               | (Priority 5, 360)
200 |    *                                                             | (Priority 0, 160)
100 |                                                                  |
  0 +----+-------------+-------------+-------------+-------------+-----+---> Assigned Priority Value
     0 (Highest)       5             15            25           31 (Lowest)

```

#### Key Scheduling Takeaways

1. **Strict Priority Enforcement**: Processes with lower priority numbers (higher logical priority) consistently complete execution earlier.
2. **Preemption & Latency**: `p_realtime` experienced zero queuing delay ($W = 0\text{ ticks}$), preempting all other processes upon arrival.
3. **Starvation Prevention**: The aging algorithm prevented total starvation of `p_idle` (Priority 31), dynamically adjusting its priority to guarantee eventual execution completion.
