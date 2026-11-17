
#ifndef FIELDDROP_PACKET_HPP
#define FIELDDROP_PACKET_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fielddrop {

constexpr std::uint16_t kMagic = 0x4644;
constexpr std::uint8_t kVersion = 1;
constexpr std::size_t kHeaderBytes = 20;
constexpr std::size_t kMaxPayloadBytes = 512;
constexpr std::size_t kMaxFileBytes = 262144;
constexpr int kTimeoutMs = 500;
constexpr int kMaxTransmissions = 10;
constexpr int kReceiverIdleMs = 15000;
constexpr int kFinLingerMs = 6000;

enum class PacketType : std::uint8_t {
    Hello = 1,
    Data = 2,
    Ack = 3,
    Fin = 4,
    FinAck = 5,
};

struct Packet {
    PacketType type{};
    std::uint32_t sessionId = 0;
    std::uint32_t sequence = 0;
    std::vector<std::uint8_t> payload;
};

bool serializePacket(
    const Packet& packet,
    std::vector<std::uint8_t>& output,
    std::string* error = nullptr
);

bool parsePacket(
    const std::uint8_t* data,
    std::size_t size,
    Packet& packet,
    std::string* error = nullptr
);

} // namespace fielddrop

#endif
