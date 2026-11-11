
#include "fielddrop_packet.hpp"

#include <zlib.h>

#include <string>
#include <vector>

namespace fielddrop {
namespace {

// Store a 16-bit integer in network byte order.
void write16(std::vector<std::uint8_t>& bytes,
             std::size_t offset,
             std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8);
    bytes[offset + 1] = static_cast<std::uint8_t>(value);
}

// Store a 32-bit integer in network byte order.
void write32(std::vector<std::uint8_t>& bytes,
             std::size_t offset,
             std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 24);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 16);
    bytes[offset + 2] = static_cast<std::uint8_t>(value >> 8);
    bytes[offset + 3] = static_cast<std::uint8_t>(value);
}

// Read a 16-bit integer from network byte order.
std::uint16_t read16(const std::uint8_t* data,
                     std::size_t offset) {
    return (static_cast<std::uint16_t>(data[offset]) << 8) |
           static_cast<std::uint16_t>(data[offset + 1]);
}

// Read a 32-bit integer from network byte order.
std::uint32_t read32(const std::uint8_t* data,
                     std::size_t offset) {
    return (static_cast<std::uint32_t>(data[offset]) << 24) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
           static_cast<std::uint32_t>(data[offset + 3]);
}

// Calculate CRC-32 over header bytes 0-15 and the payload.
std::uint32_t calculateCRC(const std::uint8_t* data,
                            std::size_t payloadLength) {
    uLong crc = crc32(0L, Z_NULL, 0);

    crc = crc32(
        crc,
        reinterpret_cast<const Bytef*>(data),
        16
    );

    if (payloadLength > 0) {
        crc = crc32(
            crc,
            reinterpret_cast<const Bytef*>(
                data + kHeaderBytes
            ),
            static_cast<uInt>(payloadLength)
        );
    }

    return static_cast<std::uint32_t>(crc);
}

bool fail(std::string* error, const char* message) {
    if (error != nullptr) {
        *error = message;
    }
    return false;
}

// Validate packet-specific sequence and payload rules.
bool validatePacket(const Packet& packet, std::string* error) {
    if (packet.sessionId == 0) {
        return fail(error, "Session ID cannot be zero");
    }

    if (packet.payload.size() > kMaxPayloadBytes) {
        return fail(error, "Payload exceeds 512 bytes");
    }

    switch (packet.type) {
        case PacketType::Hello:
            if (packet.sequence != 0) {
                return fail(error, "HELLO sequence must be 0");
            }
            if (packet.payload.size() != 4) {
                return fail(error, "HELLO payload must be 4 bytes");
            }
            break;

        case PacketType::Data:
            if (packet.sequence == 0) {
                return fail(error, "DATA sequence must be at least 1");
            }
            if (packet.payload.empty()) {
                return fail(error, "DATA payload cannot be empty");
            }
            break;

        case PacketType::Ack:
            if (!packet.payload.empty()) {
                return fail(error, "ACK cannot contain a payload");
            }
            break;

        case PacketType::Fin:
            if (packet.sequence == 0) {
                return fail(error, "FIN sequence must be at least 1");
            }
            if (!packet.payload.empty()) {
                return fail(error, "FIN cannot contain a payload");
            }
            break;

        case PacketType::FinAck:
            if (packet.sequence == 0) {
                return fail(error, "FIN_ACK sequence must be at least 1");
            }
            if (!packet.payload.empty()) {
                return fail(error, "FIN_ACK cannot contain a payload");
            }
            break;

        default:
            return fail(error, "Unknown packet type");
    }

    return true;
}

} 

bool serializePacket(const Packet& packet,
                     std::vector<std::uint8_t>& output,
                     std::string* error) {
    output.clear();

    if (error != nullptr) {
        error->clear();
    }

    if (!validatePacket(packet, error)) {
        return false;
    }

    const std::size_t payloadLength = packet.payload.size();

    // Allocate header plus payload.
    output.resize(kHeaderBytes + payloadLength, 0);

    // Header fields.
    write16(output, 0, kMagic);
    output[2] = kVersion;
    output[3] = static_cast<std::uint8_t>(packet.type);
    write32(output, 4, packet.sessionId);
    write32(output, 8, packet.sequence);
    write16(output, 12,
            static_cast<std::uint16_t>(payloadLength));


    // Copy payload after the 20-byte header.
    for (std::size_t i = 0; i < payloadLength; ++i) {
        output[kHeaderBytes + i] = packet.payload[i];
    }

    // Calculate and insert CRC-32.
    const std::uint32_t crc =
        calculateCRC(output.data(), payloadLength);

    write32(output, 16, crc);

    return true;
}

bool parsePacket(const std::uint8_t* data,
                 std::size_t size,
                 Packet& packet,
                 std::string* error) {
    if (error != nullptr) {
        error->clear();
    }

    if (data == nullptr) {
        return fail(error, "Null packet data");
    }

    // A valid packet must contain at least the 20-byte header.
    if (size < kHeaderBytes) {
        return fail(error, "Packet is shorter than the header");
    }

    // Validate magic number.
    if (read16(data, 0) != kMagic) {
        return fail(error, "Invalid magic number");
    }

    // Validate protocol version.
    if (data[2] != kVersion) {
        return fail(error, "Unsupported protocol version");
    }

    // Validate packet type.
    if (data[3] < static_cast<std::uint8_t>(PacketType::Hello) ||
        data[3] > static_cast<std::uint8_t>(PacketType::FinAck)) {
        return fail(error, "Unknown packet type");
    }

    // Validate reserved field.
    if (read16(data, 14) != 0) {
        return fail(error, "Reserved field must be zero");
    }

    const std::uint32_t sessionId = read32(data, 4);
    const std::uint32_t sequence = read32(data, 8);
    const std::uint16_t payloadLength = read16(data, 12);
    const std::uint32_t receivedCRC = read32(data, 16);

    if (sessionId == 0) {
        return fail(error, "Session ID cannot be zero");
    }

    if (payloadLength > kMaxPayloadBytes) {
        return fail(error, "Payload exceeds 512 bytes");
    }

    // Datagram must contain exact sized payload.
    if (size != kHeaderBytes + payloadLength) {
        return fail(error, "Datagram length does not match header");
    }

    // Verify CRC-32.
    const std::uint32_t calculatedCRC =
        calculateCRC(data, payloadLength);

    if (receivedCRC != calculatedCRC) {
        return fail(error, "CRC-32 verification failed");
    }

    // Build a temporary packet so invalid packets never partially
    Packet parsed;
    parsed.type = static_cast<PacketType>(data[3]);
    parsed.sessionId = sessionId;
    parsed.sequence = sequence;

    for (std::size_t i = 0; i < payloadLength; ++i) {
        parsed.payload.push_back(data[kHeaderBytes + i]);
    }

    if (!validatePacket(parsed, error)) {
        return false;
    }

    packet = parsed;
    return true;
}

} 
