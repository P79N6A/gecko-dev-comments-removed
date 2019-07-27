#define	JEMALLOC_CHUNK_C_
#include "jemalloc/internal/jemalloc_internal.h"




const char	*opt_dss = DSS_DEFAULT;
size_t		opt_lg_chunk = LG_CHUNK_DEFAULT;

malloc_mutex_t	chunks_mtx;
chunk_stats_t	stats_chunks;







static extent_tree_t	chunks_szad_mmap;
static extent_tree_t	chunks_ad_mmap;
static extent_tree_t	chunks_szad_dss;
static extent_tree_t	chunks_ad_dss;

rtree_t		*chunks_rtree;


size_t		chunksize;
size_t		chunksize_mask; 
size_t		chunk_npages;







static void	chunk_dalloc_core(void *chunk, size_t size);



static void *
chunk_recycle(extent_tree_t *chunks_szad, extent_tree_t *chunks_ad,
    void *new_addr, size_t size, size_t alignment, bool base, bool *zero)
{
	void *ret;
	extent_node_t *node;
	extent_node_t key;
	size_t alloc_size, leadsize, trailsize;
	bool zeroed;

	if (base) {
		





		return (NULL);
	}

	alloc_size = size + alignment - chunksize;
	
	if (alloc_size < size)
		return (NULL);
	key.addr = new_addr;
	key.size = alloc_size;
	malloc_mutex_lock(&chunks_mtx);
	node = extent_tree_szad_nsearch(chunks_szad, &key);
	if (node == NULL || (new_addr && node->addr != new_addr)) {
		malloc_mutex_unlock(&chunks_mtx);
		return (NULL);
	}
	leadsize = ALIGNMENT_CEILING((uintptr_t)node->addr, alignment) -
	    (uintptr_t)node->addr;
	assert(node->size >= leadsize + size);
	trailsize = node->size - leadsize - size;
	ret = (void *)((uintptr_t)node->addr + leadsize);
	zeroed = node->zeroed;
	if (zeroed)
	    *zero = true;
	
	extent_tree_szad_remove(chunks_szad, node);
	extent_tree_ad_remove(chunks_ad, node);
	if (leadsize != 0) {
		
		node->size = leadsize;
		extent_tree_szad_insert(chunks_szad, node);
		extent_tree_ad_insert(chunks_ad, node);
		node = NULL;
	}
	if (trailsize != 0) {
		
		if (node == NULL) {
			






			malloc_mutex_unlock(&chunks_mtx);
			node = base_node_alloc();
			if (node == NULL) {
				chunk_dalloc_core(ret, size);
				return (NULL);
			}
			malloc_mutex_lock(&chunks_mtx);
		}
		node->addr = (void *)((uintptr_t)(ret) + size);
		node->size = trailsize;
		node->zeroed = zeroed;
		extent_tree_szad_insert(chunks_szad, node);
		extent_tree_ad_insert(chunks_ad, node);
		node = NULL;
	}
	malloc_mutex_unlock(&chunks_mtx);

	if (node != NULL)
		base_node_dalloc(node);
	if (*zero) {
		if (!zeroed)
			memset(ret, 0, size);
		else if (config_debug) {
			size_t i;
			size_t *p = (size_t *)(uintptr_t)ret;

			JEMALLOC_VALGRIND_MAKE_MEM_DEFINED(ret, size);
			for (i = 0; i < size / sizeof(size_t); i++)
				assert(p[i] == 0);
		}
	}
	return (ret);
}







