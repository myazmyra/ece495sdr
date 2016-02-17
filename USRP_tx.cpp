#include "USRP_tx.h"

USRP_tx::USRP_tx(double sample_rate, double f_c, size_t spb) : args("serial=901"), ref("internal"), cpufmt("fc32"), otw("sc16"),
		     											sample_rate(sample_rate), spb(spb) { //initialize the constants

	//give thread priority to this thread
	uhd::set_thread_priority_safe();

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    //Lock mboard clocks
    usrp->set_clock_source(ref);

    //set the sample_rate
    std::cout << std::endl << boost::format("Setting TX Rate: %f Msps...") % (sample_rate/1e6) << std::endl;
    usrp->set_tx_rate(sample_rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    //allow for some setup time
    boost::this_thread::sleep(boost::posix_time::seconds(1));

    //create a transmit streamer
    uhd::stream_args_t stream_args(cpufmt, otw); //stream arguments
    tx_stream = usrp->get_tx_stream(stream_args);



    //set timestamp to ZERO
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl << std::endl;
    usrp->set_time_now(0.0);

	//initialize metadata
    md.start_of_burst = false;
    md.end_of_burst = false;

    //Check Ref and LO Lock detect
    sensor_names = usrp->get_tx_sensor_names(0);
    if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked") != sensor_names.end()) {
        uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked",0);
        std::cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string() << std::endl << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }
}

USRP_tx::~USRP_tx() {
    //finished
    std::cout << "Destroying the USRP_tx object..." << std::endl << std::endl;
}

int USRP_tx::transmit(std::vector< std::complex<float> > buff) {
	if(md.end_of_burst == true) {
		std::cout << "runtime_error: invalid attempt to transmit after md.end_of_burst is set to true" << std::endl;
		throw new std::runtime_error("invalid attempt to transmit after md.end_of_burst is set to true");
	} else if(md.start_of_burst == true) {
		std::cout << "runtime_error: invalid attempt to transmit start burst" << std::endl;
		throw new std::runtime_error("invalid attempt to transmit start burst");
	}
	//send the entire buffer
	tx_stream->send(&buff.front(), buff.size(), md);
  	return 1;
}

void USRP_tx::send_start_of_burst() {
	if(md.start_of_burst == true) {
		std::cout << "runtime_error: md.start_of_burst is already set to true" << std::endl;
		throw new std::runtime_error("md.start_of_burst is already set to true");
	}
	md.start_of_burst = true;
	std::vector< std::complex<float> > buff(spb);
	tx_stream->send(&buff.front(), buff.size(), md);
	md.start_of_burst = false;
}

void USRP_tx::send_end_of_burst() {
	if(md.end_of_burst == true) {
		std::cout << "runtime_error: md.end_of_burst is already set to true" << std::endl;
		throw new std::runtime_error("md.end_of_burst is already set to true");
	}
	//last time transmitting
	//send a mini EOB packet
	md.end_of_burst = true;
	tx_stream->send("", 0, md);
}
