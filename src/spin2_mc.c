#include "_spin2_api.h"


// --- Macro ---
#define MASK_SHIFT_R(v, m, s) (((v) & (m)) >> (s))
#define MASK_SHIFT_L(v, m, s) (((v) & (m)) << (s))

#define BIT_TOGGLE(v, b) ((v) ^= 1 << (b))
#define BIT_SET(v, b, x) ((v) ^= (-(x) ^ (v)) & (1 << (b)))
#define BIT_CHECK(v, b)  ((((v) >> (b)) & 1) == 1)


// --- Defines - Multilevel Sync ---
#define SYNC_LEVELS 4
#define SYNC_LVL1 0x01
#define SYNC_LVL2 0x02
#define SYNC_LVL3 0x04
#define SYNC_FREE 0x08


// --- Defines - MCM v.5b (MCM 19w17) ---
#define MC_X_SH 27
#define MC_Y_SH 24
#define MC_P_SH 20

#define W_MC_X_MSK 0x07
#define W_MC_Y_MSK 0x07
#define W_MC_P_MSK 0x0F

#define MC_LATENCY 5  // unit: us, default: 5


// --- Pointer Types ---
typedef uint32_t * uint32_t_p;
typedef uint16_t * uint16_t_p;
typedef uint8_t * uint8_t_p;

// --- Global Variables ---
static spin2_mc_callback_t callback = NULL;

spin2_core_t core;

static bool sync_lock;
bool sync_check[SYNC_LEVELS];
uint8_t sync_count[SYNC_LEVELS];
uint32_t sync_max[SYNC_LEVELS];

uint8_t **reconstruction_buffers;
uint16_t reconstruction_buffers_length[RECONSTRUCTION_BUFFERS];
uint16_t reconstruction_buffers_keys[RECONSTRUCTION_BUFFERS];

uint32_t template_ucast;
uint32_t template_bcast;
uint32_t template_multilevel_sync;

// sync core
bool lock_sync[8][8][16];
bool lock_ack;

// --- Public Functions ---

/**
 * @brief Initialize Spin1 callback and Spin2 MC support
 *
 */
void spin2_mc_init(){
    // PRIO 1
    uint cpsr;

    cpsr = cpu_int_disable();
    mc_init();
    cpu_int_restore(cpsr);

    return;
}


/**
 * @brief TODO
 *
 */
void spin2_mc_callback_register(spin2_mc_callback_t c){
    // PRIO 1
    uint cpsr;

    cpsr = cpu_int_disable();
    callback = c;
    cpu_int_restore(cpsr);

    return;
}


/**
 * @brief TODO
 *
 */
void spin2_mc_callback_reset(){
    uint cpsr;
    
    cpsr = cpu_int_disable();
    callback = NULL;
    cpu_int_restore(cpsr);

    return;
}


