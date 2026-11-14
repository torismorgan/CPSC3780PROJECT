// fielddrop-send INPUT_FILE RECEIVER_HOST RECEIVER_PORT
//
// Stop-and-wait FieldDrop sender:
//   HELLO(seq 0, file size) -> ACK 0
//   DATA(seq 1..N, <=512 bytes each) -> ACK seq
//   FIN(seq N+1) -> FIN_ACK N+1
// Each packet is sent up to kMaxTransmissions times, waiting kTimeoutMs after
// every transmission for the matching reply.

#include "fielddrop_packet.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace fielddrop;
using Clock = std::chrono::steady_clock;

const Clock::time_point kStart = Clock::now();

void usage(const char* program) {
    std::cerr << "Usage: " << program
              << " INPUT_FILE RECEIVER_HOST RECEIVER_PORT\n";
}

const char* typeName(PacketType type) {
    switch (type) {
        case PacketType::Hello:  return "HELLO";
        case PacketType::Data:   return "DATA";
        case PacketType::Ack:    return "ACK";
        case PacketType::Fin:    return "FIN";
        case PacketType::FinAck: return "FIN_ACK";
    }
    return "UNKNOWN";
}

// Timestamped log line on stderr: "[   12.345] message".
std::ostream& logLine() {
    const double seconds =
        std::chrono::duration<double>(Clock::now() - kStart).count();
    std::cerr << "[" << std::fixed << std::setprecision(3) << std::setw(9)
              << seconds << "] ";
    return std::cerr;
}

// Strict decimal port parse: digits only, 1..65535.
bool parsePort(const char* text, std::uint16_t& port) {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    unsigned long value = 0;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return false;
        }
        value = value * 10 + static_cast<unsigned long>(*p - '0');
        if (value > 65535) {
            return false;
        }
    }
    if (value == 0) {
        return false;
    }
    port = static_cast<std::uint16_t>(value);
    return true;
}

// Read the whole file once so the advertised size and the sent bytes come
// from the same snapshot. Reads at most kMaxFileBytes + 1 bytes so an
// oversized file is detected without loading all of it.
bool readInput(const char* path, std::vector<std::uint8_t>& contents,
               std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = std::string("cannot open input file '") + path + "': " +
                std::strerror(errno);
        return false;
    }
    contents.clear();
    contents.resize(kMaxFileBytes + 1);
    in.read(reinterpret_cast<char*>(contents.data()),
            static_cast<std::streamsize>(contents.size()));
    if (in.bad()) {
        error = std::string("error reading input file '") + path + "'";
        return false;
    }
    contents.resize(static_cast<std::size_t>(in.gcount()));
    if (contents.size() > kMaxFileBytes) {
        error = "input file is larger than " + std::to_string(kMaxFileBytes) +
                " bytes";
        return false;
    }
    return true;
}

// Resolve RECEIVER_HOST to one IPv4 address.
bool resolveReceiver(const char* host, std::uint16_t port,
                     sockaddr_in& dest, std::string& error) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    addrinfo* result = nullptr;
    const int rc = getaddrinfo(host, nullptr, &hints, &result);
    if (rc != 0 || result == nullptr) {
        error = std::string("cannot resolve IPv4 address for '") + host +
                "': " + gai_strerror(rc);
        return false;
    }
    std::memcpy(&dest, result->ai_addr, sizeof(sockaddr_in));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    freeaddrinfo(result);
    return true;
}

std::uint32_t newSessionId() {
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<std::uint32_t> dist(1, 0xFFFFFFFFu);
    return dist(generator);
}

class Sender {
public:
    Sender(int socketFd, const sockaddr_in& dest, std::uint32_t sessionId)
        : fd_(socketFd), dest_(dest), sessionId_(sessionId) {}

