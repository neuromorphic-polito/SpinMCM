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
// spin2_mc.c: Multicast communication functions for SpinMCM
// ==========================================================================

#include "_spin2_api.h"

#define MASK_SHIFT_R(v, m, s) (((v) & (m)) >> (s))
#define MASK_SHIFT_L(v, m, s) (((v) & (m)) << (s))

#define BIT_TOGGLE(v, b) ((v) ^= 1 << (b))
#define BIT_SET(v, b, x) ((v) ^= (-(x) ^ (v)) & (1 << (b)))
#define BIT_CHECK(v, b)  ((((v) >> (b)) & 1) == 1)

#define MC_LATENCY 5  // micro second 5

#define BOARD_X_MAX 8
#define BOARD_Y_MAX 8

typedef uint32_t * uint32_t_p;


// --- Local Prototypes ---
void generate_broadcast_router_keys();
bool valid_chip(int32_t x, int32_t y);
void generate_broadcast_router_key(uint32_t x, uint32_t y);
void generate_safe_route(uint32_t *route, uint32_t direction);
void add_rtr_rule(uint32_t key, uint32_t mask, uint32_t route);
//void sync1_execute(mc_msg_t *msg);
//void sync2_execute(mc_msg_t *msg);
//void sync3_execute(mc_msg_t *msg);

void _spin2_mc_wfs();
void _spin2_mc_wfs_sync1();
void _spin2_mc_wfs_sync2();
void _spin2_mc_wfs_sync3();


// --- Globals - MC PKT Header v4
const uint32_t MC_X_MSK = 0xFF000000;
const uint32_t MC_X_SH = 24;
const uint32_t MC_Y_MSK = 0x00FF0000;
const uint32_t MC_Y_SH = 16;
const uint32_t MC_P_MSK = 0x0000F000;
const uint32_t MC_P_SH = 12;
const uint32_t MC_CTRL_MSK = 0x00000C00;
const uint32_t MC_CTRL_SH = 10;
const uint32_t MC_SYNC_MSK = 0x00000380;
const uint32_t MC_SYNC_SH = 7;
const uint32_t MC_ID_MSK = 0x0000007F;
const uint32_t MC_ID_SH = 0;

const uint32_t W_MC_X_MSK = 0xFF;
const uint32_t W_MC_Y_MSK = 0xFF;
const uint32_t W_MC_P_MSK = 0x0F;
const uint32_t W_MC_CTRL_MSK = 0x03;
const uint32_t W_MC_SYNC_MSK = 0x07;
const uint32_t W_MC_ID_MSK = 0x7F;

const uint32_t ROUTE_LINK_E = 0;
const uint32_t ROUTE_LINK_NE = 1;
const uint32_t ROUTE_LINK_N = 2;
const uint32_t ROUTE_LINK_W = 3;
const uint32_t ROUTE_LINK_SW = 4;
const uint32_t ROUTE_LINK_S = 5;
const uint32_t ROUTE_ALL_AP = 0x1FFFE << 6;
const uint32_t ROUTE_MODIFICATORS[][2] = {
    {+1, 0}, {+1, +1}, {0, +1}, {-1, 0}, {-1, -1}, {0, -1}};

// --- Local Variables ---
static callback_t _spinn2_mc_callback = NULL;

uint32_t core_x;
uint32_t core_y;
uint32_t core_p;

// --- Debug variables ---
uint32_t *debug_mc_w;
uint32_t *debug_mc_we;
uint32_t *debug_mc_r;
uint32_t *debug_mc_re;

// --- Sync Variable ---
//const uint8_t SYNC_LEVELS = 7;
#define SYNC_LEVELS 7
bool sync_lock;

bool sync_check[SYNC_LEVELS];
uint32_t sync_count[SYNC_LEVELS];
uint32_t sync_max[SYNC_LEVELS];