bool spin2_mc_send(
    uint8_t *data_head, uint16_t length_head,
    uint8_t *data_body, uint16_t length_body,
    uint8_t *data_tail, uint16_t length_tail,
    spin2_mc_channel channel, spin2_core_t *destination){

  int i;
  bool last, finish, extendend_payload;
  
  uint8_t counter;
  uint8_t patch[6];
  uint8_t fragment_length_max, a, b, c;
  uint16_t patch_length;
  uint16_t global_length;

  uint32_t key;
  spin2_core_t *core_key;

  global_length = length_head + length_body + length_tail;

  debug_printf("[SPIN2-MC] Send:\n");
  if (channel == SPIN2_MC_P2P){
    debug_printf("           SPIN2_MC_P2P\n");
    extendend_payload = false;
    fragment_length_max = 4;
    key = template_ucast;
    core_key = destination;
  }
  else if (channel == SPIN2_MC_BD) {
    debug_printf("           SPIN2_MC_BC\n");
    extendend_payload = true;
    fragment_length_max = 6;
    key = template_bcast;
    core_key = &core;
  }
  else{
    error_printf("[SPIN2-MC] Invalid Channel\n");
    rt_error(RTE_ABORT);
  }
  debug_printf("           Extended: %d\n", extendend_payload);
  debug_printf("           Fragment: %d\n", fragment_length_max);
  debug_printf("           Key: %08x\n", key);
  debug_printf("           Core Key: %d %d %d\n", core_key->x, core_key->y, core_key->p);

  last = false;
  counter = 0;

  // Send Head
  finish = length_head > 0 ? false : true;
  while(!finish) {
    if (length_head <= fragment_length_max) {  // Last packet for current segment
      a = length_head;                         // Space taken for current segment
      b = fragment_length_max - a;             // Space free
      c = length_body < b ? length_body : b;   // Space taken for next segment

      for (i=0; i<a; ++i) {                    // Load current segment
        patch[i] = data_head[i];
      }
      data_head += a;
      length_head -= a;

      for (i=0; i<c; ++i){                     // Load next segment
        patch[i+a] = data_body[i];
      }
      data_body += c;
      length_body -= c;

      patch_length = a+c;                      // Total packet length
      global_length -= patch_length;           // Update gloabal length
      if (global_length <= 0) {                // Last fragment if no more data
        last = true;
      }

      mc_send(
        &patch[0], 
        patch_length,
        last, 
        counter, 
        extendend_payload,
        key, 
        core_key);
      
      finish = true;
    }
    else {
      global_length -= fragment_length_max;
      
      mc_send(
        data_head, 
        fragment_length_max,
        last, 
        counter, 
        extendend_payload,
        key, 
        core_key);

      data_head += fragment_length_max;
      length_head -= fragment_length_max;
    }
    counter +=1;
  }

  // Send Body
  finish = length_body > 0 ? false : true;
  while(!finish) {
    if (length_body <= fragment_length_max){
      a = length_body;
      b = fragment_length_max - a;
      c = length_tail < b ? length_tail : b;

      for (i=0; i<a; ++i){
        patch[i] = data_body[i];
      }
      data_body += a;
      length_body -= a;

      for (i=0; i<c; ++i){
        patch[i+a] = data_tail[i];
      }
      data_tail += c;
      length_tail -= c;

      patch_length = a+c;
      global_length -= patch_length;
      if (global_length <= 0) {
        last = true;
      }

      mc_send(
        &patch[0], 
        patch_length,
        last, 
        counter, 
        extendend_payload,
        key, 
        core_key);
     
      finish = true;
    }
    else {
      global_length -= fragment_length_max;

      mc_send(
        data_body, 
        fragment_length_max,
        last, 
        counter, 
        extendend_payload,
        key, 
        core_key);

      data_body += fragment_length_max;
      length_body -= fragment_length_max;
    }
    counter +=1;
  }

  // Send Tail
  finish = length_tail>0 ? false : true;
  while(!finish){
    if (length_tail <= fragment_length_max){

      global_length -= length_tail;
      if (global_length<=0){
        last = true;
      }

      mc_send(
        data_tail, 
        length_tail,
        last, 
        counter, 
        extendend_payload,
        key, 
        core_key);

      finish = true;
    }
    else {
      global_length -= fragment_length_max;
      
      mc_send(
        data_tail, 
        fragment_length_max,
        last, 
        counter, 
        extendend_payload,
        key, 
        core_key);

      data_tail += fragment_length_max;
      length_tail -= fragment_length_max;
    }

    counter +=1;
  }

  // Metrics
  if (channel==SPIN2_MC_P2P) {
    spin2_metrics_update(spin2_metrics_ucast_packet_send);
  } else if(channel==SPIN2_MC_BD) {
    spin2_metrics_update(spin2_metrics_bcast_packet_send);
  }

  return true;
}


/**
 * @brief TODO
 * @param level
 * @param value
 */
void spin2_mc_sync_max_set(uint32_t level, uint32_t value){
  sync_max[level] = value;
  debug_printf("[SPIN2-MC] Set sync%02d max: %d\n", level, sync_max[level]);
  return;
}


/**
 * @brief TODO
 */
