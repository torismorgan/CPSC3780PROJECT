// Encodes the five published ABC packets (Session ID 0x01020304) with our
// serializePacket(), prints the produced bytes next to the published bytes,
// and checks that parsePacket() accepts each one and rejects a corrupted copy.
// Build and run with: make check-vectors

#include "fielddrop_packet.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace fielddrop;

namespace {

std::vector<std::uint8_t> hexToBytes(const std::string& hex) {
    std::vector<std::uint8_t> bytes;
    std::istringstream in(hex);
    std::string token;
    while (in >> token) {
        bytes.push_back(static_cast<std::uint8_t>(std::stoul(token, nullptr, 16)));
    }
    return bytes;
}

std::string bytesToHex(const std::vector<std::uint8_t>& bytes) {
    std::string out;
    char buf[4];
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        std::snprintf(buf, sizeof(buf), "%02x", bytes[i]);
        if (i > 0) out += ' ';
        out += buf;
    }
    return out;
}

struct Vector {
    const char* name;
    Packet packet;
    const char* published;
};

} // namespace

int main() {
    const std::uint32_t session = 0x01020304;
    std::vector<Vector> vectors = {
        {"HELLO (size 3, seq 0)",
         {PacketType::Hello, session, 0, {0x00, 0x00, 0x00, 0x03}},
         "46 44 01 01 01 02 03 04 00 00 00 00 00 04 00 00 a5 ea 20 c2 00 00 00 03"},
        {"DATA (seq 1, ABC)",
         {PacketType::Data, session, 1, {'A', 'B', 'C'}},
         "46 44 01 02 01 02 03 04 00 00 00 01 00 03 00 00 4b 5a 87 8c 41 42 43"},
        {"ACK (seq 1)",
         {PacketType::Ack, session, 1, {}},
         "46 44 01 03 01 02 03 04 00 00 00 01 00 00 00 00 b9 64 71 fc"},
        {"FIN (seq 2)",
         {PacketType::Fin, session, 2, {}},
         "46 44 01 04 01 02 03 04 00 00 00 02 00 00 00 00 83 b7 0d 74"},
        {"FIN_ACK (seq 2)",
         {PacketType::FinAck, session, 2, {}},
         "46 44 01 05 01 02 03 04 00 00 00 02 00 00 00 00 5e 21 d4 f1"},
    };

    int failures = 0;
    for (const Vector& v : vectors) {
        std::vector<std::uint8_t> produced;
        std::string error;
        const std::vector<std::uint8_t> expected = hexToBytes(v.published);

        std::cout << v.name << "\n";
        if (!serializePacket(v.packet, produced, &error)) {
            std::cout << "  ENCODE FAILED: " << error << "\n\n";
            ++failures;
            continue;
        }

        const bool bytesMatch = produced == expected;
        const bool crcMatch = produced.size() >= 20 && expected.size() >= 20 &&
            std::equal(produced.begin() + 16, produced.begin() + 20,
                       expected.begin() + 16);
        std::cout << "  produced:  " << bytesToHex(produced) << "\n"
                  << "  published: " << v.published << "\n"
                  << "  CRC-32 (bytes 16-19): "
                  << (crcMatch ? "match" : "MISMATCH") << "\n"
                  << "  all bytes: " << (bytesMatch ? "MATCH" : "MISMATCH")
                  << "\n";
        if (!bytesMatch) ++failures;

        // The parser must accept the published bytes and decode the same fields.
        Packet parsed;
        const bool parsedOk =
            parsePacket(expected.data(), expected.size(), parsed, &error);
        const bool fieldsOk = parsedOk && parsed.type == v.packet.type &&
                              parsed.sessionId == v.packet.sessionId &&
                              parsed.sequence == v.packet.sequence &&
                              parsed.payload == v.packet.payload;
        std::cout << "  parse published bytes: "
                  << (fieldsOk ? "accepted, fields match" : "FAILED " + error)
                  << "\n";
        if (!fieldsOk) ++failures;

        // Flipping one byte must make the parser reject the datagram.
        std::vector<std::uint8_t> corrupted = expected;
        corrupted.back() ^= 0xFF;
        const bool rejected =
            !parsePacket(corrupted.data(), corrupted.size(), parsed, &error);
        std::cout << "  parse with last byte flipped: "
                  << (rejected ? "rejected (" + error + ")" : "WRONGLY ACCEPTED")
                  << "\n\n";
        if (!rejected) ++failures;
    }

    std::cout << (failures == 0 ? "RESULT: all packet vectors match\n"
                                : "RESULT: " + std::to_string(failures) +
                                      " check(s) failed\n");
    return failures == 0 ? 0 : 1;
}
