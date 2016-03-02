#include "Parameters_rx.hpp"
#include "BPSK_rx.hpp"

#include <iostream>
#include <cstdint>
#include <fstream>
#include <thread>
#include <mutex>

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

    size_t spb = parameters_rx->get_spb();
    //you will need to create new buffer each time you receive something
    std::vector< std::complex<float> >* buff_ptr;

    while(true) {
        buff_ptr = new std::vector< std::complex< float > >(spb);
        infile.read((char*) &buff_ptr, spb * sizeof(std::complex<float>));
        if(infile.eof()) break;

        /*
        optimization needed in process(), probably pass reference to the buffer
        *this optimization needs to be done in so many other places*
        even in the TX side
        */
        //also needed to implement how many errors were detected
        //and return number of samples recovered
        std::vector<uint8_t> data = bpsk_rx->process(buff_ptr, spb);
        for(int i = 0; i < (int) data.size(); i++) {
            outfile.write((char*) &data[i], sizeof(char));
        }
    }

    std::cout << "Done receiving from input file: " << inFile << std::endl << std::endl;

    infile.close();
    outfile.close();
}

void printHelp() {
    std::cout << "Usage: " << std::endl << std::endl;
    std::cout << "./main_rx --mode usrp [infile_path]" << std::endl << std::endl;
    std::cout << "  or" << std::endl << std::endl;
    std::cout << "./main_tx --mode local [infile_path] [outfile_path]" << std::endl << std::endl;
}