void spin2_mc_wfs() {
  if (!sync_check[1] && !sync_check[2] && !sync_check[3]) {
    mc_wfs();
  } else if (sync_check[1] && !sync_check[2] && !sync_check[3]) {
    mc_wfs_sync1();
  } else if (sync_check[1] && sync_check[2] && !sync_check[3]){
    mc_wfs_sync2();
  } else if (sync_check[1] && sync_check[2] && sync_check[3]){
    mc_wfs_sync3();
  } else {
    io_printf(IO_BUF, "[SPIN2-MC] Wrong sync check conf!\n");
    rt_error(RTE_ABORT);
  }

  sark.vcpu->user0 = 0;
  sark.vcpu->user1 = 0;
  sark.vcpu->user2 = 0;
  sark.vcpu->user3 = 0;
}


// --- Private Functions ---

/**
 * @brief TODO
 *
 */
bool mc_send(uint8_t *data, 
             uint8_t length,
             uint8_t last, 
             uint8_t counter, 
             bool extended_payload,
             uint32_t packet_key, 
             spin2_core_t *pivot_key) {
    int i;
    uint32_t packet_payload;
    uint8_t packet_shift;
    uint8_t ctrl;

    // ctrl field - if last fragment then <padding> else <counter>
    if (!extended_payload){
      ctrl = last ? 4-length : counter;
    } else {
      ctrl = last ? 6-length : counter;
    }

    // Build routing key: field[start:#bits]
    // pivot_x[27,3] pivot_y[24,3] pivot_p[20,4] lf[19,1] ctrl[16,3]
    packet_key |= ((pivot_key->x & 0x7) << 27);
    packet_key |= ((pivot_key->y & 0x7) << 24);
    packet_key |= (((pivot_key->p-1) & 0xF) << 20);
    packet_key |= ((last & 0x1) << 19);
    packet_key |= ((ctrl & 0x7) << 16);

    // Init payload
    packet_payload = 0;
    
    // 4 Byte in payload
    packet_shift = 0;
    for (i=0; i<length && i<4; ++i) {
      packet_payload |= MASK_SHIFT_L(data[i], 0xFF, packet_shift);
      packet_shift += 8;
    }

    // 2 Byte in routing key (extended payload)
    packet_shift = 0;
    if (extended_payload) {
      for(i=4; i<length && i<6; ++i) {
        packet_key |= MASK_SHIFT_L(data[i], 0xFF, packet_shift);
        packet_shift += 8;
      }
    }

    debug_printf("[SPIN2-MC] Send MC\n");
    debug_printf("           Key: %08x\n", packet_key);
    debug_printf("           Payload: %08x\n", packet_payload);

    spin1_delay_us(MC_LATENCY);
    
    // Send MC
    if(!spin1_send_mc_packet(packet_key, packet_payload, 1)) {
      io_printf(IO_BUF, "[SPIN2-MC] Error sending MC\n");
      rt_error(RTE_ABORT);
    }

    // Metrics
    if (extended_payload){
      spin2_metrics_update(spin2_metrics_bcast_fragment_send);
      if(last){
        spin2_metrics_update(spin2_metrics_bcast_fragment_send_last);
      }
    } else {
      spin2_metrics_update(spin2_metrics_ucast_fragment_send);
      if(last){
        spin2_metrics_update(spin2_metrics_ucast_fragment_send_last);
      }
    }

    return true;
}


/**
 * @brief TODO
 * @param key
 * @param payload
 */
