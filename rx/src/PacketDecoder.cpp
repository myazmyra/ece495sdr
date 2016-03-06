#include "PacketDecoder.hpp"

PacketDecoder::PacketDecoder() : preamble_size(2), data_size(12), checksum_size(2),
                                 packet_size(preamble_size + data_size + checksum_size),
                                 LFSR_one(30), LFSR_two(178), num_packets(3) {
    //form one vector with just preamble in it (all other bytes are zero),...
    //...then use it to construct two vectors
    std::vector<uint8_t> empty_packet;
    //push two preamble bytes
    empty_packet.push_back(LFSR_one);
    empty_packet.push_back(LFSR_two);
    //push 12 data bytes (all ZEROS)
    for(int i = 0; i < data_size; i++) {
        empty_packet.push_back(0);
    }
    //push two checksum bytes (all ZEROS)
    empty_packet.push_back(0);
    empty_packet.push_back(0);

    preamble_vector = bytes_to_bits(empty_packet);
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

std::vector<uint8_t> PacketDecoder::correlate(std::vector<uint8_t> x, std::vector<uint8_t> y) {
    std::reverse(x.begin(), x.end());
    std::vector<uint8_t> r(x.size() + y.size() - 1);
    for(int i = 0; i < (int) r.size(); i++) {
        int ii = i;
        uint8_t tmp = 0;
        for(int j = 0; j < (int) y.size(); j++) {
            if(ii >= 0 && ii < (int) x.size()) {
                tmp += x[ii] * y[j];
            }
            ii--;
            r[i] = tmp;
        }
    }
    return r;
}

std::vector<uint8_t> PacketDecoder::bytes_to_bits(std::vector<uint8_t> packets) {
    std::vector<uint8_t> bits;
    for(int i = 0; i < (int) packets.size(); i++) {
        uint8_t byte = packets[i];
        for(int j = 0; j < 8; j++) {
            bits.push_back(byte & 1);
            byte >>= 1;
        }
    }
    return bits;
}
