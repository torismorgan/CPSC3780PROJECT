# FieldDrop (CPSC 3780, Fall 2026)

Stop-and-wait file transfer over IPv4 UDP. `fielddrop-send` sends one file to
`fielddrop-recv`, which writes it to a new output file.

## Build (course Linux PC)

Requires `g++`, `make` and the zlib development files (all on the course image).

```sh
make clean && make
```

This produces `./fielddrop-send` and `./fielddrop-recv`. `make clean` removes
them (and the packet-vector checker).

## Normal transfer

Terminal 1 (receiver; the output file must not already exist):

```sh
./fielddrop-recv received.bin 43000
```

Terminal 2 (sender):

```sh
./fielddrop-send tests/abc.txt 127.0.0.1 43000
```

Both programs exit 0 on success. The receiver stays up for about six seconds
after FIN to answer a repeated FIN; that wait is expected. Check the result:

```sh
sha256sum tests/abc.txt received.bin
```

The sender prints a timestamped log of every packet sent, received, ignored or
timed out on stderr.

Exit statuses (sender): `0` success, `2` wrong argument count or invalid port,
`1` any other failure (unreadable or oversized input, unresolvable host, no
reply after 10 transmissions).

## Packet-vector check

```sh
make check-vectors
```

Encodes the published ABC packets and compares them with
`tests/packet-vectors.txt`. See `tests/PACKET_VECTORS.md`.

## Course tests

Course tools location: extract the course-tool ZIP matching `uname -m`
(`FieldDrop_Course_Tools/`) **next to** the unmodified
`FieldDrop_Student_Starter/` folder; the course script reads fixtures from the
starter folder. Then, from this project folder:

```sh
sh tests/run_cp1.sh PATH_TO/FieldDrop_Course_Tools
```

This builds the project, runs Checkpoint 1 cases 1, 2, 3, 4, 15, 16 with our
sender and receiver, plus case 2 in both reference-peer combinations, and saves
logs, hashes and PASS/FAIL lines in `evidence/cp1/`. A single case can also be
run directly:

```sh
PATH_TO/FieldDrop_Course_Tools/scripts/run_case.sh 2 "$PWD/fielddrop-send" "$PWD/fielddrop-recv"
```

## Layout

- `source/` – C++ sources: `fielddrop_packet.*` (encode/parse/CRC-32),
  `sender.cpp`, `receiver.cpp`.
- `tests/` – fixtures, packet-vector checker, test procedures and summaries.
- `evidence/` – logs, hashes and course-script output.
