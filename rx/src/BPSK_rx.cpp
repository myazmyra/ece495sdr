#include "BPSK_rx.hpp"

BPSK_rx::BPSK_rx(double sample_rate,
                 double f_IF,
                 double bit_rate,
                 size_t spb,
                 int decimation_factor) :
                 sample_rate(sample_rate),
                 f_IF(f_IF),
                 bit_rate(bit_rate),
                 spb(spb),
                 decimation_factor(decimation_factor) {

    //power_desired = 0.5;

    //250 S/s == 100 MS / s * 2.5 / M
    //clock drift is 2.5ppm
    //recompute_period = (int) ((bit_rate / ((double) 250)) *  (double) spb);
    start_index = 0;
    //n_bits_received = 0;

    downsample_factor = 25;
    if(downsample_factor >= decimation_factor) {
        std::cout << "BPSK_rx: downsample_factor cannot be greater than or equal to decimation_factor" << std::endl << std::endl;
        throw new std::runtime_error("BPSK_rx: downsample_factor cannot be greater than or equal to decimation_factor");
    }
    samps_per_bit = spb / downsample_factor;
    if(samps_per_bit <= 1) {
        std::cout << "BPSK_rx: Must have more than one sample per bit" << std::endl << std::endl;
        throw new std::runtime_error("BPSK_rx: Must have more than one sample per bit");
    }

    //build the matched filter
    for(int i = 0; i < samps_per_bit; i++) {
        h_matched.push_back(1.0);
    }

    //build the preamble vector
    uint8_t LFSR_one = 120;
    uint8_t LFSR_two = 77;
    std::vector<uint8_t> preamble_bytes;
    preamble_bytes.push_back(LFSR_one);
    preamble_bytes.push_back(LFSR_two);
    std::vector<uint8_t> preamble_bits = bytes_to_bits(preamble_bytes);

    std::vector<float> preamble_vector;
    for(int i = 0; i < (int) preamble_bits.size(); i++) {
        for(int j = 0; j < samps_per_bit; j++) {
            preamble_vector.push_back(preamble_bits[i] ? 1.0 : -1.0);
        }
    }

    //build the preamble detector
    std::vector<float> xcorr = conv(h_matched, preamble_vector);
    preamble_detect.insert(preamble_detect.begin(),
                           xcorr.begin() + (h_matched.size() - 1), xcorr.end() - (h_matched.size() - 1));

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
    //std::cout << "buff.size(): " << buffers.size() << std::endl;

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
    //std::vector<float> normalized_signal = agc(received_signal);
    std::vector<float> normalized_signal = received_signal;

    //demodulate
    std::vector<float> demodulated_signal = costas_loop(normalized_signal);

    //downsample again
    std::vector<float> downsampled_signal;
    for(int i = 0; i < (int) demodulated_signal.size(); i += downsample_factor) {
        downsampled_signal.push_back(demodulated_signal[i]);
    }
    //std::cout << "downsampled_signal.size(): " << downsampled_signal.size() << std::endl;

    /*
    //recompute sampling time every (bit_rate / 250) * spb
    if(n_bits_received == recompute_period) {
        n_bits_received = 0;
        start_index = symbol_offset_synch(demodulated_signal);
    }
    n_bits_received += downsampled.size() / spb; //this is just bit_rate
    */

    std::vector<float> filtered_signal = conv(h_matched, downsampled_signal);
    //std::cout << "filtered_signal.size(): " << filtered_signal.size() << std::endl;
    int polarity;
    start_index = symbol_offset_synch(filtered_signal, &polarity);
    start_index = start_index < 0 ? start_index + samps_per_bit : start_index;
    //push previous samples
    //...

    //std::cout << "start: " << start_index << std::endl;

    //obtain samples
    std::vector<int> pulses;
    for(int i = start_index; i < (int) filtered_signal.size(); i += samps_per_bit) {
        pulses.push_back(polarity * (filtered_signal[i] > 0 ? 1 : -1));
    }
    //std::cout << "pulses.size(): " << pulses.size() << std::endl;
    //std::cout << "start_index: " << start_index << std::endl;

    //store leftover to previous samples vector
    //...

    return pulses;
}

std::vector<uint8_t> BPSK_rx::bytes_to_bits(std::vector<uint8_t> bytes) {
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

std::vector<float> BPSK_rx::correlate_rx(std::vector<float> x, std::vector<float> y) {
    std::vector<float> r(x.size() + y.size() - 1);
    for(int i = 0; i < (int) r.size(); i++) {
        int ii = ((int) y.size()) - i - 1;
        float tmp = 0.0;
        for(int j = 0; j < (int) x.size(); j++) {
            if(ii >= 0 && ii < (int) y.size()) {
                tmp += y[ii] * x[j];
            }
            ii++;
        }
        r[i] = tmp;
    }
    return r;
}

std::vector<float> BPSK_rx::costas_loop(std::vector<float> r) {
    std::vector<float> result(r.size());
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

        result[n] = lpf_cos;

        theta -= mu * lpf_sin * lpf_cos;
    }
    //std::cout << "theta: " << theta << std::endl;

    return result;

}

int BPSK_rx::symbol_offset_synch(std::vector<float> filtered_signal, int* polarity) {
    std::vector<float> xcorr = correlate_rx(filtered_signal, preamble_detect);
    int ind_max = 0;
    float max_val = std::abs(xcorr[ind_max]);

    for(int i = 1; i < (int) xcorr.size(); i++) {
        if(std::abs(xcorr[i]) > max_val) {
            ind_max = i;
            max_val = std::abs(xcorr[i]);
        }
    }

    *polarity = xcorr[ind_max] >= 0 ? 1 : -1;
    return ((ind_max) - (int) preamble_detect.size()) % samps_per_bit;
}
