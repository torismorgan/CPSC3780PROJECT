# Checkpoint 1 progress

Status key: **complete**, **partial**, **not started**.

## Implementation areas

| Area | Status | Observation | Evidence |
|---|---|---|---|
| 1. Build and command lines | Sender: complete. Receiver: _fill in_ | `make` builds both programs; `make clean` removes them. Sender rejects a wrong argument count and ports that are not decimal 1–65535 (`0`, `70000`, `12a`) with exit 2 and a message. | `README.md`, `evidence/cp1/build.txt` |
| 2. Packet encoding and validation | complete | Our encoder reproduces all five ABC vectors byte-for-byte, CRC-32 included. Parser checks magic, version, type, reserved field, Session ID, payload length vs datagram size, per-type sequence/length rules and CRC-32; a one-byte corruption is rejected. | `tests/PACKET_VECTORS.md`, `tests/packet-vector-check-output.txt`, `evidence/cp1/packet-vector-check-linux.txt` |
| 3. Normal stop-and-wait transfer | Sender: complete. Receiver: _fill in_ | Sender does HELLO→ACK 0, one DATA at a time with matching ACK, FIN→FIN_ACK. Accepts replies only from the chosen receiver address/port with the current Session ID. 500 ms timeout and 10-transmission limit already implemented. | `evidence/cp1/case01-empty`, `case02-abc`, `case03-allbytes`, `case04-8k-delay` |
| 4. File guards | Sender: complete. Receiver: _fill in_ | Sender reads the whole input first and rejects anything over 262,144 bytes before opening a socket. Receiver must refuse an existing output and clean up its temp file. | `evidence/cp1/case15-oversize`, `case16-existing` |

## Required tests

| Run | Pair | Status | Observation | Evidence |
|---|---|---|---|---|
| 1 Empty input | ours → ours | _fill in after lab run_ | | `evidence/cp1/case01-empty/` |
| 2 ABC input | ours → ours | _fill in_ | | `evidence/cp1/case02-abc/` |
| 3 All-byte input | ours → ours | _fill in_ | | `evidence/cp1/case03-allbytes/` |
| 4 8 KiB, first DATA delayed 200 ms | ours → ours | _fill in_ | | `evidence/cp1/case04-8k-delay/` |
| 15 262,145-byte input | ours → ours | _fill in_ | | `evidence/cp1/case15-oversize/` |
| 16 Output already exists | ours → ours | _fill in_ | | `evidence/cp1/case16-existing/` |
| 2 ABC | our sender → reference receiver | _fill in_ | | `evidence/cp1/case02-oursend-refrecv/` |
| 2 ABC | reference sender → our receiver | _fill in_ | | `evidence/cp1/case02-refsend-ourrecv/` |

Test summary with exit statuses and hashes: `tests/CP1_RESULTS.md`.

## Remaining for the final project

- Receiver duplicate-DATA and duplicate-FIN handling, 15 s inactivity timeout,
  6 s FIN linger (receiver side).
- Fault cases 5–14, 17, 18 with the relay, in both reference-peer directions,
  plus packet logs for at least three fault cases.
- Kathará pc1→pc2 transfer evidence.
- Field trial (8–64 KiB original file, two fault settings, before/after change).
- Final report (≤ 8 pages) and live workstation demonstration.

## Current work by member

- **Victoria Morgan** – sender (`source/sender.cpp`), Makefile, packet-vector
  checker and evidence, Checkpoint 1 test script and documentation.
- **Joshua Oko** – receiver (`source/receiver.cpp`). _Joshua: add details._
