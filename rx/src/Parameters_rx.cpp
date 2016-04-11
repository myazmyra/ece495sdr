#include "Parameters_rx.hpp"

Parameters_rx::Parameters_rx() {

  sample_rate_tx = 12.5e6;
  f_c_tx = 4e6;
  spb_tx = 8000;
  d_factor = 40;
  sample_rate = sample_rate_tx / (double) d_factor;
  f_IF = fmod(sample_rate - fmod(f_c_tx, sample_rate), sample_rate);
  spb = spb_tx / (size_t) d_factor;
  d_factor_new = 25;
  spb_new = spb / (size_t) d_factor_new;

  preamble_size = 2;
  data_size = 12;
  checksum_size = 2;
  packet_size = preamble_size + data_size + checksum_size;

  //build preamble_vector
  int m = (int) lround(log2((double) preamble_size * 8));
  preamble_vector = build_lfsr(m);
  //build_lfsr returns 2^m - 1 length vector, pad -1 to this vector
  preamble_vector.push_back(-1);

  //agc parameters
  power_desired = 0.5;
  mu_agc = 0.1;

  //costas loop parameters
  mu_pll = 0.15;
  float const h_lp_pll_tmp[FILTER_SIZE] = {0.0187, 0.0106, 0.0086, 0.0035, -0.0026, -0.0069, -0.0068, -0.0011,
                                           0.0100, 0.0249, 0.0411, 0.0568, 0.0702, 0.0807, 0.0878, 0.0913,
                                           0.0913, 0.0878, 0.0807, 0.0702, 0.0568, 0.0411, 0.0249, 0.0100,
                                           -0.0011, -0.0068, -0.0069, -0.0026, 0.0035, 0.0086, 0.0106, 0.0187};

  filter_size = (size_t) FILTER_SIZE;
  for(int i = 0; i < (int) filter_size; i++) {
    h_lp_pll.push_back(h_lp_pll_tmp[i]);
  }

  //TODO: validate the parameters if they satisfy design logic

}

Parameters_rx::~Parameters_rx() {
    std::cout << "Destroying the Parameters_rx object..." << std::endl << std::endl;
}

double Parameters_rx::get_sample_rate_tx() const { return sample_rate_tx; }
double Parameters_rx::get_f_c_tx() const { return f_c_tx; }
size_t Parameters_rx::get_spb_tx() const { return spb_tx; }
int Parameters_rx::get_d_factor() const { return d_factor; }
double Parameters_rx::get_sample_rate() const { return sample_rate; }
double Parameters_rx::get_f_IF() const { return f_IF; }
size_t Parameters_rx::get_spb() const { return spb; }
int Parameters_rx::get_d_factor_new() const { return d_factor_new; }
size_t Parameters_rx::get_spb_new() const { return spb_new; }
size_t Parameters_rx::get_preamble_size() const { return preamble_size; }
size_t Parameters_rx::get_data_size() const { return data_size; }
size_t Parameters_rx::get_checksum_size() const { return checksum_size; }
size_t Parameters_rx::get_packet_size() const { return packet_size; }
std::vector<int> Parameters_rx::get_preamble_vector() const { return preamble_vector; }
float Parameters_rx::get_power_desired() const { return power_desired; }
float Parameters_rx::get_mu_agc() const { return mu_agc; }
float Parameters_rx::get_mu_pll() const { return mu_pll; }
size_t Parameters_rx::get_filter_size() const { return filter_size; }
std::vector<float> Parameters_rx::get_h_lp_pll() const { return h_lp_pll; }

std::vector<int> Parameters_rx::build_lfsr(int m) const {
    //m should be greater than 1 and is a power of 2
    if(m < 2 || ((m - 1) & m) != 0) {
        std::cout << "Invalid LFSR parameter m" << std::endl << std::endl;
        throw std::runtime_error("Invalid LFSR parameter m");
    }

    int n = (1 << m) - 1;

    std::vector<int> x(n);
    std::vector<int> buffer(m);
    buffer[0] = 1;

    for(int k = 0; k < n; k++) {
        x[k] = buffer[m - 1];
        buffer.insert(buffer.begin() + 1, buffer.begin(), buffer.end() - 1);
        buffer[0] = buffer[0] ^ x[k];
    }

    for(int k = 0; k < n; k++) {
        x[k] = 2 * x[k] - 1;
    }

    return x;

}
