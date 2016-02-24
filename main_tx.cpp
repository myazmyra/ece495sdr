#include "USRP_tx.h"
#include "BPSK_tx.h"

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int junk) {
    stop_signal_called = true;
}

/***********************************************************************
 * Function Declarations
 **********************************************************************/

 //void transmit(Parameters p, USRP_tx usrp_tx, BPSK_tx bpsk_tx, std::vector<int> bits);
 //void feedback(Parameters p, USRP_tx usrp_tx);

int main(int argc, char *argv[]) {
    //give thread priority to this thread
	uhd::set_thread_priority_safe();

    //initialize BPSK_tx
    BPSK_tx bpsk_tx;

    //obtain useful values
    const double sample_rate = bpsk_tx.get_sample_rate();
    const double f_c = bpsk_tx.get_f_c();
    const size_t spb = bpsk_tx.get_spb();
    const double bit_rate = bpsk_tx.get_bit_rate();

    //initialize USRP_tx
    USRP_tx usrp_tx(sample_rate, f_c, spb);

    //makes it possible to stop the program by pressing Ctrl+C
    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

    //std::vector<int> bytes = readFile(argv[1]);
    //std::vector<int> bits = formPackets(bytes);

    std::vector<int> bits(120 * (int) bit_rate, 1);

    //start burst
    usrp_tx.send_start_of_burst();

    //transmit data
    int i = 0;
    while(not stop_signal_called) {
        usrp_tx.transmit(bpsk_tx.modulate(bits[i]), spb);
        i++;
        i %= (int) bits.size();
    }

    //stop transmission
    usrp_tx.send_end_of_burst();

    return EXIT_SUCCESS;
}
