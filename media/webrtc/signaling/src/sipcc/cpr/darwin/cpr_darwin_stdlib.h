






































#ifndef _CPR_DARWIN_STDLIB_H_
#define _CPR_DARWIN_STDLIB_H_

#include "stdlib.h"
#include "string.h"

boolean cpr_mem_high_water_mark(void);
#define CPR_REACH_MEMORY_HIGH_WATER_MARK cpr_mem_high_water_mark()

#endif
