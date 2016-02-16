#include "USRP_tx.h"

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

USRP_tx::USRP_tx() : args("serial=901"), ref("internal"), cpufmt("fc32"), otw("sc16"),
										 rate(12.5e6), spb(9075), f_c(4e6) { //initialize the constants

    //give thread priority to usrp
    uhd::set_thread_priority_safe();

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    //Lock mboard clocks
    usrp->set_clock_source(ref);

    //set the sample rate
    std::cout << std::endl << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    //allow for some setup time
    boost::this_thread::sleep(boost::posix_time::seconds(1));

    //create a transmit streamer
    uhd::stream_args_t stream_args(cpufmt, otw); //stream arguments
    tx_stream = usrp->get_tx_stream(stream_args);
    //first time transmitting
    md.start_of_burst = true;
    md.end_of_burst = false;

    //set timestamp to ZERO
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl << std::endl;
    usrp->set_time_now(0.0);

    //Check Ref and LO Lock detect
    sensor_names = usrp->get_tx_sensor_names(0);
    if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked") != sensor_names.end()) {
        uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked",0);
        std::cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string() << std::endl << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }

    //makes it possible to stop the program by pressing Ctrl+C
    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

}

USRP_tx::~USRP_tx() {
    //last time transmitting
    //send a mini EOB packet
    if(md.end_of_burst == false) {
			md.end_of_burst = true;
			tx_stream->send("", 0, md);
    }

    //finished
    std::cout << "Done!" << std::endl << std::endl;
}

int USRP_tx::transmit(std::vector< std::complex<float> > buff) {
    if(not stop_signal_called) {
			//send the entire buffer
			tx_stream->send(&buff.front(), buff.size(), md);
			md.start_of_burst = false;
    }
    return 1;
}

size_t USRP_tx::get_spb() {
    return spb;
}

int UHD_SAFE_MAIN(int argc, char *argv[]) {
    USRP_tx* controller = new USRP_tx();
    std::vector< std::complex<float> > buff(controller->get_spb());
    controller->transmit(buff);
    return EXIT_SUCCESS;
}
