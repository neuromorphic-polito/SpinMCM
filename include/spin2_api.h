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
// spin2_api.h: Main header File for SpinMCM
// ==========================================================================

#ifndef __SPIN2_API_H__
#define __SPIN2_API_H__

#include <stdint.h>
#include <stdbool.h>

#include "sark.h"
#include "spin1_api.h"

//#define SPIN2_DEBUG
#define SPIN2_SDP_PORTS 8


// --- MC ---
typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t p;
  uint8_t ctrl;
  uint8_t sync;
  uint8_t id;

  uint32_t key;
  uint32_t payload;
} mc_msg_t;

void spin2_mc_init();

void spin2_mc_callback(uint key, uint payload);
void spin2_mc_callback_register(callback_t c);
void spin2_mc_callback_reset();

mc_msg_t *spin2_mc_alloc();
void spin2_mc_free(mc_msg_t *mc_msg);
bool spin2_mc_send(mc_msg_t *mc_msg, bool last);

void spin2_mc_sync_max_set(uint32_t level, uint32_t value);
//void spin2_mc_sync1_max_set(uint32_t v);
//void spin2_mc_sync2_max_set(uint32_t v);

void spin2_mc_wfs();

// --- SDP ---
void spin2_sdp_callback(uint mailbox, uint port);
bool spin2_sdp_callback_on(uint sdp_port, callback_t sdp_callback);
bool spin2_sdp_callback_off(uint sdp_port);

// -- Hash table ---
typedef enum {
  SPIN2_HT_TINY = ((1 << 0x5) - 1),   //  31
  SPIN2_HT_SMALL = ((1 << 0x7) - 1), //  127
  SPIN2_HT_NORMAL = ((1 << 0x8) - 5), //  251
  SPIN2_HT_BIG = ((1 << 0x9) - 3),    //  509
  SPIN2_HT_VBIG = ((1 << 0xA) - 3),   //  1021
  SPIN2_HT_HUGE = ((1 << 0xB) - 9)    //  2039
} spin2_ht_size_t;
typedef struct spin2_hash_table spin2_ht_t;
typedef int32_t (*spin2_ht_compare_fp)(void *, void *);
typedef uint32_t (*spin2_ht_key_fp)(void *);

bool spin2_ht_create(
    spin2_ht_t **st,
    spin2_ht_size_t size,
    spin2_ht_compare_fp f_compare,
    spin2_ht_key_fp f_key
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
bool spin2_ht_remove(
    spin2_ht_t *st,
    uint32_t remove_key,
    void **out_item
);

uint32_t spin2_ht_get_len(spin2_ht_t *st);
uint32_t spin2_ht_get_len_max(spin2_ht_t *st);

//bool spin2_ht_test(uint32_t elements);

#endif //__SPIN2_API_H__
