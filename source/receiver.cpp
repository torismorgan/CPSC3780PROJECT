
#include "fielddrop_packet.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace fielddrop;
using Clock = std::chrono::steady_clock;

void usage(const char* program) {
    std::cerr << "Usage: " << program << " OUTPUT_FILE LISTEN_PORT\n";
}

bool parsePort(const char* text, std::uint16_t& port) {
    if (text == nullptr || *text == '\0') return false;

    unsigned long value = 0;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return false;

        value = value * 10 + static_cast<unsigned long>(*p - '0');
        if (value > 65535) return false;
    }

    if (value == 0) return false;

    port = static_cast<std::uint16_t>(value);
    return true;
}

bool pathExists(const std::string& path) {
    struct stat info {};
    return ::lstat(path.c_str(), &info) == 0 || errno != ENOENT;
}

bool sameEndpoint(const sockaddr_in& a, const sockaddr_in& b) {
    return a.sin_family == AF_INET &&
           b.sin_family == AF_INET &&
           a.sin_addr.s_addr == b.sin_addr.s_addr &&
           a.sin_port == b.sin_port;
}

std::uint32_t readFileSize(const std::vector<std::uint8_t>& payload) {
    return (static_cast<std::uint32_t>(payload[0]) << 24) |
           (static_cast<std::uint32_t>(payload[1]) << 16) |
           (static_cast<std::uint32_t>(payload[2]) << 8) |
           static_cast<std::uint32_t>(payload[3]);
}

