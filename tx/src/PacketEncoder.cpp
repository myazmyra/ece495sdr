#include "PacketEncoder.hpp"

PacketEncoder::PacketEncoder(size_t preamble_size,
                             size_t data_size,
                             size_t checksum_size,
                             size_t packet_size,
                             std::vector<uint8_t> preamble_bytes) :
                             preamble_size(preamble_size),
                             data_size(data_size),
                             checksum_size(checksum_size),
                             packet_size(packet_size) {

    this->preamble_size = preamble_size;
    this->data_size = data_size;
    this->checksum_size = checksum_size;
    this->packet_size = packet_size;

    if(this->preamble_size != preamble_bytes.size()) {
      std::cout << "preamble_size is not equal to preamble_bytes.size()" << std::endl;
      throw new std::runtime_error("preamble_size is not equal to preamble_bytes.size()");
    }
    for(int i = 0; i < (int) preamble_bytes.size(); i++) {
        (this->preamble_bytes).push_back(preamble_bytes[i]);
    }
}

PacketEncoder::~PacketEncoder() {
    std::cout << "Destroying PacketEncoder object..." << std::endl << std::endl;
}

std::vector<uint8_t> PacketEncoder::form_packets(char* data, size_t size) const {

    //TODO: first, encode how many bytes are there in total

    size_t num_packets = size / data_size;

    if(num_packets < 1) {
        std::cout << "num_packets cannot be ZERO" << std::endl << std::endl;
        throw new std::runtime_error("num_packets cannot be ZERO");
    }

    std::vector<uint8_t> packets;
    for(int i = 0; i < (int) num_packets; i++) {
        for(int i = 0; (int) preamble_bytes.size(); i++) {
          packets.push_back(preamble_bytes[i]);
        }
        //first part
        uint8_t checksum1 = 0;
        for(int j = 0; j < (int) data_size / 2; j++) {
            uint8_t byte = (uint8_t) data[data_size * i + j];
            checksum1 ^= byte;
            packets.push_back(byte);
        }
        //second part
        uint8_t checksum2 = 0;
        for(int j = data_size / 2; j < (int) data_size; j++) {
            uint8_t byte = (uint8_t) data[data_size * i + j];
            checksum2 ^= byte;
            packets.push_back(byte);
        }
        packets.push_back(checksum1);
        packets.push_back(checksum2);
    }

    int remaining_bytes = size % data_size;

    //if added everything, return
    if(remaining_bytes == 0) {
        //transform bytes to bits and return
        return bytes_to_bits(packets);
    }

    //pad anything leftover
    for(int i = 0; i < (int) preamble_bytes.size(); i++) {
      packets.push_back(preamble_bytes[i]);
    }

    //count number of bytes added, could be calculated from the loop index...
    //...but code would become even uglier
    int count = 0;
    //first part
    uint8_t checksum1 = 0;
    uint8_t checksum2 = 0;
    for(int i = num_packets * data_size; i < (int) (num_packets * data_size + remaining_bytes); i++) {
        count++;
        uint8_t byte = (uint8_t) data[i];
        if(count <= (int) data_size / 2) {
            checksum1 ^= byte;
        } else {
            checksum2 ^= byte;
        }
        packets.push_back(byte);
    }
    //second part
    //pad zeros
    int num_padded_zeros = data_size - remaining_bytes;
    for(int i = 0; i < num_padded_zeros; i++) {
        packets.push_back(0);
    }
    packets.push_back(checksum1);
    packets.push_back(checksum2);

    //transform bytes to bits and return
    return bytes_to_bits(packets);
}

std::vector<uint8_t> PacketEncoder::bytes_to_bits(std::vector<uint8_t> packets) const {
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