void mc_recv(uint key, uint payload){
  uint8_t packet_type;
  uint8_t ucast_sync_ack;
  uint8_t sync_level;

  //packet_type = (uint8_t)((key >> 30) & 0x3);
  packet_type = (uint8_t)(key >> 30);

  // Multilevel SYNC
  if (packet_type == MC_SYNC){
    //spin2_metrics_update(spin2_metrics_mlsyn_recv);
    //mc_recv_sync(key, payload);
    sync_level = key & 0xF;

    switch (sync_level) {
      case SYNC_LVL1:
        sark.vcpu->user1++;
        sync_count[1]++;
        break;
      case SYNC_LVL2:
        sark.vcpu->user2++;
        sync_count[2]++;
        break;
      case SYNC_LVL3:
        sark.vcpu->user3++;
        sync_count[3]++;
        break;
      case SYNC_FREE:
        debug_printf("[SPIN2-MC] Received FREE!\n");
        sync_lock = false;
        break;
      default:
        io_printf(IO_BUF, "[SPIN2-MC] Error! Wrong value in the SYNC field\n");
        rt_error(RTE_ABORT);
        break;
    }
  } 
  // Unicast
  else if(packet_type == MC_P2P){
    ucast_sync_ack = (uint8_t)((key >> 14) & 0x3);

    // TODO enum for ucast sync field
    if (ucast_sync_ack == 0) {
      spin2_metrics_update(spin2_metrics_ucast_fragment_recv);
      mc_recv_p2p(key, payload);
    }
    else if(ucast_sync_ack == 1) {
      spin2_metrics_update(spin2_metrics_syn_recv);
      mc_recv_p2p_syn(key, payload);
    }
    else if(ucast_sync_ack == 2) {
      spin2_metrics_update(spin2_metrics_ack_recv);
      mc_recv_p2p_ack(key, payload);
    }
    else{
      // TODO, SYN/ACK or Error?
    }
  } 
  // Broadcast
  else if (packet_type == MC_BC) {
    spin2_metrics_update(spin2_metrics_bcast_fragment_recv);
    mc_recv_bc(key, payload);
  } 
  // Multicast
  else {
    // TODO
  }

  return;
}


void mc_pass(uint channel_id, uint channel_key) {
  spin2_core_t source;
  spin2_mc_channel channel;

  source.x = ((channel_key >> 11) & 0x7);
  source.y = ((channel_key >>  8) & 0x7);
  source.p = ((channel_key >>  4) & 0xF) + 1;

  if ((channel_key & 0b1) == 1){
    debug_printf("[SPIN2-MC] Finish Channel %d (BC)\n", channel_id);
    debug_printf("           From %d %d %d\n", source.x, source.y, source.p);
    channel = SPIN2_MC_BD;
    spin2_metrics_update(spin2_metrics_bcast_packet_recv);
  }
  else {
    debug_printf("[SPIN2-MC] Finish Channel %d (PP)\n", channel_id);
    debug_printf("           From %d %d %d\n", source.x, source.y, source.p);
    channel = SPIN2_MC_P2P;
    spin2_metrics_update(spin2_metrics_ucast_packet_recv);
  }

  callback(reconstruction_buffers[channel_id],
           reconstruction_buffers_length[channel_id],
           channel, 
           &source);

  uint cpsr;

  cpsr = cpu_int_disable();
  reconstruction_buffers_keys[channel_id] = 0;
  reconstruction_buffers_length[channel_id] = 0;
  cpu_int_restore(cpsr);
}


void mc_recv_p2p(uint key, uint payload) {
  int i;
  bool found;
  uint8_t last, ctr_pad, payload_length, payload_shift;
  uint16_t channel, channel_key;

  channel_key = key & 0x00003FF0;
  last = (key >> 19) & 0x1;
  ctr_pad = (key >> 16) & 0x7;

  // Find channel
  found = false;
  for (channel=0; channel<RECONSTRUCTION_BUFFERS; ++channel) {
      if(reconstruction_buffers_keys[channel] == channel_key){
        found = true;
        break;
      }
  }

  // Find new reconstruction buffers
  if (!found) {
    for (channel=0; channel<RECONSTRUCTION_BUFFERS; ++channel) {
      if(reconstruction_buffers_keys[channel] == 0) {
        reconstruction_buffers_keys[channel] = channel_key;
        reconstruction_buffers_length[channel] = 0;
        found = true;
        break;
      }
    }
  }
  if (!found) {
    io_printf(IO_BUF, "[SPIN2-MC] P2P No reconstruction buffers available\n");
    rt_error(RTE_ABORT);
  }

  // Insert Payload
  payload_length = 4;
  if (last){
    payload_length -= ctr_pad;
  }

  payload_shift = 0;
  for (i=0; i<payload_length; ++i){
    reconstruction_buffers[channel][reconstruction_buffers_length[channel]+i] =
        (payload >> payload_shift) & 0xFF;
    payload_shift += 8;
  }
  
  reconstruction_buffers_length[channel] += payload_length;

  // Send upper level
  if (last) {
    spin1_trigger_user_event(channel, channel_key);
    spin2_metrics_update(spin2_metrics_ucast_fragment_recv_last);
  }
  return;
}


