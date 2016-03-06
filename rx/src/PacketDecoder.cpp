#include "PacketDecoder.hpp"

PacketDecoder::PacketDecoder() : data_per_packet(12),
                                 LFSR_one(30), LFSR_two(178) {
    int i = 0;
    i++;
}

PacketDecoder::~PacketDecoder() {
    std::cout << "Destroying PacketDecoder object..." << std::endl << std::endl;
}

std::vector<uint8_t> PacketDecoder::decode(std::vector<uint8_t> bits) {
    return bits;
}
