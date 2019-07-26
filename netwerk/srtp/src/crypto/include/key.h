











































#ifndef KEY_H
#define KEY_H

#include "rdbx.h"   
#include "err.h"

typedef struct key_limit_ctx_t *key_limit_t;

typedef enum {
   key_event_normal,
   key_event_soft_limit,
   key_event_hard_limit
} key_event_t;

err_status_t
key_limit_set(key_limit_t key, const xtd_seq_num_t s);

err_status_t
key_limit_clone(key_limit_t original, key_limit_t *new_key);

err_status_t
key_limit_check(const key_limit_t key);

key_event_t
key_limit_update(key_limit_t key);

typedef enum { 
   key_state_normal,
   key_state_past_soft_limit,
   key_state_expired
} key_state_t;

typedef struct key_limit_ctx_t {
  xtd_seq_num_t num_left;
  key_state_t   state;
} key_limit_ctx_t;

#endif 
