#include <iostream>

#ifndef _Included_Parameters_tx
#define _Included_Parameters_tx

class Parameters_tx {

  public:

    Parameters_tx();
    ~Parameters_tx();
    double get_sample_rate() const;
    double get_f_c() const;
    size_t get_spb() const;
    double get_bit_rate() const;
    size_t get_preamble_size() const;
    size_t get_data_size() const;
    size_t get_checksum_size() const;
    size_t get_packet_size() const;

  private:

    double sample_rate; //sample rate between the PC and Motherboard...
                        //or just think about it as sample rate...
                        //...has to satisfy (100e6 / sample_rate) mod 2 == 0
    double f_c; //carrier frequency
    size_t spb; //samples per bit (samples per buffer)...
                //...has to satisfy -> spb = k * Fs/Fc for some integer k...
                //...spb must also be an integer itself
    double bit_rate; //sample_rate / spb; to make it easier to generate each bit
                     //...this is because usrp will send one buffer per bit (by my design)

    size_t preamble_size;
    size_t data_size;
    size_t checksum_size;
    size_t packet_size;

};

#endif
