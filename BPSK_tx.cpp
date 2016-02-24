#include "USRP_tx.h"
#include "BPSK_tx.h"
#include <cstdlib>

BPSK_tx::BPSK_tx() : sample_rate(12.5e6), f_c(4e6), bit_rate(1377.4104625), spb(9075),
                     positive(spb), negative(spb) {
    //do something, maybe check for aliasing after you get your 3-dB frequencies from the analog guys?
    double T_s = 1 / sample_rate;
    std::vector< std::complex<float> >::iterator it_pos = positive.begin();
    std::vector< std::complex<float> >::iterator it_neg = negative.begin();
    for(int n = 0; n < (int) spb; n++) {
        real(*it_pos) = std::cos(2 * M_PI * f_c * n * T_s);
        real(*it_neg) = -std::cos(2 * M_PI * f_c * n * T_s);
        it_pos++;
        it_neg++;
    }
}

BPSK_tx::~BPSK_tx() {
    std::cout << "Destroying the BPSK_tx object..." << std::endl << std::endl;
}

double BPSK_tx::get_sample_rate() {
    return sample_rate;
}

double BPSK_tx::get_f_c() {
    return f_c;
}

size_t BPSK_tx::get_spb() {
    return spb;
}

double BPSK_tx::get_bit_rate() {
    return bit_rate;
}

std::vector< std::complex<float> > BPSK_tx::modulate(int bit) {
    return bit == 1 ? positive : negative;
}
