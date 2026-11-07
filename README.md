# FieldDrop student starter

This folder is a starting point for the CPSC 3780 FieldDrop project. Read `ASSIGNMENT.md` before writing code. The sender and receiver currently display usage information and otherwise exit with a `not implemented` message; a successful build is **not** a working transfer.

## Build

On the course Linux environment:

```sh
make
./fielddrop-send --help
./fielddrop-recv --help
make clean
```

The Makefile uses C++17 and links zlib. Add your own `.cpp` files to the Makefile as needed.

## Suggested implementation order

1. Implement packet serialization, parsing, and CRC-32. Compare output with `tests/packet-vectors.txt`.
2. Implement a basic localhost transfer for empty, small, and multipacket files.
3. Add retransmission, duplicate handling, validation, and temporary-file completion.
4. Test each executable against the course reference peer and fault relay.
5. Run the supplied Kathará practice lab, then prepare the final ZIP. Keep an identical copy of that submitted ZIP for your workstation demonstration.

`source/fielddrop_packet.hpp` defines only constants and wire-format names. It does not provide a packet encoder, parser, or reliability implementation. Keep the protocol in the assignment authoritative if starter comments and the assignment ever differ.

## Starter contents

- `source/`: C++ scaffolding; implement the sender, receiver, and packet operations here.
- `tests/`: published packet examples and deterministic fixtures.
- `kathara/`: a basic two-device topology and `prepare_shared.sh` helper. Course staff should test it on the class Kathará image before distribution.
- `kathara/stage_submission.sh`: stages the exact submitted Makefile and source in a clean lab for the assessed workstation visit.
- `field-trial/`, `evidence/`: places for your own results.
- `ACKNOWLEDGMENTS.md`, `CONTRIBUTIONS.md`, `report-outline.md`: submission writing aids.

The instructor reference peers, fault relay, and packet injector are separate course tools and are not included in this starter.

## Live workstation readiness

After submission, extract the same ZIP you submitted on a university Linux lab PC. Use a clean copy of this starter lab and run `sh kathara/stage_submission.sh PATH_TO_EXTRACTED_GROUP_FOLDER` from its root. Start `pc1` and `pc2` with `kathara lstart --shared` from `kathara/`, build the staged code, and verify their addresses and ping. Leave the assessed sender and receiver stopped until the marker arrives. The marker will create fresh input only in `pc1`'s local `/tmp` and choose a new output path in `pc2`'s local `/tmp`. Each member should be ready to explain their work and a submitted fault trace. See `kathara/README.md` and the assignment's demonstration section.
