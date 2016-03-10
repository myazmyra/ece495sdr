#include <iostream>
#include <math.h>
#include <stdexcept>

#ifndef _Included_Parameters_rx
#define _Included_Parameters_rx

class Parameters_rx {

  public:

    Parameters_rx();
    ~Parameters_rx();
    double get_sample_rate() const;
    double get_f_IF() const;
    size_t get_spb() const;
    double get_bit_rate() const;
    int get_decimation_factor() const;
    size_t get_spb_tx() const;

  private:

    const double f_c_tx;
    const double decimation_factor;
    const double sample_rate; //sample rate (frequency) between the PC and Motherboard
    const double f_IF; //intermediate frequency (used for PLL)
    double bit_rate; //sample_rate / spb; to make it easier to generate each bit
    size_t spb_tx; //samples per buffer on the TX side
    size_t spb; //samples per buffer; has to satisfy -> spb = k * Fs/Fc for some integer k; spb must also be an integer itself

};

#endif
