#define	JEMALLOC_HUGE_C_
#include "jemalloc/internal/jemalloc_internal.h"





static malloc_mutex_t	huge_mtx;




static extent_tree_t	huge;

void *
huge_malloc(tsd_t *tsd, arena_t *arena, size_t size, bool zero, bool try_tcache)
{
	size_t usize;

	usize = s2u(size);
	if (usize == 0) {
		
		return (NULL);
	}

	return (huge_palloc(tsd, arena, usize, chunksize, zero, try_tcache));
}

void *
huge_palloc(tsd_t *tsd, arena_t *arena, size_t usize, size_t alignment,
    bool zero, bool try_tcache)
{
	void *ret;
	extent_node_t *node;
	bool is_zeroed;

	

	
	node = ipalloct(tsd, CACHELINE_CEILING(sizeof(extent_node_t)),
	    CACHELINE, false, try_tcache, NULL);
	if (node == NULL)
		return (NULL);

	



	is_zeroed = zero;
	arena = arena_choose(tsd, arena);
	if (unlikely(arena == NULL) || (ret = arena_chunk_alloc_huge(arena,
	    usize, alignment, &is_zeroed)) == NULL) {
		idalloct(tsd, node, try_tcache);
		return (NULL);
	}

	
	node->addr = ret;
	node->size = usize;
	node->zeroed = is_zeroed;
	node->arena = arena;

	malloc_mutex_lock(&huge_mtx);
	extent_tree_ad_insert(&huge, node);
	malloc_mutex_unlock(&huge_mtx);

	if (zero || (config_fill && unlikely(opt_zero))) {
		if (!is_zeroed)
			memset(ret, 0, usize);
	} else if (config_fill && unlikely(opt_junk_alloc))
		memset(ret, 0xa5, usize);

	return (ret);
}

#ifdef JEMALLOC_JET
#undef huge_dalloc_junk
#define	huge_dalloc_junk JEMALLOC_N(huge_dalloc_junk_impl)
#endif
static void
huge_dalloc_junk(void *ptr, size_t usize)
{

	if (config_fill && have_dss && unlikely(opt_junk_free)) {
		



		if (!config_munmap || (have_dss && chunk_in_dss(ptr)))
			memset(ptr, 0x5a, usize);
	}
}
#ifdef JEMALLOC_JET
#undef huge_dalloc_junk
#define	huge_dalloc_junk JEMALLOC_N(huge_dalloc_junk)
huge_dalloc_junk_t *huge_dalloc_junk = JEMALLOC_N(huge_dalloc_junk_impl);
#endif

static void
huge_ralloc_no_move_similar(void *ptr, size_t oldsize, size_t usize,
    size_t size, size_t extra, bool zero)
{
	size_t usize_next;
	bool zeroed;
	extent_node_t *node, key;
	arena_t *arena;

	
	while (usize < s2u(size+extra) && (usize_next = s2u(usize+1)) < oldsize)
		usize = usize_next;

	if (oldsize == usize)
		return;

	
	if (oldsize > usize) {
		size_t sdiff = CHUNK_CEILING(usize) - usize;
		zeroed = (sdiff != 0) ? !pages_purge((void *)((uintptr_t)ptr +
		    usize), sdiff) : true;
		if (config_fill && unlikely(opt_junk_free)) {
			memset((void *)((uintptr_t)ptr + usize), 0x5a, oldsize -
			    usize);
			zeroed = false;
		}
	} else
		zeroed = true;

	malloc_mutex_lock(&huge_mtx);
	key.addr = ptr;
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);
	assert(node->addr == ptr);
	arena = node->arena;
	
	assert(node->size != usize);
	node->size = usize;
	
	node->zeroed = (node->zeroed && zeroed);
	malloc_mutex_unlock(&huge_mtx);

	arena_chunk_ralloc_huge_similar(arena, ptr, oldsize, usize);

	
	if (oldsize < usize) {
		if (zero || (config_fill && unlikely(opt_zero))) {
			if (!zeroed) {
				memset((void *)((uintptr_t)ptr + oldsize), 0,
				    usize - oldsize);
			}
		} else if (config_fill && unlikely(opt_junk_alloc)) {
			memset((void *)((uintptr_t)ptr + oldsize), 0xa5, usize -
			    oldsize);
		}
	}
}

static void
huge_ralloc_no_move_shrink(void *ptr, size_t oldsize, size_t usize)
{
	size_t sdiff;
	bool zeroed;
	extent_node_t *node, key;
	arena_t *arena;

	sdiff = CHUNK_CEILING(usize) - usize;
	zeroed = (sdiff != 0) ? !pages_purge((void *)((uintptr_t)ptr + usize),
	    sdiff) : true;
	if (config_fill && unlikely(opt_junk_free)) {
		huge_dalloc_junk((void *)((uintptr_t)ptr + usize), oldsize -
		    usize);
		zeroed = false;
	}

	malloc_mutex_lock(&huge_mtx);
	key.addr = ptr;
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);
	assert(node->addr == ptr);
	arena = node->arena;
	
	node->size = usize;
	
	node->zeroed = (node->zeroed && zeroed);
	malloc_mutex_unlock(&huge_mtx);

	
	arena_chunk_ralloc_huge_shrink(arena, ptr, oldsize, usize);
}