bool writeAll(int fd, const std::uint8_t* data, std::size_t size) {
    std::size_t written = 0;

    while (written < size) {
        const ssize_t result =
            ::write(fd, data + written, size - written);

        if (result < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        if (result == 0) return false;
        written += static_cast<std::size_t>(result);
    }

    return true;
}

bool sendPacket(int fd, const sockaddr_in& peer, const Packet& packet) {
    std::vector<std::uint8_t> wire;
    std::string error;

    if (!serializePacket(packet, wire, &error)) {
        std::cerr << "receiver: cannot encode response: "
                  << error << "\n";
        return false;
    }

    const ssize_t sent = ::sendto(
        fd,
        wire.data(),
        wire.size(),
        0,
        reinterpret_cast<const sockaddr*>(&peer),
        sizeof(peer)
    );

    if (sent < 0 || static_cast<std::size_t>(sent) != wire.size()) {
        std::cerr << "receiver: sendto failed: "
                  << std::strerror(errno) << "\n";
        return false;
    }

    return true;
}

Packet makeReply(PacketType type,
                 std::uint32_t session,
                 std::uint32_t sequence) {
    Packet reply;
    reply.type = type;
    reply.sessionId = session;
    reply.sequence = sequence;
    return reply;
}

bool createTemporaryFile(const std::string& outputPath,
                         std::string& temporaryPath,
                         int& temporaryFd) {
    // The temporary file is placed beside OUTPUT_FILE so the final
    // rename happens on the same filesystem.
    for (int attempt = 0; attempt < 100; ++attempt) {
        temporaryPath =
            outputPath + ".fielddrop.tmp." +
            std::to_string(static_cast<long long>(getpid())) +
            "." + std::to_string(attempt);

        temporaryFd = ::open(
            temporaryPath.c_str(),
            O_WRONLY | O_CREAT | O_EXCL,
            0600
        );

        if (temporaryFd >= 0) return true;

        if (errno != EEXIST) {
            std::cerr << "receiver: cannot create temporary file: "
                      << std::strerror(errno) << "\n";
            return false;
        }
    }

    std::cerr << "receiver: could not find an unused temporary filename\n";
    return false;
}

void removeTemporaryFile(int& fd, const std::string& path) {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }

    if (!path.empty()) {
        ::unlink(path.c_str());
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        usage(argv[0]);
        return 0;
    }

    if (argc != 3) {
        std::cerr << "fielddrop-recv: expected 2 arguments\n";
        usage(argv[0]);
        return 2;
    }

    const std::string outputPath = argv[1];

    std::uint16_t port = 0;
    if (!parsePort(argv[2], port)) {
        std::cerr << "fielddrop-recv: invalid LISTEN_PORT; "
                     "expected a decimal integer from 1 to 65535\n";
        return 2;
    }

    // Do not overwrite an existing file, directory, or symbolic link.
    if (pathExists(outputPath)) {
        std::cerr << "fielddrop-recv: output path already exists: "
                  << outputPath << "\n";
        return 1;
    }

    const int socketFd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketFd < 0) {
        std::cerr << "fielddrop-recv: cannot create UDP socket: "
                  << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_in local {};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);

    if (::bind(socketFd,
               reinterpret_cast<const sockaddr*>(&local),
               sizeof(local)) < 0) {
        std::cerr << "fielddrop-recv: bind failed: "
                  << std::strerror(errno) << "\n";
        ::close(socketFd);
        return 1;
    }

    std::cerr << "fielddrop-recv: listening on UDP port "
              << port << "\n";

    bool active = false;
    bool finished = false;

    sockaddr_in activePeer {};
    std::uint32_t activeSession = 0;
    std::uint32_t advertisedSize = 0;
    std::uint32_t receivedBytes = 0;
    std::uint32_t expectedSequence = 1;
    std::uint32_t finalSequence = 0;

    std::string temporaryPath;
    int temporaryFd = -1;

    Clock::time_point lastActivity;
    Clock::time_point lastFin;

    // One byte larger than the largest legal datagram, so an oversized
    // datagram cannot be mistaken for a valid truncated packet.
    std::uint8_t buffer[kHeaderBytes + kMaxPayloadBytes + 1];

    while (true) {
        int timeout = -1;

        if (finished) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    lastFin + std::chrono::milliseconds(kFinLingerMs)
                    - Clock::now()
                ).count();

            if (remaining <= 0) break;
            timeout = static_cast<int>(remaining);
        } else if (active) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    lastActivity +
                    std::chrono::milliseconds(kReceiverIdleMs)
                    - Clock::now()
                ).count();

            if (remaining <= 0) {
                std::cerr << "fielddrop-recv: transfer timed out\n";
                removeTemporaryFile(temporaryFd, temporaryPath);
                ::close(socketFd);
                return 1;
            }

            timeout = static_cast<int>(remaining);
        }

        pollfd descriptor {socketFd, POLLIN, 0};
        const int ready = ::poll(&descriptor, 1, timeout);

        if (ready < 0) {
            if (errno == EINTR) continue;

            std::cerr << "fielddrop-recv: poll failed: "
                      << std::strerror(errno) << "\n";
            removeTemporaryFile(temporaryFd, temporaryPath);
            ::close(socketFd);
            return 1;
        }

        if (ready == 0) {
            if (finished) break;

            if (active) {
                std::cerr << "fielddrop-recv: transfer timed out\n";
                removeTemporaryFile(temporaryFd, temporaryPath);
                ::close(socketFd);
                return 1;
            }

            continue;
        }

        sockaddr_in from {};
        socklen_t fromLength = sizeof(from);

        const ssize_t count = ::recvfrom(
            socketFd,
            buffer,
            sizeof(buffer),
            0,
            reinterpret_cast<sockaddr*>(&from),
            &fromLength
        );

        if (count < 0) {
            if (errno == EINTR) continue;

            std::cerr << "fielddrop-recv: recvfrom failed: "
                      << std::strerror(errno) << "\n";
            removeTemporaryFile(temporaryFd, temporaryPath);
            ::close(socketFd);
            return 1;
        }

        if (fromLength != sizeof(from) || from.sin_family != AF_INET) {
            continue;
        }

        Packet packet;
        std::string parseError;

        if (!parsePacket(buffer, static_cast<std::size_t>(count),
                         packet, &parseError)) {
            std::cerr << "fielddrop-recv: ignored invalid packet: "
                      << parseError << "\n";
            continue;
        }

        // Before a transfer starts, only a valid HELLO can establish
        // the active sender and session.
        if (!active) {
            if (packet.type != PacketType::Hello ||
                packet.payload.size() != 4) {
                continue;
            }

            const std::uint32_t fileSize = readFileSize(packet.payload);

            if (fileSize > kMaxFileBytes) {
                std::cerr << "fielddrop-recv: advertised file is too large\n";
                continue;
            }

            // Check again immediately before creating the temporary file.
            if (pathExists(outputPath)) {
                std::cerr << "fielddrop-recv: output path already exists: "
                          << outputPath << "\n";
                ::close(socketFd);
                return 1;
            }

            if (!createTemporaryFile(
                    outputPath, temporaryPath, temporaryFd)) {
                ::close(socketFd);
                return 1;
            }

            active = true;
            activePeer = from;
            activeSession = packet.sessionId;
            advertisedSize = fileSize;
            receivedBytes = 0;
            expectedSequence = 1;
            lastActivity = Clock::now();

            std::cerr << "fielddrop-recv: accepted HELLO, session 0x"
                      << std::hex << activeSession << std::dec
                      << ", advertised size " << advertisedSize << " bytes\n";

            if (!sendPacket(socketFd, activePeer,
                            makeReply(PacketType::Ack,
                                      activeSession, 0))) {
                removeTemporaryFile(temporaryFd, temporaryPath);
                ::close(socketFd);
                return 1;
            }

            continue;
        }

        // Only the endpoint and session established by the first HELLO
        // may control this transfer.
        if (!sameEndpoint(from, activePeer) ||
            packet.sessionId != activeSession) {
            continue;
        }

        lastActivity = Clock::now();

        // After completion, keep responding to duplicate FIN packets
        // during the required FIN linger period.
        if (finished) {
            if (packet.type == PacketType::Fin &&
                packet.sequence == finalSequence) {
                sendPacket(socketFd, activePeer,
                           makeReply(PacketType::FinAck,
                                     activeSession, finalSequence));
                lastFin = Clock::now();
            }
            continue;
        }

        switch (packet.type) {
            case PacketType::Hello: {
                // A repeated HELLO from the same sender/session must not
                // reset the transfer or discard data already received.
                if (packet.sequence == 0 &&
                    packet.payload.size() == 4 &&
                    readFileSize(packet.payload) == advertisedSize) {
                    sendPacket(socketFd, activePeer,
                               makeReply(PacketType::Ack,
                                         activeSession, 0));
                }
                break;
            }

            case PacketType::Data: {
                if (packet.sequence == expectedSequence) {
                    const std::size_t amount = packet.payload.size();

                    if (amount > advertisedSize - receivedBytes) {
                        std::cerr << "fielddrop-recv: DATA exceeds "
                                     "advertised file size\n";
                        break;
                    }

                    if (!writeAll(temporaryFd,
                                  packet.payload.data(), amount)) {
                        std::cerr << "fielddrop-recv: write failed: "
                                  << std::strerror(errno) << "\n";
                        removeTemporaryFile(temporaryFd, temporaryPath);
                        ::close(socketFd);
                        return 1;
                    }

                    receivedBytes += static_cast<std::uint32_t>(amount);
                    const std::uint32_t acknowledged = expectedSequence;
                    ++expectedSequence;

                    sendPacket(socketFd, activePeer,
                               makeReply(PacketType::Ack,
                                         activeSession, acknowledged));

                    std::cerr << "fielddrop-recv: received DATA "
                              << acknowledged << ", "
                              << receivedBytes << "/" << advertisedSize
                              << " bytes\n";
                } else if (expectedSequence > 1 &&
                           packet.sequence == expectedSequence - 1) {
                    // Re-ACK the immediately previous DATA packet without
                    // writing its payload a second time.
                    sendPacket(socketFd, activePeer,
                               makeReply(PacketType::Ack,
                                         activeSession, packet.sequence));
                }

                break;
            }

            case PacketType::Fin: {
                if (packet.sequence != expectedSequence ||
                    receivedBytes != advertisedSize) {
                    std::cerr << "fielddrop-recv: ignored FIN: "
                                 "sequence or file size does not match\n";
                    break;
                }

                // Flush the completed file before publishing it.
                if (::fsync(temporaryFd) < 0) {
                    std::cerr << "fielddrop-recv: fsync failed: "
                              << std::strerror(errno) << "\n";
                    removeTemporaryFile(temporaryFd, temporaryPath);
                    ::close(socketFd);
                    return 1;
                }

                if (::close(temporaryFd) < 0) {
                    temporaryFd = -1;
                    std::cerr << "fielddrop-recv: closing temporary file failed\n";
                    removeTemporaryFile(temporaryFd, temporaryPath);
                    ::close(socketFd);
                    return 1;
                }
                temporaryFd = -1;

                // Do not overwrite a path that appeared during the transfer.
                if (pathExists(outputPath)) {
                    std::cerr << "fielddrop-recv: output path appeared "
                                 "during transfer; refusing to overwrite\n";
                    removeTemporaryFile(temporaryFd, temporaryPath);
                    ::close(socketFd);
                    return 1;
                }

                // The temporary file is in the output directory, so rename
                // publishes the completed file on the same filesystem.
                if (::rename(temporaryPath.c_str(), outputPath.c_str()) < 0) {
                    std::cerr << "fielddrop-recv: cannot rename completed file: "
                              << std::strerror(errno) << "\n";
                    removeTemporaryFile(temporaryFd, temporaryPath);
                    ::close(socketFd);
                    return 1;
                }

                temporaryPath.clear();
                finished = true;
                finalSequence = packet.sequence;
                lastFin = Clock::now();

                std::cerr << "fielddrop-recv: file completed: "
                          << outputPath << " (" << receivedBytes
                          << " bytes)\n";

                sendPacket(socketFd, activePeer,
                           makeReply(PacketType::FinAck,
                                     activeSession, finalSequence));
                break;
            }

            case PacketType::Ack:
            case PacketType::FinAck:
                // These are sender-side response types, not receiver input
                // for an active transfer.
                break;
        }
    }

    ::close(socketFd);

    std::cerr << "fielddrop-recv: finished successfully\n";
    return 0;
}
