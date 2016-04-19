#include "BPSK_tx.hpp"

BPSK_tx::BPSK_tx(double sample_rate,
                 double f_c,
                 size_t spb) :
                 sample_rate(sample_rate),
                 f_c(f_c),
                 spb(spb) {

    double T_s = 1 / sample_rate;
    double bit_rate = sample_rate / (double) spb;
    double beta = 0.5;

    std::vector<float> pulse_shape(spb);
    for(int n = 0; n < (int) spb; n++) {
        pulse_shape[n] = std::cos(M_PI * beta * n * T_s / bit_rate) / (1 - (2 * beta * n * T_s / bit_rate) * (2 * beta * n * T_s / bit_rate)) *
                         std::sin(M_PI * n * T_s / bit_rate) / (M_PI * n * T_s / bit_rate);
    }

    for(int n = 0; n < (int) spb; n++) {
        positive.push_back(std::cos(2 * M_PI * f_c * n * T_s) * pulse_shape[n]);
        negative.push_back(-std::cos(2 * M_PI * f_c * n * T_s) * pulse_shape[n]);
    }

}

BPSK_tx::~BPSK_tx() {
    std::cout << "Destroying the BPSK_tx object..." << std::endl << std::endl;
}

std::vector< std::complex<float> > BPSK_tx::modulate(uint8_t bit) {
    return bit ? positive : negative;
}
