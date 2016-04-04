#include "BPSK_tx.hpp"

BPSK_tx::BPSK_tx(double sample_rate,
                 double f_c,
                 size_t spb) :
                 sample_rate(sample_rate),
                 f_c(f_c),
                 spb(spb) {

    double T_s = 1 / sample_rate;

    for(int n = 0; n < (int) spb; n++) {
        positive.push_back(0.03 + 0.03 * std::cos(2 * M_PI * (f_c) * n * T_s + 0.9));
        negative.push_back(0.03 + 0.03 * (-std::cos(2 * M_PI * (f_c) * n * T_s + 0.9)));
    }

}

BPSK_tx::~BPSK_tx() {
    std::cout << "Destroying the BPSK_tx object..." << std::endl << std::endl;
}

std::vector< std::complex<float> > BPSK_tx::modulate(uint8_t bit) {
    return bit ? positive : negative;
}
