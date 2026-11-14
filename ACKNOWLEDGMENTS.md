# Acknowledgments

Record substantial documentation, code, and AI assistance here. For each item, state what it helped with and what the group tested or verified independently.

## AI assistance

- **Claude (Anthropic), via Claude Code** – helped write `source/sender.cpp`,
  the Makefile change that links `source/fielddrop_packet.cpp`,
  `tests/packet_vector_check.cpp`, `tests/run_cp1.sh`, and drafts of
  `README.md`, `PROGRESS.md` and `tests/PACKET_VECTORS.md`.
  - **What we verified:** the packet-vector checker reproduces all five
    published ABC packets byte-for-byte; the sender completed empty, ABC,
    all-byte, 8 KiB and 262,144-byte localhost transfers with matching bytes;
    it retransmitted correctly when a DATA packet was withheld; it rejected a
    262,145-byte input, bad ports and wrong argument counts before sending.
    The course-script runs in `evidence/cp1/` were executed by the group on a
    course Linux PC.

## Other sources

- CPSC 3780 FieldDrop assignment, Checkpoint 1 handout, starter README and
  course tool guide.
- zlib `crc32` (linked with `-lz`) for CRC-32.
