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

    size_t spb = parameters_rx->get_spb();
    //vector to store all the buffer popinters
    std::vector< std::vector< std::complex<float> >* > buffers;
    //you will need to create new buffer each time you receive something...
    //...to be thread safe
    std::vector< std::complex<float> >* buff_ptr;
    while(true) {
        buff_ptr = new std::vector< std::complex<float> >(spb);
        infile.read((char*) &(buff_ptr->front()), spb * sizeof(std::complex<float>));
        if(infile.eof()) break;
        buffers.push_back(buff_ptr);
    }

    std::vector<uint8_t> bits = bpsk_rx->receive_from_file(buffers);
    std::vector<uint8_t> bytes = packet_decoder->decode(bits);

     //delete all the allocated buffer pointers
     for(int i = 0; i < (int) buffers.size(); i++) {
         delete buffers[i];
     }

    std::cout << "Done receiving from input file: " << input_filename << std::endl << std::endl;

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
