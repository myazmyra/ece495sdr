#ifndef _Included_BPSK_tx
#define _Included_BPSK_tx

class BPSK_tx {
  public:

    BPSK_tx();
    virtual ~BPSK_tx();
    double get_sample_rate();
    double get_f_c();
    size_t get_spb();
    modulate(std::vector<int> bits);

  private:

    const double sample_rate;// = 12.5e6; //sample rate (frequency) between the PC and Motherboard
    const double f_c;// = 4e6; //carrier frequency
    const size_t spb;// = 9075; //samples per buffer; has to satisfy -> spb = k * Fs/Fc * tx_streamer->get_max_num_samps(); for some integer k; here k = 8
    const unsigned int bit_rate;
    std::vector< std::complex<float> > positive;
    std::vector< std::complex<float> > negative;
};


#endif
