






































#ifndef _CPR_H_
#define _CPR_H_

#include "cpr_types.h"
#include "cpr_ipc.h"
#include "cpr_locks.h"
#include "cpr_timers.h"
#include "cpr_threads.h"
#include "cpr_debug.h"
#include "cpr_memory.h"

__BEGIN_DECLS 

















cprRC_t
cprPreInit(void);
















cprRC_t
cprPostInit(void);

__END_DECLS 

#endif

