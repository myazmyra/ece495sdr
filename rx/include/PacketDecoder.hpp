#include <iostream>
#include <vector>
#include <algorithm>

#ifndef _Included_PacketDecoder
#define _Included_PacketDecoder

class PacketDecoder {

  public:

    PacketDecoder(size_t preamble_size,
                  size_t data_size,
                  size_t checksum_size,
                  size_t packet_size,
                  std::vector<int> preamble_vector);

    ~PacketDecoder();
    std::vector<uint8_t> decode(std::vector<int> bits);
    std::vector<int> correlate(std::vector<int> const &x, std::vector<int> const &y) const;
    std::vector<uint8_t> pulses_to_bytes(std::vector<int> const &pulses, int start_index);
    std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> const &packets) const;

  private:

      size_t preamble_size; //number of preamble bytes in a packet
      size_t data_size; //number of data bytes in a packet
      size_t checksum_size; //number of checksum bytes in a packet
      size_t packet_size; //total number of bytes in a packet

      size_t total_size;
      size_t received_size;
      bool streaming_started;
      bool streaming_ended;

      std::vector<int> preamble_vector;
      std::vector<int> previous_pulses;

};

#endif
