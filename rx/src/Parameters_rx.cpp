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
