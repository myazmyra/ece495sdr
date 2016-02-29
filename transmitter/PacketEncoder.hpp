#include <iostream>
#include <cstdint>
#include <vector>

#ifndef _Included_PacketEncoder
#define _Included_PacketEncoder

class PacketEncoder {

  public:

      static std::vector<uint8_t> formPackets(char* data, int size);
      static std::vector<uint8_t> bytes2bits(std::vector<uint8_t> packets);

};

#endif
