
















































#ifndef __MEMORY_H
#define __MEMORY_H

typedef struct MemoryPool_rec MemoryPool;

extern MemoryPool *mem_CreatePool(size_t chunksize, unsigned int align);
extern void mem_FreePool(MemoryPool *);
extern void *mem_Alloc(MemoryPool *p, size_t size);
extern void *mem_Realloc(MemoryPool *p, void *old, size_t oldsize, size_t newsize);
extern int mem_AddCleanup(MemoryPool *p, void (*fn)(void *), void *arg);

#endif 