    // Send `packet` and wait for a reply of `replyType` carrying the same
    // sequence number. Returns false after kMaxTransmissions timeouts or on a
    // socket error.
    bool exchange(const Packet& packet, PacketType replyType) {
        std::vector<std::uint8_t> wire;
        std::string error;
        if (!serializePacket(packet, wire, &error)) {
            logLine() << "internal error encoding " << typeName(packet.type)
                  << ": " << error << "\n";
            return false;
        }

        for (int attempt = 1; attempt <= kMaxTransmissions; ++attempt) {
            const ssize_t sent = sendto(
                fd_, wire.data(), wire.size(), 0,
                reinterpret_cast<const sockaddr*>(&dest_), sizeof(dest_));
            if (sent < 0 || static_cast<std::size_t>(sent) != wire.size()) {
                logLine() << "sendto failed: " << std::strerror(errno) << "\n";
                return false;
            }
            ++transmissions_;
            if (attempt > 1) {
                ++retransmissions_;
            }
            logLine() << "send " << typeName(packet.type) << " seq "
                  << packet.sequence << " len " << packet.payload.size()
                  << " (transmission " << attempt << "/" << kMaxTransmissions
                  << ")\n";

            // The timer runs from this transmission; invalid or unrelated
            // replies do not restart it.
            const Clock::time_point deadline =
                Clock::now() + std::chrono::milliseconds(kTimeoutMs);
            if (waitForReply(replyType, packet.sequence, deadline)) {
                return true;
            }
            logLine() << "timeout waiting for " << typeName(replyType) << " seq "
                  << packet.sequence << "\n";
        }
        logLine() << "giving up on " << typeName(packet.type) << " seq "
              << packet.sequence << " after " << kMaxTransmissions
              << " transmissions\n";
        return false;
    }

    int transmissions() const { return transmissions_; }
    int retransmissions() const { return retransmissions_; }

private:
    bool waitForReply(PacketType replyType, std::uint32_t sequence,
                      Clock::time_point deadline) {
        // One byte larger than the biggest legal datagram so oversize
        // datagrams are seen as the wrong length rather than silently cut.
        std::uint8_t buffer[kHeaderBytes + kMaxPayloadBytes + 1];

        while (true) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - Clock::now()).count();
            if (remaining <= 0) {
                return false;
            }

            pollfd pfd{fd_, POLLIN, 0};
            const int ready = poll(&pfd, 1, static_cast<int>(remaining));
            if (ready < 0) {
                if (errno == EINTR) {
                    continue;
                }
                logLine() << "poll failed: " << std::strerror(errno) << "\n";
                return false;
            }
            if (ready == 0) {
                return false;
            }

            sockaddr_in from{};
            socklen_t fromLen = sizeof(from);
            const ssize_t n = recvfrom(
                fd_, buffer, sizeof(buffer), 0,
                reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (n < 0) {
                // e.g. ECONNREFUSED from an ICMP error: keep waiting.
                if (errno != EINTR) {
                    logLine() << "recvfrom: " << std::strerror(errno)
                          << " (ignored)\n";
                }
                continue;
            }

            // Only accept replies from the chosen receiver endpoint.
            if (fromLen != sizeof(from) || from.sin_family != AF_INET ||
                from.sin_addr.s_addr != dest_.sin_addr.s_addr ||
                from.sin_port != dest_.sin_port) {
                char ip[INET_ADDRSTRLEN] = "?";
                inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
                logLine() << "ignored datagram from unexpected endpoint " << ip
                      << ":" << ntohs(from.sin_port) << "\n";
                continue;
            }

            Packet reply;
            std::string error;
            if (!parsePacket(buffer, static_cast<std::size_t>(n), reply,
                             &error)) {
                logLine() << "ignored invalid datagram (" << n
                      << " bytes): " << error << "\n";
                continue;
            }
            if (reply.sessionId != sessionId_) {
                logLine() << "ignored " << typeName(reply.type)
                      << " with wrong session 0x" << std::hex
                      << reply.sessionId << std::dec << "\n";
                continue;
            }
            if (reply.type != replyType || reply.sequence != sequence) {
                logLine() << "ignored " << typeName(reply.type) << " seq "
                      << reply.sequence << " (waiting for "
                      << typeName(replyType) << " seq " << sequence << ")\n";
                continue;
            }

            logLine() << "recv " << typeName(reply.type) << " seq "
                  << reply.sequence << "\n";
            return true;
        }
    }

    int fd_;
    sockaddr_in dest_;
    std::uint32_t sessionId_;
    int transmissions_ = 0;
    int retransmissions_ = 0;
};

