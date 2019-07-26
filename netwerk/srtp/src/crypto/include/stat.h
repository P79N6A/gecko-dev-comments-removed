













































#ifndef STAT_H
#define STAT_H

#include "datatypes.h"       
#include "err.h"             
#include "rand_source.h"     

err_status_t
stat_test_monobit(uint8_t *data);

err_status_t
stat_test_poker(uint8_t *data);

err_status_t
stat_test_runs(uint8_t *data);

err_status_t
stat_test_rand_source(rand_source_func_t rs);

err_status_t
stat_test_rand_source_with_repetition(rand_source_func_t source, unsigned num_trials);

#endif 
