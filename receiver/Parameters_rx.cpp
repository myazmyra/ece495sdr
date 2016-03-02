#include "Parameters_rx.hpp"

Parameters_rx::Parameters_rx() : sample_rate(12.5e6), f_c(4e6) {

    spb = 9075;
    bit_rate = 1377.4104625; //sample_rate / spb
    //bw = 4 * bit_rate = 5,509.64185

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
