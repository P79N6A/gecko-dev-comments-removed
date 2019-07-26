
#ifdef JEMALLOC_H_TYPES

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

bool	pages_purge(void *addr, size_t length);

void	*chunk_alloc_mmap(size_t size, size_t alignment, bool *zero);
bool	chunk_dealloc_mmap(void *chunk, size_t size);

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 

