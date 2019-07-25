












































#include "xpt_arena.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>




#if 0 && defined(DEBUG_jband)
#define XPT_ARENA_LOGGING 1
#endif

#ifdef XPT_ARENA_LOGGING

#define LOG_MALLOC(_a, _req, _used)                             \
    do{                                                         \
    XPT_ASSERT((_a));                                           \
    ++(_a)->LOG_MallocCallCount;                                \
    (_a)->LOG_MallocTotalBytesRequested += (_req);              \
    (_a)->LOG_MallocTotalBytesUsed += (_used);                  \
    } while(0)

#define LOG_REAL_MALLOC(_a, _size)                              \
    do{                                                         \
    XPT_ASSERT((_a));                                           \
    ++(_a)->LOG_RealMallocCallCount;                            \
    (_a)->LOG_RealMallocTotalBytesRequested += (_size);         \
    } while(0)

#define LOG_FREE(_a)                                            \
    do{                                                         \
    XPT_ASSERT((_a));                                           \
    ++(_a)->LOG_FreeCallCount;                                  \
    } while(0)

#define LOG_DONE_LOADING(_a)                                    \
    do{                                                         \
    XPT_ASSERT((_a));                                           \
    (_a)->LOG_LoadingFreeCallCount = (_a)->LOG_FreeCallCount;   \
    } while(0)

#define PRINT_STATS(_a)       xpt_DebugPrintArenaStats((_a))
static void xpt_DebugPrintArenaStats(XPTArena *arena);

#else 

#define LOG_MALLOC(_a, _req, _used)   ((void)0)
#define LOG_REAL_MALLOC(_a, _size)    ((void)0)
#define LOG_FREE(_a)                  ((void)0)

#define LOG_DONE_LOADING(_a)          ((void)0)        
#define PRINT_STATS(_a)               ((void)0)

#endif 




typedef struct BLK_HDR BLK_HDR;
struct BLK_HDR
{
    BLK_HDR *next;
    size_t   size;
};

#define XPT_MIN_BLOCK_SIZE 32


#define ALIGN_RND(s,a) ((a)==1?(s):((((s)+(a)-1)/(a))*(a)))

struct XPTArena
{
    BLK_HDR *first;
    PRUint8 *next;
    size_t   space;
    size_t   alignment;
    size_t   block_size;
    char    *name;

#ifdef XPT_ARENA_LOGGING
    PRUint32 LOG_MallocCallCount;
    PRUint32 LOG_MallocTotalBytesRequested;
    PRUint32 LOG_MallocTotalBytesUsed;
    PRUint32 LOG_FreeCallCount;
    PRUint32 LOG_LoadingFreeCallCount;
    PRUint32 LOG_RealMallocCallCount;
    PRUint32 LOG_RealMallocTotalBytesRequested;
#endif 
};

XPT_PUBLIC_API(XPTArena *)
XPT_NewArena(PRUint32 block_size, size_t alignment, const char* name)
{
    XPTArena *arena = calloc(1, sizeof(XPTArena));
    if (arena) {
        XPT_ASSERT(alignment);
        if (alignment > sizeof(double))
            alignment = sizeof(double);
        arena->alignment = alignment;

        if (block_size < XPT_MIN_BLOCK_SIZE)
            block_size = XPT_MIN_BLOCK_SIZE;
        arena->block_size = ALIGN_RND(block_size, alignment);

        
        XPT_ASSERT(arena->block_size >= 
                   ALIGN_RND(sizeof(BLK_HDR), alignment) +
                   ALIGN_RND(1, alignment));

        if (name) {
            arena->name = XPT_STRDUP(arena, name);           
#ifdef XPT_ARENA_LOGGING
            
            arena->LOG_MallocCallCount = 0;
            arena->LOG_MallocTotalBytesRequested = 0;
            arena->LOG_MallocTotalBytesUsed = 0;
#endif 
        }
    }
    return arena;        
}

XPT_PUBLIC_API(void)
XPT_DestroyArena(XPTArena *arena)
{
    BLK_HDR* cur;
    BLK_HDR* next;
        
    cur = arena->first;
    while (cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }
    free(arena);
}

XPT_PUBLIC_API(void)
XPT_DumpStats(XPTArena *arena)
{
    PRINT_STATS(arena);
}        







