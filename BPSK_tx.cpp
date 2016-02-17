#include "USRP_tx.h"
#include "BPSK_tx.h"

BPSK_tx::BPSK_tx() : sample_rate(12.5e6), f_c(4e6), spb(9075), bit_rate(1000) {
    //do something, maybe check for aliasing after you get your 3-dB frequencies from the analog guys?
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

int main(int argc, char *argv[]) {
    //give thread priority to usrp
    uhd::set_thread_priority_safe();

    BPSK_tx* bpsk_tx = new BPSK_tx();
    const double sample_rate = bpsk_tx->get_sample_rate();
    const double f_c = bpsk_tx->get_f_c();
    const size_t spb = bpsk_tx->get_spb();
    USRP_tx* usrp_tx = new USRP_tx(sample_rate, f_c, spb);

    std::vector< std::complex<float> > buff(spb);
    usrp_tx->send_start_of_burst();
    usrp_tx->transmit(buff);
    usrp_tx->send_end_of_burst();

    return EXIT_SUCCESS;
}
