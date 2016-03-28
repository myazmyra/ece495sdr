#include <boost/thread.hpp>
#include <iostream>

#ifndef _Included_BPSK_tx
#define _Included_BPSK_tx

class BPSK_tx {
  public:

    BPSK_tx(double sample_rate, double f_c, size_t spb);
    ~BPSK_tx();
    std::vector< std::complex<float> > modulate(uint8_t bit);

  private:

    double sample_rate;
    double f_c;
    size_t spb;

    std::vector< std::complex<float> > positive; //cos(2*pi*f_c*(0:spb-1)/F_s)
    std::vector< std::complex<float> > negative; //-cos(2*pi*f_c*(0:spb-1)/F_s)
};

#endif
