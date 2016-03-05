#include "BPSK_rx.hpp"

BPSK_rx::BPSK_rx(double sample_rate, double f_c, double bit_rate, size_t spb) :
                 sample_rate(sample_rate), f_c(f_c), bit_rate(bit_rate), spb(spb),
                 carrier(spb) {

    double T_s = 1 / sample_rate;
    std::vector< std::complex<float> >::iterator it = carrier.begin();
    for(int n = 0; n < (int) spb; n++) {
        (*it).real(std::cos(2 * M_PI * f_c * n * T_s));
        it++;
    }
}

BPSK_rx::~BPSK_rx() {
    std::cout << "Destroying the BPSK_rx object..." << std::endl << std::endl;
}

std::vector<uint8_t> BPSK_rx::receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers) {
    std::vector<uint8_t> bits;
    return bits;
}
