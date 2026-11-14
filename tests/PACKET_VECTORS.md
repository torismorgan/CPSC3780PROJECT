# ABC packet-vector check

## How the bytes were generated

`tests/packet_vector_check.cpp` builds the five published ABC packets
(Session ID `0x01020304`) as `fielddrop::Packet` values and encodes each one with
our own `serializePacket()` from `source/fielddrop_packet.cpp`. It then:

1. prints the bytes our encoder produced next to the published bytes from
   `tests/packet-vectors.txt`;
2. compares the CRC-32 field (bytes 16–19) and the whole datagram;
3. parses the published bytes with our `parsePacket()` and checks that type,
   Session ID, sequence and payload decode to the same values;
4. flips the last byte of each published packet and checks that
   `parsePacket()` rejects it (CRC-32 failure), showing malformed datagrams are
   not accepted as transfer data.

Command (from the project root):

```sh
make check-vectors
```

The program exits 0 only if every check passes.

## Result

Full output: [`packet-vector-check-output.txt`](packet-vector-check-output.txt).

| Packet | Produced bytes | Matches published bytes | CRC-32 matches | Parser accepts | Corrupted copy rejected |
|---|---|---|---|---|---|
| HELLO (size 3, seq 0) | `46 44 01 01 01 02 03 04 00 00 00 00 00 04 00 00 a5 ea 20 c2 00 00 00 03` | Yes | Yes (`a5 ea 20 c2`) | Yes | Yes |
| DATA (seq 1, `ABC`) | `46 44 01 02 01 02 03 04 00 00 00 01 00 03 00 00 4b 5a 87 8c 41 42 43` | Yes | Yes (`4b 5a 87 8c`) | Yes | Yes |
| ACK (seq 1) | `46 44 01 03 01 02 03 04 00 00 00 01 00 00 00 00 b9 64 71 fc` | Yes | Yes (`b9 64 71 fc`) | Yes | Yes |
| FIN (seq 2) | `46 44 01 04 01 02 03 04 00 00 00 02 00 00 00 00 83 b7 0d 74` | Yes | Yes (`83 b7 0d 74`) | Yes | Yes |
| FIN_ACK (seq 2) | `46 44 01 05 01 02 03 04 00 00 00 02 00 00 00 00 5e 21 d4 f1` | Yes | Yes (`5e 21 d4 f1`) | Yes | Yes |

**Differences:** none. All five encoded packets are byte-for-byte identical to
the published vectors, so byte order, header layout and CRC-32 coverage (header
bytes 0–15 plus payload, zlib `crc32` starting at 0) are correct.

The saved output was first produced on macOS (Apple clang, zlib). `tests/run_cp1.sh`
re-runs the check on the course Linux PC and saves that output to
`evidence/cp1/packet-vector-check-linux.txt`.
