#include <iostream>
#include <cmath>
#include <stdexcept>
#include <vector>

#ifndef _Included_Parameters_rx
#define _Included_Parameters_rx

#define FILTER_SIZE 32

class Parameters_rx {

  public:

    Parameters_rx();
    ~Parameters_rx();
    double get_sample_rate_tx() const;
    double get_f_c_tx() const;
    size_t get_spb_tx() const;
    int get_d_factor() const;
    double get_sample_rate() const;
    double get_f_IF() const;
    size_t get_spb() const;
    int get_d_factor_new() const;
    size_t get_spb_new() const;
    size_t get_preamble_size() const;
    size_t get_data_size() const;
    size_t get_checksum_size() const;
    size_t get_packet_size() const;
    std::vector<int> get_preamble_vector() const;
    float get_power_desired() const;
    float get_mu_agc() const;
    float get_mu_pll() const;
    size_t get_filter_size() const;
    std::vector<float> get_h_lp_pll() const;

  private:

    double sample_rate_tx;
    double f_c_tx;
    size_t spb_tx;
    int d_factor;
    double sample_rate;
    double f_IF;
    size_t spb;
    int d_factor_new;
    size_t spb_new;

    size_t preamble_size;
    size_t data_size;
    size_t checksum_size;
    size_t packet_size;

    //preamble parameters and builder
    std::vector<int> preamble_vector;
    std::vector<int> build_lfsr(int m) const;

    //agc parameters
    float power_desired, mu_agc;

    //costas_loop parameters
    float mu_pll;
    size_t filter_size;
    std::vector<float> h_lp_pll;

};

#endif
