#include "_spin2_api.h"

// --- Structure ---
struct spin2_hash_table_node {
  struct spin2_hash_table_node *next;
  void *item;
};

struct spin2_hash_table {
  struct spin2_hash_table_node **head;
  spin2_ht_compare_fp f_compare;
  spin2_ht_key_fp f_key;
  uint32_t len;
  uint32_t len_max;
};

typedef struct spin2_hash_table_node htn_t;
typedef struct spin2_hash_table ht_t;
// --- HASH Functions ---
/**
 * jenkins - full avalanche
 *
 * @param data
 * @return
 */
uint32_t hash(uint32_t data) {
  data = (data + 0x7ed55d16) + (data << 12);
  data = (data ^ 0xc761c23c) ^ (data >> 19);
  data = (data + 0x165667b1) + (data << 5);
  data = (data + 0xd3a2646c) ^ (data << 9);
  data = (data + 0xfd7046c5) + (data << 3);
  data = (data ^ 0xb55a4f09) ^ (data >> 16);
  return data;
}

/**
 *
 * @param data
 * @param length
 * @return
 */
uint32_t hash_fnv1a(uint8_t *data, uint32_t length) {
  return 0;
}

//--- HASH Table Functions ---
/**
 * Hash table create
 *
 * @param st
 * @param f_compare
 * @param f_key
 * @return
 */
bool spin2_ht_create(
    spin2_ht_t **st,
    spin2_ht_size_t size,
    spin2_ht_compare_fp f_compare,
    spin2_ht_key_fp f_key) {
  ht_t *_st;

  _st = sark_alloc(1, sizeof(ht_t));
  _st->len = 0;
  _st->len_max = size;
  _st->f_key = f_key;
  _st->f_compare = f_compare;

  _st->head = sark_alloc(_st->len_max, sizeof(htn_t *));
  for (int i = 0; i < _st->len_max; ++i) {
    _st->head[i] = NULL;
  }

  *st = _st;
  return true;
}

/**
 * Hash table destroy
 *
 * Warning: this function doesn't destroy the items!
 * @param st
 * @return
 */
bool spin2_ht_destroy(spin2_ht_t **st) {
  ht_t *_st;
  htn_t *node;
  htn_t *node_to_free;

  _st = *st;
  if (_st == NULL)
    return false;

  for (int i = 0; i < _st->len_max; ++i) {
    node = _st->head[i];
    while (node != NULL) {
      node_to_free = node;
      node = node->next;
      sark_free(node_to_free);
    }
  }
  sark_free(_st->head);
  sark_free(_st);

  *st = NULL;
  return true;
}

/**
 * Hash table insert
 *
 * @param st
 * @param item
 * @return
 */
bool spin2_ht_insert(spin2_ht_t *st, void *item) {
  uint32_t key;
  htn_t *node;

  key = st->f_key(item);
  key = hash(key) % st->len_max;

  node = sark_alloc(1, sizeof(htn_t));
  node->item = item;
  node->next = st->head[key];

  st->head[key] = node;
  st->len++;
  return true;
}

/**
 * Hash table get
 *
 * @param st
 * @param search_key
 * @param out_item
 * @return
 */
bool spin2_ht_get(spin2_ht_t *st, uint32_t search_key, void **out_item) {
  htn_t *node;
  uint32_t key;

  if (out_item == NULL) {
    return false;
  }

  key = hash(search_key) % st->len_max;
  node = st->head[key];
  *out_item = NULL;

  while (node != NULL) {
    if (st->f_key(node->item) == search_key) {
      *out_item = node->item;
      return true;
    }
    node = node->next;
  }
  return false;
}

/**
 * Hash table get/remove
 *
 * @param st
 * @param search_key
 * @param out_item
 * @return
 */
bool spin2_ht_remove(spin2_ht_t *st, uint32_t remove_key, void **out_item) {
  htn_t *node;
  htn_t *node_back;
  uint32_t key;

  if (out_item == NULL) {
    return false;
  }

  key = hash(remove_key) % st->len_max;
  node = st->head[key];
  node_back = NULL;
  *out_item = NULL;

  while (node != NULL) {
    if (st->f_key(node->item) == remove_key) {
      *out_item = node->item;
      if (node_back == NULL) {
        st->head[key] = node->next;
      } else {
        node_back->next = node->next;
      }
      sark_free(node);
      st->len--;
      return true;
    }
    node_back = node;
    node = node->next;
  }
  return false;
}

/**
 *
 * @param st
 * @return
 */
uint32_t spin2_ht_get_len(spin2_ht_t *st) {
  return st->len;
}

/**
 *
 * @param st
 * @return
 */
uint32_t spin2_ht_get_len_max(spin2_ht_t *st) {
  return st->len_max;
}
