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
    std::vector<int> receive(std::vector< std::complex<float> > const &buff);
    std::vector<int> receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers);

  private:

    //function declarations
    std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> const &bytes) const;
    std::vector<float> conv(std::vector<float> const &x, std::vector<float> const &h) const;
    std::vector<float> correlate_rx(std::vector<float> const &x, std::vector<float> const &y) const;
    std::vector<float> agc(std::vector<float> &received_signal);
    std::vector<float> costas_loop(std::vector<float> &normalized_signal);
    int symbol_offset_synch(std::vector<float> const &filtered_signal, int* polarity) const;

    //used to compute sample time recomputation period
    //const float usrp_sample_rate;
    //const float clock_drift_rate;

    //basic rx parameters
    double const sample_rate;
    double const T_s;
    double const f_IF;
    int const d_factor; //this is set by Parameters_rx, used in bandpass sampling
    size_t const spb;
    int const d_factor_new; // < decimation_factor, used to reduce the load on pc
    size_t const spb_new; //spb / decimation_factor

    //intermediate vectors
    std::vector<float> received_signal;
    std::vector<float> downsampled_signal;
    std::vector<int> pulses;

    //matched filter
    std::vector<float> h_matched;

    //agc parameters
    float const power_desired, mu_agc;

    //symbol offset detector parameters
    std::vector<float> preamble_detect; //vector used in detecting start_index

    //costas loop filters and buffers
    float mu_pll;
    size_t filter_size;
    std::vector<float> h_lp_pll, z_sin, z_cos;

};

#endif
