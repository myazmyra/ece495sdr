#include <iostream>

#ifndef _Included_BPSK_rx
#define _Included_BPSK_rx

class BPSK_rx {
  public:

    BPSK_rx(double sample_rate, double f_c, double bit_rate, size_t spb);
    ~BPSK_rx();

  private:

    const double sample_rate;// = 12.5e6; //sample rate (frequency) between the PC and Motherboard
    const double f_c;// = 4e6; //carrier frequency
    double bit_rate; // = sample_rate / spb; to make it easier to generate each bit
    size_t spb;// = 9075; //samples per buffer; has to satisfy -> spb = k * Fs/Fc * tx_streamer->get_max_num_samps(); for some integer k; here k = 8
    std::vector< std::complex<float> > positive;
    std::vector< std::complex<float> > negative;
    
};

#endif
