




#ifndef PLARENAS_H
#define PLARENAS_H

PR_BEGIN_EXTERN_C

typedef struct PLArenaPool      PLArenaPool;










PR_EXTERN(void) PL_InitArenaPool(
    PLArenaPool *pool, const char *name, PRUint32 size, PRUint32 align);




PR_EXTERN(void) PL_ArenaFinish(void);






PR_EXTERN(void) PL_FreeArenaPool(PLArenaPool *pool);




PR_EXTERN(void) PL_FinishArenaPool(PLArenaPool *pool);





PR_EXTERN(void) PL_CompactArenaPool(PLArenaPool *pool);




PR_EXTERN(void *) PL_ArenaAllocate(PLArenaPool *pool, PRUint32 nb);

PR_EXTERN(void *) PL_ArenaGrow(
    PLArenaPool *pool, void *p, PRUint32 size, PRUint32 incr);

PR_EXTERN(void) PL_ArenaRelease(PLArenaPool *pool, char *mark);




PR_EXTERN(void) PL_ClearArenaPool(PLArenaPool *pool, PRInt32 pattern);





typedef size_t (*PLMallocSizeFn)(const void *ptr);





PR_EXTERN(size_t) PL_SizeOfArenaPoolExcludingPool(
    const PLArenaPool *pool, PLMallocSizeFn mallocSizeOf);

PR_END_EXTERN_C

#endif 
