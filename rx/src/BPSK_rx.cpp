#include "BPSK_rx.hpp"

BPSK_rx::BPSK_rx(double sample_rate, double f_IF, double bit_rate,
                 size_t spb, int decimation_factor) :
                 sample_rate(sample_rate), f_IF(f_IF), bit_rate(bit_rate),
                 spb(spb), decimation_factor(decimation_factor) {

    double T_s = 1 / sample_rate;
    for(int n = 0; n < (int) spb; n++) {
        mixer_IF.push_back(std::cos(2 * M_PI * f_IF * n * T_s));
        matched_filter.push_back(1.0);
    }
}

BPSK_rx::~BPSK_rx() {
    std::cout << "Destroying the BPSK_rx object..." << std::endl << std::endl;
}

std::vector<float> BPSK_rx::conv(std::vector<float> x, std::vector<float> h) {
    std::vector<float> y(x.size() + h.size() - 1);
    for(int i = 0; i < (int) y.size(); i++) {
        int ii = i;
        float tmp = 0.0;
        for(int j = 0; j < (int) h.size(); j++) {
            if(ii >= 0 && ii < (int) x.size()) {
                tmp += x[ii] * h[j];
            }
            ii--;
            y[i] = tmp;
        }
    }
    return y;
}

std::vector<uint8_t> BPSK_rx::receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers) {
    //downsample and accumulate everything in one buffer, just like in Matlab
    std::vector<float> downsampled(buffers.size() * spb);
    for(int i = 0; i < (int) buffers.size(); i++) {
        for(int j = 0; j < (int) buffers[i]->size(); j += decimation_factor) {
            downsampled.push_back(real(buffers[i]->at(j)));
        }
    }
    //convolve
    std::vector<float> y = conv(downsampled, matched_filter);
    //obtain samples
    std::vector<uint8_t> bits;
    for(int i = spb - 1; i < (int) y.size(); i+= spb) {
        bits.push_back(y[i] > 0 ? 1 : 0);
    }
    return bits;
}
