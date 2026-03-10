# Tangle

Associating events across machines by mapping each machine's local monotonic
time into a shared UTC reference and then back into another machine's local
monotonic time.

This repository is an early prototype. The included slide deck,
`Tangle.pdf`, describes a much broader time-correlation system based on
precision time synchronization, windows of uncertainty, and multi-machine event
analysis. The code in this repository implements a smaller proof-of-concept:

- sample the local machine's `CLOCK_MONOTONIC_RAW` time against UTC
- store those mappings in a circular buffer
- answer TCP requests that translate `raw -> UTC` and `UTC -> raw`
- provide a client that converts a local raw timestamp into the corresponding
  raw timestamp on a remote machine

## What problem Tangle is trying to solve

The central problem from `Tangle.pdf` is straightforward:

- every machine timestamps events in its own local time scale
- local time scales are not directly comparable across machines
- event correlation across machines requires a common reference
- UTC can act as that reference if each machine can convert local time to UTC
  with enough precision

The slide deck frames this as "pseudo entanglement": two events on different
machines can be probabilistically associated when both are mapped into a common
time frame and evaluated inside a "window of uncertainty" (WOU).

The deck also emphasizes that the required timing precision depends on the
domain:

- CPU level: instruction-scale timing can be in the nanosecond to picosecond
  range
- OS level: kernel and logging timestamps often quantize events more coarsely
- distributed systems level: network latency and clock discipline widen the
  uncertainty window further

## What is implemented in this repository

This repository contains two C programs and one small shared protocol header:

- `timesrv.c`: a background service that tracks local raw monotonic time
  against UTC and answers network queries
- `tangle.c`: a command-line client that resolves a local raw timestamp into a
  remote machine's raw timestamp
- `msg.h`: the on-wire request and response structures

This is not yet the full architecture shown in the slides. There is no PHC/PTP
integration, no GNSS input, no explicit uncertainty model, no one-way-latency
estimation, and no direct RDTSC/PTM pipeline in the current code. The existing
implementation is the smallest working shape of the raw-time/UTC/raw-time idea.

## Repository contents

- `README.md`: project overview and usage notes
- `Tangle.pdf`: 15-page concept deck
- `timesrv.c`: server/service implementation
- `tangle.c`: CLI client
- `msg.h`: protocol message definitions
- `Makefile`: build, format, and clean targets

## Build

Build both binaries with:

```sh
make
```

Produced binaries:

- `timesrv`
- `tangle`

Other targets:

```sh
make format
make clean
```

The code is plain C with pthreads and BSD sockets. The design is Linux-oriented
because it uses `CLOCK_MONOTONIC_RAW` and the client usage comment refers to
`/proc/uptime` semantics.

## How the prototype works

### 1. Local time sampling

`timesrv` creates a circular buffer with capacity `10000` entries. Once per
second, a background thread records:

- `CLOCK_MONOTONIC_RAW` via `clock_gettime(...)`
- current UTC epoch seconds via `time(...)`

Each sample becomes one `(raw_time, utc_time)` mapping in the ring buffer.

At the current sampling rate of one sample per second, the buffer covers about
`10000` seconds of history, which is roughly `2 hours 46 minutes 40 seconds`.

### 2. Query service

`timesrv` also starts a TCP server on port `5214`. It accepts two kinds of
lookups:

- given a raw monotonic timestamp, find the nearest matching UTC epoch
- given a UTC epoch, find the nearest matching raw monotonic timestamp

The service keeps up to `10` concurrent client sockets in its `select()` loop.

### 3. Cross-machine mapping

The `tangle` client performs a two-step translation:

1. Query the local `timesrv` at `127.0.0.1:5214` to translate the supplied
   local raw time into UTC.
2. Query the remote `timesrv` at `<remote_ip>:5214` to translate that UTC value
   into the remote machine's raw monotonic time.

The result is effectively:

`raw(local) -> UTC -> raw(remote)`

This makes it possible to compare locally recorded events against another
machine's local monotonic timeline.

## Running the prototype

Start `timesrv` on every machine that should participate in translation:

```sh
./timesrv
```

Then, on the machine where you have the source event, run:

```sh
./tangle <remote_ip> <local_raw_sec> <local_raw_nsec>
```

Example shape:

```sh
./tangle 192.0.2.10 12345 678901234
```

The client prints a triplet in this form:

```text
{raw(local): [SEC.NSEC]} {utc: HUMAN_READABLE_TIME [EPOCH sec since epoch]} {raw(remote): [SEC.NSEC]}
```

If the requested raw timestamp is outside the server's tracked range, the
client reports:

```text
No matching time for requested input [SEC:NSEC]
```

