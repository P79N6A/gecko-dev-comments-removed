
#ifdef JEMALLOC_H_TYPES

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

void	*chunk_alloc_dss(size_t size, size_t alignment, bool *zero);
bool	chunk_in_dss(void *chunk);
bool	chunk_dss_boot(void);
void	chunk_dss_prefork(void);
void	chunk_dss_postfork_parent(void);
void	chunk_dss_postfork_child(void);

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 

