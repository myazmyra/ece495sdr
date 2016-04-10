#include "USRP_rx.hpp"
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
static bool stop_signal_called = false;
void sig_int_handler(int junk) {
    stop_signal_called = true;
}

std::string mode = "";
std::string input_filename = "";
std::string output_filename = "";

std::mutex mtx;

/***********************************************************************
 * Function Declarations
 **********************************************************************/
int validate_input(int argc, char** argv);
void print_help();

void receive(Parameters_rx * const parameters_rx,
             USRP_rx * const usrp_rx,
             BPSK_rx * const bpsk,
             PacketDecoder * const packet_decoder);

void receive_from_file(Parameters_rx * const parameters_rx,
                       BPSK_rx * const bpsk_rx,
                       PacketDecoder * const packet_decoder);

int main(int argc, char** argv) {

    if(validate_input(argc, argv) == 0) {
        return EXIT_FAILURE;
    }

    Parameters_rx * parameters_rx = new Parameters_rx();

    BPSK_rx * bpsk_rx = new BPSK_rx(parameters_rx->get_sample_rate(),
                                   parameters_rx->get_f_IF(),
                                   parameters_rx->get_d_factor(),
                                   parameters_rx->get_spb(),
                                   parameters_rx->get_d_factor_new(),
                                   parameters_rx->get_spb_new(),
                                   parameters_rx->get_preamble_vector());

    //probably better idea to set LFSR_one and LFSR_two and data_per_packet...
    //...in Parameters_rx object
    PacketDecoder * packet_decoder = new PacketDecoder(parameters_rx->get_preamble_size(),
                                                      parameters_rx->get_data_size(),
                                                      parameters_rx->get_checksum_size(),
                                                      parameters_rx->get_packet_size(),
                                                      parameters_rx->get_preamble_vector());

    //maybe use enums later
    if(mode == std::string("local")) {
        std::cout << std::endl; //aesthetic purposes
        receive_from_file(parameters_rx, bpsk_rx, packet_decoder);

    } else if(mode == std::string("usrp")) {
        std::cout << std::endl; //aesthetic purposes
        USRP_rx * usrp_rx = new USRP_rx(parameters_rx->get_sample_rate_tx(),
                                        parameters_rx->get_spb_tx(),
                                        parameters_rx->get_d_factor());

        receive(parameters_rx,
                usrp_rx,
                bpsk_rx,
                packet_decoder);

        delete usrp_rx;

    } else {
        std::cout << std::endl;
        std::cout << "Input validation does not work, please fix" << std::endl;
    }



    delete parameters_rx;
    delete bpsk_rx;
    delete packet_decoder;

    std::cout << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}


void receive(Parameters_rx * const parameters_rx,
             USRP_rx * const usrp_rx,
             BPSK_rx * const bpsk_rx,
             PacketDecoder * const packet_decoder) {
    std::ofstream outfile(output_filename, std::ofstream::binary);
    if(outfile.is_open() == false) {
      std::cout << "Unable to open output file: " << output_filename << std::endl << std::endl;
      throw std::runtime_error("Unable to open output file: " + output_filename);
    }
    std::cout << "Successfully opened ouput file: " << output_filename << std::endl << std::endl;

    size_t packet_size = parameters_rx->get_packet_size();
    size_t spb = parameters_rx->get_spb();

    std::vector< std::complex<float> > buff(spb);
    std::vector< std::complex<float> > buff_accumulate;
    size_t total_num_rx_samps = 0;
    usrp_rx->issue_start_streaming();
    while(not stop_signal_called) {
        size_t num_rx_samps = usrp_rx->receive(buff);
        if(num_rx_samps != spb) {
            std::cout << "Did not receive enough samples" << std::endl << std::endl;
            throw std::runtime_error("Did not receive enough samples");
        }
        buff_accumulate.insert(buff_accumulate.end(), buff.begin(), buff.end());
        total_num_rx_samps += num_rx_samps;
        if(total_num_rx_samps == spb * 2 * packet_size * 8) {
            total_num_rx_samps = 0;
            std::vector<int> pulses = bpsk_rx->receive(buff_accumulate);
            buff_accumulate.clear();
            std::vector<uint8_t> bytes = packet_decoder->decode(pulses);
            /*if(bytes.size() != 0) {
                //std::cout << "bytes.size(): " << bytes.size() << std::endl;
                for(auto b : bytes) {
                    std::cout << (char) b;
                }
                std::cout << std::endl;
            }
            */
            outfile.write((char *) &bytes.front(), bytes.size() * sizeof(char));
            outfile.flush();
        }
    }
    usrp_rx->issue_stop_streaming();
    outfile.close();
}


void receive_from_file(Parameters_rx* const parameters_rx,
                       BPSK_rx* const bpsk_rx,
                       PacketDecoder* packet_decoder) {
    std::ifstream infile(input_filename, std::ifstream::binary);
    std::ofstream outfile(output_filename, std::ofstream::binary);

    if(infile.is_open() == false) {
        std::cout << "Unable to open input file: " << input_filename << std::endl << std::endl;
        throw std::runtime_error("Unable to open input file: " + input_filename);
    } else if(outfile.is_open() == false) {
        std::cout << "Unable to open output file: " << output_filename << std::endl << std::endl;
        throw std::runtime_error("Unable to open output file: " + output_filename);
    }

    std::cout << "Successfully opened input file: " << input_filename << std::endl << std::endl;
    std::cout << "Successfully opened ouput file: " << output_filename << std::endl << std::endl;

    size_t spb_tx = parameters_rx->get_spb_tx();
    size_t packet_size = parameters_rx->get_packet_size();

    //vector to store all the buffer popinters
    std::vector< std::vector< std::complex<float> >* > buffers;
    //you will need to create new buffer each time you receive something...
    //...to be thread safe
    std::vector< std::complex<float> >* buff_ptr;
    while(true) {
        buff_ptr = new std::vector< std::complex<float> >(spb_tx);
        infile.read((char*) &(buff_ptr->front()), spb_tx * sizeof(std::complex<float>));
        if(infile.eof()) break;
        buffers.push_back(buff_ptr);
        if(buffers.size() == 2 * packet_size * 8) {
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
    std::cout << "./main_rx --mode usrp [outfile_path]" << std::endl << std::endl;
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
        if(mode == std::string("local") && argc < 5) {
            print_help();
            return 0;
        }
        if(mode == std::string("local")) {
            input_filename = std::string(argv[3]);
            output_filename = std::string(argv[4]);
        } else if(mode == std::string("usrp")) {
            output_filename = std::string(argv[3]);
        }
    }
    return 1;
}
