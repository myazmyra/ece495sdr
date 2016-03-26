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
    size_t get_spb_tx() const;
    size_t get_spb() const;
    double get_bit_rate() const;
    int get_decimation_factor() const;

  private:

    double sample_rate_tx;
    double f_c_tx;
    size_t spb_tx;
    int d_factor;
    double sample_rate;
    double f_IF;
    size_t spb;
    int d_factor_new;

};

#endif
