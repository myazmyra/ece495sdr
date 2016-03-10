#include "PacketDecoder.hpp"

PacketDecoder::PacketDecoder() : preamble_size(2), data_size(12), checksum_size(2),
                                 packet_size(preamble_size + data_size + checksum_size),
                                 LFSR_one(30), LFSR_two(178), num_packets_per_call(2) {
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
    std::vector<int> empty_packet_pulses(empty_packet_bits.size());
    for(int i = 0; i < (int) empty_packet_bits.size(); i++) {
        empty_packet_pulses[i] = empty_packet_bits[i] ? 1 : -1;
    }
    for(int i = 0; i < num_packets_per_call; i++) {
        preamble_vector.insert(preamble_vector.end(), empty_packet_pulses.begin(), empty_packet_pulses.end());
    }
}

PacketDecoder::~PacketDecoder() {
    std::cout << "Destroying PacketDecoder object..." << std::endl << std::endl;
}

std::vector<uint8_t> PacketDecoder::decode(std::vector<int> pulses) {
    std::vector<int> r = correlate(pulses, preamble_vector);
    //find start of the preamble by correlating witht the preamble_vector and then moding with 16 * 8 = 128
    int start_index = (std::distance(r.begin(), std::max_element(r.begin(), r.end())) % pulses.size() + 1) % (packet_size * 8);
    std::cout << "start_index: " << start_index << std::endl << std::endl;

    std::vector<uint8_t> bytes;

    //decode stuff from previous buffer first
    previous_pulses.insert(previous_pulses.end(), pulses.begin(), pulses.begin() + start_index);
    std::vector<uint8_t> previous_packet = packet_to_bytes(previous_pulses, 0);
    bytes.insert(bytes.end(), previous_packet.begin(), previous_packet.end());

    previous_pulses.clear();
    size_t new_size = previous_pulses.size() + (size_t) (pulses.end() -
                      (pulses.begin() + start_index + packet_size * (num_packets_per_call - 1) * 8));
    //need to check if a previous packet is fully formed
    //it will fail to form in case of very low SNR, no transmission of packets, etc.
    //because preamble correlator will detect wrong start of the packet (obviously)
    if((int) new_size != packet_size * 8) {
        std::cout << "whaat: " << (int) new_size << std::endl << std::endl;
        previous_pulses.clear();
    }
    previous_pulses.insert(previous_pulses.end(), pulses.begin() + start_index + packet_size * (num_packets_per_call - 1) * 8, pulses.end());

    //decode the num_packets_per_call-1 guaranteed packets to exist
    for(int i = 0; i < num_packets_per_call - 1; i++) {
        std::vector<uint8_t> data = packet_to_bytes(pulses, start_index + i * (packet_size * 8));
        bytes.insert(bytes.end(), data.begin(), data.end());
    }

    if(((int) previous_pulses.size()) == packet_size * 8) {
        //means full packet is formed
        std::vector<uint8_t> previous_packet = packet_to_bytes(previous_pulses, 0);
        bytes.insert(bytes.end(), previous_packet.begin(), previous_packet.end());
        previous_pulses.clear();
    }

    return bytes;
}

std::vector<int> PacketDecoder::correlate(std::vector<int> x, std::vector<int> y) {
    std::vector<int> r(x.size() + y.size() - 1);
    for(int i = 0; i < (int) r.size(); i++) {
        int ii = ((int) y.size()) - i - 1;
        int tmp = 0;
        for(int j = 0; j < (int) x.size(); j++) {
            if(ii >= 0 && ii < (int) y.size()) {
                tmp += y[ii] * x[j];
            }
            ii++;
        }
        r[i] = tmp;
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

int PacketDecoder::get_packet_size() const {
    return packet_size;
}

int PacketDecoder::get_num_packets_per_call() const {
    return num_packets_per_call;
}
