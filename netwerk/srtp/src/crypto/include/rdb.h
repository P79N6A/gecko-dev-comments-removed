









#ifndef REPLAY_DB_H
#define REPLAY_DB_H

#include "integers.h"         
#include "datatypes.h"        
#include "err.h"              






typedef struct {
  uint32_t window_start;   
  v128_t bitmask;  
} rdb_t;

#define rdb_bits_in_bitmask (8*sizeof(v128_t))   









err_status_t
rdb_init(rdb_t *rdb);











err_status_t
rdb_check(const rdb_t *rdb, uint32_t rdb_index);  










err_status_t
rdb_add_index(rdb_t *rdb, uint32_t rdb_index);


















err_status_t
rdb_increment(rdb_t *rdb);





uint32_t
rdb_get_value(const rdb_t *rdb);


#endif  
