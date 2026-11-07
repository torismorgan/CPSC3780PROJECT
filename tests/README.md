# Test fixtures

`abc.txt` is the three-byte file used in the packet vectors. `empty.bin` has zero bytes. `all_bytes.bin` contains each byte value once. `max_256k.bin` contains the byte sequence 0–255 repeated 1,024 times, for a total of 262,144 bytes. `kathara_8k.bin` is an 8,192-byte binary fixture copied by `kathara/prepare_shared.sh`. Create any additional fixtures you need and document how you did so.

At a minimum, record the command, fault setting, expected outcome, observed outcome, exit status, and SHA-256 comparison for each required test. The reference peers and fault relay are supplied separately by the course.

Use `RESULTS_TEMPLATE.md` to organize your evidence. It lists the expected result for each of the 18 required tests, but does not replace the actual logs and hashes.