// --- Functions ---
void mc_msg_key_write(mc_msg_t *mc_msg) {
  mc_msg->key = 0;
  mc_msg->key |= MASK_SHIFT_L(mc_msg->x, W_MC_X_MSK, MC_X_SH);
  mc_msg->key |= MASK_SHIFT_L(mc_msg->y, W_MC_Y_MSK, MC_Y_SH);
  mc_msg->key |= MASK_SHIFT_L(mc_msg->p - 1, W_MC_P_MSK, MC_P_SH);
  mc_msg->key |= MASK_SHIFT_L(mc_msg->ctrl, W_MC_CTRL_MSK, MC_CTRL_SH);
  mc_msg->key |= MASK_SHIFT_L(mc_msg->sync, W_MC_SYNC_MSK, MC_SYNC_SH);
  mc_msg->key |= MASK_SHIFT_L(mc_msg->id, W_MC_ID_MSK, MC_ID_SH);
  return;
}

void mc_msg_key_read(mc_msg_t *mc_msg) {
  mc_msg->x = MASK_SHIFT_R(mc_msg->key, MC_X_MSK, MC_X_SH);
  mc_msg->y = MASK_SHIFT_R(mc_msg->key, MC_Y_MSK, MC_Y_SH);
  mc_msg->p = MASK_SHIFT_R(mc_msg->key, MC_P_MSK, MC_P_SH) + 1;
  mc_msg->ctrl = MASK_SHIFT_R(mc_msg->key, MC_CTRL_MSK, MC_CTRL_SH);
  mc_msg->sync = MASK_SHIFT_R(mc_msg->key, MC_SYNC_MSK, MC_SYNC_SH);
  mc_msg->id = MASK_SHIFT_R(mc_msg->key, MC_ID_MSK, MC_ID_SH);
  return;
}

/**
 *
 * @return
 */
mc_msg_t *spin2_mc_alloc(){
  mc_msg_t *mc_msg;
  mc_msg = sark_alloc(1, sizeof(mc_msg_t));

  mc_msg->x = core_x;
  mc_msg->y = core_y;
  mc_msg->p = core_p;
  mc_msg->ctrl = 0;
  mc_msg->sync = 0;
  mc_msg->id = 0;

  return mc_msg;
}

/**
 *
 * @param mc_msg
 */
void spin2_mc_free(mc_msg_t *mc_msg){
  sark_free(mc_msg);
  return;
}


/**
 *
 * @param mc_msg
 * @param last
 * @return
 */
bool spin2_mc_send(mc_msg_t *mc_msg, bool last){
  bool r;

  if(last){
    mc_msg->ctrl = 0b11;
  }
  else if(mc_msg->id == 0){
    mc_msg->ctrl = 0b00;
  }
  else{
    // TODO Toggle ctrl flags if id overflow
    mc_msg->ctrl = 0b01;
  }

  mc_msg_key_write(mc_msg);

  if(spin1_send_mc_packet(mc_msg->key, mc_msg->payload, 1)){
//    io_printf(IO_BUF, "[SPIN2-MC] Send Ctrl %01x Id %02x\n",
//              mc_msg->ctrl, mc_msg->id);
    mc_msg->id += 1;
    *debug_mc_w += 1;
    r = true;
  } else{
    io_printf(IO_BUF, "[SPIN2-MC] Error! Impossible send MC\n");
    *debug_mc_we += 1;
    rt_error(RTE_ABORT);
//    r = false;
  }

  spin1_delay_us(MC_LATENCY);
  return r;
}


/**
 *
 * @param key
 * @param payload
 */
void spin2_mc_callback(uint key, uint payload){
  mc_msg_t mc_msg;

  if(_spinn2_mc_callback != NULL) {
    mc_msg.key = key;
    mc_msg.payload = payload;
    mc_msg_key_read(&mc_msg);

//    io_printf(IO_BUF, "[SPIN2-MC] Rcv %08x %08x\n", key, payload);

    switch (mc_msg.sync){
      case 0:
        _spinn2_mc_callback((uint)&mc_msg, 0);
        break;
      case SYNC_LEVELS:
        sync_lock = false;
        sark.vcpu->user1 += 1;
        break;
      default:
        sync_count[mc_msg.sync] += 1;
        sark.vcpu->user1 += 1;
        break;
    }

    *debug_mc_r += 1;
  }
  return;
}

void spin2_mc_sync_max_set(uint32_t level, uint32_t value){
  sync_max[level] = value;
  io_printf(IO_BUF, "[SPIN2-MC] Set sync%d max: %d\n", level, sync_max[level]);
  return;
}

