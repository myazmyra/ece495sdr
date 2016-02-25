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

    const double sample_rate;// = 12.5e6; //sample rate (frequency) between the PC and Motherboard
    const double f_c;// = 4e6; //carrier frequency
    double bit_rate; // = sample_rate / spb; to make it easier to generate each bit
    size_t spb;// = 9075; //samples per buffer; has to satisfy -> spb = k * Fs/Fc * tx_streamer->get_max_num_samps(); for some integer k; here k = 8

};

#endif
