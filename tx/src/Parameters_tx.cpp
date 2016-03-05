#include "Parameters_tx.hpp"

Parameters_tx::Parameters_tx() : sample_rate(12.5e6), f_c(4e6) {

    spb = 9075;
    bit_rate = sample_rate / spb; //sends one buffer per bit

}

Parameters_tx::~Parameters_tx() {
    std::cout << "Destroying the Parameters_tx object..." << std::endl << std::endl;
}

double Parameters_tx::get_sample_rate() const {
    return sample_rate;
}

double Parameters_tx::get_f_c() const {
    return f_c;
}

size_t Parameters_tx::get_spb() const {
    return spb;
}

double Parameters_tx::get_bit_rate() const {
    return bit_rate;
}
