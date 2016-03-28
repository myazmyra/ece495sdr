#include "PacketDecoder.hpp"

PacketDecoder::PacketDecoder(int preamble_size,
                             int data_size,
                             int checksum_size,
                             int packet_size,
                             std::vector<int> preamble_vector) :
                             preamble_size(preamble_size),
                             data_size(data_size),
                             checksum_size(checksum_size),
                             packet_size(data_size) {

    this->preamble_vector = preamble_vector;

}

PacketDecoder::~PacketDecoder() {
    std::cout << "Destroying PacketDecoder object..." << std::endl << std::endl;
}

std::vector<uint8_t> PacketDecoder::decode(std::vector<int> pulses) {

    std::vector<int> r = correlate(pulses, preamble_vector);
    int start_index = std::distance(r.begin(),
                                    std::max_element(r.begin(), r.end())) + 1;
    start_index -=  (int) preamble_vector.size();
    start_index = start_index < 0 ? start_index + (int) preamble_vector.size() :
                  start_index;

    std::vector<uint8_t> packet;
    std::vector<uint8_t> bytes;

    //complete the previous_pulses size to packet_size * 8
    if((int) previous_pulses.size() != 0) {
        previous_pulses.insert(previous_pulses.end(), pulses.begin(),
                               pulses.begin() + (packet_size * 8 - previous_pulses.size()));
        packet = pulses_to_bytes(previous_pulses, 0);
        bytes.insert(bytes.end(), packet.begin(), packet.end());
        packet.clear();
    }
    previous_pulses.clear();

    //if full packet can be formed from the current start_index...
    //...decode bits starting from start_index
    if(start_index + packet_size * 8 <= (int) pulses.size() && start_index < (int) packet_size * 8) {
        packet = pulses_to_bytes(pulses, start_index);
        bytes.insert(bytes.end(), packet.begin(), packet.end());
        packet.clear();
        previous_pulses.insert(previous_pulses.end(),
                               pulses.begin() + start_index + packet_size * 8,
                               pulses.end());
    } else {
        previous_pulses.insert(previous_pulses.end(),
                               pulses.begin() + start_index, pulses.end());
    }

    return bytes;
}

std::vector<int> PacketDecoder::correlate(std::vector<int> const &x, std::vector<int> const &y) const {
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

std::vector<uint8_t> PacketDecoder::pulses_to_bytes(std::vector<int> const &pulses, int start_index) const {
    std::vector<uint8_t> bytes;
    if(start_index + packet_size * 8 > (int) pulses.size()) {
        std::cout << "Unexpected start_index and pulses.size()" << std::endl;
        throw new std::runtime_error("Unexpected start_index and pulses.size()");
    }
    //get the bytes
    int data_start_index = start_index + preamble_size * 8;
    int data_end_index = data_start_index + data_size * 8;
    for(int i = data_start_index; i < data_end_index; i += 8) { //iterate through bytes
        uint8_t byte = 0;
        uint8_t mask = 1;
        for(int j = i; j < i + 8; j++) { //iterate through bits
            byte |= ((pulses[j] > 0) ? mask : 0);
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
        for(int j = i; j < i + 8; j++) { //iterate through bits
            checksum1 |= ((pulses[j] > 0) ? mask : 0);
            mask <<= 1;
        }
    }
    //get the second checksum
    uint8_t checksum2 = 0;
    checksum_start_index = checksum_end_index;
    checksum_end_index = checksum_start_index + (checksum_size / 2) * 8;
    for(int i = checksum_start_index; i < checksum_end_index; i += 8) {
        uint8_t mask = 1;
        for(int j = i; j < i + 8; j++) { //iterate through bits
            checksum2 |= ((pulses[j] > 0) ? mask : 0);
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
    //if everything went well
    return bytes;
}