static void *
chunk_alloc_core(void *new_addr, size_t size, size_t alignment, bool base,
    bool *zero, dss_prec_t dss_prec)
{
	void *ret;

	assert(size != 0);
	assert((size & chunksize_mask) == 0);
	assert(alignment != 0);
	assert((alignment & chunksize_mask) == 0);

	
	if (have_dss && dss_prec == dss_prec_primary) {
		if ((ret = chunk_recycle(&chunks_szad_dss, &chunks_ad_dss,
		    new_addr, size, alignment, base, zero)) != NULL)
			return (ret);
		if ((ret = chunk_alloc_dss(new_addr, size, alignment, zero))
		    != NULL)
			return (ret);
	}
	
	if ((ret = chunk_recycle(&chunks_szad_mmap, &chunks_ad_mmap, new_addr,
	    size, alignment, base, zero)) != NULL)
		return (ret);
	
	if (new_addr == NULL &&
	    (ret = chunk_alloc_mmap(size, alignment, zero)) != NULL)
		return (ret);
	
	if (have_dss && dss_prec == dss_prec_secondary) {
		if ((ret = chunk_recycle(&chunks_szad_dss, &chunks_ad_dss,
		    new_addr, size, alignment, base, zero)) != NULL)
			return (ret);
		if ((ret = chunk_alloc_dss(new_addr, size, alignment, zero))
		    != NULL)
			return (ret);
	}

	
	return (NULL);
}

static bool
chunk_register(void *chunk, size_t size, bool base)
{

	assert(chunk != NULL);
	assert(CHUNK_ADDR2BASE(chunk) == chunk);

	if (config_ivsalloc && !base) {
		if (rtree_set(chunks_rtree, (uintptr_t)chunk, 1))
			return (true);
	}
	if (config_stats || config_prof) {
		bool gdump;
		malloc_mutex_lock(&chunks_mtx);
		if (config_stats)
			stats_chunks.nchunks += (size / chunksize);
		stats_chunks.curchunks += (size / chunksize);
		if (stats_chunks.curchunks > stats_chunks.highchunks) {
			stats_chunks.highchunks =
			    stats_chunks.curchunks;
			if (config_prof)
				gdump = true;
		} else if (config_prof)
			gdump = false;
		malloc_mutex_unlock(&chunks_mtx);
		if (config_prof && opt_prof && opt_prof_gdump && gdump)
			prof_gdump();
	}
	if (config_valgrind)
		JEMALLOC_VALGRIND_MAKE_MEM_UNDEFINED(chunk, size);
	return (false);
}

void *
chunk_alloc_base(size_t size)
{
	void *ret;
	bool zero;

	zero = false;
	ret = chunk_alloc_core(NULL, size, chunksize, true, &zero,
	    chunk_dss_prec_get());
	if (ret == NULL)
		return (NULL);
	if (chunk_register(ret, size, true)) {
		chunk_dalloc_core(ret, size);
		return (NULL);
	}
	return (ret);
}

void *
chunk_alloc_arena(chunk_alloc_t *chunk_alloc, chunk_dalloc_t *chunk_dalloc,
    unsigned arena_ind, void *new_addr, size_t size, size_t alignment,
    bool *zero)
{
	void *ret;

	ret = chunk_alloc(new_addr, size, alignment, zero, arena_ind);
	if (ret != NULL && chunk_register(ret, size, false)) {
		chunk_dalloc(ret, size, arena_ind);
		ret = NULL;
	}

	return (ret);
}


void *
chunk_alloc_default(void *new_addr, size_t size, size_t alignment, bool *zero,
    unsigned arena_ind)
{
	arena_t *arena;

	arena = arena_get(tsd_fetch(), arena_ind, false, true);
	



	assert(arena != NULL);

	return (chunk_alloc_core(new_addr, size, alignment, false, zero,
	    arena->dss_prec));
}

