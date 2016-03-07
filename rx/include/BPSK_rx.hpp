#include <iostream>
#include <vector>
#include <complex>

#ifndef _Included_BPSK_rx
#define _Included_BPSK_rx

class BPSK_rx {
  public:

    BPSK_rx(double sample_rate, double f_IF, double bit_rate, size_t spb, int decimation_factor);
    ~BPSK_rx();
    std::vector<int> receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers);
    std::vector<float> conv(std::vector<float> x, std::vector<float> h);

  private:

    const double sample_rate;
    double f_IF;
    double bit_rate;
    size_t spb;
    int decimation_factor;
    std::vector<float> mixer_IF;
    std::vector<float> matched_filter;

};

#endif
