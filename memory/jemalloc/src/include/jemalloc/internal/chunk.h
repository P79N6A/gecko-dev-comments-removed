
#ifdef JEMALLOC_H_TYPES





#define	LG_CHUNK_DEFAULT	22


#define	CHUNK_ADDR2BASE(a)						\
	((void *)((uintptr_t)(a) & ~chunksize_mask))


#define	CHUNK_ADDR2OFFSET(a)						\
	((size_t)((uintptr_t)(a) & chunksize_mask))


#define	CHUNK_CEILING(s)						\
	(((s) + chunksize_mask) & ~chunksize_mask)

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

extern size_t		opt_lg_chunk;


extern malloc_mutex_t	chunks_mtx;

extern chunk_stats_t	stats_chunks;

extern rtree_t		*chunks_rtree;

extern size_t		chunksize;
extern size_t		chunksize_mask; 
extern size_t		chunk_npages;
extern size_t		map_bias; 
extern size_t		arena_maxclass; 

void	*chunk_alloc(size_t size, size_t alignment, bool base, bool *zero);
void	chunk_dealloc(void *chunk, size_t size, bool unmap);
bool	chunk_boot(void);
void	chunk_prefork(void);
void	chunk_postfork_parent(void);
void	chunk_postfork_child(void);

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 


#include "jemalloc/internal/chunk_dss.h"
#include "jemalloc/internal/chunk_mmap.h"
