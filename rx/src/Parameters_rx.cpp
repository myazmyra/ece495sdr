#include "Parameters_rx.hpp"

Parameters_rx::Parameters_rx() : decimation_factor(50),
                                 sample_rate(12.5e6 / decimation_factor),
                                 f_IF(sample_rate / 5) {

    //explanation: sample_rate on the TX side is 12.5e6
    //need a sample rate which is even divisor of 100e6
    //need a decimation factor (50) to be a divisor of 8000
    //thus: sample rate is 12.5e6 / 50

    spb = 8000 / decimation_factor;
    bit_rate = sample_rate / spb; //sample_rate / spb

}

Parameters_rx::~Parameters_rx() {
    std::cout << "Destroying the Parameters_rx object..." << std::endl << std::endl;
}

double Parameters_rx::get_sample_rate() const {
    return sample_rate;
}

double Parameters_rx::get_f_IF() const {
    return f_IF;
}

size_t Parameters_rx::get_spb() const {
    return spb;
}

double Parameters_rx::get_bit_rate() const {
    return bit_rate;
}

int Parameters_rx::get_decimation_factor() const {
    return decimation_factor;
}
