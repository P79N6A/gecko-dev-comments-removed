
#ifdef JEMALLOC_H_TYPES

typedef struct extent_node_s extent_node_t;

#endif 

#ifdef JEMALLOC_H_STRUCTS


struct extent_node_s {
	
	rb_node(extent_node_t)	link_szad;

	
	rb_node(extent_node_t)	link_ad;

	
	prof_ctx_t		*prof_ctx;

	
	void			*addr;

	
	size_t			size;

	
	bool			zeroed;
};
typedef rb_tree(extent_node_t) extent_tree_t;

#endif 

#ifdef JEMALLOC_H_EXTERNS

rb_proto(, extent_tree_szad_, extent_tree_t, extent_node_t)

rb_proto(, extent_tree_ad_, extent_tree_t, extent_node_t)

#endif 

#ifdef JEMALLOC_H_INLINES

#endif 


