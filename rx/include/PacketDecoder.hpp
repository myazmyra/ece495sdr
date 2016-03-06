#include <iostream>
#include <vector>

#ifndef _Included_PacketDecoder
#define _Included_PacketDecoder

class PacketDecoder {
  public:

    PacketDecoder();
    ~PacketDecoder();
    std::vector<uint8_t> decode(std::vector<uint8_t> bits);

  private:

      //a packet will consist of:
      //2 bytes of preamble + 12 bytes of data + 2 bytes of checksum = 16 bytes
      int const data_per_packet;
      uint8_t const LFSR_one; //first byte of 15 bit LFSR
      uint8_t const LFSR_two; //second byte of 15 bit LFSR padded with 0

};

#endif
