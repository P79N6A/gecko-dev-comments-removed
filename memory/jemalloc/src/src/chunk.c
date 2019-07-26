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
size_t		map_bias;
size_t		arena_maxclass; 




static void	*chunk_recycle(extent_tree_t *chunks_szad,
    extent_tree_t *chunks_ad, size_t size, size_t alignment, bool base,
    bool *zero);
static void	chunk_record(extent_tree_t *chunks_szad,
    extent_tree_t *chunks_ad, void *chunk, size_t size);



static void *
chunk_recycle(extent_tree_t *chunks_szad, extent_tree_t *chunks_ad, size_t size,
    size_t alignment, bool base, bool *zero)
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
	node = extent_tree_szad_nsearch(chunks_szad, &key);
	if (node == NULL) {
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
				chunk_dealloc(ret, size, true);
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
		base_node_dealloc(node);
	if (*zero) {
		if (zeroed == false)
			memset(ret, 0, size);
		else if (config_debug) {
			size_t i;
			size_t *p = (size_t *)(uintptr_t)ret;

			VALGRIND_MAKE_MEM_DEFINED(ret, size);
			for (i = 0; i < size / sizeof(size_t); i++)
				assert(p[i] == 0);
		}
	}
	return (ret);
}







void *
chunk_alloc(size_t size, size_t alignment, bool base, bool *zero,
    dss_prec_t dss_prec)
{
	void *ret;

	assert(size != 0);
	assert((size & chunksize_mask) == 0);
	assert(alignment != 0);
	assert((alignment & chunksize_mask) == 0);

	
	if (config_dss && dss_prec == dss_prec_primary) {
		if ((ret = chunk_recycle(&chunks_szad_dss, &chunks_ad_dss, size,
		    alignment, base, zero)) != NULL)
			goto label_return;
		if ((ret = chunk_alloc_dss(size, alignment, zero)) != NULL)
			goto label_return;
	}
	
	if ((ret = chunk_recycle(&chunks_szad_mmap, &chunks_ad_mmap, size,
	    alignment, base, zero)) != NULL)
		goto label_return;
	if ((ret = chunk_alloc_mmap(size, alignment, zero)) != NULL)
		goto label_return;
	
	if (config_dss && dss_prec == dss_prec_secondary) {
		if ((ret = chunk_recycle(&chunks_szad_dss, &chunks_ad_dss, size,
		    alignment, base, zero)) != NULL)
			goto label_return;
		if ((ret = chunk_alloc_dss(size, alignment, zero)) != NULL)
			goto label_return;
	}

	
	ret = NULL;
label_return:
	if (ret != NULL) {
		if (config_ivsalloc && base == false) {
			if (rtree_set(chunks_rtree, (uintptr_t)ret, 1)) {
				chunk_dealloc(ret, size, true);
				return (NULL);
			}
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
			VALGRIND_MAKE_MEM_UNDEFINED(ret, size);
	}
	assert(CHUNK_ADDR2BASE(ret) == ret);
	return (ret);
}

static void
chunk_record(extent_tree_t *chunks_szad, extent_tree_t *chunks_ad, void *chunk,
    size_t size)
{
	bool unzeroed;
	extent_node_t *xnode, *node, *prev, *xprev, key;

	unzeroed = pages_purge(chunk, size);
	VALGRIND_MAKE_MEM_NOACCESS(chunk, size);

	





	xnode = base_node_alloc();
	
	xprev = NULL;

	malloc_mutex_lock(&chunks_mtx);
	key.addr = (void *)((uintptr_t)chunk + size);
	node = extent_tree_ad_nsearch(chunks_ad, &key);
	
	if (node != NULL && node->addr == key.addr) {
		




		extent_tree_szad_remove(chunks_szad, node);
		node->addr = chunk;
		node->size += size;
		node->zeroed = (node->zeroed && (unzeroed == false));
		extent_tree_szad_insert(chunks_szad, node);
	} else {
		
		if (xnode == NULL) {
			





			goto label_return;
		}
		node = xnode;
		xnode = NULL; 
		node->addr = chunk;
		node->size = size;
		node->zeroed = (unzeroed == false);
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
		base_node_dealloc(xnode);
	if (xprev != NULL)
		base_node_dealloc(xprev);
}

void
chunk_unmap(void *chunk, size_t size)
{
	assert(chunk != NULL);
	assert(CHUNK_ADDR2BASE(chunk) == chunk);
	assert(size != 0);
	assert((size & chunksize_mask) == 0);

	if (config_dss && chunk_in_dss(chunk))
		chunk_record(&chunks_szad_dss, &chunks_ad_dss, chunk, size);
	else if (chunk_dealloc_mmap(chunk, size))
		chunk_record(&chunks_szad_mmap, &chunks_ad_mmap, chunk, size);
}

void
chunk_dealloc(void *chunk, size_t size, bool unmap)
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

	if (unmap)
		chunk_unmap(chunk, size);
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
