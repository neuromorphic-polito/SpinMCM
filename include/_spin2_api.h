#ifndef __SPIN2_API_PRIVATE_H__
#define __SPIN2_API_PRIVATE_H__

#include "spin2_api.h"

#ifdef SPIN2_DEBUG
#define debug_printf(...) \
  do { io_printf(IO_BUF, __VA_ARGS__); } while (0)
#else
#define debug_printf(...) \
  do { } while (0)
#endif

#define error_printf(...) \
  do { io_printf(IO_BUF, __VA_ARGS__); } while (0)

#define RECONSTRUCTION_BUFFERS_DTCM true 
#define RECONSTRUCTION_BUFFERS 4  // Can be reduced
#define RECONSTRUCTION_BUFFER_SIZE 272
#define SPIN2_SDP_PORTS 8


#ifdef SPIN2_METRICS
uint32_t spin2_metrics[18];
#define spin2_metrics_init() for(int i=0; i<18; i++) {spin2_metrics[i]=0;}
#define spin2_metrics_update(type) spin2_metrics[type]++
#else
#define spin2_metrics_init() do { } while (0)
#define spin2_metrics_update(spin2_metrics_t) do { } while (0)
#endif

typedef enum {
    // Packet unicast
    spin2_metrics_ucast_packet_send,
    spin2_metrics_ucast_packet_recv,
    // Packet broadcast
    spin2_metrics_bcast_packet_send,
    spin2_metrics_bcast_packet_recv,
    // Fragment unicast
    spin2_metrics_ucast_fragment_send,
    spin2_metrics_ucast_fragment_send_last,
    spin2_metrics_ucast_fragment_recv,
    spin2_metrics_ucast_fragment_recv_last,
    // Fragment broadcast
    spin2_metrics_bcast_fragment_send,
    spin2_metrics_bcast_fragment_send_last,
    spin2_metrics_bcast_fragment_recv,
    spin2_metrics_bcast_fragment_recv_last,
    // Sync
    spin2_metrics_syn_send,
    spin2_metrics_syn_recv,
    // Ack
    spin2_metrics_ack_send,
    spin2_metrics_ack_recv,
    // Multilayer Sync
    spin2_metrics_mlsyn_send,
    spin2_metrics_mlsyn_recv
} spin2_metrics_t;


// --- Multicast Communication Middleware - Private Types ---
typedef enum {
  MC_P2P=0,  // 0b00
  MC_BC=1,   // 0b01
  MC_SYNC=2, // 0b10
  MC_MC=3    // 0b11 [Not supported]
} mc_packet_t;


// --- Multicast Communication Middleware - Private Functions ---
void mc_init();

bool mc_send(
  uint8_t *data, 
  uint8_t length,
  uint8_t last, 
  uint8_t counter, 
  bool extended_payload,
  uint32_t packet_key, 
  spin2_core_t *core_key
);

void mc_recv(uint key, uint payload);
void mc_recv_p2p(uint key, uint payload);
void mc_recv_p2p_syn(uint key, uint payload);
void mc_recv_p2p_ack(uint key, uint payload);
void mc_recv_bc(uint key, uint payload);
void mc_recv_sync(uint key, uint payload);

void mc_pass(uint channel, uint arg1);

void mc_wfs();
void mc_wfs_sync1();
void mc_wfs_sync2();
void mc_wfs_sync3();


// --- Hash Table - Private Functions ---
// TODO Implement alternatives to Jenkins Full Avalanche? 
//      FNV1A, OAT, MURMUR3, TOMASWANG

#endif //__SPIN2_API_PRIVATE_H__
