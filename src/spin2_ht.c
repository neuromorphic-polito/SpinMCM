#include "_spin2_api.h"


// --- Structures ---
struct spin2_hash_table_node {
  struct spin2_hash_table_node *next;
  void *item;
}; // 8 Byte

struct spin2_hash_table {
  struct spin2_hash_table_node **head;
  spin2_ht_compare_fp f_compare;
  spin2_ht_item_free f_free;
  spin2_ht_key_fp f_key;
  uint32_t len;
  uint32_t len_max;
}; // 24 Byte


// --- Types ---
typedef struct spin2_hash_table_node htn_t;
typedef struct spin2_hash_table ht_t;


// --- HASH Functions ---

/**
 * Jenkins - Full Avalanche
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


// --- HASH Table Functions ---

/**
 * Hash table create
 *
 * @param st
 * @param size
 * @param f_compare
 * @param f_key
 * @return
 */
bool spin2_ht_create(
    spin2_ht_t **st,
    spin2_ht_size_t size,
    spin2_ht_compare_fp f_compare,
    spin2_ht_key_fp f_key,
    spin2_ht_item_free f_free) {
  
  int i;
  ht_t *_st;

  _st = sark_alloc(1, sizeof(ht_t));
  _st->len = 0;
  _st->len_max = size;
  _st->f_key = f_key;
  _st->f_compare = f_compare;
  _st->f_free = f_free;

  _st->head = sark_alloc(_st->len_max, sizeof(htn_t *));
  for (i = 0; i < _st->len_max; ++i) {
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
  
  int i;
  ht_t *_st;
  htn_t *node;
  htn_t *node_to_free;

  _st = *st;
  if (_st == NULL)
    return false;

  for (i = 0; i < _st->len_max; ++i) {
    node = _st->head[i];
    while (node != NULL) {
      node_to_free = node;
      node = node->next;
      if(_st->f_free != NULL)
        _st->f_free(node_to_free->item);
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

  if (st == NULL){
    error_printf("[SPIN2-HT] Error: hash table pointer is NULL\n");
    rt_error(RTE_ABORT);
  }

  key = st->f_key(item);
  key = hash(key) % st->len_max;

  node = sark_alloc(1, sizeof(htn_t));
  node->item = item;
  node->next = st->head[key];

  st->head[key] = node;
  st->len++;
  return true;
}


htn_t * _ht_search(spin2_ht_t *st, uint32_t search_key) {
  
  htn_t *node;
  uint32_t key;

  if (st == NULL){
    error_printf("[SPIN2-HT] Error: hash table pointer is NULL\n");
    rt_error(RTE_ABORT);
  }

  key = hash(search_key) % st->len_max;
  node = st->head[key];

  while (node != NULL) {
    if (st->f_key(node->item) == search_key) {
      return node;
    }
    node = node->next;
  }

  return NULL;
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

  if (st == NULL){
    error_printf("[SPIN2-HT] Error: hash table pointer is NULL\n");
    rt_error(RTE_ABORT);
  }

  if (out_item == NULL) {
    return false;
  }

  node = _ht_search(st, search_key);
  if (node != NULL){
    *out_item = node->item;
    return true;
  }
  return false;
}

/**
 * Hash table exist
 *
 * @param st
 * @param search_key
 * @return
 */
bool spin2_ht_exist(spin2_ht_t *st, uint32_t search_key) {

  return _ht_search(st, search_key);
}


/**
 * Hash table get and remove (pop)
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

  if (st == NULL){
    error_printf("[SPIN2-HT] Error: hash table pointer is NULL\n");
    rt_error(RTE_ABORT);
  }

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
      }
      else {
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
