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

std::string mode = "";
std::string input_filename = "";
std::string output_filename = "";

std::mutex mtx;
int transmitted = 0;
int received = 0;

/***********************************************************************
 * Function Declarations
 **********************************************************************/
int validate_input(int argc, char** argv);
void print_help();
std::vector<uint8_t> read_file();
void transmit(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits);
void receive(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx);
void send_to_file(Parameters_tx* parameters_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits);

int main(int argc, char** argv) {

    //input validation
    if(validate_input(argc, argv) == 0) {
        return EXIT_FAILURE;
    }

    //reads the file, forms packets, converts to bit sequences
    std::vector<uint8_t> bits = read_file();

    //initialize Parameters
    Parameters_tx* parameters_tx = new Parameters_tx();

    //initialize BPSK_tx
    BPSK_tx* bpsk_tx = new BPSK_tx(parameters_tx->get_sample_rate(),
                                   parameters_tx->get_f_c(),
                                   parameters_tx->get_bit_rate(),
                                   parameters_tx->get_spb());

    if(mode == std::string("local")) {
        send_to_file(parameters_tx, bpsk_tx, bits);

        delete bpsk_tx;
        delete parameters_tx;

        std::cout << "Done!" << std::endl << std::endl;

        return EXIT_SUCCESS;
    }

    //else, broadcast mode, transmit through usrp
    //give thread priority to this thread
	uhd::set_thread_priority_safe();
    std::cout << std::endl;

    //initialize USRP_tx
    USRP_tx* usrp_tx = new USRP_tx(parameters_tx->get_sample_rate(),
                                   parameters_tx->get_f_c(), parameters_tx->get_spb());

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
    std::cout << "Transmit thread called: " << transmitted << " times" << std::endl;
    std::cout << "Receive thread called: " << received << " times" << std::endl;
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

    std::vector< std::complex<float> > positive = bpsk_tx->modulate(1);
    std::vector< std::complex<float> > negative = bpsk_tx->modulate(0);

    size_t spb = parameters_tx->get_spb();

    //start burst
    usrp_tx->send_start_of_burst();

    while(not stop_signal_called) {
        mtx.lock();
        usrp_tx->transmit(bits[i] ? positive : negative, spb);
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

std::vector<uint8_t> read_file() {
    std::ifstream infile(input_filename, std::ios::ate | std::ios::binary);
    std::ifstream::pos_type size = 0;
    char* bytes = NULL;

    if(infile.is_open() == false) {
        std::cout << "Unable to open input file: " << input_filename << std::endl << std::endl;
        throw new std::runtime_error("Unable to open input file: " + input_filename);
    }

    std::cout << "Successfully opened input file: " << input_filename << std::endl << std::endl;
    size = infile.tellg();
    bytes = new char[size];
    infile.seekg(0, std::ios::beg);
    infile.read(bytes, size);
    infile.close();

    //this will automatically form packets with preamble and checksum and then
    //create bit sequences from bytes
    std::vector<uint8_t> bits = PacketEncoder::form_packets(bytes, (int) size);
    delete[] bytes;
    return bits;
}

void send_to_file(Parameters_tx* parameters_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits) {
    std::ofstream outfile(output_filename, std::ofstream::binary);
    if(outfile.is_open() == false) {
        std::cout << "Unable to open output file: " << output_filename << std::endl << std::endl;
        throw new std::runtime_error("Unable to open output file: " + output_filename);
    }

    std::cout << "Successfully opened output file: " << output_filename << std::endl << std::endl;

    int size = (int) bits.size();
    int spb = parameters_tx->get_spb();

    //send some random bits
    int n_rand_bits = 140;

    for(int i = 0; i < n_rand_bits; i++) {
        uint8_t rand_bit = std::rand() % 2;
        std::vector< std::complex<float> > buff = bpsk_tx->modulate(rand_bit);
        outfile.write((char*) &(buff.front()), spb * sizeof(std::complex<float>));
    }

    for(int i = 0; i < size; i++) {
        std::vector< std::complex<float> > buff = bpsk_tx->modulate(bits[i]);
        outfile.write((char*) &(buff.front()), spb * sizeof(std::complex<float>));
    }

    int remaining_n_rand_bits = 2 * 16 * 8 - (n_rand_bits % 2 * 16 * 8) + 1024;
    for(int i = 0; i < remaining_n_rand_bits; i++) {
        uint8_t rand_bit = std::rand() % 2;
        std::vector< std::complex<float> > buff = bpsk_tx->modulate(rand_bit);
        outfile.write((char*) &(buff.front()), spb * sizeof(std::complex<float>));
    }

    outfile.close();

    std::cout << "Done writing to output file: " << output_filename << std::endl << std::endl;
}

void print_help() {
    std::cout << "Usage: " << std::endl << std::endl;
    std::cout << "./main_tx.o --mode usrp [infile_path]" << std::endl << std::endl;
    std::cout << "  or" << std::endl << std::endl;
    std::cout << "./main_tx.o --mode local [infile_path] [outfile_path]" << std::endl << std::endl;
}

int validate_input(int argc, char** argv) {
    if(argc < 4) {
        print_help();
        return 0;
    } else {
        if(std::string(argv[1]) != std::string("--mode")) {
            print_help();
            return 0;
        }
        mode = std::string(argv[2]);
        if(mode != std::string("usrp") && mode != std::string("local")) {
            print_help();
            return 0;
        }
        input_filename = std::string(argv[3]);
        if(mode == std::string("local") && argc < 5) {
            print_help();
            return 0;
        }
        if(mode == std::string("local")) {
            output_filename = std::string(argv[4]);
        }
    }
    return 1;
}
