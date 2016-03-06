#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <csignal>
#include <math.h>
#include <stdexcept>

#ifndef _Included_USRP_tx
#define _Included_USRP_tx

class USRP_tx {
 public:

    USRP_tx(double sample_rate, double f_c, size_t spb);
    ~USRP_tx();
    int transmit(std::vector< std::complex<float> > buff, size_t N);
    void send_start_of_burst();
    void send_end_of_burst();

 private:

    const std::string args; //single uhd device address args (example: "addr=192.168.10.2", "serial=901", "type=usrp2")
    const std::string ref; //clock reference (internal, external, mimo, gpsdo)
    const std::string cpufmt; //cpu sample format e.g. "fc32"
    const std::string otw; //specify over the wire sample mode e.g. "sc16"
    const double sample_rate; //sample_rate between the PC and Motherboard
    size_t spb; //samples per buffer; has to satisfy -> spb = k * Fs/Fc for some integer k, spb must also be an integer itself

    uhd::usrp::multi_usrp::sptr usrp_tx; //usrp device
    uhd::stream_args_t stream_args; //stream arguments
    uhd::tx_streamer::sptr tx_stream; //streamer object
    std::vector<std::string> sensor_names; //sensor names to be used when checking for clock locking etc.
    uhd::tx_metadata_t md; //metadata for the streamer object

};

#endif
