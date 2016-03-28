#include <iostream>
#include <cstdint>
#include <vector>
#include <stdexcept>

#ifndef _Included_PacketEncoder
#define _Included_PacketEncoder

class PacketEncoder {

  public:

      PacketEncoder(size_t preamble_size,
                    size_t data_size,
                    size_t checksum_size,
                    size_t packet_size);
      ~PacketEncoder();

      std::vector<uint8_t> form_packets(char* data, size_t size) const;

  private:

      size_t preamble_size;
      size_t data_size;
      size_t checksum_size;
      size_t packet_size;

      uint8_t LFSR_one; //first byte of 15 bit LFSR
      uint8_t LFSR_two; //second byte of 15 bit LFSR padded with 0

      std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> packets) const;

};

#endif
