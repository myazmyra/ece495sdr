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

  private:

      std::vector<int> correlate(std::vector<int> const &x, std::vector<int> const &y) const;
      std::vector<uint8_t> pulses_to_bytes(std::vector<int> const &pulses, int start_index);
      std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> const &packets) const;

      size_t preamble_size,  data_size,  checksum_size,  packet_size;

      std::vector<int> preamble_vector;
      
      std::vector<int> previous_pulses;

      size_t total_size, received_size;
      bool streaming_started, streaming_ended;

};

#endif
