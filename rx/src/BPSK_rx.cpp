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

    power_desired = 0.5;

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
    uint8_t LFSR_one = 30;
    uint8_t LFSR_two = 178;
    std::vector<uint8_t> preamble_bytes;
    preamble_bytes.push_back(LFSR_one);
    preamble_bytes.push_back(LFSR_two);
    std::vector<uint8_t> preamble_bits = bytes_to_bits(preamble_bytes);
    std::vector<float> preamble_vector;
    for(int i = 0; i < (int) preamble_bits.size(); i++) {
        preamble_vector.push_back(preamble_bits[i] ? 1.0 : -1.0);
    }

    //build the preamble detector
    preamble_detect = correlate(preamble_vector, h_matched);

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
    //std::vector<float> normalized_signal = agc(received_signal);
    std::vector<float> normalized_signal = received_signal;

    //demodulate
    std::vector<float> demodulated_signal = costas_loop(normalized_signal);

    //downsample again
    std::vector<float> downsampled_signal;
    for(int i = 0; i < (int) demodulated_signal.size(); i += downsample_factor) {
        downsampled_signal.push_back(demodulated_signal[i]);
    }

    /*
    //recompute sampling time every (bit_rate / 250) * spb
    if(n_bits_received == recompute_period) {
        n_bits_received = 0;
        start_index = symbol_offset_synch(demodulated_signal);
    }
    n_bits_received += downsampled.size() / spb; //this is just bit_rate
    */

    std::vector<float> filtered_signal = conv(downsampled_signal, h_matched);
    start_index = symbol_offset_synch(filtered_signal);


    //push previous samples
    //...

    //obtain samples
    std::vector<int> pulses;
    for(int i = samples_per_bit + start_index - 1; i < (int) filtered_signal.size(); i += samples_per_bit) {
        pulses.push_back(filtered_signal[i] > 0 ? 1 : -1);
    }

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

std::vector<float> BPSK_rx::correlate(std::vector<float> x, std::vector<float> y) {
    std::vector<float> r(x.size() + y.size() - 1);
    for(int i = 0; i < (int) r.size(); i++) {
        int ii = ((int) y.size()) - i - 1;
        int tmp = 0;
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
    float h_lowpass[FILTER_SIZE] = {0.0187, 0.0106, 0.0086, 0.0035, -0.0026, -0.0069, -0.0068, -0.0011,
                                    0.0100, 0.0249, 0.0411, 0.0568, 0.0702, 0.0807, 0.0878, 0.0913,
                                    0.0913, 0.0878, 0.0807, 0.0702, 0.0568, 0.0411, 0.0249, 0.0100,
                                    -0.0011, -0.0068, -0.0069, -0.0026, 0.0035, 0.0086, 0.0106, 0.0187};

    float z_sin[FILTER_SIZE];
    float z_cos[FILTER_SIZE];
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
        for(int i_h = FILTER_SIZE - 1, i_z = start_index; i_z != end_index; i_h--, i_z = (i_z + 1) % FILTER_SIZE) {
            lpf_sin += h_lowpass[i_h] * z_sin[i_z];
        }

        //LPF COSINE
        float lpf_cos = 0;
        for(int i_h = FILTER_SIZE - 1, i_z = start_index; i_z != end_index; i_h--, i_z = (i_z + 1) % FILTER_SIZE) {
            lpf_cos += h_lowpass[i_h] * z_cos[i_z];
        }

        result[n] = lpf_cos;

        theta -= mu * lpf_sin * lpf_cos;
    }

    returnr result;

}

int BPSK_rx::symbol_offset_synch(std::vector<float> filtered_signal) {
    return 0;
}
