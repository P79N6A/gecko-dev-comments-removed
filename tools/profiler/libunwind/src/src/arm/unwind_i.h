























#ifndef unwind_i_h
#define unwind_i_h

#include <memory.h>
#include <stdint.h>

#include <libunwind-arm.h>

#include "libunwind_i.h"

#define arm_lock			UNW_OBJ(lock)
#define arm_local_resume		UNW_OBJ(local_resume)
#define arm_local_addr_space_init	UNW_OBJ(local_addr_space_init)

extern void arm_local_addr_space_init (void);
extern int arm_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			     void *arg);

#endif 
