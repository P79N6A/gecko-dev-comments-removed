
#ifdef JEMALLOC_H_TYPES

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

void	*base_alloc(size_t size);
void	*base_calloc(size_t number, size_t size);
extent_node_t *base_node_alloc(void);
void	base_node_dealloc(extent_node_t *node);
bool	base_boot(void);
void	base_prefork(void);
void	base_postfork_parent(void);
void	base_postfork_child(void);

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 