class SocketGuard {
public:
    explicit SocketGuard(int fd) : fd_(fd) {}
    ~SocketGuard() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }
    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;

private:
    int fd_;
};

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        usage(argv[0]);
        return 0;
    }
    if (argc != 4) {
        std::cerr << "fielddrop-send: expected 3 arguments, got "
                  << (argc - 1) << "\n";
        usage(argv[0]);
        return 2;
    }

    const char* inputPath = argv[1];
    const char* host = argv[2];

    // Everything below is checked before any packet is sent.
    std::uint16_t port = 0;
    if (!parsePort(argv[3], port)) {
        std::cerr << "fielddrop-send: invalid RECEIVER_PORT '" << argv[3]
                  << "' (must be a decimal integer from 1 to 65535)\n";
        return 2;
    }

    std::vector<std::uint8_t> contents;
    std::string error;
    if (!readInput(inputPath, contents, error)) {
        std::cerr << "fielddrop-send: " << error << "\n";
        return 1;
    }

    sockaddr_in dest{};
    if (!resolveReceiver(host, port, dest, error)) {
        std::cerr << "fielddrop-send: " << error << "\n";
        return 1;
    }

    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "fielddrop-send: cannot create UDP socket: "
                  << std::strerror(errno) << "\n";
        return 1;
    }
    SocketGuard guard(fd);

    const std::uint32_t sessionId = newSessionId();
    char ip[INET_ADDRSTRLEN] = "?";
    inet_ntop(AF_INET, &dest.sin_addr, ip, sizeof(ip));
    logLine() << "sending '" << inputPath << "' (" << contents.size()
          << " bytes) to " << ip << ":" << port << ", session 0x" << std::hex
          << sessionId << std::dec << "\n";

    Sender sender(fd, dest, sessionId);

    // HELLO: sequence 0, payload is the 4-byte file size.
    Packet hello;
    hello.type = PacketType::Hello;
    hello.sessionId = sessionId;
    hello.sequence = 0;
    const auto size = static_cast<std::uint32_t>(contents.size());
    hello.payload = {
        static_cast<std::uint8_t>(size >> 24),
        static_cast<std::uint8_t>(size >> 16),
        static_cast<std::uint8_t>(size >> 8),
        static_cast<std::uint8_t>(size),
    };
    if (!sender.exchange(hello, PacketType::Ack)) {
        std::cerr << "fielddrop-send: no ACK 0 for HELLO; transfer failed\n";
        return 1;
    }

    // DATA: 512-byte chunks, one outstanding at a time.
    std::uint32_t sequence = 1;
    for (std::size_t offset = 0; offset < contents.size();
         offset += kMaxPayloadBytes, ++sequence) {
        const std::size_t length =
            std::min(kMaxPayloadBytes, contents.size() - offset);
        Packet data;
        data.type = PacketType::Data;
        data.sessionId = sessionId;
        data.sequence = sequence;
        data.payload.assign(contents.begin() + offset,
                            contents.begin() + offset + length);
        if (!sender.exchange(data, PacketType::Ack)) {
            std::cerr << "fielddrop-send: no ACK for DATA " << sequence
                      << "; transfer failed\n";
            return 1;
        }
    }

    // FIN: next sequence after the final DATA (1 for an empty file).
    Packet fin;
    fin.type = PacketType::Fin;
    fin.sessionId = sessionId;
    fin.sequence = sequence;
    if (!sender.exchange(fin, PacketType::FinAck)) {
        std::cerr << "fielddrop-send: no FIN_ACK for FIN " << sequence
                  << "; transfer failed\n";
        return 1;
    }

    logLine() << "transfer complete: " << contents.size() << " bytes, "
          << sender.transmissions() << " transmissions, "
          << sender.retransmissions() << " retransmissions\n";
    return 0;
}
