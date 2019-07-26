






































#ifndef _CPR_DARWIN_MEMORY_H_
#define _CPR_DARWIN_MEMORY_H_






#define BUF_AVAILABLE 0
#define BUF_USED_FROM_POOL      1
#define BUF_USED_FROM_HEAP      2


typedef struct cprLinuxBuffer {
    int32_t inUse;
} cprLinuxBuffer_t;

#endif  
