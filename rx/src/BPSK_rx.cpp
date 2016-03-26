#include "BPSK_rx.hpp"

BPSK_rx::BPSK_rx(double sample_rate,
                 double f_IF,
                 size_t spb_tx,
                 int d_factor,
                 size_t spb,
                 int d_factor_new)
                 :
                 sample_rate(sample_rate),
                 f_IF(f_IF),
                 spb_tx(spb_tx),
                 d_factor(d_factor),
                 spb(spb),
                 d_factor_new(d_factor_new) {

    //power_desired = 0.5;
    start_index = 0;

    //250 S/s == 100 MS / s * 2.5 / M
    //clock drift is 2.5ppm
    //recompute_period = (int) ((bit_rate / ((double) 250)) *  (double) spb);
    //n_bits_received = 0;

    spb_new = spb / d_factor_new;

    //build the matched filter
    for(int n = 0; n < spb_new; n++) {
        h_matched.push_back(1.0);
    }

    //build the preamble bits
    uint8_t LFSR_one = 120;
    uint8_t LFSR_two = 77;
    std::vector<uint8_t> preamble_bytes;
    preamble_bytes.push_back(LFSR_one);
    preamble_bytes.push_back(LFSR_two);
    std::vector<uint8_t> preamble_bits = bytes_to_bits(preamble_bytes);

    //upsample the preamble bits
    std::vector<float> preamble_vector;
    for(int i = 0; i < (int) preamble_bits.size(); i++) {
        for(int j = 0; j < spb_new; j++) {
            preamble_vector.push_back(preamble_bits[i] ? 1.0 : -1.0);
        }
    }

    //build the preamble detector by convolving with matched filter...
    //...then truncating the transient points from left and right
    std::vector<float> xcorr = conv(h_matched, preamble_vector);
    preamble_detect.insert(preamble_detect.begin(),
                           xcorr.begin() + (h_matched.size() - 1),
                           xcorr.end() - (h_matched.size() - 1));

}

BPSK_rx::~BPSK_rx() {
    std::cout << "Destroying the BPSK_rx object..." << std::endl << std::endl;
}

std::vector<uint8_t>& BPSK_rx::bytes_to_bits(std::vector<uint8_t>& bytes) const {
    std::vector<uint8_t> bits;
    for(int i = 0; i < (int) bytes.size(); i++) {
        uint8_t byte = bytes[i];
        for(int j = 0; j < 8; j++) {
            bits.push_back(byte & 1);
            byte >>= 1;
        }
    }
    return bits;
}

std::vector<int>& BPSK_rx::receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers) {

    //downsample and accumulate everything in one buffer, just like in Matlab
    std::vector<float> received_signal(buffers.size() * spb_tx / (size_t) d_factor);
    for(int i = 0; i < (int) buffers.size(); i++) {
        for(int j = 0, n = 0; n < (int) buffers[i]->size(); j++, n += d_factor) {
            received_signal[j] = real(buffers[i]->at(n));
        }
    }

    //go through agc
    //std::vector<float> normalized_signal = agc(received_signal);
    std::vector<float> normalized_signal = received_signal;

    //demodulate
    std::vector<float> demodulated_signal = costas_loop(normalized_signal);

    //downsample again
    std::vector<float> downsampled_signal(received_signal.size() / (size_t) downsampled_factor);
    for(int i = 0, n = 0; n < (int) demodulated_signal.size(); i++, n += d_factor_new) {
        downsampled_signal[i] = demodulated_signal[n];
    }

    std::vector<float> filtered_signal = conv(h_matched, downsampled_signal);
    int polarity;
    start_index = symbol_offset_synch(filtered_signal, &polarity);

    std::vector<int> pulses(downsampled_signal.size() / spb_new);
    for(int i = 0, n = (spb_new - 1) + start_index; n < (int) filtered_signal.size(); i++, n += spb_new) {
        pulses[i] = polarity * (filtered_signal[n] > 0 ? 1 : -1);
    }

    return pulses;
}

std::vector<float>& BPSK_rx::conv(std::vector<float>& x, std::vector<float>& h) const {
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

std::vector<float>& BPSK_rx::correlate_rx(std::vector<float>& x, std::vector<float>& y) const {
    std::vector<float> xcorr(x.size() + y.size() - 1);
    for(int i = 0; i < (int) r.size(); i++) {
        int ii = ((int) y.size()) - i - 1;
        float tmp = 0.0;
        for(int j = 0; j < (int) x.size(); j++) {
            if(ii >= 0 && ii < (int) y.size()) {
                tmp += y[ii] * x[j];
            }
            ii++;
        }
        xcorr[i] = tmp;
    }
    return xcorr;
}

void BPSK_rx::costas_loop(std::vector<float>& r) {
    int start_index = 0, end_index = FILTER_SIZE - 1;

    static float theta = 0;

    float mu = 0.0015;

    double T_s = 1 / sample_rate;
    for(int n = 0; n < (int) r.size(); n++) {
        z_sin[end_index] = 2.0 * r[n] * sin(2 * M_PI * f_IF * n * T_s + theta);
        z_cos[end_index] = 2.0 * r[n] * cos(2 * M_PI * f_IF * n * T_s + theta);

        start_index = (start_index + 1) % FILTER_SIZE;
        end_index = (end_index + 1) % FILTER_SIZE;

        //LPF SINE
        float lpf_sin = 0;
        for(int i_h = FILTER_SIZE - 1, i_z = start_index; i_h >= 0; i_h--, i_z = (i_z + 1) % FILTER_SIZE) {
            lpf_sin += h_lowpass[i_h] * z_sin[i_z];
        }

        //LPF COSINE
        float lpf_cos = 0;
        for(int i_h = FILTER_SIZE - 1, i_z = start_index; i_h >= 0; i_h--, i_z = (i_z + 1) % FILTER_SIZE) {
            lpf_cos += h_lowpass[i_h] * z_cos[i_z];
        }

        r[n] = lpf_cos;

        theta -= mu * lpf_sin * lpf_cos;
    }

}

int BPSK_rx::symbol_offset_synch(std::vector<float>& filtered_signal, int* polarity) const {
    std::vector<float> xcorr = correlate_rx(filtered_signal, preamble_detect);
    int max_index = 0;
    float max_value = std::abs(xcorr[max_index]);

    for(int i = 1; i < (int) xcorr.size(); i++) {
        if(std::abs(xcorr[i]) > max_value) {
            max_index = i;
            max_value = std::abs(xcorr[i]);
        }
    }

    *polarity = xcorr[max_index] >= 0 ? 1 : -1;
    return (max_index + 1) % (int) spb_new;
}