void mc_recv_bc(uint key, uint payload){
  
  int i;
  bool found;
  uint8_t last, ctr_pad, payload_length, payload_shift;
  uint16_t channel, channel_key;

  channel_key = ((key & 0x3FF00000) >> 16) +1;
  last = (key >> 19) & 0x1;
  ctr_pad = (key >> 16) & 0x7;

  // Find channel
  found = false;
  for (channel=0; channel<RECONSTRUCTION_BUFFERS; ++channel) {
    if(reconstruction_buffers_keys[channel] == channel_key){
      found = true;
      break;
    }
  }

    // Find new reconstruction buffers
  if (!found) {
    for (channel=0; channel<RECONSTRUCTION_BUFFERS; ++channel) {
      if(reconstruction_buffers_keys[channel] == 0) {
        reconstruction_buffers_keys[channel] = channel_key;
        reconstruction_buffers_length[channel] = 0;
        found = true;
        break;
      }
    }
  }
  if (!found) {
    io_printf(IO_BUF, "[SPIN2-MC] P2P No reconstruction buffers available\n");
    rt_error(RTE_ABORT);
  }

  // Insert Payload
  payload_length = 6;
  if (last){
    payload_length -= ctr_pad;
  }

  payload_shift = 0;
  for (i=0; i<payload_length && i<4; ++i){
    reconstruction_buffers[channel][reconstruction_buffers_length[channel]+i] =
        (payload >> payload_shift) & 0xFF;
    payload_shift += 8;
  }

  // Insert Extended Payload
  payload_shift = 0;
  for (i=4; i<payload_length && i<6; ++i){
    reconstruction_buffers[channel][reconstruction_buffers_length[channel]+i] =
        (key >> payload_shift) & 0xFF;
    payload_shift += 8;
  }

  reconstruction_buffers_length[channel] += payload_length;

  // Send upper level
  if (last) {
    spin1_trigger_user_event(channel, channel_key);
    spin2_metrics_update(spin2_metrics_bcast_fragment_recv_last);
  }

  return;
}


void mc_recv_sync(uint key, uint payload){
  uint32_t sync_level = key & 0xF;

  switch (sync_level) {
    case SYNC_LVL1:
      sark.vcpu->user1++;
      sync_count[1]++;
      break;
    case SYNC_LVL2:
      sark.vcpu->user2++;
      sync_count[2]++;
      break;
    case SYNC_LVL3:
      sark.vcpu->user3++;
      sync_count[3]++;
      break;
    case SYNC_FREE:
      debug_printf("[SPIN2-MC] Received FREE!\n");
      sync_lock = false;
      break;
    default:
      io_printf(IO_BUF, "[SPIN2-MC] Error! Wrong value in the SYNC field\n");
      rt_error(RTE_ABORT);
      break;
  }
  
  return;
}


inline bool mc_send_sync(uint8_t level){
  spin2_metrics_update(spin2_metrics_mlsyn_send);
  return spin1_send_mc_packet(template_multilevel_sync|level, 0, 1);
}

/**
 * @brief TODO
 * MCM - wait for synchronization 
 * Implement a Multilevel Global Barier in the whole execution context
 */
void mc_wfs(){
  bool lock;
  bool *plock = &sync_lock;
  uint sema;

  // Send signal to self-chip syncronizer
  sema = sark_app_raise();
  sark.vcpu->user0++;

  // Wait unlock
  do {
    lock = spin2_swap_bool(true, plock);
    if(lock) {
      spin1_delay_us(1);
    }
  } while(lock);

  sark_app_lower();

  return;
}


/**
 * @brief TODO
 * MCM - wait for synchronization - level 1
 * Implement a Multilevel Global Barier in the whole execution context
 */
