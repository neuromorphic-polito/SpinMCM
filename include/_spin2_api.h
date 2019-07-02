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

/*
typedef enum {
  SPIN2_HASH_FUNCTION_FNV1A,
  SPIN2_HASH_FUNCTION_OAT,
  SPIN2_HASH_FUNCTION_MURMUR3,
  SPIN2_HASH_FUNCTION_TOMASWANG
} spin2_ht_function_t;
*/

#endif //__SPIN2_API_PRIVATE_H__