XPT_PUBLIC_API(void *)
XPT_ArenaMalloc(XPTArena *arena, size_t size)
{
    PRUint8 *cur;
    size_t bytes;

    if (!size)
        return NULL;

    if (!arena) {
        XPT_ASSERT(0);
        return NULL;
    }

    bytes = ALIGN_RND(size, arena->alignment);
    
    LOG_MALLOC(arena, size, bytes);

    if (bytes > arena->space) {
        BLK_HDR* new_block;
        size_t block_header_size = ALIGN_RND(sizeof(BLK_HDR), arena->alignment);
        size_t new_space = arena->block_size;
         
        while (bytes > new_space - block_header_size)
            new_space += arena->block_size;

        new_block = (BLK_HDR*) calloc(new_space/arena->alignment, 
                                      arena->alignment);
        if (!new_block) {
            arena->next = NULL;
            arena->space = 0;
            return NULL;
        }

        LOG_REAL_MALLOC(arena, new_space);

        
        new_block->next = arena->first;
        arena->first = new_block;

        
        new_block->size = new_space;

        
        arena->next  = ((PRUint8*)new_block) + block_header_size;
        arena->space = new_space - block_header_size;

#ifdef DEBUG
        
        memset(arena->next, 0xcd, arena->space);
#endif
    } 
    
#ifdef DEBUG
    {
        
        size_t i;
        for (i = 0; i < bytes; ++i) {
            XPT_ASSERT(arena->next[i] == 0xcd);        
        }
        
        memset(arena->next, 0, bytes);
    }        
#endif

    cur = arena->next;
    arena->next  += bytes;
    arena->space -= bytes;
    
    return cur;    
}


XPT_PUBLIC_API(char *)
XPT_ArenaStrDup(XPTArena *arena, const char * s)
{
    size_t len;
    char* cur;

    if (!s)
        return NULL;

    len = strlen(s)+1;
    cur = XPT_ArenaMalloc(arena, len);
    memcpy(cur, s, len);
    return cur;
}

XPT_PUBLIC_API(void)
XPT_NotifyDoneLoading(XPTArena *arena)
{
#ifdef XPT_ARENA_LOGGING
    if (arena) {
        LOG_DONE_LOADING(arena);        
    }
#endif
}

XPT_PUBLIC_API(void)
XPT_ArenaFree(XPTArena *arena, void *block)
{
    LOG_FREE(arena);
}

#ifdef XPT_ARENA_LOGGING
static void xpt_DebugPrintArenaStats(XPTArena *arena)
{
    printf("()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()\n");
    printf("Start xpt arena stats for \"%s\"\n", 
            arena->name ? arena->name : "unnamed arena");
    printf("\n");
    printf("%d times arena malloc called\n", (int) arena->LOG_MallocCallCount);       
    printf("%d total bytes requested from arena malloc\n", (int) arena->LOG_MallocTotalBytesRequested);       
    printf("%d average bytes requested per call to arena malloc\n", (int)arena->LOG_MallocCallCount ? (arena->LOG_MallocTotalBytesRequested/arena->LOG_MallocCallCount) : 0);
    printf("%d average bytes used per call (accounts for alignment overhead)\n", (int)arena->LOG_MallocCallCount ? (arena->LOG_MallocTotalBytesUsed/arena->LOG_MallocCallCount) : 0);
    printf("%d average bytes used per call (accounts for all overhead and waste)\n", (int)arena->LOG_MallocCallCount ? (arena->LOG_RealMallocTotalBytesRequested/arena->LOG_MallocCallCount) : 0);
    printf("\n");
    printf("%d during loading times arena free called\n", (int) arena->LOG_LoadingFreeCallCount);       
    printf("%d during loading approx total bytes not freed\n", (int) arena->LOG_LoadingFreeCallCount * (int) (arena->LOG_MallocCallCount ? (arena->LOG_MallocTotalBytesUsed/arena->LOG_MallocCallCount) : 0));       
    printf("\n");
    printf("%d total times arena free called\n", (int) arena->LOG_FreeCallCount);       
    printf("%d approx total bytes not freed until arena destruction\n", (int) arena->LOG_FreeCallCount * (int) (arena->LOG_MallocCallCount ? (arena->LOG_MallocTotalBytesUsed/arena->LOG_MallocCallCount) : 0 ));       
    printf("\n");
    printf("%d times arena called system malloc\n", (int) arena->LOG_RealMallocCallCount);       
    printf("%d total bytes arena requested from system\n", (int) arena->LOG_RealMallocTotalBytesRequested);       
    printf("%d byte block size specified at arena creation time\n", (int) arena->block_size);       
    printf("%d byte block alignment specified at arena creation time\n", (int) arena->alignment);       
    printf("\n");
    printf("End xpt arena stats\n");
    printf("()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()\n");
}        
#endif



#ifdef DEBUG
XPT_PUBLIC_API(void)
XPT_AssertFailed(const char *s, const char *file, PRUint32 lineno)
{
    fprintf(stderr, "Assertion failed: %s, file %s, line %d\n",
            s, file, lineno);
    abort();
}        
#endif

XPT_PUBLIC_API(size_t)
XPT_SizeOfArena(XPTArena *arena, xptMallocSizeOfFun mallocSizeOf)
{
    size_t n = mallocSizeOf(arena);

    




    BLK_HDR* cur;
    BLK_HDR* next;
        
    cur = arena->first;
    while (cur) {
        next = cur->next;
        n += mallocSizeOf(cur);
        cur = next;
    }

    return n;
}
