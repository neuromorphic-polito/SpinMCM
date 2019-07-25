/**
 * @file spin2_api.h
 * @author Barchi Francesco
 * @date 2017-2018
 * @brief The second SpiNNaker API
 *
 * The SPIN2 library provides:
 * An SDP callback multiplexer, based on SDP port.
 * A unicast and broadcast protocol, based on MC packets, for communication and sync,
 * An hash table ADT.
 */

#ifndef __SPIN2_API_H__
#define __SPIN2_API_H__

#include <stdint.h>
#include <stdbool.h>

#include "sark.h"
#include "spin1_api.h"

//#define SPIN2_DEBUG
#define SPIN2_VERSION "19w19"


// --- Multicast Communication Middleware - Pubblic Types ---

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t p;
} spin2_core_t;


typedef enum {
  SPIN2_MC_P2P,  // UCAST over MC
  SPIN2_MC_BD    // BCAST over MC
} spin2_mc_channel;


// Callback prototype for MC packet
typedef void (*spin2_mc_callback_t)(
    uint8_t *,
    uint16_t,
    spin2_mc_channel,
    spin2_core_t *);


// --- Multicast Communication Middleware - Pubblic Functions ---

// Spin2 MCM - Init
void spin2_mc_init();
void spin2_mc_callback_register(spin2_mc_callback_t c);
void spin2_mc_callback_reset();

// Spin2 MCM - Send - UCAST over MC, BCAST over MC
bool spin2_mc_send(
    uint8_t *data_head, uint16_t length_head,  // Stream Head
    uint8_t *data_body, uint16_t length_body,  // Stream Body
    uint8_t *data_tail, uint16_t length_tail,  // Stream Tail
    spin2_mc_channel channel, spin2_core_t *destination);

// Spin2 Multilevel Sync
void spin2_mc_wfs();
void spin2_mc_sync_max_set(uint32_t level, uint32_t value);

// Spin2 PP Sync
void spin2_mc_syn(spin2_core_t *destination);
void spin2_mc_wfs_core(spin2_core_t *source);

bool spin2_mc_syn_set(spin2_core_t *core, bool value);
bool spin2_mc_ack_set(spin2_core_t *core, bool value);

// --- Spin2-SDP - Pubblic Functions ---
void spin2_sdp_init();
bool spin2_sdp_callback_on(uint sdp_port, callback_t sdp_callback);
bool spin2_sdp_callback_off(uint sdp_port);


// --- Hash Table - Public Types ---

/**
 * @brief ADT for Spin2 Hash Table
 *
 * The Abstract Data Type for the hash table data structure.
 */
typedef struct spin2_hash_table spin2_ht_t;


/**
 * @brief This enumerative define valid hash table sizes
 *
 * The hash table size must be a prime number, in this implementation
 * we define six valid sizes using prime numbers near to powers of two.
 */
typedef enum {
  SPIN2_HT_TINY = ((1 << 0x5) - 1),  /**< Tiny: 31 elements */
  SPIN2_HT_SMALL = ((1 << 0x7) - 1), /**< Small: 127 elements */
  SPIN2_HT_NORMAL = ((1 << 0x8) - 5), /**< Normal: 251 elements */
  SPIN2_HT_BIG = ((1 << 0x9) - 3),  /**< Big: 509 elements */
  SPIN2_HT_VBIG = ((1 << 0xA) - 3),  /**< Very big: 1021 elements */
  SPIN2_HT_HUGE = ((1 << 0xB) - 9)  /**< Huge: 2039 elements */
} spin2_ht_size_t;

typedef int32_t (*spin2_ht_compare_fp)(void *, void *);
typedef uint32_t (*spin2_ht_key_fp)(void *);
typedef void (*spin2_ht_item_free)(void *);


// --- Hash Table - Public Functions ---

bool spin2_ht_create(
    spin2_ht_t **st,
    spin2_ht_size_t size,
    spin2_ht_compare_fp f_compare,
    spin2_ht_key_fp f_key,
    spin2_ht_item_free f_free
);

bool spin2_ht_destroy(
    spin2_ht_t **st
);

bool spin2_ht_insert(
    spin2_ht_t *st,
    void *item
);

bool spin2_ht_get(
    spin2_ht_t *st,
    uint32_t search_key,
    void **out_item
);

bool spin2_ht_exist(
    spin2_ht_t *st,
    uint32_t search_key
);

bool spin2_ht_remove(
    spin2_ht_t *st,
    uint32_t remove_key,
    void **out_item
);

uint32_t spin2_ht_get_len(
    spin2_ht_t *st
);

uint32_t spin2_ht_get_len_max(
    spin2_ht_t *st
);


static inline bool spin2_swap_bool(bool set, bool *test) {
    uint8_t check;
    __asm__ __volatile__ ("swpb %[check], %[set], [%[test]]" 
        : [check] "=&r" (check) // & force to use different registers
        : [set] "r" (set), 
          [test] "r" (test) );
    return check;
}

static inline uint8_t spin2_swap_uint8(uint8_t set, uint8_t *test) {
    uint8_t check;
    __asm__ __volatile__ ("swpb %[check], %[set], [%[test]]" 
        : [check] "=&r" (check) // & force to use different registers
        : [set] "r" (set), 
          [test] "r" (test) );
    return check;
}

// --- Metrics ---

//#define SPIN2_METRICS
void spin2_metrics_print();

#endif //__SPIN2_API_H__
