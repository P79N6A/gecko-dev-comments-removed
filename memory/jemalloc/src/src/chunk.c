#define	JEMALLOC_CHUNK_C_
#include "jemalloc/internal/jemalloc_internal.h"




size_t	opt_lg_chunk = LG_CHUNK_DEFAULT;

malloc_mutex_t	chunks_mtx;
chunk_stats_t	stats_chunks;







static extent_tree_t	chunks_szad;
static extent_tree_t	chunks_ad;

rtree_t		*chunks_rtree;


size_t		chunksize;
size_t		chunksize_mask; 
size_t		chunk_npages;
size_t		map_bias;
size_t		arena_maxclass; 




static void	*chunk_recycle(size_t size, size_t alignment, bool base,
    bool *zero);
static void	chunk_record(void *chunk, size_t size);



static void *
chunk_recycle(size_t size, size_t alignment, bool base, bool *zero)
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
	key.addr = NULL;
	key.size = alloc_size;
	malloc_mutex_lock(&chunks_mtx);
	node = extent_tree_szad_nsearch(&chunks_szad, &key);
	if (node == NULL) {
		malloc_mutex_unlock(&chunks_mtx);
		return (NULL);
	}
	leadsize = ALIGNMENT_CEILING((uintptr_t)node->addr, alignment) -
	    (uintptr_t)node->addr;
	assert(node->size >= leadsize + size);
	trailsize = node->size - leadsize - size;
	ret = (void *)((uintptr_t)node->addr + leadsize);
	
	extent_tree_szad_remove(&chunks_szad, node);
	extent_tree_ad_remove(&chunks_ad, node);
	if (leadsize != 0) {
		
		node->size = leadsize;
		extent_tree_szad_insert(&chunks_szad, node);
		extent_tree_ad_insert(&chunks_ad, node);
		node = NULL;
	}
	if (trailsize != 0) {
		
		if (node == NULL) {
			






			malloc_mutex_unlock(&chunks_mtx);
			node = base_node_alloc();
			if (node == NULL) {
				chunk_dealloc(ret, size, true);
				return (NULL);
			}
			malloc_mutex_lock(&chunks_mtx);
		}
		node->addr = (void *)((uintptr_t)(ret) + size);
		node->size = trailsize;
		extent_tree_szad_insert(&chunks_szad, node);
		extent_tree_ad_insert(&chunks_ad, node);
		node = NULL;
	}
	malloc_mutex_unlock(&chunks_mtx);

	zeroed = false;
	if (node != NULL) {
		if (node->zeroed) {
			zeroed = true;
			*zero = true;
		}
		base_node_dealloc(node);
	}
	if (zeroed == false && *zero) {
		VALGRIND_MAKE_MEM_UNDEFINED(ret, size);
		memset(ret, 0, size);
	}
	return (ret);
}







void *
chunk_alloc(size_t size, size_t alignment, bool base, bool *zero)
{
	void *ret;

	assert(size != 0);
	assert((size & chunksize_mask) == 0);
	assert(alignment != 0);
	assert((alignment & chunksize_mask) == 0);

	ret = chunk_recycle(size, alignment, base, zero);
	if (ret != NULL)
		goto label_return;

	ret = chunk_alloc_mmap(size, alignment, zero);
	if (ret != NULL)
		goto label_return;

	if (config_dss) {
		ret = chunk_alloc_dss(size, alignment, zero);
		if (ret != NULL)
			goto label_return;
	}

	
	ret = NULL;
label_return:
	if (config_ivsalloc && base == false && ret != NULL) {
		if (rtree_set(chunks_rtree, (uintptr_t)ret, ret)) {
			chunk_dealloc(ret, size, true);
			return (NULL);
		}
	}
	if ((config_stats || config_prof) && ret != NULL) {
		bool gdump;
		malloc_mutex_lock(&chunks_mtx);
		if (config_stats)
			stats_chunks.nchunks += (size / chunksize);
		stats_chunks.curchunks += (size / chunksize);
		if (stats_chunks.curchunks > stats_chunks.highchunks) {
			stats_chunks.highchunks = stats_chunks.curchunks;
			if (config_prof)
				gdump = true;
		} else if (config_prof)
			gdump = false;
		malloc_mutex_unlock(&chunks_mtx);
		if (config_prof && opt_prof && opt_prof_gdump && gdump)
			prof_gdump();
	}
	if (config_debug && *zero && ret != NULL) {
		size_t i;
		size_t *p = (size_t *)(uintptr_t)ret;

		VALGRIND_MAKE_MEM_DEFINED(ret, size);
		for (i = 0; i < size / sizeof(size_t); i++)
			assert(p[i] == 0);
	}
	assert(CHUNK_ADDR2BASE(ret) == ret);
	return (ret);
}

