#include "Parameters_tx.hpp"

Parameters_tx::Parameters_tx() {

    sample_rate = 12.5e6;
    f_c = 4e6;
    spb = 8000;

    preamble_size = 2;
    data_size = 12;
    checksum_size = 2;
    packet_size = preamble_size + data_size + checksum_size;

    int m = (int) lround(log2((double) preamble_size * 8));
    std::vector<int> preamble_vector = build_lfsr(m);
    preamble_vector.push_back(-1);
    preamble_bytes = pulses_to_bytes(preamble_vector);

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
std::vector<uint8_t> Parameters_tx::get_preamble_bytes() const { return preamble_bytes; }

std::vector<int> Parameters_tx::build_lfsr(int m) const {
  //m should be greater than 1 and is a power of 2
  if(m < 2 || ((m - 1) & m) != 0) {
      std::cout << "Invalid LFSR parameter m" << std::endl << std::endl;
      throw new std::runtime_error("Invalid LFSR parameter m");
  }

  int n = (1 << m) - 1;

  std::vector<int> x(n);
  std::vector<int> buffer(m);
  buffer[0] = 1;

  for(int k = 0; k < n; k++) {
      x[k] = buffer[m - 1];
      buffer.insert(buffer.begin() + 1, buffer.begin(), buffer.end() - 1);
      buffer[0] = buffer[0] ^ x[k];
  }

  for(int k = 0; k < n; k++) {
      x[k] = 2 * x[k] - 1;
  }

  return x;
}

std::vector<uint8_t> Parameters_tx::pulses_to_bytes(std::vector<int> pulses) const {
  std::vector<uint8_t> bytes;
  for(int i = 0; i < (int) pulses.size(); i += 8) { //iterate through bytes
      uint8_t byte = 0;
      uint8_t mask = 1;
      for(int j = i; j < i + 8; j++) { //iterate through bits
          byte |= ((pulses[j] > 0) ? mask : 0);
          mask <<= 1;
      }
      bytes.push_back(byte);
  }
  return bytes;
}
