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
void transmit(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx, std::vector<uint8_t> bits);
void receive(Parameters_tx* parameters_tx, USRP_tx* usrp_tx, BPSK_tx* bpsk_tx);
std::vector<uint8_t> readFile(std::string fileName);
std::vector<uint8_t> formPackets(char* bytes, int size);

int main(int argc, char** argv) {
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

    const std::string fileName = "testfile.txt";
    std::vector<uint8_t> bits = readFile(fileName);

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

std::vector<uint8_t> readFile(std::string fileName) {
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
    std::ifstream::pos_type size = 0;
    char* bytes = NULL;
    if(file.is_open()) {
        std::cout << "Successfully opened file: " << fileName << std::endl << std::endl;
        size =  file.tellg();
        bytes = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(bytes, size);
        file.close();
    } else {
        std::cout << "Unable to open file: " << fileName << std::endl << std::endl;
        throw new std::runtime_error("Unable to open input file" + fileName);
    }

    std::vector<uint8_t> packets = formPackets(bytes, (int) size);

    std::vector<uint8_t> bits;
    for(int i = 0; i < (int) packets.size(); i++) {
        uint8_t byte = packets[i];
        for(int j = 0; j < 8; j++) {
            bits.push_back(byte & 1);
            byte >>= 1;
        }
    }
    return bits;
}

std::vector<uint8_t> formPackets(char* data, int size) {
    //a packet will consist of:
    //2 bytes of preamble + 12 bytes of data + 2 bytes of checksum = 16 bytes
    int data_per_packet = 12;
    int num_packets = size / data_per_packet;

    const uint8_t LFSR_one = 30; //first byte of 15 bit LFSR
    const uint8_t LFSR_two = 178; //second byte of 15 bit LFSR padded with 0

    std::vector<uint8_t> packets;
    for(int i = 0; i < num_packets; i++) {
        packets.push_back(LFSR_one);
        packets.push_back(LFSR_two);
        //first part
        uint8_t checksum1 = 0;
        for(int j = 0; j < data_per_packet / 2; j++) {
            uint8_t byte = (uint8_t) data[data_per_packet * i + j];
            checksum1 ^= byte;
            packets.push_back(byte);
        }
        //second part
        uint8_t checksum2 = 0;
        for(int j = data_per_packet / 2; j < data_per_packet; j++) {
            uint8_t byte = (uint8_t) data[data_per_packet * i + j];
            checksum2 ^= byte;
            packets.push_back(byte);
        }
        packets.push_back(checksum1);
        packets.push_back(checksum2);
    }

    int remaining_bytes = size % data_per_packet;

    //if added everything, return
    if(remaining_bytes == 0) return packets;

    //pad anything leftover
    packets.push_back(LFSR_one);
    packets.push_back(LFSR_two);

    //first part
    uint8_t checksum1 = 0;
    for(int i = num_packets; i < num_packets + remaining_bytes / 2; i++) {
        uint8_t byte = (uint8_t) data[i];
        checksum1 ^= byte;
        packets.push_back(byte);
    }
    //second part
    uint8_t checksum2 = 0;
    for(int i = num_packets + remaining_bytes / 2; i < num_packets + remaining_bytes; i++) {
        uint8_t byte = (uint8_t) data[i];
        checksum2 ^= byte;
        packets.push_back(byte);
    }
    //pad zeros
    int num_padded_zeros = data_per_packet - remaining_bytes;
    for(int i = 0; i < num_padded_zeros; i++) {
        packets.push_back(0);
    }
    packets.push_back(checksum1);
    packets.push_back(checksum2);
    return packets;
}
