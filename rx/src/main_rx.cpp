#include "Parameters_rx.hpp"
#include "BPSK_rx.hpp"

#include <iostream>
#include <cstdint>
#include <fstream>
#include <thread>
#include <mutex>

//NEED TO ADD PACKET DECODER CLASS

/***********************************************************************
 * Miscellaneous
 **********************************************************************/
std::mutex mtx;

/***********************************************************************
 * Function Declarations
 **********************************************************************/
void receiveFromFile(Parameters_rx* parameters_rx, BPSK_rx* bpsk_rx, std::string inFile, std::string outFile);
void printHelp();

int main(int argc, char** argv) {

    std::string mode;
    std::string inFile;
    std::string outFile;

    //input validation
    if(argc < 4) {
        printHelp();
        return EXIT_FAILURE;
    } else {
        if(std::string(argv[1]) != std::string("--mode")) {
            printHelp();
            return EXIT_FAILURE;
        }
        mode = std::string(argv[2]);
        if(mode != std::string("usrp") && mode != std::string("local")) {
            printHelp();
            return EXIT_FAILURE;
        }
        inFile = std::string(argv[3]);
        if(mode == std::string("local") && argc < 5) {
            printHelp();
            return EXIT_FAILURE;
        }
        if(mode == std::string("local")) {
            outFile = std::string(argv[4]);
        }
    }

    Parameters_rx* parameters_rx = new Parameters_rx();

    //obtain useful values
    const double sample_rate = parameters_rx->get_sample_rate();
    const double f_c = parameters_rx->get_f_c();
    size_t spb = parameters_rx->get_spb();
    double bit_rate = parameters_rx->get_bit_rate();

    BPSK_rx* bpsk_rx = new BPSK_rx(sample_rate, f_c, bit_rate, spb);

    //maybe use enums later
    if(mode == std::string("local")) {
        receiveFromFile(parameters_rx, bpsk_rx, inFile, outFile);
    }

    std::cout << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

void receiveFromFile(Parameters_rx* parameters_rx, BPSK_rx* bpsk_rx, std::string inFile, std::string outFile) {
    std::ifstream infile(inFile, std::ifstream::binary);
    std::ofstream outfile(outFile, std::ofstream::binary);

    if(not infile.is_open()) {
        std::cout << "Unable to open input file: " << inFile << std::endl << std::endl;
        throw new std::runtime_error("Unable to open input file: " + inFile);
    } else if(not outfile.is_open()) {
        std::cout << "Unable to open output file: " << outFile << std::endl << std::endl;
        throw new std::runtime_error("Unable to open output file: " + outFile);
    }

    std::cout << "Successfully opened input file: " << inFile << std::endl << std::endl;
    std::cout << "Successfully opened ouput file: " << outFile << std::endl << std::endl;

    size_t spb = parameters_rx->get_spb();
    //you will need to create new buffer each time you receive something
    std::vector< std::complex<float> >* buff_ptr;

    std::vector<uint8_t> bits;

    /***********************************************************************
     * START: This portion of code is for demo, it needs to be rewritten
     **********************************************************************/
    while(true) {
        buff_ptr = new std::vector< std::complex<float> >(spb);
        infile.read((char*) &(buff_ptr->front()), spb * sizeof(std::complex<float>));
        if(infile.eof()) break;
        /*
        optimization needed in process(), probably pass reference to the buffer
        *this optimization needs to be done in so many other places*
        even in the TX side
        */
        //also needed to implement how many errors were detected
        //and return number of samples recovered
        /*
        std::vector<uint8_t> data = bpsk_rx->process(buff_ptr, spb);
        for(int i = 0; i < (int) 30; i++) {
            outfile.write((char*) &data[i], sizeof(char));
        }
        const char* nl = "\n";
        outfile.write((char*) nl, sizeof(char));
        */
        bits.push_back(real(buff_ptr->at(0)) > 0 ? 1 : 0);
        delete buff_ptr;
    }

    int preamble_size = 2;
    int bytes_per_packet = 12;
    int checksum_size = 2;
    int packet_size = preamble_size + bytes_per_packet + checksum_size;

    for(int i = 0; i < (int) bits.size(); i += packet_size * 8) {//iterate through packets
        for(int j = i + preamble_size * 8; j < i + (preamble_size + bytes_per_packet) * 8; j += 8) { //iterate through data bytes
            char c = 0;
            uint8_t mask = 1;
            for(int k = j; k < j + 8; k++) {//iterate through bits
                c |= bits[k] ? mask : 0;
                mask <<= 1;
            }
            outfile.write((char*) &c, sizeof(char));
        }
    }
    /***********************************************************************
     * END: This portion of code is for demo, it needs to be rewritten
     **********************************************************************/

    std::cout << "Done receiving from input file: " << inFile << std::endl << std::endl;

    infile.close();
    outfile.close();
}

void printHelp() {
    std::cout << "Usage: " << std::endl << std::endl;
    std::cout << "./main_rx --mode usrp [infile_path]" << std::endl << std::endl;
    std::cout << "  or" << std::endl << std::endl;
    std::cout << "./main_rx --mode local [infile_path] [outfile_path]" << std::endl << std::endl;
}
