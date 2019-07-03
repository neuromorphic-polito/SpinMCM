// ==========================================================================
//                                  SpinMCM
// ==========================================================================
// This file is part of SpinMCM.
//
// SpinMCM is Free Software: you can redistribute it and/or modify it
// under the terms found in the LICENSE[.md|.rst] file distributed
// together with this file.
//
// SpinMCM is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// ==========================================================================
// Autor: Francesco Barchi <francesco.barchi@polito.it>
// ==========================================================================
// spin2_sdp.c: SDP callback functions for SpinMCM
// ==========================================================================

#include "_spin2_api.h"

// --- Local Variables ---
static callback_t _spinn2_sdp_callback[SPIN2_SDP_PORTS] = {NULL};


// --- Functions ---
void spin2_sdp_callback(uint mailbox, uint port) {
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
