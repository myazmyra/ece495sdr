#include "USRP_rx.hpp"

USRP_rx::USRP_rx(double sample_rate_tx,
                 size_t spb_tx,
                 int d_factor) :
                 sample_rate_tx(sample_rate_tx),
                 spb_tx(spb_tx),
                 d_factor(d_factor),
                 buff(spb_tx),
                 args("serial=F2A017"),
                 ref("internal"),
                 cpufmt("fc32"),
                 otw("sc16"),
                 stream_args(cpufmt, otw),
                 stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS) {

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    usrp_rx = uhd::usrp::multi_usrp::make(args);

    //Lock mboard clocks
    usrp_rx->set_clock_source(ref);

    std::cout << boost::format("Using Device: %s") % usrp_rx->get_pp_string() << std::endl;

    //set the sample rate
    if(sample_rate_tx <= 0.0) {
        std::cerr << "Please specify a valid sample rate" << std::endl;
        throw std::runtime_error("Please specify a valid sample rate");
    }

    std::cout << boost::format("Setting RX Rate: %f Msps...") % (sample_rate_tx / 1e6) << std::endl;
    usrp_rx->set_rx_rate(sample_rate_tx);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp_rx->get_rx_rate() / 1e6) << std::endl << std::endl;

    double setup_time = 2.0;
    boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup time

    //set timestamp to ZERO
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl << std::endl;
    usrp_rx->set_time_now(0.0);

    //Check Ref and LO Lock detect
    sensor_names = usrp_rx->get_tx_sensor_names(0);
    if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked") != sensor_names.end()) {
		//allow for some time, need more sleeping time for SBX
		boost::this_thread::sleep(boost::posix_time::seconds(setup_time));
        uhd::sensor_value_t lo_locked = usrp_rx->get_tx_sensor("lo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % lo_locked.to_pp_string() << std::endl << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }

    stream_cmd.num_samps = 0;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = uhd::time_spec_t();
    rx_stream = usrp_rx->get_rx_stream(stream_args);

}

USRP_rx::~USRP_rx() {
    //finished
    std::cout << "Destroying the USRP_rx object..." << std::endl << std::endl;
}

void USRP_rx::issue_start_streaming() {
    if(stream_cmd.stream_now == true) {
        std::cout << "USRP_rx::issue_start_streaming(): USRP_rx is already streaming" << std::endl << std::endl;
        throw std::runtime_error("USRP_rx is already streaming");
    }
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);
}

void USRP_rx::issue_stop_streaming() {
    if(stream_cmd.stream_now == false) {
        std::cout << "USRP_rx::issue_stop_streaming(): USRP_rx has already stopped streaming" << std::endl << std::endl;
        throw std::runtime_error("USRP_rx::issue_stop_streaming(): USRP_rx has already stopped streaming");
    }
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    stream_cmd.stream_now = false;
    rx_stream->issue_stream_cmd(stream_cmd);
}

size_t USRP_rx::receive(std::vector< std::complex<float> > &buff_downsampled) {
    size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0, false);

    if(md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
        std::cout << "Timeout while streaming" << std::endl;
        throw std::runtime_error("Timeout while streaming");
    }
    if(md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
        std::cout << "Overflow" << std::endl << std::endl;
    }
    if(md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
        std::string error = "Receive error: " + md.strerror();
        std::cout << error << std::endl << std::endl;
        throw std::runtime_error(error);
    }

    if(num_rx_samps != spb_tx) {
        std::cout << "Did not receive enough samples" << std::endl << std::endl;
        throw std::runtime_error("Did not receive enough samples");
    }

    for(int i = 0; i < (int) buff_downsampled.size(); i++) {
        buff_downsampled[i] = buff[i * d_factor];
    }

    return num_rx_samps / d_factor;
}
