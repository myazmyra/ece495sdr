#include "USRP_tx.h"
#include "BPSK_tx.h"

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

BPSK_tx::BPSK_tx() : sample_rate(12.5e6), f_c(4e6), spb(9075), bit_rate(1000),
                     samples_per_bit(((int) sample_rate) / bit_rate),
                     positive(samples_per_bit), negative(samples_per_bit) {
    //do something, maybe check for aliasing after you get your 3-dB frequencies from the analog guys?
    double T_s = 1 / sample_rate;
    std::vector< std::complex<float> >::iterator it_pos = positive.begin();
    std::vector< std::complex<float> >::iterator it_neg = negative.begin();
    for(int n = 0; n < samples_per_bit; n++) {
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

void BPSK_tx::modulate(std::vector<int> bits, int N, std::vector< std::complex<float> > buff) {
    int transmit_size = N * samples_per_bit;
    for(int i = 0; i < N; i++) {
        if(bits[i] == 1) {
            for(int n = 0; n < transmit_size; n++) {

            }
        }
    }
}

int main(int argc, char *argv[]) {
    BPSK_tx bpsk_tx;
    const double sample_rate = bpsk_tx.get_sample_rate();
    const double f_c = bpsk_tx.get_f_c();
    const size_t spb = bpsk_tx.get_spb();
    const unsigned int bit_rate = bpsk_tx.get_bit_rate();
    USRP_tx usrp_tx(sample_rate, f_c, spb);
    const size_t samples_per_bit = ((int) sample_rate) / bit_rate;

    //makes it possible to stop the program by pressing Ctrl+C
    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

    std::vector< std::complex<float> > buff(spb);
    usrp_tx.send_start_of_burst();
    usrp_tx.transmit(buff, buff.size());
    usrp_tx.send_end_of_burst();

    return EXIT_SUCCESS;
}
