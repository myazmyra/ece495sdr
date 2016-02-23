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
    virtual ~USRP_tx();
    int transmit(std::vector< std::complex<float> > buff, size_t N);
    void send_start_of_burst();
    void send_end_of_burst();

 private:

    const std::string args;// = "serial=901"; //single uhd device address args (example: "addr=192.168.10.2", "serial=901", "type=usrp2")
    const std::string ref;// = "internal"; //clock reference (internal, external, mimo, gpsdo)
    const std::string cpufmt;// = "fc32"; //cpu sample format
    const std::string otw;// = "sc16"; //specify over the wire sample mode
    const double sample_rate;// = 12.5e6; //sample_rate between the PC and Motherboard
    const size_t spb;// = 9075; //samples per buffer; has to satisfy -> spb = k * Fs/Fc * usrp->get_max_num_samps(); for some integer k; here k = 8

    uhd::usrp::multi_usrp::sptr usrp_tx; //usrp device
    uhd::stream_args_t stream_args; //stream arguments
    uhd::tx_streamer::sptr tx_stream; //streamer object
    std::vector<std::string> sensor_names; //sensor names to be used when checking for clock locking etc.
    uhd::tx_metadata_t md; //metadata for the streamer object

};

#endif
