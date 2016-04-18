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
    size_t decode(std::vector<int> const &pulses, size_t pulses_size, std::vector<uint8_t> &bytes);

  private:

    size_t pulses_to_bytes(std::vector<int> const &pulses, int start_index, std::vector<uint8_t> &bytes, int insert_index);
    size_t correlate(std::vector<int> const &x, std::vector<int> const &y, std::vector<int> &rxy) const;

    //packet parameters
    size_t preamble_size,  data_size,  checksum_size,  packet_size;

    //vector to hold preamble pulses used in packet_start detection
    std::vector<int> preamble_vector;

    //vector that holds yet undecoded pulses
    std::vector<int> previous_pulses;
    size_t previous_pulses_size;

    //variables to determine start and end of streaming, file_size
    size_t total_size, received_size;
    bool streaming_started, streaming_ended;

};

#endif
