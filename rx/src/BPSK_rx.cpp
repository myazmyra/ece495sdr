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
                 packet_size(packet_size),
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
                 h_lp_pll(h_lp_pll),
                 z_sin(filter_size),
                 z_cos(filter_size) {

    std::cout << "mu_agc: " << filter_size << std::endl;
    for(int i = 0; i < (int) h_lp_pll.size(); i++) {
        std::cout << h_lp_pll[i] << ", ";
    }
    std::cout << std::endl;

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
    std::vector<float> v_conv(h_matched.size() + preamble_vector_upsampled.size() - 1);
    size_t v_conv_size = conv(h_matched, preamble_vector_upsampled, v_conv);
    preamble_detect.insert(preamble_detect.begin(),
                           v_conv.begin() + (h_matched.size() - 1),
                           v_conv.begin() + v_conv_size - (h_matched.size() - 1));

}

BPSK_rx::~BPSK_rx() {
    std::cout << "Destroying the BPSK_rx object..." << std::endl << std::endl;
}

size_t BPSK_rx::receive(std::vector< std::complex<float> > const &complex_signal, std::vector<int> &pulses) {


    //remove the DC component
    //float sum = std::accumulate(complex_signal.begin(), complex_signal.end(),
    //             std::complex<float>(0.0, 0.0)).real();
    //float mean = sum / (float) complex_signal.size();

    static std::vector<float> received_signal(spb * 2 * packet_size * 8);
    if(complex_signal.size() != received_signal.size()) {
        std::cout << "Unexpected error: received_signal.size() and complex_signal.size() don't match" << std::endl;
        throw std::runtime_error("Unexpected error: received_signal.size() and complex_signal.size() don't match");
    }
    for(int i = 0; i < (int) complex_signal.size(); i++) {
        received_signal[i] = complex_signal[i].real();// - mean;
    }
    //float sq_sum = std::inner_product(received_signal.begin(), received_signal.end(), received_signal.begin(), 0) - mean * mean;
    //std::cout << "mean: " << mean << std::endl;
    //std::cout << "var: " << sq_sum << std::endl;

    //go through agc
    agc(received_signal);
    //for(int i = 0; i < (int) received_signal.size(); i++) {
    //    std::cout << received_signal[i] << ", ";
    //}
    //std::cout << std::endl;
    float mean = std::accumulate(received_signal.begin(), received_signal.end(), 0) / (float) received_signal.size();
    float sq_sum = std::inner_product(received_signal.begin(), received_signal.end(), received_signal.begin(), 0) / (float) received_signal.size() - mean * mean;
    std::cout << "new mean: " << mean << std::endl;
    std::cout << "new var: " << sq_sum << std::endl;

    //demodulate
    costas_loop(received_signal);

    //downsample
    static std::vector<float> downsampled_signal(spb_new * 2 * packet_size * 8);
    for(int i = 0, n = 0; n < (int) received_signal.size(); i++, n += d_factor_new) {
        downsampled_signal[i] = received_signal[n];
    }

    static std::vector<float> filtered_signal(h_matched.size() + downsampled_signal.size() - 1);
    size_t filtered_signal_size = conv(h_matched, downsampled_signal, filtered_signal);
    int polarity, start_index = symbol_offset_synch(filtered_signal, &polarity);
    //std::cout << "start_index: " << start_index << ", polarity: " << polarity << std::endl;

    for(int i = 0, n = (spb_new - 1) + start_index; n < (int) filtered_signal_size; i++, n += spb_new) {
        pulses[i] = polarity * (filtered_signal[n] > 0 ? 1 : -1);
        //std::cout << filtered_signal[n] << ", ";
    }
    //std::cout << std::endl;

    return pulses.size();

}

/*
size_t BPSK_rx::receive_from_file(std::vector< std::vector< std::complex<float> >* > buffers, std::vector<int> &pulses) {

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
*/

size_t BPSK_rx::conv(std::vector<float> const &x, std::vector<float> const &h, std::vector<float> &y) const {
    if(x.size() + h.size() - 1 > y.size()) {
        std::cout << "Not enough space in y to store conv(x, h) result" << std::endl;
        throw std::runtime_error("Not enough space in y to store conv(x, h) result");
    }
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
    return x.size() + h.size() - 1;
}

size_t BPSK_rx::correlate(std::vector<float> const &x, std::vector<float> const &y, std::vector<float> &rxy) const {
    if(x.size() + y.size() - 1 > rxy.size()) {
        std::cout << "Not enough space to store correlate(x, y) result" << std::endl;
        throw std::runtime_error("Not enough space in rxy to store correlate(x, y) result");
    }
    for(int i = 0; i < (int) rxy.size(); i++) {
        int ii = ((int) y.size()) - i - 1;
        float tmp = 0.0;
        for(int j = 0; j < (int) x.size(); j++) {
            if(ii >= 0 && ii < (int) y.size()) {
                tmp += y[ii] * x[j];
            }
            ii++;
        }
        rxy[i] = tmp;
    }
    return x.size() + y.size() - 1;
}

void BPSK_rx::agc(std::vector<float> &r) {

    static float g = 1.0;

    for(int i = 0; i < (int) r.size(); i++) {
        r[i] = g * r[i];
        float update = (g > 0 ? 1.0 : -1.0) * (r[i] * r[i] - power_desired);
        g -= mu_agc * update;
    }

}

void BPSK_rx::costas_loop(std::vector<float> &r) {

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

        theta -= 140 * mu_pll * lpf_sin * lpf_cos;
    }
    std::cout << "theta: " << theta << std::endl;

}

int BPSK_rx::symbol_offset_synch(std::vector<float> const &filtered_signal, int * const polarity) const {
    static std::vector<float> rxy(filtered_signal.size() + preamble_detect.size() - 1);
    if(filtered_signal.size() + preamble_detect.size() - 1 > rxy.size()) {
        rxy.resize(filtered_signal.size() + preamble_detect.size() - 1);
    }
    size_t rxy_size = correlate(filtered_signal, preamble_detect, rxy);
    int max_index = 0;
    float max_value = std::abs(rxy[0]);

    for(int i = 1; i < (int) rxy_size; i++) {
        if(std::abs(rxy[i]) > max_value) {
            max_index = i;
            max_value = std::abs(rxy[i]);
        }
    }

    *polarity = rxy[max_index] > 0 ? 1 : -1;
    return (max_index + 1) % (int) spb_new;
}
