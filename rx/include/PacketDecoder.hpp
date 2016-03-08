#include <iostream>
#include <vector>
#include <algorithm>

#ifndef _Included_PacketDecoder
#define _Included_PacketDecoder

class PacketDecoder {

  public:

    PacketDecoder();
    ~PacketDecoder();
    std::vector<uint8_t> decode(std::vector<int> bits);
    std::vector<int> correlate(std::vector<int> x, std::vector<int> y);
    std::vector<uint8_t> packet_to_bytes(std::vector<int> pulses, int start_index);
    std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> packets);

  private:

      //a packet will consist of:
      //2 bytes of preamble + 12 bytes of data + 2 bytes of checksum = 16 bytes
      int const preamble_size; //number of preamble bytes in a packet
      int const data_size; //number of data bytes in a packet
      int const checksum_size; //number of checksum bytes in a packet
      int const packet_size; //total number of bytes in a packet
      uint8_t const LFSR_one; //first byte of 15 bit LFSR
      uint8_t const LFSR_two; //second byte of 15 bit LFSR padded with 0

      int const num_packets_per_call; //number of packets to analyze in one call

      std::vector<int> preamble_vector;

      std::vector<int> previous_pulses;

};

#endif
