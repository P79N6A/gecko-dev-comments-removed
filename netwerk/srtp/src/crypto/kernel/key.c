











































#include "key.h"

#define soft_limit 0x10000

err_status_t
key_limit_set(key_limit_t key, const xtd_seq_num_t s) {
#ifdef NO_64BIT_MATH
  if (high32(s) == 0 && low32(s) < soft_limit)
    return err_status_bad_param;
#else
  if (s < soft_limit)
    return err_status_bad_param;
#endif
  key->num_left = s;
  key->state = key_state_normal;
  return err_status_ok;
}

err_status_t
key_limit_clone(key_limit_t original, key_limit_t *new_key) {
  if (original == NULL)
    return err_status_bad_param;
  *new_key = original;
  return err_status_ok;
}

err_status_t
key_limit_check(const key_limit_t key) {
  if (key->state == key_state_expired)
    return err_status_key_expired;
  return err_status_ok;
}

key_event_t
key_limit_update(key_limit_t key) {
#ifdef NO_64BIT_MATH
  if (low32(key->num_left) == 0)
  {
	  
	  key->num_left = make64(high32(key->num_left)-1,low32(key->num_left) - 1);
  }
  else
  {
	  
	  key->num_left = make64(high32(key->num_left),low32(key->num_left) - 1);
  }
  if (high32(key->num_left) != 0 || low32(key->num_left) >= soft_limit) {
    return key_event_normal;   
  }
#else
  key->num_left--;
  if (key->num_left >= soft_limit) {
    return key_event_normal;   
  }
#endif
  if (key->state == key_state_normal) {
    
    key->state = key_state_past_soft_limit;
  }
#ifdef NO_64BIT_MATH
  if (low32(key->num_left) == 0 && high32(key->num_left == 0))
#else
  if (key->num_left < 1)
#endif
  { 
    key->state = key_state_expired;
    return key_event_hard_limit;
  }
   return key_event_soft_limit;
}

