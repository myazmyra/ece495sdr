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

  private:

    const double sample_rate; //sample rate between the PC and Motherboard (or just think about it as sample rate), has to satisfy (100e6 / sample_rate) mod 2 == 0
    const double f_c; //carrier frequency
    double bit_rate; //sample_rate / spb; to make it easier to generate each bit
    size_t spb; //samples per buffer; has to satisfy -> spb = k * Fs/Fc for some integer k, spb must also be an integer itself

};

#endif
