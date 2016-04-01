#include "USRP_rx.hpp"

USRP_rx::USRP_rx(double sample_rate,
                 size_t spb) :
                 sample_rate(sample_rate),
                 args(""),
                 ref("internal"),
                 cpufmt("fc32"),
                 otw("sc16"),
                 stream_args(cpufmt, otw),
                 stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS) {

    rx_stream = usrp_rx->get_rx_stream(stream_args);
    stream_cmd.num_samps = 0;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = uhd::time_spec_t();
    rx_stream->issue_stream_cmd(stream_cmd);

}

void USRP_rx::issue_start_streaming() {
    if(stream_cmd.stream_now == true) {
        std::cout << "USRP_rx::issue_start_streaming(): USRP_rx is already streaming" << std::endl << std::endl;
        throw std::runtime_error("USRP_rx is already streaming");
    }
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
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

size_t USRP_rx::receive(std::vector< std::complex<float> > &buff) {
    size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0, false);

    /*if(md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
        std::cout << "Timeout while streaming" << std::endl;
        throw std::runtime_error("Timeout while streaming");
    }
    if(md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
        std::cout << "Overflow" << std::endl << std::endl;
    }
    if(md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
        std::string error = "Receiver error: " << md.strerror() << std::endl << std::endl;
        throw std::runtime_error(error);
    }
    */
    
    return num_rx_samps;
}