void mc_wfs_sync1(){
  bool lock;
  bool *plock = &sync_lock;

  // Wait signals from self-chip processors
  do {
    spin1_delay_us(1);
  } while(sark_app_sema() != sync_max[1]);

  // Send signal to self-ring syncronizer
  mc_send_sync(SYNC_LVL2);
  sark.vcpu->user0++;
  
  // Wait unlock
  do {
    lock = spin2_swap_bool(true, plock);
    if(lock) {
      spin1_delay_us(1);
    }
  } while(lock);

  return;
}


/**
 * @brief TODO
 * MCM - wait for synchronization - level 2
 * Implement a Multilevel Global Barier in the whole execution context
 */
void mc_wfs_sync2(){
  bool lock;
  bool *plock = &sync_lock;

  uint8_t count;
  uint8_t count_max;
  uint8_t *pcount;

  // Wait signals from self-chip processors
  do {
    spin1_delay_us(1);
  } while(sark_app_sema() != sync_max[1]);
  
  // Wait signals from self-ring chip-syncronizers
  count = 0;
  count_max = sync_max[2];
  pcount = &sync_count[2];
  while(count < count_max) {
    spin1_delay_us(1);
    count += spin2_swap_uint8(0, pcount);
  }

  // Send signal to self-board syncronizer
  mc_send_sync(SYNC_LVL3);
  sark.vcpu->user0++;

  // Wait unlock
  do {
    lock = spin2_swap_bool(true, plock);
    if(lock) {
      spin1_delay_us(1);
    }
  } while(lock);

  return;
}


/**
 * @brief TODO
 * MCM - wait for synchronization - level 3
 * Implement a Multilevel Global Barier in the whole execution context
 */
void mc_wfs_sync3(){
  uint8_t count;
  uint8_t count_max;
  uint8_t *pcount;

  // Wait signals from self-chip processors
  do {
    spin1_delay_us(1);
  } while(sark_app_sema() != sync_max[1]);

  // Wait signals from self-ring chip-syncronizers
  // Skip

  // Wait signals from self-board ring-syncronizers
  count = 0;
  count_max = sync_max[3];
  pcount = &sync_count[3];
  while(count < count_max) {
    spin1_delay_us(1);
    count += spin2_swap_uint8(0, pcount);
  }

  // Send signal to all processors
  mc_send_sync(SYNC_FREE);

  return;
}


/**
 * @brief TODO
 *
 */
void mc_init() {
  int i;
  int x, y, p;

  debug_printf("[SPIN2-MC] Init\n");

  core.x = sark_chip_id() >> 8;
  core.y = sark_chip_id() & 0xFF;
  core.p = sark_core_id();

  sark.vcpu->user0 = 0;
  sark.vcpu->user1 = 0;
  sark.vcpu->user2 = 0;
  sark.vcpu->user3 = 0;

  // Set lock for multilevel sync
  sync_lock = true;

  // Init multilevel sync behavior
  for(i=0; i<SYNC_LEVELS; i++) {
    sync_check[i] = false;
    sync_count[i] = 0;
    sync_max[i] = 0;
  }

  if (core.p == 1) {
    sync_check[1] = true;
    if(core.x == core.y){
      sync_check[2] = true;
      if (core.x == 0){
        sync_check[3] = true;
      }
    }
  }

  // Create reconstruction buffers
  if (RECONSTRUCTION_BUFFERS_DTCM){
    reconstruction_buffers = (uint8_t **) sark_alloc(
      RECONSTRUCTION_BUFFERS, sizeof(uint8_t_p));
  } else {
    reconstruction_buffers = (uint8_t **) sark_xalloc(
      sv->sdram_heap, RECONSTRUCTION_BUFFERS*sizeof(uint8_t_p), 0, ALLOC_LOCK);
  }

  if (reconstruction_buffers == NULL){
    io_printf(IO_BUF, "[SPIN2-MC] Cannot allocate reconstruction buffers (1)\n");
    rt_error(RTE_ABORT);
  }

  for (i=0; i<RECONSTRUCTION_BUFFERS; ++i) {
    reconstruction_buffers_keys[i] = 0;
    reconstruction_buffers_length[i] = 0;

    if (RECONSTRUCTION_BUFFERS_DTCM){
      reconstruction_buffers[i] = (uint8_t *) sark_alloc(
        1, RECONSTRUCTION_BUFFER_SIZE);
    } else {
      reconstruction_buffers[i] = (uint8_t *) sark_xalloc(
        sv->sdram_heap, RECONSTRUCTION_BUFFER_SIZE, 0, ALLOC_LOCK);
    }

    if (reconstruction_buffers[i] == NULL){
      io_printf(IO_BUF, "[SPIN2-MC] Cannot allocate reconstruction buffers (2) [%d]\n", i);
      rt_error(RTE_ABORT);
    }
  }

  // Sync core
  lock_ack = true;
  for (x=0; x<8; x++) {
    for (y=0; y<8; y++) {
      for (p=0; p<16; p++) {
          lock_sync[x][y][p] = true; 
      }
    } 
  }

  // Template Ucast
  template_ucast = 0x00000000;
  template_ucast |= ((core.x & 0x7) << 11);
  template_ucast |= ((core.y & 0x7) << 8);
  template_ucast |= (((core.p-1) & 0xF) << 4);

  // Template Bcast
  template_bcast = 0x40000000;

  // Template Multilevel Sync
  template_multilevel_sync = 0x80000000;

  spin2_metrics_init();

  spin1_callback_on(MCPL_PACKET_RECEIVED, mc_recv, -1);
  spin1_callback_on(USER_EVENT, mc_pass, 0);

  return;
}

