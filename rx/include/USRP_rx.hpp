#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <csignal>
#include <complex>
#include <stdexcept>
#include <vector>

#ifndef _Included_USRP_rx
#define _Included_USRP_rx

class USRP_rx {
 public:

    USRP_rx(double sample_rate_tx, size_t spb_tx, int d_factor, size_t spb);
    ~USRP_rx();
    size_t receive(std::vector< std::complex<float> > &buff_downsampled, size_t total_num_rx_samps);
    void issue_start_streaming();
    void issue_stop_streaming();

 private:

    double const sample_rate_tx; //sample_rate between the PC and Motherboard
    size_t spb_tx;
    int d_factor;
    size_t spb;
    std::vector< std::complex<float> > buff;

    std::string const &args; //single uhd device address args (example: "addr=192.168.10.2", "serial=901", "type=usrp2")
    std::string const &ref; //clock reference (internal, external, mimo, gpsdo)
    std::string const &cpufmt; //cpu sample format e.g. "fc32"
    std::string const &otw; //specify over the wire sample mode e.g. "sc16"

    uhd::usrp::multi_usrp::sptr usrp_rx; //usrp device
    uhd::stream_args_t stream_args; //stream arguments
    uhd::rx_streamer::sptr rx_stream; //streamer object
    std::vector<std::string> sensor_names; //sensor names to be used when checking for clock locking etc.
    uhd::rx_metadata_t md; //metadata for the streamer object
    uhd::stream_cmd_t stream_cmd;

};

#endif
