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

std::vector<uint8_t> PacketEncoder::form_packets(char * data, size_t file_size) const {
    if(file_size == 0) {
      std::cout << "num_packets cannot be ZERO" << std::endl << std::endl;
      throw new std::runtime_error("num_packets cannot be ZERO");
    }

    std::vector<uint8_t> packets;
    //TODO: first, encode how many bytes are there in total
    //send total file_size
    uint8_t max_int = ~0; //i.e. 255
    size_t header_size = file_size / max_int + ((file_size % max_int) == 0 ? 0 : 1);
    char * header = (char *) malloc(header_size * sizeof(char));
    size_t count = 0;
    int i = 0;
    while(count + max_int <= file_size) {
      header[i] = (char) max_int;
      count += max_int;
      i++;
    }
    if(count < file_size) {
        header[i] = (char) (file_size % max_int);
    }

    size_t num_header_packets = header_size / data_size + ((header_size % data_size) == 0 ? 0 : 1);
    for(int i = 0; i < (int) num_header_packets; i++) {
      //push the preamble vector
      for(int j = 0; j < (int) preamble_size; j++) {
        packets.push_back(preamble_bytes[j]);
      }
      //push the header bytes and compute checksums
      std::vector<uint8_t> checksums;
      for(int j = 0; j < (int) checksum_size; j++) {
          uint8_t checksum = 0;
          for(int k = (int) ((j + 1) * data_size / checksum_size - data_size / checksum_size);
                  k < (int) ((j + 1) * data_size / checksum_size);
                  k++) {
                      if((int) data_size * i + k < (int) file_size) {
                          uint8_t byte = (uint8_t) header[data_size * i + k];
                          checksum ^= byte;
                          packets.push_back(byte);
                      } else {
                          packets.push_back(0);
                      }
          }
          //inverting the checksum will indicate whether it's a header byte or file byte
          checksums.push_back(~checksum);
      }
      //push the checksums
      for(int j = 0; j < (int) checksum_size; j++) {
          packets.push_back(checksums[j]);
      }
    }

    size_t num_packets = file_size / data_size + ((file_size % data_size) == 0 ? 0 : 1);
    for(int i = 0; i < (int) num_packets; i++) {
        //push the preamble vector
        for(int j = 0; j < (int) preamble_size; j++) {
          packets.push_back(preamble_bytes[j]);
        }
        //push the bytes and compute checksums
        std::vector<uint8_t> checksums;
        for(int j = 0; j < (int) checksum_size; j++) {
            uint8_t checksum = 0;
            for(int k = (int) ((j + 1) * data_size / checksum_size - data_size / checksum_size);
                    k < (int) ((j + 1) * data_size / checksum_size);
                    k++) {
                        if((int) data_size * i + k < (int) file_size) {
                            uint8_t byte = (uint8_t) data[data_size * i + k];
                            checksum ^= byte;
                            packets.push_back(byte);
                        } else {
                            packets.push_back(0);
                        }
            }
            checksums.push_back(checksum);
        }
        //push the checksums
        for(int j = 0; j < (int) checksum_size; j++) {
            packets.push_back(checksums[j]);
        }
    }

    //transform bytes to bits and return
    return bytes_to_bits(packets);
}

std::vector<uint8_t> PacketEncoder::bytes_to_bits(std::vector<uint8_t> const &packets) const {
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