// --- Sync Core ---
void spin2_mc_syn(spin2_core_t *destination) {
  uint32_t packet_key;
  bool lock;
  bool *lock_p;

  // Send sync
  packet_key = template_ucast;
  packet_key |= ((destination->x & 0x7) << 27);
  packet_key |= ((destination->y & 0x7) << 24);
  packet_key |= (((destination->p-1) & 0xF) << 20);
  packet_key |= (1 << 14);
  spin1_send_mc_packet(packet_key, 0, 1);
  spin2_metrics_update(spin2_metrics_syn_send);

  // Polling ack lock -> Test and set bit in uint16_t (Set true)
  lock_p = &lock_ack;
  do{
    spin1_delay_us(1);
    lock = spin2_swap_bool(true, lock_p);
  } while(lock);
  
  return;
}

void spin2_mc_wfs_core(spin2_core_t *source) {
  uint32_t packet_key;
  bool lock;
  bool *lock_p;

  // Polling sync lock -> Test and set bit in uint16_t (Set true)
  lock_p = &lock_sync[source->x][source->y][source->p-1];
  do{
    spin1_delay_us(1);
    lock = spin2_swap_bool(true, lock_p);
  } while(lock);

  // Send ack
  packet_key = template_ucast;
  packet_key |= ((source->x & 0x7) << 27);
  packet_key |= ((source->y & 0x7) << 24);
  packet_key |= (((source->p-1) & 0xF) << 20);
  packet_key |= (1 << 15);
  spin1_send_mc_packet(packet_key, 0, 1);
  spin2_metrics_update(spin2_metrics_ack_send);
  
  return;
}

bool spin2_mc_syn_set(spin2_core_t *core, bool value) {
  bool lock;
  bool *lock_p;
  
  lock_p = &lock_sync[core->x][core->y][core->p-1];
  lock = spin2_swap_bool(value, lock_p);
  return lock;
}

bool spin2_mc_ack_set(spin2_core_t *core, bool value) {
  bool lock;
  bool *lock_p;

  lock_p = &lock_ack;
  lock = spin2_swap_bool(value, lock_p);
  return lock;
}

void mc_recv_p2p_syn(uint key, uint payload) {
  spin2_core_t core;
  bool *lock_p;

  core.x = ((key >> 11) & 0x7);
  core.y = ((key >>  8) & 0x7);
  core.p = ((key >>  4) & 0xF);

  lock_p = &lock_sync[core.x][core.y][core.p];
  spin2_swap_bool(false, lock_p);
  return;
}

void mc_recv_p2p_ack(uint key, uint payload){
  bool *lock_p;

  lock_p = &lock_ack;
  spin2_swap_bool(false, lock_p);
  return;
}