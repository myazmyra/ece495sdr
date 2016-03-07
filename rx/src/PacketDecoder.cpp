#include "PacketDecoder.hpp"

PacketDecoder::PacketDecoder() : preamble_size(2), data_size(12), checksum_size(2),
                                 packet_size(preamble_size + data_size + checksum_size),
                                 LFSR_one(30), LFSR_two(178), num_packets_per_call(3) {
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
    preamble_vector.insert(preamble_vector.end(), empty_packet_bits.begin(), empty_packet_bits.end());
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

    //decode stuff from previous buffer first
    previous.insert(previous.end(), pulses.begin(), pulses.begin() + start_index);
    //bytes.insert() blah blah


    previous.clear();
    previous.insert(previous.end(), pulses.begin() + start_index * packet_size * (num_packets_per_call - 1) * 8, pulses.end());

    //decode the two guaranteed packets to exist
    std::vector<uint8_t> first_packet = packet_to_bytes(pulses, start_index);
    std::vector<uint8_t> second_packet = packet_to_bytes(pulses, start_index + packet_size * 8);

    bytes.insert(bytes.end(), first_packet.begin(), first_packet.end());
    bytes.insert(bytes.end(), second_packet.begin(), second_packet.end());

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

std::vector<uint8_t> PacketDecoder::packet_to_bytes(std::vector<int> pulses, int start_index) {
    std::vector<uint8_t> bytes;
    if(start_index >= (int) pulses.size()) return bytes;
    //get the bytes
    int data_start_index = start_index + preamble_size * 8;
    int data_end_index = data_start_index + data_size * 8;
    for(int i = data_start_index; i < data_end_index; i += 8) {//iterate through bytes
        uint8_t byte = 0;
        uint8_t mask = 1;
        for(int j = i; j < i + 8; j++) {//iterate through bits
            byte |= pulses[j] > 0 ? mask : 0;
            mask <<= 1;
        }
        bytes.push_back(byte);
    }
    //get the first checksum
    uint8_t checksum1 = 0;
    int checksum_start_index = data_end_index;
    int checksum_end_index = checksum_start_index + (checksum_size / 2) * 8;
    for(int i = checksum_start_index; i < checksum_end_index; i += 8) {
        uint8_t mask = 1;
        for(int j = i; j < i + 8; j++) {//iterate through bits
            checksum1 |= pulses[j] > 0 ? mask : 0;
            mask <<= 1;
        }
    }
    //get the second checksum
    uint8_t checksum2 = 0;
    checksum_start_index = checksum_end_index;
    checksum_end_index = checksum_start_index + (checksum_size / 2) * 8;
    for(int i = checksum_start_index; i < checksum_end_index; i += 8) {
        uint8_t mask = 1;
        for(int j = i; j < i + 8; j++) {//iterate through bits
            checksum2 |= pulses[j] > 0 ? mask : 0;
            mask <<= 1;
        }
    }
    //check the checksums
    for(int i = 0; i < data_size / 2; i++) {
        checksum1 ^= bytes[i];
    }
    for(int i = data_size / 2; i < data_size; i++) {
        checksum2 ^= bytes[i];
    }
    if((checksum1 != 0) || (checksum2 != 0)) {
        std::cout << "Errors detected" << std::endl << std::endl;
        std::vector<uint8_t> empty_bytes;
        return empty_bytes;
    }
    //everything went well
    return bytes;
}

std::vector<uint8_t> PacketDecoder::bytes_to_bits(std::vector<uint8_t> bytes) {
    std::vector<uint8_t> bits;
    for(int i = 0; i < (int) bytes.size(); i++) {
        uint8_t byte = bytes[i];
        for(int j = 0; j < 8; j++) {
            bits.push_back(byte & 1);
            byte >>= 1;
        }
    }
    return bits;
}
