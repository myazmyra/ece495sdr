#include "Parameters_tx.hpp"

Parameters_tx::Parameters_tx() {

    sample_rate = 12.5e6;
    f_c = 4e6;
    spb = 8000;

    preamble_size = 2;
    data_size = 12;
    checksum_size = 2;
    packet_size = preamble_size + data_size + checksum_size;

    //TODO: validate parameters to satisfy design logic

}

Parameters_tx::~Parameters_tx() {
    std::cout << "Destroying the Parameters_tx object..." << std::endl << std::endl;
}

double Parameters_tx::get_sample_rate() const {return sample_rate; }
double Parameters_tx::get_f_c() const { return f_c; }
size_t Parameters_tx::get_spb() const { return spb; }
double Parameters_tx::get_bit_rate() const { return bit_rate; }
size_t Parameters_tx::get_preamble_size() const { return preamble_size; }
size_t Parameters_tx::get_data_size() const { return data_size; }
size_t Parameters_tx::get_checksum_size() const { return checksum_size; }
size_t Parameters_tx::get_packet_size() const { return packet_size; }
