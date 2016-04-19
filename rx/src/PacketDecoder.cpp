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
                             previous_pulses(packet_size * 8),
                             previous_pulses_size(0),
                             total_size(0),
                             received_size(0),
                             streaming_started(false),
                             streaming_ended(false) {
}

PacketDecoder::~PacketDecoder() {
    std::cout << "Destroying PacketDecoder object..." << std::endl << std::endl;
}

size_t PacketDecoder::decode(std::vector<int> const &pulses, size_t pulses_size, std::vector<uint8_t> &bytes) {

    //return size
    size_t bytes_size = 0;

    static std::vector<int> rxy(2 * packet_size * 8 + preamble_vector.size() - 1);
    if(pulses_size + preamble_vector.size() - 1 > rxy.size()) {
        rxy.resize(pulses_size + preamble_vector.size() - 1);
    }
    size_t rxy_size = correlate(pulses, preamble_vector, rxy);
    int start_index = std::distance(rxy.begin(), std::max_element(rxy.begin(), rxy.begin() + rxy_size)) + 1;

    start_index -=  (int) preamble_vector.size();
    start_index = start_index < 0 ? start_index + (int) preamble_vector.size() : start_index;

    //complete the previous_pulses size to packet_size * 8
    if((int) previous_pulses_size != 0) {
        std::copy(pulses.begin(), pulses.begin() + (packet_size * 8 - previous_pulses_size),
                  previous_pulses.begin() + previous_pulses_size);
        bytes_size += pulses_to_bytes(previous_pulses, 0, bytes, bytes_size);
        previous_pulses_size = 0;
    }

    //if full packet can be formed from the current start_index...
    //...decode bits starting from start_index
    if(start_index < (int) packet_size * 8 && start_index + (int) packet_size * 8 <= (int) pulses.size()) {
        bytes_size += pulses_to_bytes(pulses, start_index, bytes, bytes_size);
        std::copy(pulses.begin() + start_index + packet_size * 8, pulses.end(),
                  previous_pulses.begin() + previous_pulses_size);
        previous_pulses_size = packet_size * 8 - start_index;
    } else {
        std::copy(pulses.begin() + start_index, pulses.end(),
                  previous_pulses.begin() + previous_pulses_size);
        previous_pulses_size = 2 * packet_size * 8 - start_index;
    }

    return bytes_size;
}

size_t PacketDecoder::pulses_to_bytes(std::vector<int> const &pulses, int start_index, std::vector<uint8_t> &bytes, size_t bytes_size)  {
    if(streaming_ended) {
        return 0;
    }
    if(start_index + (int) packet_size * 8 > (int) pulses.size()) {
        std::cout << "Unexpected start_index and pulses.size()" << std::endl;
        throw std::runtime_error("Unexpected start_index and pulses.size()");
    }

    //get the bytes
    int data_start_index = start_index + preamble_size * 8;
    int data_end_index = data_start_index + data_size * 8;
    for(int i = data_start_index, insert_index = (int) bytes_size; i < data_end_index; i += 8, insert_index++) { //iterate through bytes
        uint8_t byte = 0;
        uint8_t mask = 1;
        for(int j = i; j < i + 8; j++) { //iterate through bits
            byte |= ((pulses[j] > 0) ? mask : 0);
            mask <<= 1;
        }
        bytes[insert_index] = byte;
    }
    if(streaming_started) {
        received_size += data_size;
    }

    static std::vector<uint8_t> checksums(checksum_size);
    for(int i = 0; i < (int) checksum_size; i++) {
        uint8_t checksum = 0;
        uint8_t mask = 1;
        for(int j = 0; j < 8; j++) {
            checksum |= ((pulses[data_end_index + i * 8 + j] > 0) ? mask : 0);
            mask <<= 1;
        }
        checksums[i] = checksum;
    }

    for(int i = 0; i < (int) checksum_size; i++) {
        uint8_t checksum = checksums[i];
        for(int j = (int) ((i + 1) * data_size / checksum_size - data_size / checksum_size);
                j < (int) ((i + 1) * data_size / checksum_size);
                j++) {
                    uint8_t byte = (uint8_t) bytes[(int) bytes_size + j];
                    checksum ^= byte;
                    //std::cout << (int) checksum << std::endl;
        }
        if(checksum == (uint8_t) (~0) && total_size == 0) {
            /*std::cout << (int) checksum << std::endl;
            for(int p = 0; p < (int) packet_size * 8; p++) std::cout << pulses[start_index + p] << ", ";
            std::cout << std::endl;
            */
            int shift_size = 0;
            //TX and RX machines should have same number of bytes per int
            for(int j = 0; j < (int) sizeof(int); j++) {
                total_size |= (((int) bytes[j]) << shift_size);
                shift_size += 8;
            }
            return 0;
        } else if(checksum == (uint8_t) (~0) && total_size != 0) {
            return 0;
        } else if(checksum != 0) {
            std::cout << "fail" << std::endl;
            return 0;
        }
    }
    //if first packet checksums pass
    if(streaming_started == false && total_size != 0) {
        std::cout << "File streaming started" << std::endl;
        std::cout << "Total file size to receive in bytes: " << total_size << std::endl << std::endl;
        streaming_started = true;
        received_size = data_size;
    }
    //remove redundant bytes if needed
    if(streaming_started == true && streaming_ended == false && received_size >= total_size) {
        streaming_ended = true;
        std::cout << "File streaming ended" << std::endl << std::endl;
        return data_size - (received_size - total_size);
    }
    if(streaming_started == true && total_size != 0) {
        std::cout << "success" << std::endl;
        return data_size;
    }
    return 0;
}

size_t PacketDecoder::correlate(std::vector<int> const &x, std::vector<int> const &y, std::vector<int> &rxy) const {
    if(x.size() + y.size() - 1 > rxy.size()) {
        std::cout << "Not enough space to store correlate(x, y) result" << std::endl;
        throw std::runtime_error("Not enough space in rxy to store correlate(x, y) result");
    }
    for(int i = 0; i < (int) rxy.size(); i++) {
        int ii = ((int) y.size()) - i - 1;
        int tmp = 0;
        for(int j = 0; j < (int) x.size(); j++) {
            if(ii >= 0 && ii < (int) y.size()) {
                tmp += y[ii] * x[j];
            }
            ii++;
        }
        rxy[i] = tmp;
    }
    return x.size() + y.size() - 1;
}
