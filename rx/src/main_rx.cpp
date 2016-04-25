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

std::string receive_filename;
std::string output_filename;

/***********************************************************************
 * Function Declarations
 **********************************************************************/
void receive(Parameters_rx * const parameters_rx,
             USRP_rx * const usrp_rx,
             BPSK_rx * const bpsk,
             PacketDecoder * const packet_decoder);

void transmit(USRP_rx * const usrp_rx);

int main(int argc, char** argv) {

    Parameters_rx * parameters_rx = new Parameters_rx();

    BPSK_rx * bpsk_rx = new BPSK_rx(parameters_rx->get_packet_size(),
                                    parameters_rx->get_sample_rate(),
                                    parameters_rx->get_f_IF(),
                                    parameters_rx->get_d_factor(),
                                    parameters_rx->get_spb(),
                                    parameters_rx->get_d_factor_new(),
                                    parameters_rx->get_spb_new(),
                                    parameters_rx->get_preamble_vector(),
                                    parameters_rx->get_power_desired(),
                                    parameters_rx->get_mu_agc(),
                                    parameters_rx->get_mu_pll(),
                                    parameters_rx->get_filter_size(),
                                    parameters_rx->get_h_lp_pll());

    //probably better idea to set LFSR_one and LFSR_two and data_per_packet...
    //...in Parameters_rx object
    PacketDecoder * packet_decoder = new PacketDecoder(parameters_rx->get_preamble_size(),
                                                       parameters_rx->get_data_size(),
                                                       parameters_rx->get_checksum_size(),
                                                       parameters_rx->get_packet_size(),
                                                       parameters_rx->get_preamble_vector());

    std::cout << std::endl; //aesthetic purposes
    USRP_rx * usrp_rx = new USRP_rx(parameters_rx->get_sample_rate_tx(),
                                    parameters_rx->get_spb_tx(),
                                    parameters_rx->get_d_factor(),
                                    parameters_rx->get_spb());

    while(not stop_signal_called) {
        transmit(usrp_rx);
        receive(parameters_rx, usrp_rx, bpsk_rx, packet_decoder);
    }

    delete parameters_rx;
    delete bpsk_rx;
    delete packet_decoder;
    delete usrp_rx;

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

    size_t data_size = parameters_rx->get_data_size();
    size_t packet_size = parameters_rx->get_packet_size();
    size_t spb = parameters_rx->get_spb();
    double timeout_seconds = 15;

    //buffers to be used
    std::vector<int> pulses(2 * packet_size * 8);
    std::vector<uint8_t> bytes(2 * data_size);

    std::vector< std::complex<float> > buff(spb * 2 * packet_size * 8);
    size_t total_num_rx_samps = 0;
    boost::posix_time::ptime start_time = boost::posix_time::second_clock::local_time();
    usrp_rx->issue_start_streaming();
    //size_t received_size = 0;
    while(not stop_signal_called && packet_decoder->is_streaming_ended() == false) {
        size_t num_rx_samps = usrp_rx->receive(buff, total_num_rx_samps);
        if(num_rx_samps != spb) {
            std::cout << "Did not receive enough samples" << std::endl << std::endl;
            throw std::runtime_error("Did not receive enough samples");
        }
        total_num_rx_samps += num_rx_samps;
        if(total_num_rx_samps == spb * 2 * packet_size * 8) {
            total_num_rx_samps = 0;
            size_t pulses_size = bpsk_rx->receive(buff, pulses);
            size_t bytes_size = packet_decoder->decode(pulses, pulses_size, bytes);
            boost::posix_time::ptime current_time = boost::posix_time::second_clock::local_time();
            if(packet_decoder->is_streaming_started() == false && (current_time - start_time).total_seconds() >= timeout_seconds) {
                std::cout << "Timeout reached waiting for the file to start streaming. Please try again." << std::endl << std::endl;
                packet_decoder->reset();
                break;
            }
            outfile.write((char *) &bytes.front(), bytes_size * sizeof(char));
            outfile.flush();
        }
    }
    packet_decoder->reset();
    usrp_rx->issue_stop_streaming();
    outfile.close();
    output_filename.clear();
    //turn on the heartbeat
    usrp_rx->transmit(1);
    std::cout << "Please wait for " << 10 << " seconds." << std::endl << std::endl;
    boost::this_thread::sleep(boost::posix_time::seconds(10));
}

void transmit(USRP_rx * const usrp_rx) {
    std::cout << "Please enter the name of the file you would like to receive: ";
    std::cin >> receive_filename;
    std::cout << std::endl;
    output_filename = "bin/" + receive_filename;
    std::cout << "Your file will be saved in " + output_filename << std::endl;
    for(int i = 0; i < ((int) receive_filename.size() <= 8 ? (int) receive_filename.size() : 8); i++) {
        usrp_rx->transmit((uint8_t) receive_filename[i]);
    }
    receive_filename.clear();
    boost::this_thread::sleep(boost::posix_time::seconds(7));
    //turn off the heartbeat
    usrp_rx->transmit(0);
}
