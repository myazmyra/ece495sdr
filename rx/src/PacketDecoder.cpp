#include "PacketDecoder.hpp"

PacketDecoder::PacketDecoder(size_t preamble_size,
                             size_t data_size,
                             size_t checksum_size,
                             size_t packet_size,
                             std::vector<int> preamble_vector) :
                             preamble_size(preamble_size),
                             data_size(data_size),
                             checksum_size(checksum_size),
                             packet_size(packet_size),
                             preamble_vector(preamble_vector),
                             total_size(0),
                             received_size(0),
                             streaming_started(false),
                             streaming_started(false) {
}

PacketDecoder::~PacketDecoder() {
    std::cout << "Destroying PacketDecoder object..." << std::endl << std::endl;
}

std::vector<uint8_t> PacketDecoder::decode(std::vector<int> pulses) {

    std::vector<int> r = correlate(pulses, preamble_vector);
    int start_index = std::distance(r.begin(), std::max_element(r.begin(), r.end())) + 1;

    start_index -=  (int) preamble_vector.size();
    start_index = start_index < 0 ? start_index + (int) preamble_vector.size() : start_index;

    std::vector<uint8_t> packet;
    std::vector<uint8_t> bytes;

    //complete the previous_pulses size to packet_size * 8
    if((int) previous_pulses.size() != 0) {
        previous_pulses.insert(previous_pulses.end(), pulses.begin(),
                               pulses.begin() + (packet_size * 8 - previous_pulses.size()));
        packet = pulses_to_bytes(previous_pulses, 0);
        previous_pulses.clear();
        bytes.insert(bytes.end(), packet.begin(), packet.end());
        packet.clear();
    }

    //if full packet can be formed from the current start_index...
    //...decode bits starting from start_index
    if(start_index + (int) packet_size * 8 <= (int) pulses.size() && start_index < (int) packet_size * 8) {
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

std::vector<uint8_t> PacketDecoder::pulses_to_bytes(std::vector<int> const &pulses, int start_index)  {
    if(streaming_ended) {
        std::vector<uint8_t> empty_bytes;
        return empty_bytes;
    }
    std::vector<uint8_t> bytes;
    if(start_index + (int) packet_size * 8 > (int) pulses.size()) {
        std::cout << "Unexpected start_index and pulses.size()" << std::endl;
        throw std::runtime_error("Unexpected start_index and pulses.size()");
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
    if(streaming_started) {
        received_size += data_size;
    }

    //get the checksums
    std::vector<uint8_t> checksums;
    for(int i = 0; i < (int) checksum_size; i++) {
        uint8_t checksum = 0;
        uint8_t mask = 1;
        for(int j = 0; j < 8; j++) {
            checksum |= ((pulses[data_end_index + i * 8 + j] > 0) ? mask : 0);
            mask <<= 1;
        }
        checksums.push_back(checksum);
    }

    for(int i = 0; i < (int) checksum_size; i++) {
        uint8_t checksum = checksums[i];
        for(int j = (int) ((i + 1) * data_size / checksum_size - data_size / checksum_size);
                j < (int) ((i + 1) * data_size / checksum_size);
                j++) {
                    uint8_t byte = (uint8_t) bytes[j];
                    checksum ^= byte;
        }
        if(checksum == (uint8_t) (~0) && total_size == 0) {
            int shift_size = 0;
            //TX and RX machines should have same number of bytes per int
            for(int j = 0; j < (int) sizeof(int); j++) {
                total_size |= (((int) bytes[j]) << shift_size);
                shift_size += 8;
            }
            std::vector<uint8_t> empty_bytes;
            return empty_bytes;
        } else if(checksum == (uint8_t) (~0) && total_size != 0) {
            std::vector<uint8_t> empty_bytes;
            return empty_bytes;
        } else if(checksum != 0) {
            std::vector<uint8_t> empty_bytes;
            return empty_bytes;
        }
    }
    //if first packet checksums pass
    if(not streaming_started && total_size != 0) {
        std::cout << "File streaming started" << std::endl;
        std::cout << "Total file size to receive in bytes: " << total_size << std::endl << std::endl;
        streaming_started = true;
        received_size = data_size;
    }
    //remove redundant bytes if needed
    if(streaming_ended == false && received_size >= total_size) {
        streaming_ended = true;
        bytes.erase(bytes.end() - (received_size - total_size), bytes.end());
        std::cout << "File streaming ended" << std::endl << std::endl;
    }
    if(streaming_started && total_size != 0) {
        return bytes;
    }
    std::vector<uint8_t> empty_bytes;
    return empty_bytes;
}
