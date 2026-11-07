CXX = g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDLIBS ?= -lz

.PHONY: all clean

all: fielddrop-send fielddrop-recv

fielddrop-send: source/sender.cpp source/fielddrop_packet.hpp
	$(CXX) $(CXXFLAGS) -o $@ source/sender.cpp $(LDLIBS)

fielddrop-recv: source/receiver.cpp source/fielddrop_packet.hpp
	$(CXX) $(CXXFLAGS) -o $@ source/receiver.cpp $(LDLIBS)

clean:
	rm -f fielddrop-send fielddrop-recv
