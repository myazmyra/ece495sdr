#include "BPSK_rx.hpp"

BPSK_rx::BPSK_rx(size_t packet_size,
                 double sample_rate,
                 double f_IF,
                 int d_factor,
                 size_t spb,
                 int d_factor_new,
                 size_t spb_new,
                 std::vector<int> preamble_vector,
                 float power_desired,
                 float mu_agc,
                 float mu_pll,
                 size_t filter_size,
                 std::vector<float> h_lp_pll) :
                 sample_rate(sample_rate),
                 T_s(1 / sample_rate),
                 f_IF(f_IF),
                 d_factor(d_factor),
                 spb(spb),
                 d_factor_new(d_factor_new),
                 spb_new(spb_new),
                 power_desired(power_desired),
                 mu_agc(mu_agc),
                 mu_pll(mu_pll),
                 received_signal(2 * packet_size * spb),
                 downsampled_signal(2 * packet_size * spb_new),
                 pulses(downsampled_signal.size() / spb_new),
                 z_sin(filter_size),
                 z_cos(filter_size),
                 h_lp_pll(h_lp_pll) {

    //build the matched filter
    for(int n = 0; n < (int) spb_new; n++) {
        h_matched.push_back(1.0);
    }

    //upsample the preamble_vector by factor of spb_new
    std::vector<float> preamble_vector_upsampled;
    for(int i = 0; i < (int) preamble_vector.size(); i++) {
        for(int j = 0; j < (int) spb_new; j++) {
            preamble_vector_upsampled.push_back((float) preamble_vector[i]);
        }
    }

    //build the preamble detector by convolving with matched filter...
    //...then truncating the transient points from left and right
    std::vector<float> xcorr = conv(h_matched, preamble_vector_upsampled);
    preamble_detect.insert(preamble_detect.begin(),
                           xcorr.begin() + (h_matched.size() - 1),
                           xcorr.end() - (h_matched.size() - 1));



}

BPSK_rx::~BPSK_rx() {
    std::cout << "Destroying the BPSK_rx object..." << std::endl << std::endl;
}

std::vector<uint8_t> BPSK_rx::bytes_to_bits(std::vector<uint8_t> const &bytes) const {
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

std::vector<int> BPSK_rx::receive(std::vector< std::complex<float> > const &complex_signal) {

    //remove the DC component
    float mean = std::accumulate(complex_signal.begin(), complex_signal.end(),
                 std::complex<float>(0.0, 0.0)).real() / (float) complex_signal.size();

    for(int i = 0; i < (int) complex_signal.size(); i++) {
        received_signal[i] = complex_signal[i].real() - mean;
    }

    //go through agc
    std::vector<float> normalized_signal = agc(received_signal);

    //demodulate
    std::vector<float> demodulated_signal = costas_loop(normalized_signal);

    //downsample
    for(int i = 0, n = 0; n < (int) demodulated_signal.size(); i++, n += d_factor_new) {
        downsampled_signal[i] = demodulated_signal[n];
    }

    std::vector<float> filtered_signal = conv(h_matched, downsampled_signal);
    int polarity, start_index = symbol_offset_synch(filtered_signal, &polarity);

    for(int i = 0, n = (spb_new - 1) + start_index; n < (int) filtered_signal.size(); i++, n += spb_new) {
        pulses[i] = polarity * (filtered_signal[n] > 0 ? 1 : -1);
    }

    return pulses;

}

std::vector<int> BPSK_rx::receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers) {

    //downsample and accumulate everything in one buffer
    for(int i = 0; i < (int) buffers.size(); i++) {
        for(int j = 0, n = 0; n < (int) buffers[i]->size(); j++, n += d_factor) {
            received_signal[i * spb + j] = real(buffers[i]->at(n));
        }
    }

    //remove the dc component
    float mean = std::accumulate(received_signal.begin(), received_signal.end(), 0.0) / (float) received_signal.size();
    for(int i = 0; i < (int) received_signal.size(); i++) {
        received_signal[i] -= mean;
    }

    //go through agc
    std::vector<float> normalized_signal = agc(received_signal);

    //demodulate
    std::vector<float> demodulated_signal = costas_loop(normalized_signal);

    //downsample again
    for(int i = 0, n = 0; n < (int) demodulated_signal.size(); i++, n += d_factor_new) {
        downsampled_signal[i] = demodulated_signal[n];
    }

    std::vector<float> filtered_signal = conv(h_matched, downsampled_signal);
    int polarity, start_index = symbol_offset_synch(filtered_signal, &polarity);

    for(int i = 0, n = (spb_new - 1) + start_index; n < (int) filtered_signal.size(); i++, n += spb_new) {
        pulses[i] = polarity * (filtered_signal[n] > 0 ? 1 : -1);
    }

    return pulses;
}

std::vector<float> BPSK_rx::conv(std::vector<float> const &x, std::vector<float> const &h) const {
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

std::vector<float> BPSK_rx::correlate_rx(std::vector<float> const &x, std::vector<float> const &y) const {
    std::vector<float> xcorr(x.size() + y.size() - 1);
    for(int i = 0; i < (int) xcorr.size(); i++) {
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

std::vector<float> BPSK_rx::agc(std::vector<float> &r) {

    static float g = 1.0;

    for(int i = 0; i < (int) r.size(); i++) {
        r[i] = g * r[i];
        float update = (g > 0 ? 1.0 : -1.0) * (r[i] * r[i] - power_desired);
        g -= mu_agc * update;
    }

    return r;

}

std::vector<float> BPSK_rx::costas_loop(std::vector<float> &r) {
    //z_sin and z_cos are circular vectors
    static int start_index = 0, end_index = filter_size - 1;
    static float theta = 0.0;

    for(int n = 0; n < (int) r.size(); n++) {
        z_sin[end_index] = 2.0 * r[n] * sin(2 * M_PI * f_IF * n * T_s + theta);
        z_cos[end_index] = 2.0 * r[n] * cos(2 * M_PI * f_IF * n * T_s + theta);

        start_index = (start_index + 1) % filter_size;
        end_index = (end_index + 1) % filter_size;

        //LPF SINE
        float lpf_sin = 0;
        for(int i_h = filter_size - 1, i_z = start_index; i_h >= 0; i_h--, i_z = (i_z + 1) % filter_size) {
            lpf_sin += h_lp_pll[i_h] * z_sin[i_z];
        }

        //LPF COSINE
        float lpf_cos = 0;
        for(int i_h = filter_size - 1, i_z = start_index; i_h >= 0; i_h--, i_z = (i_z + 1) % filter_size) {
            lpf_cos += h_lp_pll[i_h] * z_cos[i_z];
        }

        r[n] = lpf_cos;

        theta -= mu_pll * lpf_sin * lpf_cos;
    }

    return r;

}

int BPSK_rx::symbol_offset_synch(std::vector<float> const &filtered_signal, int * const polarity) const {
    std::vector<float> xcorr = correlate_rx(filtered_signal, preamble_detect);
    int max_index = 0;
    float max_value = std::abs(xcorr[0]);

    for(int i = 1; i < (int) xcorr.size(); i++) {
        if(std::abs(xcorr[i]) > max_value) {
            max_index = i;
            max_value = std::abs(xcorr[i]);
        }
    }

    *polarity = xcorr[max_index] > 0 ? 1 : -1;
    return (max_index + 1) % (int) spb_new;
}
