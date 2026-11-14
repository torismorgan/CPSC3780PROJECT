# Contributions

List each group member and their specific contributions to design, C++ implementation, tests, analysis, and report preparation.

## Victoria Morgan

- Sender (`source/sender.cpp`): argument and port validation, whole-file read
  with the 262,144-byte limit, IPv4 resolution, HELLO/DATA/FIN exchange with
  500 ms timeout and 10-transmission limit, reply filtering by endpoint,
  Session ID, type and sequence, timestamped packet log.
- Makefile update so both programs link the shared packet code; `make check-vectors`.
- Packet-vector checker (`tests/packet_vector_check.cpp`) and write-up
  (`tests/PACKET_VECTORS.md`).
- Checkpoint 1 evidence script (`tests/run_cp1.sh`), `README.md`, `PROGRESS.md`.

## Joshua Oko

- Receiver (`source/receiver.cpp`). _Joshua: list your contributions here._

## Shared

- Packet encoder/parser (`source/fielddrop_packet.cpp`). _Confirm who wrote this._