static void
chunk_record(extent_tree_t *chunks_szad, extent_tree_t *chunks_ad, void *chunk,
    size_t size)
{
	bool unzeroed;
	extent_node_t *xnode, *node, *prev, *xprev, key;

	unzeroed = pages_purge(chunk, size);
	JEMALLOC_VALGRIND_MAKE_MEM_NOACCESS(chunk, size);

	





	xnode = base_node_alloc();
	
	xprev = NULL;

	malloc_mutex_lock(&chunks_mtx);
	key.addr = (void *)((uintptr_t)chunk + size);
	node = extent_tree_ad_nsearch(chunks_ad, &key);
	
	if (node != NULL && node->addr == key.addr) {
		




		extent_tree_szad_remove(chunks_szad, node);
		node->addr = chunk;
		node->size += size;
		node->zeroed = (node->zeroed && !unzeroed);
		extent_tree_szad_insert(chunks_szad, node);
	} else {
		
		if (xnode == NULL) {
			





			goto label_return;
		}
		node = xnode;
		xnode = NULL; 
		node->addr = chunk;
		node->size = size;
		node->zeroed = !unzeroed;
		extent_tree_ad_insert(chunks_ad, node);
		extent_tree_szad_insert(chunks_szad, node);
	}

	
	prev = extent_tree_ad_prev(chunks_ad, node);
	if (prev != NULL && (void *)((uintptr_t)prev->addr + prev->size) ==
	    chunk) {
		




		extent_tree_szad_remove(chunks_szad, prev);
		extent_tree_ad_remove(chunks_ad, prev);

		extent_tree_szad_remove(chunks_szad, node);
		node->addr = prev->addr;
		node->size += prev->size;
		node->zeroed = (node->zeroed && prev->zeroed);
		extent_tree_szad_insert(chunks_szad, node);

		xprev = prev;
	}

label_return:
	malloc_mutex_unlock(&chunks_mtx);
	



	if (xnode != NULL)
		base_node_dalloc(xnode);
	if (xprev != NULL)
		base_node_dalloc(xprev);
}

void
chunk_unmap(void *chunk, size_t size)
{
	assert(chunk != NULL);
	assert(CHUNK_ADDR2BASE(chunk) == chunk);
	assert(size != 0);
	assert((size & chunksize_mask) == 0);

	if (have_dss && chunk_in_dss(chunk))
		chunk_record(&chunks_szad_dss, &chunks_ad_dss, chunk, size);
	else if (chunk_dalloc_mmap(chunk, size))
		chunk_record(&chunks_szad_mmap, &chunks_ad_mmap, chunk, size);
}

static void
chunk_dalloc_core(void *chunk, size_t size)
{

	assert(chunk != NULL);
	assert(CHUNK_ADDR2BASE(chunk) == chunk);
	assert(size != 0);
	assert((size & chunksize_mask) == 0);

	if (config_ivsalloc)
		rtree_set(chunks_rtree, (uintptr_t)chunk, 0);
	if (config_stats || config_prof) {
		malloc_mutex_lock(&chunks_mtx);
		assert(stats_chunks.curchunks >= (size / chunksize));
		stats_chunks.curchunks -= (size / chunksize);
		malloc_mutex_unlock(&chunks_mtx);
	}

	chunk_unmap(chunk, size);
}


bool
chunk_dalloc_default(void *chunk, size_t size, unsigned arena_ind)
{

	chunk_dalloc_core(chunk, size);
	return (false);
}

bool
chunk_boot(void)
{

	
	chunksize = (ZU(1) << opt_lg_chunk);
	assert(chunksize >= PAGE);
	chunksize_mask = chunksize - 1;
	chunk_npages = (chunksize >> LG_PAGE);

	if (malloc_mutex_init(&chunks_mtx))
		return (true);
	if (config_stats || config_prof)
		memset(&stats_chunks, 0, sizeof(chunk_stats_t));
	if (have_dss && chunk_dss_boot())
		return (true);
	extent_tree_szad_new(&chunks_szad_mmap);
	extent_tree_ad_new(&chunks_ad_mmap);
	extent_tree_szad_new(&chunks_szad_dss);
	extent_tree_ad_new(&chunks_ad_dss);
	if (config_ivsalloc) {
		chunks_rtree = rtree_new((ZU(1) << (LG_SIZEOF_PTR+3)) -
		    opt_lg_chunk, base_alloc, NULL);
		if (chunks_rtree == NULL)
			return (true);
	}

	return (false);
}

void
chunk_prefork(void)
{

	malloc_mutex_prefork(&chunks_mtx);
	if (config_ivsalloc)
		rtree_prefork(chunks_rtree);
	chunk_dss_prefork();
}

void
chunk_postfork_parent(void)
{

	chunk_dss_postfork_parent();
	if (config_ivsalloc)
		rtree_postfork_parent(chunks_rtree);
	malloc_mutex_postfork_parent(&chunks_mtx);
}

void
chunk_postfork_child(void)
{

	chunk_dss_postfork_child();
	if (config_ivsalloc)
		rtree_postfork_child(chunks_rtree);
	malloc_mutex_postfork_child(&chunks_mtx);
}
