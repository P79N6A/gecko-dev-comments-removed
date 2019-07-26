






































#ifndef _CPR_LINUX_MEMORY_H_
#define _CPR_LINUX_MEMORY_H_

#include "cpr_types.h"
#include <pthread.h>






#define BUF_AVAILABLE 0
#define BUF_USED_FROM_POOL      1
#define BUF_USED_FROM_HEAP      2

#define plat_os_thread_mutex_t pthread_mutex_t
#define plat_os_thread_mutex_init pthread_mutex_init
#define plat_os_thread_mutex_lock pthread_mutex_lock
#define plat_os_thread_mutex_unlock pthread_mutex_unlock
#define plat_os_thread_mutex_destroy pthread_mutex_destroy

typedef struct cprLinuxBuffer {
    int32_t inUse;
} cprLinuxBuffer_t;

#endif  
