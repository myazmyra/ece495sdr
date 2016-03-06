#include "PacketDecoder.hpp"

PacketDecoder::PacketDecoder() : preamble_size(2), data_size(12), checksum_size(2),
                                 packet_size(preamble_size + data_size + checksum_size),
                                 LFSR_one(30), LFSR_two(178) {
    int i = 0;
    i++;
}

PacketDecoder::~PacketDecoder() {
    std::cout << "Destroying PacketDecoder object..." << std::endl << std::endl;
}

std::vector<uint8_t> PacketDecoder::decode(std::vector<uint8_t> bits) {
    std::vector<uint8_t> bytes;
    for(int i = 0; i < (int) bits.size(); i += packet_size * 8) {//iterate through packets
        for(int j = i + preamble_size * 8; j < i + (preamble_size + data_size) * 8; j += 8) { //iterate through data bytes
            uint8_t byte = 0;
            uint8_t mask = 1;
            for(int k = j; k < j + 8; k++) {//iterate through bits
                byte |= bits[k] ? mask : 0;
                mask <<= 1;
            }
            bytes.push_back(byte);
        }
    }
    return bytes;
}
