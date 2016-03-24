#include <iostream>
#include <stdexcept>
#include <vector>
#include <complex>

#ifndef _Included_BPSK_rx
#define _Included_BPSK_rx

#define FILTER_SIZE 32

class BPSK_rx {
  public:

    BPSK_rx(double sample_rate, double f_IF, double bit_rate, size_t spb, int decimation_factor);
    ~BPSK_rx();
    std::vector<int> receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers);
    std::vector<float> conv(std::vector<float> x, std::vector<float> h);
    std::vector<float> correlate(std::vector<float> x, std::vector<float> y);
    //std::vector<float> agc(std::vector<float> received_signal);
    std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> bytes);
    std::vector<float> correlate(std::vector<float> x, std::vector<float> y);
    std::vector<float> costas_loop(std::vector<float> normalized_signal);
    int symbol_offset_synch(std::vector<float> filtered_signal);

  private:

    //used to compute sample time recomputation period
    //const float usrp_sample_rate;
    //const float clock_drift_rate;

    const double sample_rate;
    double f_IF;
    double bit_rate;
    size_t spb;
    int decimation_factor; //this is set by Parameters_rx
    std::vector<float> mixer_IF;
    std::vector<float> matched_filter;

    float power_desired; //for agc

    //int recompute_period;
    int start_index;
    //int n_bits_received;

    int downsample_factor; // < decimation_factor
    int samps_per_bit; //spb / downsample_factor

    std::vector<float> h_matched;
    std::vector<float> preamble_detect;

};

#endif
