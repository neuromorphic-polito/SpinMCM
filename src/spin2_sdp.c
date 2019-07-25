#include "_spin2_api.h"


// --- Local Prototypes ---

void _spin2_sdp_callback(uint mailbox, uint port);


// --- Local Variables ---

static callback_t _spinn2_sdp_callback[SPIN2_SDP_PORTS] = {NULL};


// --- Global function ---

void spin2_sdp_init(){
  uint cpsr;

  cpsr = cpu_int_disable();
  spin1_callback_on(SDP_PACKET_RX, _spin2_sdp_callback, 0);
  cpu_int_restore(cpsr);

  return;
}


// --- Local Function ---

void _spin2_sdp_callback(uint mailbox, uint port) {
  if (_spinn2_sdp_callback[port] != NULL) {
    _spinn2_sdp_callback[port](mailbox, port);
  }
  else{
    sdp_msg_t *msg = (sdp_msg_t *) mailbox;
    sark_msg_free(msg);
  }
  return;
}


bool spin2_sdp_callback_on(uint sdp_port, callback_t callback) {
  _spinn2_sdp_callback[sdp_port] = callback;
  return true;
}


bool spin2_sdp_callback_off(uint sdp_port) {
  _spinn2_sdp_callback[sdp_port] = NULL;
  return true;
}