//void spin2_mc_sync1_max_set(uint32_t v){
//  sync_max[1] = v;
//  io_printf(IO_BUF, "[SPIN2-MC] Set sync1 max: %d\n", sync_max[1]);
//  return;
//}
//
//void spin2_mc_sync2_max_set(uint32_t v){
//  sync_max[2] = v;
//  io_printf(IO_BUF, "[SPIN2-MC] Set sync2 max: %d\n", sync_max[2]);
//  return;
//}

void spin2_mc_wfs() {
  if (!sync_check[1] && !sync_check[2] && !sync_check[3]) {
    _spin2_mc_wfs();
  } else if (sync_check[1] && !sync_check[2] && !sync_check[3]) {
    _spin2_mc_wfs_sync1();
  } else if (sync_check[1] && sync_check[2] && !sync_check[3]){
    _spin2_mc_wfs_sync2();
  } else {
    _spin2_mc_wfs_sync3();
  }
}

void _spin2_mc_wfs(){
  bool finish = false;
  uint cpsr;

  spin1_send_mc_packet(MASK_SHIFT_L(1, W_MC_SYNC_MSK, MC_SYNC_SH), 0, 1);

  do{
    cpsr = cpu_int_disable();
    if(!sync_lock) {
      finish = true;
      sync_lock = true;
    }
    cpu_int_restore(cpsr);

    if(!finish) {
//      cpu_wfi();
      spin1_delay_us(1);
    }

  } while(!finish);

  return;
}

void _spin2_mc_wfs_sync1(){
  bool finish;
  uint cpsr;

  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(sync_count[1] == (sync_max[1])) {
      finish = true;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }
  } while(!finish);

  spin1_send_mc_packet(MASK_SHIFT_L(2, W_MC_SYNC_MSK, MC_SYNC_SH), 0, 1);

  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(!sync_lock) {
      finish = true;
      sync_count[1] = 0;
      sync_lock = true;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }

  } while(!finish);

  return;
}

void _spin2_mc_wfs_sync2(){
  bool finish;
  uint cpsr;

  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(sync_count[1] == (sync_max[1])) {
      finish = true;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }
  } while(!finish);


  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(sync_count[2] == (sync_max[2])) {
      finish = true;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }

  } while(!finish);

  spin1_send_mc_packet(MASK_SHIFT_L(3, W_MC_SYNC_MSK, MC_SYNC_SH), 0, 1);

  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(!sync_lock) {
      finish = true;
      sync_count[1] = 0;
      sync_count[2] = 0;
      sync_lock = true;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }

  } while(!finish);

  return;
}


void _spin2_mc_wfs_sync3(){
  bool finish;
  uint cpsr;

  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(sync_count[1] == (sync_max[1])) {
      finish = true;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }
  } while(!finish);


  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(sync_count[2] == (sync_max[2])) {
      finish = true;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }

  } while(!finish);


  finish = false;
  do{
    cpsr = cpu_int_disable();
    if(sync_count[3] == (sync_max[3])) {
      finish = true;
      sync_count[1] = 0;
      sync_count[2] = 0;
      sync_count[3] = 0;
    }
    cpu_int_restore(cpsr);

    if(!finish){
//      cpu_wfi();
      spin1_delay_us(1);
    }

  } while(!finish);

  spin1_send_mc_packet(MASK_SHIFT_L(SYNC_LEVELS, W_MC_SYNC_MSK, MC_SYNC_SH), 0, 1);

  return;
}


//void sync1_execute(mc_msg_t *msg){
//  // Always call in FIQ priority -1
//  sync1_count += 1;
////  io_printf(IO_BUF, "[SPIN2-MC] SYNC1 %d\n", sync1_count);
//
//  if(sync1_count >= sync1_max){
//    // Send sync2 message
//    sync1_count = 0;
//    spin1_send_mc_packet(0x00000200, 0, 1);
//  }
//
//  return;
//}
//
//void sync2_execute(mc_msg_t *msg){
//  // Always call in FIQ priority -1
//  sync2_count += 1;
////  io_printf(IO_BUF, "[SPIN2-MC] SYNC2 %d\n", sync2_count);
//
//  if(sync2_count >= sync2_max){
//    // Send sync2 message
//    sync2_count = 0;
//    spin1_send_mc_packet(0x00000300, 0, 1);
//    // Packet not for me
//    sync_lock = false;
//  }
//
//  return;
//}
//
//void sync3_execute(mc_msg_t *msg){
//  // Always call in FIQ priority -1
//  io_printf(IO_BUF, "SYNC3\n");
//  sync_lock = false;
//  return;
//}

