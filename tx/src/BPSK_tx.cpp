#include "BPSK_tx.hpp"

BPSK_tx::BPSK_tx(double sample_rate, double f_c, double bit_rate, size_t spb) :
                 sample_rate(sample_rate), f_c(f_c), bit_rate(bit_rate), spb(spb),
                 positive(spb), negative(spb) {
    //do something, maybe check for aliasing after you get your 3-dB frequencies from the analog guys?
    double T_s = 1 / sample_rate;
    std::vector< std::complex<float> >::iterator it_positive = positive.begin();
    std::vector< std::complex<float> >::iterator it_negative = negative.begin();
    for(int n = 0; n < (int) spb; n++) {
        (*it_positive).real(std::cos(2 * M_PI * f_c * n * T_s));
        (*it_negative).real(-std::cos(2 * M_PI * f_c * n * T_s));
        it_positive++;
        it_negative++;
    }
}

BPSK_tx::~BPSK_tx() {
    std::cout << "Destroying the BPSK_tx object..." << std::endl << std::endl;
}

std::vector< std::complex<float> > BPSK_tx::modulate(uint8_t bit) {
    return bit ? positive : negative;
}
