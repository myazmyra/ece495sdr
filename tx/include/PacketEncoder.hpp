#include <iostream>
#include <cstdint>
#include <vector>

#ifndef _Included_PacketEncoder
#define _Included_PacketEncoder

class PacketEncoder {

  public:

      static std::vector<uint8_t> form_packets(char* data, int size);
      static std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> packets);

  private:

      static uint8_t const LFSR_one; //first byte of 15 bit LFSR
      static uint8_t const LFSR_two; //second byte of 15 bit LFSR padded with 0

};

#endif