/**
 *
 * @param c
 */
void spin2_mc_callback_register(callback_t c){
  _spinn2_mc_callback = c;
  return;
}


/**
 *
 */
void spin2_mc_callback_reset(){
  _spinn2_mc_callback = NULL;
  return;
}


// --- MC RTR TABLE INIT ---

/**
 *
 */
void spin2_mc_init(){
  int i;

  core_x = sark_chip_id() >> 8;
  core_y = sark_chip_id() & 0xFF;
  core_p = sark_core_id();

  sark.vcpu->user0 = 0;
  sark.vcpu->user1 = 0;
  sark.vcpu->user2 = 0;
  sark.vcpu->user3 = 0;

  debug_mc_w = (uint32_t_p) &sark.vcpu->user0;
  debug_mc_we = (uint32_t_p) &sark.vcpu->user1;
  debug_mc_r = (uint32_t_p) &sark.vcpu->user2;
  debug_mc_re = (uint32_t_p) &sark.vcpu->user3;

  for(i=0; i< SYNC_LEVELS; i++) {
    sync_check[i] = false;
    sync_count[i] = 0;
    sync_max[i] = 0;
  }

  sync_lock = true;

  if (core_p == 1) {
    sync_check[1] = true;
    if(core_x == core_y){
      sync_check[2] = true;
      if (core_x == 0){
        sync_check[3] = true;
      }
    }

    io_printf(IO_BUF, "[SPIN2-MC] Generate routing key\n");
    generate_broadcast_router_keys();
  }
  return;
}

void generate_broadcast_router_keys() {
  int32_t x, y;
  int32_t route;

  /// Sync3 to all
//  add_rtr_rule(0x00000300, 0x00000300, ROUTE_ALL_AP);

  /// Sync1 to me
  if(sync_check[1]) {
    /// Sync1 to me
    route = 0;
    BIT_TOGGLE(route, core_p + 6);
    add_rtr_rule(MASK_SHIFT_L(1, W_MC_SYNC_MSK, MC_SYNC_SH), MC_SYNC_MSK, route);

    if (sync_check[2]) {
      /// Sync2 to me
      route = 0;
      BIT_TOGGLE(route, core_p + 6);

      add_rtr_rule(MASK_SHIFT_L(2, W_MC_SYNC_MSK, MC_SYNC_SH), MC_SYNC_MSK, route);

      if (sync_check[3]) {
        /// Sync3 to me
        route = 0;
        BIT_TOGGLE(route, core_p + 6);
        add_rtr_rule(MASK_SHIFT_L(3, W_MC_SYNC_MSK, MC_SYNC_SH), MC_SYNC_MSK, route);
      } else {
        /// Sync3 to SW
        route = 0;
        generate_safe_route(&route, ROUTE_LINK_SW);
        add_rtr_rule(MASK_SHIFT_L(3, W_MC_SYNC_MSK, MC_SYNC_SH), MC_SYNC_MSK, route);
      }

    } else {
      if (core_x > core_y) {
        /// Sync2 to N
        route = 0;
        generate_safe_route(&route, ROUTE_LINK_N);
        add_rtr_rule(MASK_SHIFT_L(2, W_MC_SYNC_MSK, MC_SYNC_SH), MC_SYNC_MSK, route);
      } else {
        /// Sync2 to E
        route = 0;
        generate_safe_route(&route, ROUTE_LINK_E);
        add_rtr_rule(MASK_SHIFT_L(2, W_MC_SYNC_MSK, MC_SYNC_SH), MC_SYNC_MSK, route);
      }
    }
  }

  /// Broadcast
  for (x = 0; x < BOARD_X_MAX; x++) {
    for (y = 0; y < BOARD_Y_MAX; y++) {
      if (valid_chip(x, y)) {
        generate_broadcast_router_key(x, y);
      }
    }
  }

  return;
}

