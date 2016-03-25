#include "Parameters_rx.hpp"
#include "BPSK_rx.hpp"
#include "PacketDecoder.hpp"

#include <iostream>
#include <cstdint>
#include <fstream>
#include <thread>
#include <mutex>

//NEED TO ADD PACKET DECODER CLASS

/***********************************************************************
 * Miscellaneous
 **********************************************************************/
std::string mode = "";
std::string input_filename = "";
std::string output_filename = "";

std::mutex mtx;

/***********************************************************************
 * Function Declarations
 **********************************************************************/
int validate_input(int argc, char** argv);
void print_help();
void receive_from_file(Parameters_rx* parameters_rx, BPSK_rx* bpsk_rx,
                       PacketDecoder* packet_decoder);
inline float rand_float(float a, float b);
void rand_noise_generator(std::vector< std::vector< std::complex<float> >* >& buffers,
                          int n_packets, int packet_size, size_t spb);

int main(int argc, char** argv) {

    if(validate_input(argc, argv) == 0) {
        return EXIT_FAILURE;
    }

    Parameters_rx* parameters_rx = new Parameters_rx();

    BPSK_rx* bpsk_rx = new BPSK_rx(parameters_rx->get_sample_rate(),
                                   parameters_rx->get_f_IF(),
                                   parameters_rx->get_bit_rate(),
                                   parameters_rx->get_spb(),
                                   parameters_rx->get_decimation_factor());

    //probably better idea to set LFSR_one and LFSR_two and data_per_packet...
    //...in Parameters_rx object
    PacketDecoder* packet_decoder = new PacketDecoder();

    //maybe use enums later
    if(mode == std::string("local")) {
        std::cout << std::endl; //aesthetic purposes
        receive_from_file(parameters_rx, bpsk_rx, packet_decoder);
    }

    std::cout << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

void receive_from_file(Parameters_rx* const parameters_rx,
                       BPSK_rx* const bpsk_rx,
                       PacketDecoder* packet_decoder) {
    std::ifstream infile(input_filename, std::ifstream::binary);
    std::ofstream outfile(output_filename, std::ofstream::binary);

    if(infile.is_open() == false) {
        std::cout << "Unable to open input file: " << input_filename << std::endl << std::endl;
        throw new std::runtime_error("Unable to open input file: " + input_filename);
    } else if(outfile.is_open() == false) {
        std::cout << "Unable to open output file: " << output_filename << std::endl << std::endl;
        throw new std::runtime_error("Unable to open output file: " + output_filename);
    }

    std::cout << "Successfully opened input file: " << input_filename;
    std::cout << std::endl << std::endl;
    std::cout << "Successfully opened ouput file: " << output_filename << std::endl << std::endl;

    size_t spb_tx = parameters_rx->get_spb_tx();
    //all of the below parameters should be in parameters_rx and not in packet_decoder
    //pass it to the packet decoder via constructor
    int packet_size = packet_decoder->get_packet_size();
    int num_packets_per_call = packet_decoder->get_num_packets_per_call();
    //vector to store all the buffer popinters
    std::vector< std::vector< std::complex<float> >* > buffers;
    //you will need to create new buffer each time you receive something...
    //...to be thread safe
    std::vector< std::complex<float> >* buff_ptr;
    //add bunch of junk to test packet decoder
    int n_rand_packets = 0, count = n_rand_packets / num_packets_per_call;
    for(int i = 0; i < count; i++) {
        rand_noise_generator(buffers, num_packets_per_call, packet_size, spb_tx);
        std::vector<int> pulses = bpsk_rx->receive_from_file(buffers);
        std::vector<uint8_t> bytes = packet_decoder->decode(pulses);
        for(auto b : bytes) {
            outfile.write((char*) &b, sizeof(char));
        }
        //delete all the allocated buffer pointers
        for(int j = 0; j < (int) buffers.size(); j++) {
            delete buffers[j];
        }
        buffers.clear();
        n_rand_packets -= num_packets_per_call;
    }
    //check if the buffers have the right size (i.e. less than 3 packets)
    if(n_rand_packets > 2 || (int) buffers.size() >= num_packets_per_call * packet_size * 8) {
        std::cout << "Something went wrong with junk packet adding" << std::endl << std::endl;
        throw new std::runtime_error("Something went wrong with junk packet adding");
    }
    rand_noise_generator(buffers, n_rand_packets, packet_size, spb_tx);
    while(true) {
        buff_ptr = new std::vector< std::complex<float> >(spb_tx);
        infile.read((char*) &(buff_ptr->front()), spb_tx * sizeof(std::complex<float>));
        //if(!infile) break; //std::cout << "infile.count(): " << infile.gcount() << std::endl;
        //std::cout << "cos(0): " << buff_ptr->at(0) << std::endl;
        if(infile.eof()) break;
        buffers.push_back(buff_ptr);
        //the packet decoder reads num_packets_per_call packets at a time,
        //num_packets_per_call-1 guaranteed to exist. each packet is 128 bits (i.e. 128 reads from file)
        if((int) buffers.size() == num_packets_per_call * packet_size * 8) {
            std::vector<int> pulses = bpsk_rx->receive_from_file(buffers);
            std::vector<uint8_t> bytes = packet_decoder->decode(pulses);
            for(auto b : bytes) {
                outfile.write((char*) &b, sizeof(char));
            }
            //delete all the allocated buffer pointers
            for(int i = 0; i < (int) buffers.size(); i++) {
                delete buffers[i];
            }
            buffers.clear();
        }
    }

    //if packet wasnt completed to 3, generate random vectors and decode
    //like the usrp would do
    int n_packets_remaining = buffers.size() / (packet_size * 8);
    //std::cout << "pack: " << n_packets_remaining << std::endl;
    //add rand packets to complete the buffer to three packets
    if(n_packets_remaining != 0) {
        rand_noise_generator(buffers, num_packets_per_call - n_packets_remaining, packet_size, spb_tx);
        std::vector<int> pulses = bpsk_rx->receive_from_file(buffers);
        std::vector<uint8_t> bytes = packet_decoder->decode(pulses);
        for(auto b : bytes) {
            outfile.write((char*) &b, sizeof(char));
        }
    }
    //delete all the allocated buffer pointers
    for(int i = 0; i < (int) buffers.size(); i++) {
        delete buffers[i];
    }
    buffers.clear();

    std::cout << "Done receiving from input file: " << input_filename << std::endl << std::endl;
    std::cout << "Done writing to output file: " << output_filename << std::endl << std::endl;

    infile.close();
    outfile.close();
}

void print_help() {
    std::cout << "Usage: " << std::endl << std::endl;
    std::cout << "./main_rx --mode usrp [infile_path]" << std::endl << std::endl;
    std::cout << "  or" << std::endl << std::endl;
    std::cout << "./main_rx --mode local [infile_path] [outfile_path]" << std::endl << std::endl;
}

int validate_input(int argc, char** argv) {
    //input validation
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

inline float rand_float(float a, float b) {
    float random = ((float) std::rand()) / ((float) RAND_MAX);
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

void rand_noise_generator(std::vector< std::vector< std::complex<float> >* >& buffers, int n_packets, int packet_size, size_t spb) {
    float a = 1.0, b = -1.0;
    for(int i = 0; i < n_packets; i++) {
        for(int j = 0; j < packet_size * 8; j++) {
            std::vector< std::complex<float> >* buff_ptr = new std::vector< std::complex<float> >(spb);
            for(int k = 0; k < (int) spb; k++) {
                (*buff_ptr)[k] = rand_float(a, b);
            }
            buffers.push_back(buff_ptr);
        }
    }
}
