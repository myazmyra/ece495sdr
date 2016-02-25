#include "Parameters_tx.h"
#include "USRP_tx.h"
#include "BPSK_tx.h"

#include <thread>
#include <mutex>

/***********************************************************************
 * Miscellaneous
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int junk) {
    stop_signal_called = true;
}

std::mutex mtx;

/***********************************************************************
 * Function Declarations
 **********************************************************************/

 void transmit(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx, std::vector<int> bits);
 void receive(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx);

int main(int argc, char *argv[]) {
    //give thread priority to this thread
	uhd::set_thread_priority_safe();

    //initialize Parameters
    Parameters_tx* parameters_tx = new Parameters_tx();

    //obtain useful values
    const double sample_rate = parameters_tx->get_sample_rate();
    const double f_c = parameters_tx->get_f_c();
    size_t spb = parameters_tx->get_spb();
    double bit_rate = parameters_tx->get_bit_rate();

    //initialize BPSK_tx
    BPSK_tx* bpsk_tx = new BPSK_tx(sample_rate, f_c, bit_rate, spb);

    //initialize USRP_tx
    USRP_tx* usrp_tx = new USRP_tx(sample_rate, f_c, spb);

    //makes it possible to stop the program by pressing Ctrl+C
    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

    //std::vector<int> bytes = readFile(argv[1]);
    //std::vector<int> bits = formPackets(bytes);

    std::vector<int> bits(1000, 1);

    //create threads and start them
    //transmit data
    std::thread transmit_thread(transmit, parameters_tx, usrp_tx, bpsk_tx, bits);

    //receive data (feedback on channel quality)
    std::thread receive_thread(receive, parameters_tx, usrp_tx, bpsk_tx);

    transmit_thread.join();
    receive_thread.join();

    delete usrp_tx;
    delete bpsk_tx;
    delete parameters_tx;

    return EXIT_SUCCESS;
}

void transmit(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx, std::vector<int> bits) {
    //start burst
    usrp_tx->send_start_of_burst();

    int i = 0;
    while(not stop_signal_called) {
        mtx.lock();
        usrp_tx->transmit(bpsk_tx->modulate(bits[i]), parameters_tx->get_spb());
        mtx.unlock();
        i++;
        i %= (int) bits.size();
    }

    //stop transmission
    usrp_tx->send_end_of_burst();
}

void receive(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx) {
    while(not stop_signal_called) {
        //receive some stuff from the microcontroller
        //usrp_tx.receive();
        mtx.lock();
        //change the parameters
        //parameters_tx.change(double bit_rate, usrp_tx, bpsk_tx);
        mtx.unlock();
    }
}
