















































#ifndef UT_SIM_H
#define UT_SIM_H

#include "integers.h"  

#define UT_BUF 160      /* maximum amount of packet reorder */

typedef struct {
  uint32_t index;
  uint32_t buffer[UT_BUF];
} ut_connection;








void
ut_init(ut_connection *utc);






uint32_t
ut_next_index(ut_connection *utc);


#endif 
