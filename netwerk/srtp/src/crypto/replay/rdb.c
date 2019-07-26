













































#include "rdb.h"













err_status_t
rdb_init(rdb_t *rdb) {
  v128_set_to_zero(&rdb->bitmask);
  rdb->window_start = 0;
  return err_status_ok;
}





err_status_t
rdb_check(const rdb_t *rdb, uint32_t p_index) {
  
  
  if (p_index >= rdb->window_start + rdb_bits_in_bitmask)
    return err_status_ok;
  
  
  if (p_index < rdb->window_start)
    return err_status_replay_old;

  
  if (v128_get_bit(&rdb->bitmask, (p_index - rdb->window_start)) == 1)
    return err_status_replay_fail;    
      
  
  return err_status_ok;
}










err_status_t
rdb_add_index(rdb_t *rdb, uint32_t p_index) {
  int delta;  

  

  delta = (p_index - rdb->window_start);    
  if (delta < rdb_bits_in_bitmask) {

    
    v128_set_bit(&rdb->bitmask, delta);

  } else { 
    
    delta -= rdb_bits_in_bitmask - 1;

    
    v128_left_shift(&rdb->bitmask, delta);
    v128_set_bit(&rdb->bitmask, rdb_bits_in_bitmask-1);
    rdb->window_start += delta;

  }    

  return err_status_ok;
}

err_status_t
rdb_increment(rdb_t *rdb) {

  if (rdb->window_start++ > 0x7fffffff)
    return err_status_key_expired;
  return err_status_ok;
}

uint32_t
rdb_get_value(const rdb_t *rdb) {
  return rdb->window_start;
}
