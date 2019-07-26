












































#ifndef RAND_SOURCE
#define RAND_SOURCE

#include "err.h"
#include "datatypes.h"

err_status_t
rand_source_init(void);




















err_status_t
rand_source_get_octet_string(void *dest, uint32_t length);

err_status_t
rand_source_deinit(void);









typedef err_status_t (*rand_source_func_t)
     (void *dest, uint32_t num_octets);

#endif 
