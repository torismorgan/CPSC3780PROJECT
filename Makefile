CXX = g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDLIBS ?= -lz

PACKET_SRC = source/fielddrop_packet.cpp
PACKET_HDR = source/fielddrop_packet.hpp

.PHONY: all clean check-vectors

all: fielddrop-send fielddrop-recv

fielddrop-send: source/sender.cpp $(PACKET_SRC) $(PACKET_HDR)
	$(CXX) $(CXXFLAGS) -o $@ source/sender.cpp $(PACKET_SRC) $(LDLIBS)

fielddrop-recv: source/receiver.cpp $(PACKET_SRC) $(PACKET_HDR)
	$(CXX) $(CXXFLAGS) -o $@ source/receiver.cpp $(PACKET_SRC) $(LDLIBS)

# Encodes the published ABC packets and compares them with tests/packet-vectors.txt.
packet-vector-check: tests/packet_vector_check.cpp $(PACKET_SRC) $(PACKET_HDR)
	$(CXX) $(CXXFLAGS) -Isource -o $@ tests/packet_vector_check.cpp $(PACKET_SRC) $(LDLIBS)

check-vectors: packet-vector-check
	./packet-vector-check

clean:
	rm -f fielddrop-send fielddrop-recv packet-vector-check