static void
chunk_record(void *chunk, size_t size)
{
	bool unzeroed;
	extent_node_t *xnode, *node, *prev, key;

	unzeroed = pages_purge(chunk, size);

	





	xnode = base_node_alloc();

	malloc_mutex_lock(&chunks_mtx);
	key.addr = (void *)((uintptr_t)chunk + size);
	node = extent_tree_ad_nsearch(&chunks_ad, &key);
	
	if (node != NULL && node->addr == key.addr) {
		




		extent_tree_szad_remove(&chunks_szad, node);
		node->addr = chunk;
		node->size += size;
		node->zeroed = (node->zeroed && (unzeroed == false));
		extent_tree_szad_insert(&chunks_szad, node);
		if (xnode != NULL)
			base_node_dealloc(xnode);
	} else {
		
		if (xnode == NULL) {
			





			malloc_mutex_unlock(&chunks_mtx);
			return;
		}
		node = xnode;
		node->addr = chunk;
		node->size = size;
		node->zeroed = (unzeroed == false);
		extent_tree_ad_insert(&chunks_ad, node);
		extent_tree_szad_insert(&chunks_szad, node);
	}

	
	prev = extent_tree_ad_prev(&chunks_ad, node);
	if (prev != NULL && (void *)((uintptr_t)prev->addr + prev->size) ==
	    chunk) {
		




		extent_tree_szad_remove(&chunks_szad, prev);
		extent_tree_ad_remove(&chunks_ad, prev);

		extent_tree_szad_remove(&chunks_szad, node);
		node->addr = prev->addr;
		node->size += prev->size;
		node->zeroed = (node->zeroed && prev->zeroed);
		extent_tree_szad_insert(&chunks_szad, node);

		base_node_dealloc(prev);
	}
	malloc_mutex_unlock(&chunks_mtx);
}

void
chunk_dealloc(void *chunk, size_t size, bool unmap)
{

	assert(chunk != NULL);
	assert(CHUNK_ADDR2BASE(chunk) == chunk);
	assert(size != 0);
	assert((size & chunksize_mask) == 0);

	if (config_ivsalloc)
		rtree_set(chunks_rtree, (uintptr_t)chunk, NULL);
	if (config_stats || config_prof) {
		malloc_mutex_lock(&chunks_mtx);
		stats_chunks.curchunks -= (size / chunksize);
		malloc_mutex_unlock(&chunks_mtx);
	}

	if (unmap) {
		if ((config_dss && chunk_in_dss(chunk)) ||
		    chunk_dealloc_mmap(chunk, size))
			chunk_record(chunk, size);
	}
}

bool
chunk_boot(void)
{

	
	chunksize = (ZU(1) << opt_lg_chunk);
	assert(chunksize >= PAGE);
	chunksize_mask = chunksize - 1;
	chunk_npages = (chunksize >> LG_PAGE);

	if (config_stats || config_prof) {
		if (malloc_mutex_init(&chunks_mtx))
			return (true);
		memset(&stats_chunks, 0, sizeof(chunk_stats_t));
	}
	if (config_dss && chunk_dss_boot())
		return (true);
	extent_tree_szad_new(&chunks_szad);
	extent_tree_ad_new(&chunks_ad);
	if (config_ivsalloc) {
		chunks_rtree = rtree_new((ZU(1) << (LG_SIZEOF_PTR+3)) -
		    opt_lg_chunk);
		if (chunks_rtree == NULL)
			return (true);
	}

	return (false);
}

void
chunk_prefork(void)
{

	malloc_mutex_lock(&chunks_mtx);
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
