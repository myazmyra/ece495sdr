#include "PacketEncoder.hpp"

//a packet will consist of:
//2 bytes of preamble + 12 bytes of data + 2 bytes of checksum = 16 bytes
const int PacketEncoder::data_per_packet = 12;
const uint8_t PacketEncoder::LFSR_one = 120; //first byte of 15 bit LFSR
const uint8_t PacketEncoder::LFSR_two = 77; //second byte of 15 bit LFSR padded with 0


std::vector<uint8_t> PacketEncoder::form_packets(char* data, int size) {

    //TODO: first, encode how many bytes are there in total

    int num_packets = size / data_per_packet;

    std::vector<uint8_t> packets;
    for(int i = 0; i < num_packets; i++) {
        packets.push_back(LFSR_one);
        packets.push_back(LFSR_two);
        //first part
        uint8_t checksum1 = 0;
        for(int j = 0; j < data_per_packet / 2; j++) {
            uint8_t byte = (uint8_t) data[data_per_packet * i + j];
            checksum1 ^= byte;
            packets.push_back(byte);
        }
        //second part
        uint8_t checksum2 = 0;
        for(int j = data_per_packet / 2; j < data_per_packet; j++) {
            uint8_t byte = (uint8_t) data[data_per_packet * i + j];
            checksum2 ^= byte;
            packets.push_back(byte);
        }
        packets.push_back(checksum1);
        packets.push_back(checksum2);
    }

    int remaining_bytes = size % data_per_packet;

    //if added everything, return
    if(remaining_bytes == 0) {
        //transform bytes to bits and return
        return bytes_to_bits(packets);
    }

    //pad anything leftover
    packets.push_back(LFSR_one);
    packets.push_back(LFSR_two);

    //count number of bytes added, could be calculated from the loop index...
    //...but code would become even uglier
    int count = 0;
    //first part
    uint8_t checksum1 = 0;
    uint8_t checksum2 = 0;
    for(int i = num_packets * data_per_packet; i < num_packets * data_per_packet + remaining_bytes; i++) {
        count++;
        uint8_t byte = (uint8_t) data[i];
        if(count <= data_per_packet / 2) {
            checksum1 ^= byte;
        } else {
            checksum2 ^= byte;
        }
        packets.push_back(byte);
    }
    //second part
    //pad zeros
    int num_padded_zeros = data_per_packet - remaining_bytes;
    for(int i = 0; i < num_padded_zeros; i++) {
        packets.push_back(0);
    }
    packets.push_back(checksum1);
    packets.push_back(checksum2);

    //transform bytes to bits and return
    return bytes_to_bits(packets);
}

std::vector<uint8_t> PacketEncoder::bytes_to_bits(std::vector<uint8_t> packets) {
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