void generate_broadcast_router_key(uint32_t x, uint32_t y) {
  uint32_t key, key_p;
  uint32_t mask, mask_p;
  uint32_t route, route_p;

//  uint32_t rule;
  uint32_t p;

  key = 0 | MASK_SHIFT_L(x, W_MC_X_MSK, MC_X_SH) | MASK_SHIFT_L(y, W_MC_Y_MSK, MC_Y_SH);
  mask = MC_X_MSK | MC_Y_MSK;
  route = ROUTE_ALL_AP;

  if (core_x == x && core_y == y) {
    /// Generate Safe Neighbour routes
    generate_safe_route(&route, ROUTE_LINK_E);
    generate_safe_route(&route, ROUTE_LINK_NE);
    generate_safe_route(&route, ROUTE_LINK_N);
    generate_safe_route(&route, ROUTE_LINK_W);
    generate_safe_route(&route, ROUTE_LINK_SW);
    generate_safe_route(&route, ROUTE_LINK_S);

    /// Enable Processor check
    mask_p = mask | MC_P_MSK;

    /// Generate 16 rule, one for each Processors
    for (p = 1; p < 17; p++) {
      /// Add Processor to routing key
      key_p = key | MASK_SHIFT_L(p - 1, W_MC_P_MSK, MC_P_SH);

      /// Disable self connections
      route_p = route;
      BIT_TOGGLE(route_p, p + 6);

      /// Add rule
//      rule = rtr_alloc(1);
//      if (rule == 0)
//        rt_error(RTE_ABORT);
//      rtr_mc_set(rule, key_p, mask_p, route_p);
      add_rtr_rule(key_p, mask_p, route_p);
    }
  } else {
    if (core_x == x && core_y < y) {
      generate_safe_route(&route, ROUTE_LINK_SW);
      generate_safe_route(&route, ROUTE_LINK_S);
    } else if (core_x == x && core_y > y) {
      generate_safe_route(&route, ROUTE_LINK_NE);
      generate_safe_route(&route, ROUTE_LINK_N);
    } else if (core_y == y && core_x < x) {
      generate_safe_route(&route, ROUTE_LINK_N);
      generate_safe_route(&route, ROUTE_LINK_SW);
      generate_safe_route(&route, ROUTE_LINK_W);
    } else if (core_y == y && core_x > x) {
      generate_safe_route(&route, ROUTE_LINK_S);
      generate_safe_route(&route, ROUTE_LINK_NE);
      generate_safe_route(&route, ROUTE_LINK_E);
    } else if (core_x > x && core_y > y) {
      generate_safe_route(&route, ROUTE_LINK_NE);
    } else if (core_x > x && core_y < y) {
      generate_safe_route(&route, ROUTE_LINK_S);
    } else if (core_x < x && core_y < y) {
      generate_safe_route(&route, ROUTE_LINK_SW);
    } else if (core_x < x && core_y > y) {
      generate_safe_route(&route, ROUTE_LINK_N);
    }

    /// Generate 1 Rule
//    rule = rtr_alloc(1);
//    if (rule == 0)
//      rt_error(RTE_ABORT);
//    rtr_mc_set(rule, key, mask, route);
    add_rtr_rule(key, mask, route);
  }

  return;
}

void generate_safe_route(uint32_t *route, uint32_t direction) {
  int32_t x, y;

  x = core_x + ROUTE_MODIFICATORS[direction][0];
  y = core_y + ROUTE_MODIFICATORS[direction][1];

  if (valid_chip(x, y)) {
    BIT_SET(*route, direction, 1);
  }
  return;
}

bool valid_chip(int32_t x, int32_t y) {
  return y <= (x + 3) &&
      y >= (x - 4) &&
      x >= 0 &&
      y >= 0 &&
      x < BOARD_X_MAX &&
      y < BOARD_Y_MAX;
}

void add_rtr_rule(uint32_t key, uint32_t mask, uint32_t route){
  uint32_t id;

  id = rtr_alloc(1);
  if (id == 0){
    io_printf(IO_BUF, "[ERROR] MC routing table full\n");
    rt_error(RTE_ABORT);
  }
  rtr_mc_set(id, key, mask, route);
  return;
}