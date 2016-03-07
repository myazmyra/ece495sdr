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
    //create an no-data bit packet
    std::vector<uint8_t> empty_packet_bits = bytes_to_bits(empty_packet);
    //concatenate it two times to form preamble detection vector
    std::vector<int> empty_packet_pulses(empty_packet_bits.size());
    for(int i = 0; i < (int) empty_packet_bits.size(); i++) {
        empty_packet_pulses[i] = empty_packet_bits[i] ? 1 : -1;
    }
    preamble_vector = empty_packet_pulses;
    preamble_vector.insert(preamble_vector.end(), empty_packet_bits.begin(), empty_packet_bits.end());
}

PacketDecoder::~PacketDecoder() {
    std::cout << "Destroying PacketDecoder object..." << std::endl << std::endl;
}

std::vector<uint8_t> PacketDecoder::decode(std::vector<int> pulses) {
    std::vector<int> r = correlate(pulses, preamble_vector);
    //find start of the preamble by correlating witht the preamble_vector and then moding with 16 * 8 = 128
    int start_index = (std::distance(r.begin(), std::max_element(r.begin(), r.end())) % pulses.size() + 1) % (packet_size * 8);
    std::cout << "Start Index: " << start_index << std::endl << std::endl;
    std::vector<uint8_t> bytes;
    for(int i = 0; i < (int) pulses.size(); i += packet_size * 8) {//iterate through packets
        for(int j = i + preamble_size * 8; j < i + (preamble_size + data_size) * 8; j += 8) { //iterate through data bytes
            uint8_t byte = 0;
            uint8_t mask = 1;
            for(int k = j; k < j + 8; k++) {//iterate through bits
                byte |= pulses[k] > 0 ? mask : 0;
                mask <<= 1;
            }
            bytes.push_back(byte);
        }
    }
    return bytes;
}

std::vector<int> PacketDecoder::correlate(std::vector<int> x, std::vector<int> y) {
    std::reverse(x.begin(), x.end());
    std::vector<int> r(x.size() + y.size() - 1);
    for(int i = 0; i < (int) r.size(); i++) {
        int ii = i;
        int tmp = 0;
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
