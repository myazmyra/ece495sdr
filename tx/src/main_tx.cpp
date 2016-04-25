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

std::string input_filename = "";

std::mutex mtx;
bool idle = true;
std::vector<uint8_t> bits;
boost::posix_time::ptime start_time;

/***********************************************************************
 * Function Declarations
 **********************************************************************/
std::vector<uint8_t> read_file(PacketEncoder * const packet_encoder);
void transmit(Parameters_tx * const parameters_tx, USRP_tx * const usrp_tx, BPSK_tx * const bpsk_tx);
void receive(Parameters_tx * const parameters_tx, PacketEncoder * const packet_encoder, USRP_tx * const usrp_tx, BPSK_tx * const bpsk_tx);

int main(int argc, char** argv) {

    //initialize Parameters
    Parameters_tx* parameters_tx = new Parameters_tx();

    //initialize PacketEncoder
    PacketEncoder* packet_encoder = new PacketEncoder(parameters_tx->get_preamble_size(),
                                                      parameters_tx->get_data_size(),
                                                      parameters_tx->get_checksum_size(),
                                                      parameters_tx->get_packet_size(),
                                                      parameters_tx->get_preamble_bytes());

    //initialize BPSK_tx
    BPSK_tx* bpsk_tx = new BPSK_tx(parameters_tx->get_sample_rate(),
                                   parameters_tx->get_f_c(),
                                   parameters_tx->get_spb());

    //give thread priority to this thread
	uhd::set_thread_priority_safe();
    std::cout << std::endl;

    //initialize USRP_tx
    USRP_tx* usrp_tx = new USRP_tx(parameters_tx->get_sample_rate(),
                                   parameters_tx->get_f_c(),
                                   parameters_tx->get_spb());

    //makes it possible to stop the program by pressing Ctrl+C
    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

    //create threads and start them
    //transmit data
    std::thread transmit_thread(transmit, parameters_tx, usrp_tx, bpsk_tx);
    //receive data
    std::thread receive_thread(receive, parameters_tx, packet_encoder, usrp_tx, bpsk_tx);

    receive_thread.join();
    transmit_thread.join();

    delete usrp_tx;
    delete bpsk_tx;
    delete packet_encoder;
    delete parameters_tx;

    std::cout << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

void transmit(Parameters_tx * const parameters_tx, USRP_tx * const usrp_tx, BPSK_tx * const bpsk_tx) {

    std::vector< std::complex<float> > positive = bpsk_tx->modulate(1);
    std::vector< std::complex<float> > negative = bpsk_tx->modulate(0);
    size_t spb = parameters_tx->get_spb();
    int i = 0;

    //start burst
    mtx.lock();
    usrp_tx->send_start_of_burst();
    mtx.unlock();
    while(not stop_signal_called) {
        mtx.lock();
        if(idle == true) {
            usrp_tx->transmit(positive, spb);
        } else {
            if(i == 0) {
                std::cout << "Starting to stream" << std::endl;
            }
            usrp_tx->transmit(bits[i] ? positive : negative, spb);
            i++;
            if(i == (int) bits.size()) {
                std::cout << "Done sending the file: " << input_filename << std::endl << std::endl;
                i = 0;
                idle = true;
                bits.clear();
                input_filename.clear();
            }
        }
        mtx.unlock();
    }

    //stop transmission
    mtx.lock();
    usrp_tx->send_end_of_burst();
    mtx.unlock();
}

void receive(Parameters_tx * const parameters_tx, PacketEncoder * const packet_encoder, USRP_tx * const usrp_tx, BPSK_tx * const bpsk_tx) {
    while(not stop_signal_called) {
        //receive some stuff from the microcontroller
        if(idle == false) {
            continue;
        }
        bool file_opened = false;
        mtx.lock();
        uint8_t data = usrp_tx->receive();
        mtx.unlock();
        if(data != 0) {
            input_filename.push_back((char) data);
            for(int i = 0; i < 7; i++) {
                mtx.lock();
                input_filename.push_back((char) usrp_tx->receive());
                mtx.unlock();
            }
            std::cout << "Input filename received: " << input_filename << std::endl;
            try {
                bits = read_file(packet_encoder);
                file_opened = true;
            } catch(std::ifstream::failure e) {
                std::cout << "Unable to open input file: " + input_filename << std::endl;
                input_filename.clear();
                bits.clear();
            }
        }
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        if(file_opened) {
            boost::this_thread::sleep(boost::posix_time::seconds(5));
            idle = false;
        }
    }
}

std::vector<uint8_t> read_file(PacketEncoder * const packet_encoder) {
    std::ifstream infile(input_filename, std::ios::ate | std::ios::binary);
    std::ifstream::pos_type file_size = 0;
    char* bytes = NULL;

    if(infile.is_open() == false) {
        throw std::ifstream::failure("Unable to open input file: " + input_filename);
    }

    std::cout << "Successfully opened input file: " << input_filename << std::endl << std::endl;
    file_size = infile.tellg();
    bytes = new char[file_size];
    infile.seekg(0, std::ios::beg);
    infile.read(bytes, file_size);
    infile.close();

    //this will automatically form packets with preamble and checksum and then
    //create bit sequences from bytes
    std::vector<uint8_t> bits = packet_encoder->form_packets(bytes, (size_t) file_size);
    delete[] bytes;
    return bits;
}
