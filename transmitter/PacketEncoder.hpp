#include <iostream>

#ifndef _Included_PacketEncoder
#define _Included_PacketEncoder

class PacketEncoder {

  public:
      
    static std::vector<uint8_t> formPackets(char* data, int size);

  private:

      int junk;

};

#endif
