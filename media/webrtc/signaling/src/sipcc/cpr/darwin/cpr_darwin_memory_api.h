






































#ifndef _CPR_DARWIN_MEMORY_API_H_
#define _CPR_DARWIN_MEMORY_API_H_











#define MALLOC   malloc
#define CALLOC   calloc
#define REALLOC  realloc
#define FREE     free

#ifndef memalign
#define MEMALIGN(align, sz) malloc(sz)
#else
#define MEMALIGN(align, sz) memalign(align, sz)
#endif





#define BLK_SZ  1024



#define NUM_BLK 5120





#define PRIVATE_SYS_MEM_SIZE (NUM_BLK * BLK_SZ)














boolean cpr_memory_mgmt_pre_init(size_t size);










boolean cpr_memory_mgmt_post_init(void);









void cpr_memory_mgmt_destroy(void);








void cpr_crashdump(void);


#endif
