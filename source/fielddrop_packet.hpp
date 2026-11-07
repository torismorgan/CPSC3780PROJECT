#ifndef FIELDDROP_PACKET_HPP
#define FIELDDROP_PACKET_HPP

#include <cstddef>
#include <cstdint>

// Wire-format constants from the assignment. Multibyte values on the wire
// use network byte order. Implement encoding and validation in your own files.
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

// TODO: Define a host-order packet representation and functions to:
//  - serialize a packet to bytes;
//  - parse a complete UDP datagram;
//  - validate type-specific fields, size, session ID, and CRC-32.
// Do not send a C++ struct directly: padding and host byte order are not
// the specified wire format.

} // namespace fielddrop

#endif
