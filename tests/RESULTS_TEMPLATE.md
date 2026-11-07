# Required test results

Copy this table into your evidence and complete one row for each test in the assignment. Record the exact course-tool command or fault-rule file so another person can repeat the run. Use a new output path for each transfer.

| Test no. | Input file | Peer combination | Relay or injection rule | Expected result | Observed result | Sender exit | Receiver exit | SHA-256 match or failure cleanup | Evidence file |
|---:|---|---|---|---|---|---:|---:|---|---|
| 1 | | | | Success | | | | | |
| 2 | | | | Success | | | | | |
| 3 | `tests/all_bytes.bin` | | | Success | | | | | |
| 4 | | | | Success | | | | | |
| 5 | `tests/max_256k.bin` | | | Success | | | | | |
| 6 | | | | Success after retransmission | | | | | |
| 7 | | | | Success after retransmission | | | | | |
| 8 | | | | Success after retransmission without duplicate bytes | | | | | |
| 9 | | | | Success after retransmission | | | | | |
| 10 | | | | Success after corrupted packet is ignored | | | | | |
| 11 | | | | Success without duplicate bytes | | | | | |
| 12 | | | | Ignore injected packet; legitimate transfer succeeds | | | | | |
| 13 | | | | Ignore injected packet; legitimate transfer succeeds | | | | | |
| 14 | | | | Ignore early FIN; legitimate transfer succeeds | | | | | |
| 15 | 262,145-byte file | | None | Sender fails before sending | | | N/A | | |
| 16 | | | None | Receiver fails; existing output unchanged | | Nonzero if sender is started; otherwise N/A | Nonzero | | |
| 17 | | | Drop every DATA | Sender fails; receiver times out and removes temp file | | | | | |
| 18 | | | Malformed packet | Ignore injected packet; legitimate transfer succeeds | | | | | |

The receiver normally exits about six seconds after accepting FIN. Wait for its exit before recording a successful receiver status.
