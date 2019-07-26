












































#ifndef RDBX_H
#define RDBX_H

#include "datatypes.h"
#include "err.h"

  

#ifndef ROC_TEST

typedef uint16_t sequence_number_t;   
typedef uint32_t rollover_counter_t;   

#else  

typedef unsigned char sequence_number_t;         
typedef uint16_t rollover_counter_t;   

#endif

#define seq_num_median (1 << (8*sizeof(sequence_number_t) - 1))
#define seq_num_max    (1 << (8*sizeof(sequence_number_t)))






typedef uint64_t xtd_seq_num_t;







typedef struct {
  xtd_seq_num_t index;
  bitvector_t bitmask;
} rdbx_t;









err_status_t
rdbx_init(rdbx_t *rdbx, unsigned long ws);








err_status_t
rdbx_dealloc(rdbx_t *rdbx);











int
rdbx_estimate_index(const rdbx_t *rdbx,
		    xtd_seq_num_t *guess,
		    sequence_number_t s);









err_status_t
rdbx_check(const rdbx_t *rdbx, int difference);












err_status_t
rdbx_add_index(rdbx_t *rdbx, int delta);










err_status_t
rdbx_set_roc(rdbx_t *rdbx, uint32_t roc);







xtd_seq_num_t
rdbx_get_packet_index(const rdbx_t *rdbx);













unsigned long
rdbx_get_window_size(const rdbx_t *rdbx);




void
index_init(xtd_seq_num_t *pi);



void
index_advance(xtd_seq_num_t *pi, sequence_number_t s);












int
index_guess(const xtd_seq_num_t *local,
		   xtd_seq_num_t *guess,
		   sequence_number_t s);


#endif 