### Important runtime assumptions

- the local machine must also have `timesrv` running because the client always
  performs the first lookup against `127.0.0.1`
- the remote machine must be reachable over TCP port `5214`
- both peers are assumed to understand the same in-memory C struct layout
- timestamps must fall within the server's retained history window

## How to obtain the input timestamp

The input to `tangle` is not wall clock time. It expects the local machine's
uncorrected monotonic raw clock:

- seconds from `CLOCK_MONOTONIC_RAW`
- nanoseconds from `CLOCK_MONOTONIC_RAW`

In other words, the timestamp should come from the same clock source that
`timesrv` samples internally. If an event logger on the source machine records
`CLOCK_MONOTONIC_RAW` for each event, that timestamp can be fed directly into
`tangle`.

## Protocol

The wire protocol is defined in `msg.h` and consists of raw C structs written
directly over TCP.

### Message types

- `GET_UTC_FROM_RAW_REQ = 2001`
- `GET_RAW_FROM_UTC_REQ = 2002`
- `GET_UTC_FROM_RAW_RES = 3001`
- `GET_RAW_FROM_UTC_RES = 3002`

### Data structures

UTC payload:

```c
typedef struct {
   u32_t epoch;
} utc_time_t;
```

Raw clock payload:

```c
typedef struct {
   u64_t sec;
   u64_t nsec;
} raw_time_t;
```

Envelope:

```c
typedef struct tangle_api {
  tangle_msg_type_t msg;
  tangle_msg_input_t val;
} tangle_api_t;
```

### Protocol implications

Because the code writes these structs directly to the socket:

- there is no explicit serialization layer
- there is no endian conversion
- there is no versioning
- there is no compatibility handling for different ABIs or padding rules

The client source even includes a `TODO` comment noting that network byte order
is not yet handled.

## Current algorithmic behavior

The implementation is intentionally simple:

- samples are taken once per second
- UTC is stored as epoch seconds only
- lookup uses nearest matching records in the ring buffer
- matching is primarily driven by whole-second proximity

Although raw nanoseconds are stored and returned, the current matching logic is
coarse relative to the precision goals described in the slide deck.

## Conceptual architecture from `Tangle.pdf`

The slide deck describes a broader system around this prototype.

### Core ideas from the deck

- use UTC as a common reference to correlate events on different machines
- quantify a "window of uncertainty" instead of pretending timestamps are exact
- support increasing precision requirements from OS logging up to CPU-scale
  execution
- treat cross-machine event association as a probabilistic problem

### Architectural elements shown in the deck

The architecture slide includes concepts such as:

- query engine
- circular buffers
- WOU(UTC), or window of uncertainty around UTC
- precision time sources and synchronization paths
- CPU, NIC, PHC, PTM, GNSS, and PTP components

These are design targets and context, not features fully implemented in this
codebase today.

### Intended use cases listed in the deck

The slides describe capabilities such as:

- identifying concurrent events on another machine
- finding the timestamp of an event on another machine
- ranking an event chronologically across machines
- measuring one-way latency between machines
- tracing the order of event sequences across machines
- benchmarking machines with more precise runtime measurement

It also calls out application areas including:

- distributed databases
- distributed load balancers
- in-network telemetry
- distributed AI systems

## Current limitations

This repository should be read as an early prototype rather than a complete
time-correlation platform.

- Sampling is only once per second, which is far below the precision targets
  discussed in `Tangle.pdf`.
- UTC is represented as `u32_t` epoch seconds, so sub-second UTC precision is
  not preserved on the wire.
- The client hardcodes the local lookup to `127.0.0.1`; only the remote host is
  configurable.
- The server uses a fixed buffer of `10000` entries and does not persist data.
- There is no authentication, encryption, or access control on the TCP service.
- The protocol assumes identical struct layout and endianness on both peers.
- The server and sampler threads access shared state without explicit locking.
- The slide-deck concepts around uncertainty, PTP/PTM, PHC, GNSS, and hardware
  timestamping are not implemented here.

## Mental model

The easiest way to understand the current code is:

- `timesrv` builds a temporary local dictionary between monotonic raw uptime and
  UTC epoch seconds
- `tangle` uses that dictionary twice, once locally and once remotely
- the returned remote raw timestamp lets you ask, "What was the remote
  machine's local monotonic time when my local event happened?"

That is the core of the prototype.

## Authors

The protocol header lists:
- [Ahmad Byagowi](https://github.com/ahmadexp)
- Lakshmi Pradeep `<lpradeep@meta.com>`
- Hari Prasad Kalavakunta `<hkalavakunta@meta.com>`
