# Qehack_2026
# ⚡ QNX Smart Grid Simulation

> A real-time power grid fault detection and load balancing system built on **QNX Neutrino RTOS** using multi-threading, semaphores, mutexes, and IPC message passing.

---

## 📡 What Is This?

This project simulates a **3-region smart power grid** running on a real-time operating system. It detects overloads, redistributes power automatically, and isolates regions in fault scenarios — all using concurrent threads with strict priority scheduling.

Built as part of an RTOS course project to demonstrate:
- Priority-based preemptive scheduling
- Inter-thread synchronization
- QNX-specific IPC (message passing via channels)
- Real-time latency measurement

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────┐
│                    QNX Process                       │
│                                                      │
│  ┌──────────┐    sem     ┌────────────┐             │
│  │ Monitor  │ ─────────► │ Comparator │  priority 13│
│  │ Threads  │            └─────┬──────┘             │
│  │ (x3) p12 │                  │ sem                │
│  └──────────┘                  ▼                    │
│                         ┌────────────┐              │
│  ┌──────────┐  mutex    │  Balancer  │  priority 14 │
│  │   CLI    │ ────────► │  Thread    │              │
│  │  p10     │           └────────────┘              │
│  └──────────┘                                       │
│                                                      │
│  ┌──────────────┐    ┌──────────────────┐           │
│  │ Msg Handler  │    │ Status Broadcast │           │
│  │   p11        │    │      p9          │           │
│  └──────────────┘    └──────────────────┘           │
└─────────────────────────────────────────────────────┘
         ▲ MsgSend / MsgReply (QNX IPC)
         │
    ┌────┴─────┐
    │  Client  │
    │ Process  │
    └──────────┘
```

---

## 🧵 Threads Overview

| Thread | Priority | Purpose |
|---|---|---|
| `load_balancer_thread` | 14 (highest) | Redistributes or isolates overloaded regions |
| `comparator_thread` | 13 | Analyzes overloads, detects cascades |
| `monitor_region_thread` x3 | 12 | Watches each region continuously |
| `message_handler_thread` | 11 | Handles IPC messages from client process |
| `cli_thread` | 10 | User command interface |
| `status_broadcaster_thread` | 9 (lowest) | Broadcasts grid status to connected client |

Higher priority threads **preempt** lower ones when signaled — this is guaranteed by QNX's real-time scheduler.

---

## ⚙️ How It Works

### Normal Operation
Each monitor thread watches its region. When a load changes, it posts to `monitor_signal`, waking the comparator.

### Overload Detection
The comparator scans all regions and checks against `BASE_CAPACITY`:
- **1 region over** → sets `overload_region`, signals balancer
- **2+ regions over** → sets `cascade_detected = 1`, signals balancer

### Load Balancing Logic
```
overload detected
       │
       ├── cascade? ──► isolate ALL overloaded regions → BLACKOUT
       │
       └── single region?
               │
               ├── excess > MAX_TRANSFER? ──► isolate region → BLACKOUT
               │
               └── other regions have room? ──► redistribute evenly
```

---

## 🧪 Built-in Test Scenarios

| Command | Load Set | Expected Behaviour |
|---|---|---|
| `test1` | Region 0 → 320 kW | Mild overload, excess redistributed to regions 1 & 2 |
| `test2` | Region 0 → 380 kW | Severe overload, exceeds MAX_TRANSFER, region isolated |
| `test3` | Region 0 & 1 → 350 kW | Cascade detected, both regions go to blackout |

---

## 💻 CLI Commands

```
-- Commands --
  load <region 0-2> <kW>   example: load 0 320
  status
  reset
  test1 / test2 / test3

>
```

| Command | What it does |
|---|---|
| `load 0 320` | Manually set region 0 to 320 kW |
| `status` | Print current state of all 3 regions |
| `reset` | Restore default loads (250 / 280 / 260 kW) |
| `test1` | Trigger mild overload scenario |
| `test2` | Trigger severe overload scenario |
| `test3` | Trigger cascade failure scenario |

---

## 📊 Region States

| Status | Meaning |
|---|---|
| `Stable` | Load is within BASE_CAPACITY |
| `Overload` | Load exceeds BASE_CAPACITY |
| `Redistributing` | Excess being offloaded to other regions |
| `Blackout` | Region isolated, load set to 0 |

---

## 🔧 Key Constants (shared.h)

```c
#define MAX_REGIONS     3
#define BASE_CAPACITY   300   // kW per region
#define MAX_TRANSFER    30    // max kW that can be redistributed
```

---

## 📶 QNX IPC — Message Passing

The client process connects to the server via a **QNX channel**:

```c
// Server creates channel
chid = ChannelCreate(0);

// Client sends a load_command struct
MsgSend(coid, &cmd, sizeof(cmd), &reply, sizeof(reply));

// Server receives and replies
rcvid = MsgReceive(chid, &cmd, sizeof(cmd), NULL);
MsgReply(rcvid, 0, &reply, sizeof(reply));
```

The client **blocks** on `MsgSend` until the server calls `MsgReply` — this is synchronous IPC built into the QNX microkernel.

---

## ⏱️ Latency Tracking

Every message handled by the server is timed:

```
[msg handler] latency: 140 us  (min: 98 us  max: 203 us)
```

Measured using `CLOCK_MONOTONIC` from receive to reply. Tracks running min and max across all messages.

---

## 🗂️ File Structure

```
project/
├── shared.h                         # shared structs, globals, constants
├── process_one/                     # client process
│   └── src/
│       └── client.c
└── process_two/                     # server process (main RTOS logic)
    └── src/
        ├── main.c                   # thread creation, channel setup
        ├── cli_thread.c
        ├── comparator_thread.c
        ├── load_balancer_thread.c
        ├── message_handler_thread.c
        ├── monitor_region_thread.c
        └── status_broadcaster_thread.c
```

---

## 🚀 Build & Run

```bash
# Build both processes
make all

# Run server first
./process_two

# Then run client in another terminal
./process_one <server_pid>
```

> Built and tested on **QNX Neutrino 8.0** targeting `aarch64le`

---

## 👤 Author

Made as part of an RTOS systems programming course.
