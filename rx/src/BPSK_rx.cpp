#include "BPSK_rx.hpp"

BPSK_rx::BPSK_rx(double sample_rate, double f_IF, double bit_rate,
                 size_t spb, int decimation_factor) :
                 sample_rate(sample_rate), f_IF(f_IF), bit_rate(bit_rate),
                 spb(spb), decimation_factor(decimation_factor) {

    double T_s = 1 / sample_rate;

    //250 S/s == 100 MS / s * 2.5 / M
    recompute_period = (int) ((bit_rate / ((double) 250)) *  (double) spb);
    start_index = 0;
    n_bits_received = 0;

    downsample_factor = 25;

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

std::vector<int> BPSK_rx::receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers) {

    if(buffers.size() == 0) {
        std::cout << "Buffers size should never be ZERO" << std::endl << std::endl;
        throw new std::runtime_error("Buffers size should never be ZERO");
    }

    //downsample and accumulate everything in one buffer, just like in Matlab
    std::vector<float> received_signal;
    for(int i = 0; i < (int) buffers.size(); i++) {
        for(int j = 0; j < (int) buffers[i]->size(); j += decimation_factor) {
            received_signal.push_back(real(buffers[i]->at(j)));
        }
    }

    //go through agc
    std::vector<float> normalized_signal = agc(received_signal);

    //demodulate
    std::vector<float> demodulated_signal = costas_loop(normalized_signal);

    //downsample again
    std::vector<float> downsampled_signal;

    //recompute sampling time every (bit_rate / 250) * spb
    if(n_bits_received == recompute_period) {
        n_bits_received = 0;
        start_index = symbol_offset_synch(demodulated_signal);
    }
    n_bits_received += downsampled.size() / spb;

    //push previous samples
    //...

    //obtain samples
    std::vector<int> pulses;
    for(int i = start_index + spb / 2 - 1; i < (int) demodulated_signal.size(); i += spb) {
        pulses.push_back(y[i] > 0 ? 1 : -1);
    }

    //store leftover to previous samples vector
    //...

    return pulses;
}
