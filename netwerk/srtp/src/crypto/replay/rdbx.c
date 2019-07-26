












































#include "rdbx.h"










































void
index_init(xtd_seq_num_t *pi) {
#ifdef NO_64BIT_MATH
  *pi = make64(0,0);
#else
  *pi = 0;
#endif
}

void
index_advance(xtd_seq_num_t *pi, sequence_number_t s) {
#ifdef NO_64BIT_MATH
  
  
  *pi = make64(high32(*pi) + (s > ~low32(*pi) ? 1 : 0),low32(*pi) + s);
#else
  *pi += s;
#endif
}















int
index_guess(const xtd_seq_num_t *local,
		   xtd_seq_num_t *guess,
		   sequence_number_t s) {
#ifdef NO_64BIT_MATH
  uint32_t local_roc = ((high32(*local) << 16) |
						(low32(*local) >> 16));
  uint16_t local_seq = (uint16_t) (low32(*local));
#else
  uint32_t local_roc = (uint32_t)(*local >> 16);
  uint16_t local_seq = (uint16_t) *local;
#endif
  uint32_t guess_roc;
  uint16_t guess_seq;
  int difference;

  if (local_seq < seq_num_median) {
    if (s - local_seq > seq_num_median) {
      guess_roc = local_roc - 1;
      difference = seq_num_max - s + local_seq;
    } else {
      guess_roc = local_roc;
      difference = s - local_seq;
    }
  } else {
    if (local_seq - seq_num_median > s) {
      guess_roc = local_roc+1;
      difference = seq_num_max - local_seq + s;
    } else {
      difference = s - local_seq;
      guess_roc = local_roc;
    }
  }
  guess_seq = s;

  
#ifdef NO_64BIT_MATH
  *guess = make64(guess_roc >> 16,
				  (guess_roc << 16) | guess_seq);
#else
  *guess = (((uint64_t) guess_roc) << 16) | guess_seq;
#endif

  return difference;
}











err_status_t
rdbx_init(rdbx_t *rdbx, unsigned long ws) {
  if (ws == 0)
    return err_status_bad_param;

  if (bitvector_alloc(&rdbx->bitmask, ws) != 0)
    return err_status_alloc_fail;

  index_init(&rdbx->index);

  return err_status_ok;
}





err_status_t
rdbx_dealloc(rdbx_t *rdbx) {
  bitvector_dealloc(&rdbx->bitmask);

  return err_status_ok;
}









err_status_t
rdbx_set_roc(rdbx_t *rdbx, uint32_t roc) {
  bitvector_set_to_zero(&rdbx->bitmask);

#ifdef NO_64BIT_MATH
  #error not yet implemented
#else

  
  if (roc < (rdbx->index >> 16))
    return err_status_replay_old;

  rdbx->index &= 0xffff;   
  rdbx->index |= ((uint64_t)roc) << 16;  
#endif

  return err_status_ok;
}







xtd_seq_num_t
rdbx_get_packet_index(const rdbx_t *rdbx) {
  return rdbx->index;   
}







unsigned long
rdbx_get_window_size(const rdbx_t *rdbx) {
  return bitvector_get_length(&rdbx->bitmask);
}






err_status_t
rdbx_check(const rdbx_t *rdbx, int delta) {
  
  if (delta > 0) {       
    return err_status_ok;
  } else if ((int)(bitvector_get_length(&rdbx->bitmask) - 1) + delta < 0) {   
                         
    return err_status_replay_old; 
  } else if (bitvector_get_bit(&rdbx->bitmask, 
			       (int)(bitvector_get_length(&rdbx->bitmask) - 1) + delta) == 1) {
                         
    return err_status_replay_fail;    
  }
 

  return err_status_ok; 
}










err_status_t
rdbx_add_index(rdbx_t *rdbx, int delta) {
  
  if (delta > 0) {
    
    index_advance(&rdbx->index, delta);
    bitvector_left_shift(&rdbx->bitmask, delta);
    bitvector_set_bit(&rdbx->bitmask, bitvector_get_length(&rdbx->bitmask) - 1);
  } else {
    
    bitvector_set_bit(&rdbx->bitmask, bitvector_get_length(&rdbx->bitmask) -1 + delta);
  }

  
  
  return err_status_ok;
}












int
rdbx_estimate_index(const rdbx_t *rdbx,
		    xtd_seq_num_t *guess,
		    sequence_number_t s) {

  







#ifdef NO_64BIT_MATH
  
  if (high32(rdbx->index) > 0 ||
	  low32(rdbx->index) > seq_num_median)
#else
  if (rdbx->index > seq_num_median)
#endif
    return index_guess(&rdbx->index, guess, s);
  
#ifdef NO_64BIT_MATH
  *guess = make64(0,(uint32_t) s);
#else  
  *guess = s;
#endif

#ifdef NO_64BIT_MATH
  return s - (uint16_t) low32(rdbx->index);
#else
  return s - (uint16_t) rdbx->index;
#endif
}
