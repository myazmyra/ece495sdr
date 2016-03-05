#include "Parameters_rx.hpp"

Parameters_rx::Parameters_rx() : sample_rate(12.5e6 / 25), f_c(4e6) {

    //explanation: sample_rate on the TX side is 12.5e6
    //need a sample rate which is even divisor of 100e6
    //need a decimation factor (25) to be a divisor of 9075
    //thus: sample rate is 12.5e6 / 25

    spb = 363;
    bit_rate = sample_rate / spb; //sample_rate / spb

}

Parameters_rx::~Parameters_rx() {
    std::cout << "Destroying the Parameters_rx object..." << std::endl << std::endl;
}

double Parameters_rx::get_sample_rate() const {
    return sample_rate;
}

double Parameters_rx::get_f_c() const {
    return f_c;
}

size_t Parameters_rx::get_spb() const {
    return spb;
}

double Parameters_rx::get_bit_rate() const {
    return bit_rate;
}
