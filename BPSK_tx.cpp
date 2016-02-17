#include "USRP_tx.h"
#include "BPSK_tx.h"

int main(int argc, char *argv[]) {
    USRP_tx* controller = new USRP_tx();
    std::vector< std::complex<float> > buff(controller->get_spb());
    controller->transmit(buff);
    return EXIT_SUCCESS;
}
