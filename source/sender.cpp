#include "fielddrop_packet.hpp"

#include <iostream>
#include <string_view>

namespace {
void usage(const char* program) {
    std::cerr << "Usage: " << program
              << " INPUT_FILE RECEIVER_HOST RECEIVER_PORT\n";
}
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        usage(argv[0]);
        return 0;
    }
    if (argc != 4) {
        usage(argv[0]);
        return 2;
    }

    // TODO: Validate the file and port, resolve the IPv4 destination,
    // serialize packets, and implement HELLO/DATA/FIN with retransmission.
    std::cerr << "fielddrop-send: not implemented\n";
    return 2;
}
