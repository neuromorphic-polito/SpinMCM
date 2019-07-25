#include "_spin2_api.h"


void spin2_metrics_print() {
    #ifdef SPIN2_METRICS
    io_printf(IO_BUF, "[SPIN2-MCM] Unicast Packet:\n  SENT:     %6d\n  RECEIVED: %6d\n", 
        spin2_metrics[spin2_metrics_ucast_packet_send], 
        spin2_metrics[spin2_metrics_ucast_packet_recv]);
    io_printf(IO_BUF, "[SPIN2-MCM] Broadcast Packet:\n  SENT:     %6d\n  RECEIVED: %6d\n",
        spin2_metrics[spin2_metrics_bcast_packet_send], 
        spin2_metrics[spin2_metrics_bcast_packet_recv]);
    io_printf(IO_BUF, "[SPIN2-MCM] Unicast Fragment:\n  SENT:     %6d (%6d last)\n  RECEIVED: %6d (%6d last)\n",
        spin2_metrics[spin2_metrics_ucast_fragment_send],
        spin2_metrics[spin2_metrics_ucast_fragment_send_last], 
        spin2_metrics[spin2_metrics_ucast_fragment_recv],
        spin2_metrics[spin2_metrics_ucast_fragment_recv_last]);
    io_printf(IO_BUF, "[SPIN2-MCM] Broadcast Fragment:\n  SENT:     %6d (%6d last)\n  RECEIVED: %6d (%6d last)\n",
        spin2_metrics[spin2_metrics_bcast_fragment_send],
        spin2_metrics[spin2_metrics_bcast_fragment_send_last], 
        spin2_metrics[spin2_metrics_bcast_fragment_recv],
        spin2_metrics[spin2_metrics_bcast_fragment_recv_last]);
    io_printf(IO_BUF, "[SPIN2-MCM] Unicast Sync:\n  SENT:     %6d\n  RECEIVED: %6d\n",
        spin2_metrics[spin2_metrics_syn_send],
        spin2_metrics[spin2_metrics_syn_recv]);
    io_printf(IO_BUF, "[SPIN2-MCM] Unicast Ack:\n  SENT:     %6d\n  RECEIVED: %6d\n",
        spin2_metrics[spin2_metrics_ack_send],
        spin2_metrics[spin2_metrics_ack_recv]);
    io_printf(IO_BUF, "[SPIN2-MCM] Multilayer Sync:\n  SENT:     %6d\n  RECEIVED: %6d\n",
        spin2_metrics[spin2_metrics_mlsyn_send],
        spin2_metrics[spin2_metrics_mlsyn_recv]);
    #endif
    return;
}
