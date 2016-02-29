#include "BPSK_rx.h"

BPSK_rx::BPSK_rx(double sample_rate, double f_c, double bit_rate, size_t spb) :
                 sample_rate(sample_rate), f_c(f_c), bit_rate(bit_rate), spb(spb),
                 positive(spb), negative(spb) {
    //do something, maybe check for aliasing after you get your 3-dB frequencies from the analog guys?
    double T_s = 1 / sample_rate;
    std::vector< std::complex<float> >::iterator it_pos = positive.begin();
    std::vector< std::complex<float> >::iterator it_neg = negative.begin();
    for(int n = 0; n < (int) spb; n++) {
        (*it_pos).real(std::cos(2 * M_PI * f_c * n * T_s));
        (*it_neg).real(-std::cos(2 * M_PI * f_c * n * T_s));
        it_pos++;
        it_neg++;
    }
}

BPSK_rx::~BPSK_rx() {
    std::cout << "Destroying the BPSK_tx object..." << std::endl << std::endl;
}
