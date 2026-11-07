#include "fielddrop_packet.hpp"

#include <iostream>
#include <string_view>

namespace {
void usage(const char* program) {
    std::cerr << "Usage: " << program << " OUTPUT_FILE LISTEN_PORT\n";
}
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        usage(argv[0]);
        return 0;
    }
    if (argc != 3) {
        usage(argv[0]);
        return 2;
    }

    // TODO: Bind an IPv4 UDP socket, validate incoming packets, manage a
    // single session, ACK accepted data, and finish through a temporary file.
    std::cerr << "fielddrop-recv: not implemented\n";
    return 2;
}
