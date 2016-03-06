#include <boost/thread.hpp>
#include <iostream>

#ifndef _Included_BPSK_tx
#define _Included_BPSK_tx

class BPSK_tx {
  public:

    BPSK_tx(double sample_rate, double f_c, double bit_rate, size_t spb);
    ~BPSK_tx();
    std::vector< std::complex<float> > modulate(uint8_t bit);

  private:

    const double sample_rate; //sample rate (frequency) between the PC and Motherboard
    const double f_c; //carrier frequency
    double bit_rate; //sample_rate / spb; to make it easier to generate each bit
    size_t spb; //samples per buffer; has to satisfy -> spb = k * Fs/Fc for some integer k, spb must also be an integer itself
    std::vector< std::complex<float> > positive; //cos(2*pi*f_c*(0:spb-1)/F_s)
    std::vector< std::complex<float> > negative; //-cos(2*pi*f_c*(0:spb-1)/F_s)
};


#endif
