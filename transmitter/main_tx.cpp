#include "PacketEncoder.hpp"
#include "Parameters_tx.hpp"
#include "USRP_tx.hpp"
#include "BPSK_tx.hpp"

#include <iostream>
#include <cstdint>
#include <fstream>
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
int transmitted = 0;
int received = 0;

/***********************************************************************
 * Function Declarations
 **********************************************************************/
void printHelp();
void transmit(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits);
void receive(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx);
std::vector<uint8_t> readFile(std::string inFile);
void sendToFile(Parameters_tx* parameters_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits, std::string outFile);

int main(int argc, char** argv) {

    std::string mode;
    std::string inFile;
    std::string outFile;

    if(argc < 2) {
        printHelp();
        return EXIT_FAILURE;
    } else {
        if(std::string(argv[1]) != std::string("--mode")) {
            printHelp();
            return EXIT_FAILURE;
        }
        mode = std::string(argv[2]);
        if(mode != std::string("broadcast") && mode != std::string("local")) {
            printHelp();
            return EXIT_FAILURE;
        }
        inFile = std::string(argv[3]);
        if(argc > 4) {
            outFile = std::string(argv[4]);
        }
    }

    //reads the file, forms packets, converts to bit sequences
    std::vector<uint8_t> bits = readFile(inFile);

    //give thread priority to this thread
	uhd::set_thread_priority_safe();
    std::cout << std::endl;

    //initialize Parameters
    Parameters_tx* parameters_tx = new Parameters_tx();

    //obtain useful values
    const double sample_rate = parameters_tx->get_sample_rate();
    const double f_c = parameters_tx->get_f_c();
    size_t spb = parameters_tx->get_spb();
    double bit_rate = parameters_tx->get_bit_rate();

    //initialize BPSK_tx
    BPSK_tx* bpsk_tx = new BPSK_tx(sample_rate, f_c, bit_rate, spb);

    if(mode == std::string("local")) {
        sendToFile(parameters_tx, bpsk_tx, bits, outFile);

        delete bpsk_tx;
        delete parameters_tx;

        std::cout << "Done!" << std::endl << std::endl;

        return EXIT_SUCCESS;
    }

    //else, broadcast mode, transmit through usrp

    //initialize USRP_tx
    USRP_tx* usrp_tx = new USRP_tx(sample_rate, f_c, spb);

    //makes it possible to stop the program by pressing Ctrl+C
    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

    //create threads and start them
    //transmit data
    std::thread transmit_thread(transmit, parameters_tx, usrp_tx, bpsk_tx, bits);

    //receive data (feedback on channel quality)
    std::thread receive_thread(receive, parameters_tx, usrp_tx, bpsk_tx);

    transmit_thread.join();
    receive_thread.join();

    std::cout << std::endl << std::endl;
    std::cout << "Transmitted: " << transmitted << " times" << std::endl;
    std::cout << "Received: " << received << " times" << std::endl;
    std::cout << std::endl;

    delete usrp_tx;
    delete bpsk_tx;
    delete parameters_tx;

    std::cout << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

void transmit(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits) {

    int i = 0;
    int size = (int) bits.size();

    //start burst
    usrp_tx->send_start_of_burst();

    while(not stop_signal_called) {
        mtx.lock();
        usrp_tx->transmit(bpsk_tx->modulate(bits[i]), parameters_tx->get_spb());
        transmitted++;
        mtx.unlock();
        i++;
        i %= size;
    }

    //stop transmission
    usrp_tx->send_end_of_burst();
}

void receive(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx) {
    while(not stop_signal_called) {
        //receive some stuff from the microcontroller
        mtx.lock();
        //usrp_tx.receive();
        //change the parameters
        //parameters_tx.change(double bit_rate, usrp_tx, bpsk_tx);
        received++;
        mtx.unlock();
    }
}

std::vector<uint8_t> readFile(std::string inFile) {
    std::ifstream file(inFile, std::ios::ate | std::ios::binary);
    std::ifstream::pos_type size = 0;
    char* bytes = NULL;

    if(file.is_open() == false) {
        std::cout << "Unable to open input file: " << inFile << std::endl << std::endl;
        throw new std::runtime_error("Unable to open input file: " + inFile);
    }

    std::cout << "Successfully opened input file: " << inFile << std::endl << std::endl;
    size =  file.tellg();
    bytes = new char[size];
    file.seekg(0, std::ios::beg);
    file.read(bytes, size);
    file.close();

    //this will automatically form packets with preamble and checksum and then
    //create bit sequences from bytes
    return PacketEncoder::formPackets(bytes, (int) size);
}

void sendToFile(Parameters_tx* parameters_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits, std::string outFile) {
    std::ofstream file(outFile, std::ofstream::binary);
    if(file.is_open() == false) {
        std::cout << "Unable to open output file: " << outFile << std::endl << std::endl;
        throw new std::runtime_error("Unable to open output file: " + outFile);
    }

    std::cout << "Successfully opened output file: " << outFile << std::endl << std::endl;

    int size = (int) bits.size();
    int spb = parameters_tx->get_spb();

    for(int i = 0; i < size; i++) {
        std::vector< std::complex<float> > buff = bpsk_tx->modulate(bits[i]);
        file.write((const char*) &(buff.front()), spb * sizeof(std::complex<float>));
    }

    file.close();

    std::cout << "Done writing to output file: " << outFile << std::endl << std::endl;
}

void printHelp() {
    std::cout << "Usage: " << std::endl << std::endl;
    std::cout << "./main_tx.o --mode broadcast [intput_filename]" << std::endl << std::endl;
    std::cout << "  or" << std::endl << std::endl;
    std::cout << "./main_tx.o --mode local [intput_filename] [output_filename]" << std::endl << std::endl;
}
