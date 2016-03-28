#include <iostream>
#include <stdexcept>
#include <vector>
#include <complex>

#ifndef _Included_BPSK_rx
#define _Included_BPSK_rx

#define FILTER_SIZE 32

class BPSK_rx {

  public:

    BPSK_rx(double sample_rate,
            double f_IF,
            int d_factor,
            size_t spb,
            int d_factor_new,
            size_t spb_new,
            std::vector<int> preamble_vector);
    ~BPSK_rx();
    std::vector<uint8_t> bytes_to_bits(std::vector<uint8_t> const &bytes) const;
    std::vector<int> receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers);
    std::vector<float> conv(std::vector<float> const &x, std::vector<float> const &h) const;
    std::vector<float> correlate_rx(std::vector<float> const &x, std::vector<float> const &y) const;
    //void agc(std::vector<float>& received_signal);
    std::vector<float> costas_loop(std::vector<float> &normalized_signal);
    int symbol_offset_synch(std::vector<float> const &filtered_signal, int* polarity) const;

  private:

    //used to compute sample time recomputation period
    //const float usrp_sample_rate;
    //const float clock_drift_rate;

    double sample_rate;
    double f_IF;
    int d_factor; //this is set by Parameters_rx, used in bandpass sampling
    size_t spb;
    int d_factor_new; // < decimation_factor, used to reduce the load on pc
    size_t spb_new; //spb / decimation_factor

    //agc parameters
    //float power_desired;

    //symbol offset detector parameters
    //int recompute_period;
    //int n_bits_received;
    int start_index;

    std::vector<float> h_matched;
    std::vector<float> preamble_detect;

    //costas loop filters and buffers
    float const h_lowpass[FILTER_SIZE] = {0.0187, 0.0106, 0.0086, 0.0035, -0.0026, -0.0069, -0.0068, -0.0011,
                                          0.0100, 0.0249, 0.0411, 0.0568, 0.0702, 0.0807, 0.0878, 0.0913,
                                          0.0913, 0.0878, 0.0807, 0.0702, 0.0568, 0.0411, 0.0249, 0.0100,
                                          -0.0011, -0.0068, -0.0069, -0.0026, 0.0035, 0.0086, 0.0106, 0.0187};

    float z_sin[FILTER_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,};

    float z_cos[FILTER_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,};

};

#endif
