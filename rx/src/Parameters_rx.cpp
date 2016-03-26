#include "Parameters_rx.hpp"

Parameters_rx::Parameters_rx(double sample_rate_tx,
                             double f_c_tx,
                             size_t spb_tx) :
                             sample_rate_tx(sample_rate_tx),
                             f_c_tx(f_c_tx),
                             spb_tx(spb_tx),
                             d_factor(40),
                             sample_rate(sample_rate_tx / d_factor),
                             f_IF(fmod(sample_rate - fmod(f_c_tx, sample_rate), sample_rate)),
                             spb(spb_tx / d_factor),
                             d_factor_new(25) {



}

Parameters_rx::~Parameters_rx() {
    std::cout << "Destroying the Parameters_rx object..." << std::endl << std::endl;
}

double Parameters_rx::get_sample_rate() const {
    return sample_rate;
}

int Parameters_rx::get_d_factor() const {
    return d_factor;
}

double Parameters_rx::get_f_IF() const {
    return f_IF;
}

size_t Parameters_rx::get_spb() const {
    return spb;
}