static bool
huge_ralloc_no_move_expand(void *ptr, size_t oldsize, size_t size, bool zero) {
	size_t usize;
	extent_node_t *node, key;
	arena_t *arena;
	bool is_zeroed_subchunk, is_zeroed_chunk;

	usize = s2u(size);
	if (usize == 0) {
		
		return (true);
	}

	malloc_mutex_lock(&huge_mtx);
	key.addr = ptr;
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);
	assert(node->addr == ptr);
	arena = node->arena;
	is_zeroed_subchunk = node->zeroed;
	malloc_mutex_unlock(&huge_mtx);

	



	is_zeroed_chunk = zero;

	if (arena_chunk_ralloc_huge_expand(arena, ptr, oldsize, usize,
	     &is_zeroed_chunk))
		return (true);

	malloc_mutex_lock(&huge_mtx);
	
	node->size = usize;
	malloc_mutex_unlock(&huge_mtx);

	if (zero || (config_fill && unlikely(opt_zero))) {
		if (!is_zeroed_subchunk) {
			memset((void *)((uintptr_t)ptr + oldsize), 0,
			    CHUNK_CEILING(oldsize) - oldsize);
		}
		if (!is_zeroed_chunk) {
			memset((void *)((uintptr_t)ptr +
			    CHUNK_CEILING(oldsize)), 0, usize -
			    CHUNK_CEILING(oldsize));
		}
	} else if (config_fill && unlikely(opt_junk_alloc)) {
		memset((void *)((uintptr_t)ptr + oldsize), 0xa5, usize -
		    oldsize);
	}

	return (false);
}

bool
huge_ralloc_no_move(void *ptr, size_t oldsize, size_t size, size_t extra,
    bool zero)
{
	size_t usize;

	
	if (oldsize < chunksize)
		return (true);

	assert(s2u(oldsize) == oldsize);
	usize = s2u(size);
	if (usize == 0) {
		
		return (true);
	}

	



	if (CHUNK_CEILING(oldsize) >= CHUNK_CEILING(usize)
	    && CHUNK_CEILING(oldsize) <= CHUNK_CEILING(size+extra)) {
		huge_ralloc_no_move_similar(ptr, oldsize, usize, size, extra,
		    zero);
		return (false);
	}

	
	if (CHUNK_CEILING(oldsize) >= CHUNK_CEILING(usize)) {
		huge_ralloc_no_move_shrink(ptr, oldsize, usize);
		return (false);
	}

	
	if (huge_ralloc_no_move_expand(ptr, oldsize, size + extra,
	    zero)) {
		if (extra == 0)
			return (true);

		
		return (huge_ralloc_no_move_expand(ptr, oldsize, size, zero));
	}
	return (false);
}

void *
huge_ralloc(tsd_t *tsd, arena_t *arena, void *ptr, size_t oldsize, size_t size,
    size_t extra, size_t alignment, bool zero, bool try_tcache_alloc,
    bool try_tcache_dalloc)
{
	void *ret;
	size_t copysize;

	
	if (!huge_ralloc_no_move(ptr, oldsize, size, extra, zero))
		return (ptr);

	




	if (alignment > chunksize) {
		ret = huge_palloc(tsd, arena, size + extra, alignment, zero,
		    try_tcache_alloc);
	} else {
		ret = huge_malloc(tsd, arena, size + extra, zero,
		    try_tcache_alloc);
	}

	if (ret == NULL) {
		if (extra == 0)
			return (NULL);
		
		if (alignment > chunksize) {
			ret = huge_palloc(tsd, arena, size, alignment, zero,
			    try_tcache_alloc);
		} else {
			ret = huge_malloc(tsd, arena, size, zero,
			    try_tcache_alloc);
		}

		if (ret == NULL)
			return (NULL);
	}

	



	copysize = (size < oldsize) ? size : oldsize;
	memcpy(ret, ptr, copysize);
	isqalloc(tsd, ptr, oldsize, try_tcache_dalloc);
	return (ret);
}

void
huge_dalloc(tsd_t *tsd, void *ptr, bool try_tcache)
{
	extent_node_t *node, key;

	malloc_mutex_lock(&huge_mtx);
	
	key.addr = ptr;
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);
	assert(node->addr == ptr);
	extent_tree_ad_remove(&huge, node);
	malloc_mutex_unlock(&huge_mtx);

	huge_dalloc_junk(node->addr, node->size);
	arena_chunk_dalloc_huge(node->arena, node->addr, node->size);
	idalloct(tsd, node, try_tcache);
}

size_t
huge_salloc(const void *ptr)
{
	size_t ret;
	extent_node_t *node, key;

	malloc_mutex_lock(&huge_mtx);

	
	key.addr = __DECONST(void *, ptr);
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);

	ret = node->size;

	malloc_mutex_unlock(&huge_mtx);

	return (ret);
}

prof_tctx_t *
huge_prof_tctx_get(const void *ptr)
{
	prof_tctx_t *ret;
	extent_node_t *node, key;

	malloc_mutex_lock(&huge_mtx);

	
	key.addr = __DECONST(void *, ptr);
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);

	ret = node->prof_tctx;

	malloc_mutex_unlock(&huge_mtx);

	return (ret);
}

void
huge_prof_tctx_set(const void *ptr, prof_tctx_t *tctx)
{
	extent_node_t *node, key;

	malloc_mutex_lock(&huge_mtx);

	
	key.addr = __DECONST(void *, ptr);
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);

	node->prof_tctx = tctx;

	malloc_mutex_unlock(&huge_mtx);
}

bool
huge_boot(void)
{

	
	if (malloc_mutex_init(&huge_mtx))
		return (true);
	extent_tree_ad_new(&huge);

	return (false);
}

void
huge_prefork(void)
{

	malloc_mutex_prefork(&huge_mtx);
}

void
huge_postfork_parent(void)
{

	malloc_mutex_postfork_parent(&huge_mtx);
}

void
huge_postfork_child(void)
{

	malloc_mutex_postfork_child(&huge_mtx);
}
