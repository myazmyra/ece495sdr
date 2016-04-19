#include <iostream>
#include <stdexcept>
#include <vector>
#include <complex>
#include <numeric>
#include <functional>
#include <algorithm>

#ifndef _Included_BPSK_rx
#define _Included_BPSK_rx

class BPSK_rx {

  public:

    //constructor, destructor, and function declarations
    BPSK_rx(size_t packet_size,
            double sample_rate,
            double f_IF,
            int d_factor,
            size_t spb,
            int d_factor_new,
            size_t spb_new,
            std::vector<int> preamble_vector,
            float power_desired,
            float mu_agc,
            float mu_pll,
            size_t filter_size,
            std::vector<float> h_lp_pll);
    ~BPSK_rx();
    size_t receive(std::vector< std::complex<float> > const &buff, std::vector<int> &pulses);

  private:

    //function declarations
    size_t conv(std::vector<float> const &x, std::vector<float> const &h, std::vector<float> &y) const;
    size_t correlate(std::vector<float> const &x, std::vector<float> const &y, std::vector<float> &rxy) const;
    void agc(std::vector<float> &received_signal);
    void costas_loop(std::vector<float> &normalized_signal);
    int symbol_offset_synch(std::vector<float> const &filtered_signal, int* polarity) const;

    //used to compute sample time recomputation period
    //const float usrp_sample_rate;
    //const float clock_drift_rate;

    //basic rx parameters
    size_t const packet_size;
    double const sample_rate;
    double const T_s;
    double const f_IF;
    int const d_factor; //this is set by Parameters_rx, used in bandpass sampling
    size_t const spb;
    int const d_factor_new; // < decimation_factor, used to reduce the load on pc
    size_t const spb_new; //spb / decimation_factor

    //matched filter
    std::vector<float> const h_matched;

    //agc parameters
    float const power_desired, mu_agc;

    //symbol offset detector parameters
    std::vector<float> preamble_detect; //vector used in detecting start_index

    //costas loop filters and buffers
    float const mu_pll;
    size_t const filter_size;
    std::vector<float> const h_lp_pll;

};

#endif
