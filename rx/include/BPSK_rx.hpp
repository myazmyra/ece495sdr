#include <iostream>
#include <vector>
#include <complex>

#ifndef _Included_BPSK_rx
#define _Included_BPSK_rx

class BPSK_rx {
  public:

    BPSK_rx(double sample_rate, double f_IF, double bit_rate, size_t spb);
    ~BPSK_rx();
    std::vector<uint8_t> receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers);

  private:

    const double sample_rate;
    double f_IF;
    double bit_rate;
    size_t spb;
    std::vector< std::complex<float> > mixer_IF;

};

#endif
