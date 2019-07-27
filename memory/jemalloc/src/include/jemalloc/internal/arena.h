
#ifdef JEMALLOC_H_TYPES

#define	LARGE_MINCLASS		(ZU(1) << LG_LARGE_MINCLASS)


#define	LG_RUN_MAXREGS		(LG_PAGE - LG_TINY_MIN)
#define	RUN_MAXREGS		(1U << LG_RUN_MAXREGS)





#define	REDZONE_MINSIZE		16









#define	LG_DIRTY_MULT_DEFAULT	3

typedef struct arena_run_s arena_run_t;
typedef struct arena_chunk_map_bits_s arena_chunk_map_bits_t;
typedef struct arena_chunk_map_misc_s arena_chunk_map_misc_t;
typedef struct arena_chunk_s arena_chunk_t;
typedef struct arena_bin_info_s arena_bin_info_t;
typedef struct arena_bin_s arena_bin_t;
typedef struct arena_s arena_t;

#endif 

#ifdef JEMALLOC_H_STRUCTS

struct arena_run_s {
	
	index_t		binind;

	
	unsigned	nfree;

	
	bitmap_t	bitmap[BITMAP_GROUPS_MAX];
};


struct arena_chunk_map_bits_s {
	
























































	size_t				bits;
#define	CHUNK_MAP_BININD_SHIFT	4
#define	BININD_INVALID		((size_t)0xffU)

#define	CHUNK_MAP_BININD_MASK	((size_t)0xff0U)
#define	CHUNK_MAP_BININD_INVALID CHUNK_MAP_BININD_MASK
#define	CHUNK_MAP_FLAGS_MASK	((size_t)0xcU)
#define	CHUNK_MAP_DIRTY		((size_t)0x8U)
#define	CHUNK_MAP_UNZEROED	((size_t)0x4U)
#define	CHUNK_MAP_LARGE		((size_t)0x2U)
#define	CHUNK_MAP_ALLOCATED	((size_t)0x1U)
#define	CHUNK_MAP_KEY		CHUNK_MAP_ALLOCATED
};






struct arena_chunk_map_misc_s {
	






	rb_node(arena_chunk_map_misc_t)		rb_link;

	union {
		
		ql_elm(arena_chunk_map_misc_t)	dr_link;

		
		prof_tctx_t			*prof_tctx;

		
		arena_run_t			run;
	};
};
typedef rb_tree(arena_chunk_map_misc_t) arena_avail_tree_t;
typedef rb_tree(arena_chunk_map_misc_t) arena_run_tree_t;
typedef ql_head(arena_chunk_map_misc_t) arena_chunk_miscelms_t;


struct arena_chunk_s {
	
	arena_t			*arena;

	





	arena_chunk_map_bits_t	map_bits[1]; 
};


































struct arena_bin_info_s {
	
	size_t		reg_size;

	
	size_t		redzone_size;

	
	size_t		reg_interval;

	
	size_t		run_size;

	
	uint32_t	nregs;

	



	bitmap_info_t	bitmap_info;

	
	uint32_t	reg0_offset;
};

struct arena_bin_s {
	





	malloc_mutex_t	lock;

	



	arena_run_t	*runcur;

	






	arena_run_tree_t runs;

	
	malloc_bin_stats_t stats;
};

struct arena_s {
	
	unsigned		ind;

	



	unsigned		nthreads;

	






	malloc_mutex_t		lock;

	arena_stats_t		stats;
	



	ql_head(tcache_t)	tcache_ql;

	uint64_t		prof_accumbytes;

	dss_prec_t		dss_prec;

	









	arena_chunk_t		*spare;

	
	size_t			nactive;

	





	size_t			ndirty;

	



	arena_avail_tree_t	runs_avail;

	
	arena_chunk_miscelms_t	runs_dirty;

	


	chunk_alloc_t		*chunk_alloc;
	chunk_dalloc_t		*chunk_dalloc;

	
	arena_bin_t		bins[NBINS];
};

#endif 

#ifdef JEMALLOC_H_EXTERNS

extern ssize_t	opt_lg_dirty_mult;

extern arena_bin_info_t	arena_bin_info[NBINS];

extern size_t		map_bias; 
extern size_t		map_misc_offset;
extern size_t		arena_maxrun; 
extern size_t		arena_maxclass; 
extern unsigned		nlclasses; 
extern unsigned		nhclasses; 

void	*arena_chunk_alloc_huge(arena_t *arena, size_t usize, size_t alignment,
    bool *zero);
void	arena_chunk_dalloc_huge(arena_t *arena, void *chunk, size_t usize);
void	arena_chunk_ralloc_huge_similar(arena_t *arena, void *chunk,
    size_t oldsize, size_t usize);
void	arena_chunk_ralloc_huge_shrink(arena_t *arena, void *chunk,
    size_t oldsize, size_t usize);
bool	arena_chunk_ralloc_huge_expand(arena_t *arena, void *chunk,
    size_t oldsize, size_t usize, bool *zero);
void	arena_purge_all(arena_t *arena);
void	arena_tcache_fill_small(arena_t *arena, tcache_bin_t *tbin,
    index_t binind, uint64_t prof_accumbytes);
void	arena_alloc_junk_small(void *ptr, arena_bin_info_t *bin_info,
    bool zero);
#ifdef JEMALLOC_JET
typedef void (arena_redzone_corruption_t)(void *, size_t, bool, size_t,
    uint8_t);
extern arena_redzone_corruption_t *arena_redzone_corruption;
typedef void (arena_dalloc_junk_small_t)(void *, arena_bin_info_t *);
extern arena_dalloc_junk_small_t *arena_dalloc_junk_small;
#else
void	arena_dalloc_junk_small(void *ptr, arena_bin_info_t *bin_info);
#endif
void	arena_quarantine_junk_small(void *ptr, size_t usize);
void	*arena_malloc_small(arena_t *arena, size_t size, bool zero);
void	*arena_malloc_large(arena_t *arena, size_t size, bool zero);
void	*arena_palloc(arena_t *arena, size_t size, size_t alignment, bool zero);
void	arena_prof_promoted(const void *ptr, size_t size);
void	arena_dalloc_bin_junked_locked(arena_t *arena, arena_chunk_t *chunk,
    void *ptr, arena_chunk_map_bits_t *bitselm);
void	arena_dalloc_bin(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t pageind, arena_chunk_map_bits_t *bitselm);
void	arena_dalloc_small(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t pageind);
#ifdef JEMALLOC_JET
typedef void (arena_dalloc_junk_large_t)(void *, size_t);
extern arena_dalloc_junk_large_t *arena_dalloc_junk_large;
#else
void	arena_dalloc_junk_large(void *ptr, size_t usize);
#endif
void	arena_dalloc_large_junked_locked(arena_t *arena, arena_chunk_t *chunk,
    void *ptr);
void	arena_dalloc_large(arena_t *arena, arena_chunk_t *chunk, void *ptr);
#ifdef JEMALLOC_JET
typedef void (arena_ralloc_junk_large_t)(void *, size_t, size_t);
extern arena_ralloc_junk_large_t *arena_ralloc_junk_large;
#endif
bool	arena_ralloc_no_move(void *ptr, size_t oldsize, size_t size,
    size_t extra, bool zero);
void	*arena_ralloc(tsd_t *tsd, arena_t *arena, void *ptr, size_t oldsize,
    size_t size, size_t extra, size_t alignment, bool zero,
    bool try_tcache_alloc, bool try_tcache_dalloc);
dss_prec_t	arena_dss_prec_get(arena_t *arena);
bool	arena_dss_prec_set(arena_t *arena, dss_prec_t dss_prec);
void	arena_stats_merge(arena_t *arena, const char **dss, size_t *nactive,
    size_t *ndirty, arena_stats_t *astats, malloc_bin_stats_t *bstats,
    malloc_large_stats_t *lstats, malloc_huge_stats_t *hstats);
arena_t	*arena_new(unsigned ind);
void	arena_boot(void);
void	arena_prefork(arena_t *arena);
void	arena_postfork_parent(arena_t *arena);
void	arena_postfork_child(arena_t *arena);

#endif 

#ifdef JEMALLOC_H_INLINES

#ifndef JEMALLOC_ENABLE_INLINE
arena_chunk_map_bits_t	*arena_bitselm_get(arena_chunk_t *chunk,
    size_t pageind);
arena_chunk_map_misc_t	*arena_miscelm_get(arena_chunk_t *chunk,
    size_t pageind);
size_t	arena_miscelm_to_pageind(arena_chunk_map_misc_t *miscelm);
void	*arena_miscelm_to_rpages(arena_chunk_map_misc_t *miscelm);
arena_chunk_map_misc_t	*arena_run_to_miscelm(arena_run_t *run);
size_t	*arena_mapbitsp_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbitsp_read(size_t *mapbitsp);
size_t	arena_mapbits_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_unallocated_size_get(arena_chunk_t *chunk,
    size_t pageind);
size_t	arena_mapbits_large_size_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_small_runind_get(arena_chunk_t *chunk, size_t pageind);
index_t	arena_mapbits_binind_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_dirty_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_unzeroed_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_large_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_allocated_get(arena_chunk_t *chunk, size_t pageind);
void	arena_mapbitsp_write(size_t *mapbitsp, size_t mapbits);
void	arena_mapbits_unallocated_set(arena_chunk_t *chunk, size_t pageind,
    size_t size, size_t flags);
void	arena_mapbits_unallocated_size_set(arena_chunk_t *chunk, size_t pageind,
    size_t size);
void	arena_mapbits_large_set(arena_chunk_t *chunk, size_t pageind,
    size_t size, size_t flags);
void	arena_mapbits_large_binind_set(arena_chunk_t *chunk, size_t pageind,
    index_t binind);
void	arena_mapbits_small_set(arena_chunk_t *chunk, size_t pageind,
    size_t runind, index_t binind, size_t flags);
void	arena_mapbits_unzeroed_set(arena_chunk_t *chunk, size_t pageind,
    size_t unzeroed);
bool	arena_prof_accum_impl(arena_t *arena, uint64_t accumbytes);
bool	arena_prof_accum_locked(arena_t *arena, uint64_t accumbytes);
bool	arena_prof_accum(arena_t *arena, uint64_t accumbytes);
index_t	arena_ptr_small_binind_get(const void *ptr, size_t mapbits);
index_t	arena_bin_index(arena_t *arena, arena_bin_t *bin);
unsigned	arena_run_regind(arena_run_t *run, arena_bin_info_t *bin_info,
    const void *ptr);
prof_tctx_t	*arena_prof_tctx_get(const void *ptr);
void	arena_prof_tctx_set(const void *ptr, prof_tctx_t *tctx);
void	*arena_malloc(tsd_t *tsd, arena_t *arena, size_t size, bool zero,
    bool try_tcache);
size_t	arena_salloc(const void *ptr, bool demote);
void	arena_dalloc(tsd_t *tsd, arena_chunk_t *chunk, void *ptr,
    bool try_tcache);
void	arena_sdalloc(tsd_t *tsd, arena_chunk_t *chunk, void *ptr, size_t size,
    bool try_tcache);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_ARENA_C_))
#  ifdef JEMALLOC_ARENA_INLINE_A
JEMALLOC_ALWAYS_INLINE arena_chunk_map_bits_t *
arena_bitselm_get(arena_chunk_t *chunk, size_t pageind)
{

	assert(pageind >= map_bias);
	assert(pageind < chunk_npages);

	return (&chunk->map_bits[pageind-map_bias]);
}

JEMALLOC_ALWAYS_INLINE arena_chunk_map_misc_t *
arena_miscelm_get(arena_chunk_t *chunk, size_t pageind)
{

	assert(pageind >= map_bias);
	assert(pageind < chunk_npages);

	return ((arena_chunk_map_misc_t *)((uintptr_t)chunk +
	    (uintptr_t)map_misc_offset) + pageind-map_bias);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_miscelm_to_pageind(arena_chunk_map_misc_t *miscelm)
{
	arena_chunk_t *chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(miscelm);
	size_t pageind = ((uintptr_t)miscelm - ((uintptr_t)chunk +
	    map_misc_offset)) / sizeof(arena_chunk_map_misc_t) + map_bias;

	assert(pageind >= map_bias);
	assert(pageind < chunk_npages);

	return (pageind);
}

JEMALLOC_ALWAYS_INLINE void *
arena_miscelm_to_rpages(arena_chunk_map_misc_t *miscelm)
{
	arena_chunk_t *chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(miscelm);
	size_t pageind = arena_miscelm_to_pageind(miscelm);

	return ((void *)((uintptr_t)chunk + (pageind << LG_PAGE)));
}

JEMALLOC_ALWAYS_INLINE arena_chunk_map_misc_t *
arena_run_to_miscelm(arena_run_t *run)
{
	arena_chunk_map_misc_t *miscelm = (arena_chunk_map_misc_t
	    *)((uintptr_t)run - offsetof(arena_chunk_map_misc_t, run));

	assert(arena_miscelm_to_pageind(miscelm) >= map_bias);
	assert(arena_miscelm_to_pageind(miscelm) < chunk_npages);

	return (miscelm);
}

JEMALLOC_ALWAYS_INLINE size_t *
arena_mapbitsp_get(arena_chunk_t *chunk, size_t pageind)
{

	return (&arena_bitselm_get(chunk, pageind)->bits);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbitsp_read(size_t *mapbitsp)
{

	return (*mapbitsp);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_get(arena_chunk_t *chunk, size_t pageind)
{

	return (arena_mapbitsp_read(arena_mapbitsp_get(chunk, pageind)));
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_unallocated_size_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;

	mapbits = arena_mapbits_get(chunk, pageind);
	assert((mapbits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) == 0);
	return (mapbits & ~PAGE_MASK);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_large_size_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;

	mapbits = arena_mapbits_get(chunk, pageind);
	assert((mapbits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) ==
	    (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED));
	return (mapbits & ~PAGE_MASK);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_small_runind_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;

	mapbits = arena_mapbits_get(chunk, pageind);
	assert((mapbits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) ==
	    CHUNK_MAP_ALLOCATED);
	return (mapbits >> LG_PAGE);
}

JEMALLOC_ALWAYS_INLINE index_t
arena_mapbits_binind_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;
	index_t binind;

	mapbits = arena_mapbits_get(chunk, pageind);
	binind = (mapbits & CHUNK_MAP_BININD_MASK) >> CHUNK_MAP_BININD_SHIFT;
	assert(binind < NBINS || binind == BININD_INVALID);
	return (binind);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_dirty_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;

	mapbits = arena_mapbits_get(chunk, pageind);
	return (mapbits & CHUNK_MAP_DIRTY);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_unzeroed_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;

	mapbits = arena_mapbits_get(chunk, pageind);
	return (mapbits & CHUNK_MAP_UNZEROED);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_large_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;

	mapbits = arena_mapbits_get(chunk, pageind);
	return (mapbits & CHUNK_MAP_LARGE);
}

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_allocated_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;

	mapbits = arena_mapbits_get(chunk, pageind);
	return (mapbits & CHUNK_MAP_ALLOCATED);
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbitsp_write(size_t *mapbitsp, size_t mapbits)
{

	*mapbitsp = mapbits;
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbits_unallocated_set(arena_chunk_t *chunk, size_t pageind, size_t size,
    size_t flags)
{
	size_t *mapbitsp = arena_mapbitsp_get(chunk, pageind);

	assert((size & PAGE_MASK) == 0);
	assert((flags & ~CHUNK_MAP_FLAGS_MASK) == 0);
	assert((flags & (CHUNK_MAP_DIRTY|CHUNK_MAP_UNZEROED)) == flags);
	arena_mapbitsp_write(mapbitsp, size | CHUNK_MAP_BININD_INVALID | flags);
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbits_unallocated_size_set(arena_chunk_t *chunk, size_t pageind,
    size_t size)
{
	size_t *mapbitsp = arena_mapbitsp_get(chunk, pageind);
	size_t mapbits = arena_mapbitsp_read(mapbitsp);

	assert((size & PAGE_MASK) == 0);
	assert((mapbits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) == 0);
	arena_mapbitsp_write(mapbitsp, size | (mapbits & PAGE_MASK));
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbits_large_set(arena_chunk_t *chunk, size_t pageind, size_t size,
    size_t flags)
{
	size_t *mapbitsp = arena_mapbitsp_get(chunk, pageind);
	size_t mapbits = arena_mapbitsp_read(mapbitsp);
	size_t unzeroed;

	assert((size & PAGE_MASK) == 0);
	assert((flags & CHUNK_MAP_DIRTY) == flags);
	unzeroed = mapbits & CHUNK_MAP_UNZEROED; 
	arena_mapbitsp_write(mapbitsp, size | CHUNK_MAP_BININD_INVALID | flags
	    | unzeroed | CHUNK_MAP_LARGE | CHUNK_MAP_ALLOCATED);
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbits_large_binind_set(arena_chunk_t *chunk, size_t pageind,
    index_t binind)
{
	size_t *mapbitsp = arena_mapbitsp_get(chunk, pageind);
	size_t mapbits = arena_mapbitsp_read(mapbitsp);

	assert(binind <= BININD_INVALID);
	assert(arena_mapbits_large_size_get(chunk, pageind) == LARGE_MINCLASS);
	arena_mapbitsp_write(mapbitsp, (mapbits & ~CHUNK_MAP_BININD_MASK) |
	    (binind << CHUNK_MAP_BININD_SHIFT));
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbits_small_set(arena_chunk_t *chunk, size_t pageind, size_t runind,
    index_t binind, size_t flags)
{
	size_t *mapbitsp = arena_mapbitsp_get(chunk, pageind);
	size_t mapbits = arena_mapbitsp_read(mapbitsp);
	size_t unzeroed;

	assert(binind < BININD_INVALID);
	assert(pageind - runind >= map_bias);
	assert((flags & CHUNK_MAP_DIRTY) == flags);
	unzeroed = mapbits & CHUNK_MAP_UNZEROED; 
	arena_mapbitsp_write(mapbitsp, (runind << LG_PAGE) | (binind <<
	    CHUNK_MAP_BININD_SHIFT) | flags | unzeroed | CHUNK_MAP_ALLOCATED);
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbits_unzeroed_set(arena_chunk_t *chunk, size_t pageind,
    size_t unzeroed)
{
	size_t *mapbitsp = arena_mapbitsp_get(chunk, pageind);
	size_t mapbits = arena_mapbitsp_read(mapbitsp);

	arena_mapbitsp_write(mapbitsp, (mapbits & ~CHUNK_MAP_UNZEROED) |
	    unzeroed);
}

JEMALLOC_INLINE bool
arena_prof_accum_impl(arena_t *arena, uint64_t accumbytes)
{

	cassert(config_prof);
	assert(prof_interval != 0);

	arena->prof_accumbytes += accumbytes;
	if (arena->prof_accumbytes >= prof_interval) {
		arena->prof_accumbytes -= prof_interval;
		return (true);
	}
	return (false);
}

JEMALLOC_INLINE bool
arena_prof_accum_locked(arena_t *arena, uint64_t accumbytes)
{

	cassert(config_prof);

	if (likely(prof_interval == 0))
		return (false);
	return (arena_prof_accum_impl(arena, accumbytes));
}

JEMALLOC_INLINE bool
arena_prof_accum(arena_t *arena, uint64_t accumbytes)
{

	cassert(config_prof);

	if (likely(prof_interval == 0))
		return (false);

	{
		bool ret;

		malloc_mutex_lock(&arena->lock);
		ret = arena_prof_accum_impl(arena, accumbytes);
		malloc_mutex_unlock(&arena->lock);
		return (ret);
	}
}

JEMALLOC_ALWAYS_INLINE index_t
arena_ptr_small_binind_get(const void *ptr, size_t mapbits)
{
	index_t binind;

	binind = (mapbits & CHUNK_MAP_BININD_MASK) >> CHUNK_MAP_BININD_SHIFT;

	if (config_debug) {
		arena_chunk_t *chunk;
		arena_t *arena;
		size_t pageind;
		size_t actual_mapbits;
		size_t rpages_ind;
		arena_run_t *run;
		arena_bin_t *bin;
		index_t run_binind, actual_binind;
		arena_bin_info_t *bin_info;
		arena_chunk_map_misc_t *miscelm;
		void *rpages;

		assert(binind != BININD_INVALID);
		assert(binind < NBINS);
		chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		arena = chunk->arena;
		pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
		actual_mapbits = arena_mapbits_get(chunk, pageind);
		assert(mapbits == actual_mapbits);
		assert(arena_mapbits_large_get(chunk, pageind) == 0);
		assert(arena_mapbits_allocated_get(chunk, pageind) != 0);
		rpages_ind = pageind - arena_mapbits_small_runind_get(chunk,
		    pageind);
		miscelm = arena_miscelm_get(chunk, rpages_ind);
		run = &miscelm->run;
		run_binind = run->binind;
		bin = &arena->bins[run_binind];
		actual_binind = bin - arena->bins;
		assert(run_binind == actual_binind);
		bin_info = &arena_bin_info[actual_binind];
		rpages = arena_miscelm_to_rpages(miscelm);
		assert(((uintptr_t)ptr - ((uintptr_t)rpages +
		    (uintptr_t)bin_info->reg0_offset)) % bin_info->reg_interval
		    == 0);
	}

	return (binind);
}
#  endif 

#  ifdef JEMALLOC_ARENA_INLINE_B
JEMALLOC_INLINE index_t
arena_bin_index(arena_t *arena, arena_bin_t *bin)
{
	index_t binind = bin - arena->bins;
	assert(binind < NBINS);
	return (binind);
}

JEMALLOC_INLINE unsigned
arena_run_regind(arena_run_t *run, arena_bin_info_t *bin_info, const void *ptr)
{
	unsigned shift, diff, regind;
	size_t interval;
	arena_chunk_map_misc_t *miscelm = arena_run_to_miscelm(run);
	void *rpages = arena_miscelm_to_rpages(miscelm);

	



	assert((uintptr_t)ptr >= (uintptr_t)rpages +
	    (uintptr_t)bin_info->reg0_offset);

	



	diff = (unsigned)((uintptr_t)ptr - (uintptr_t)rpages -
	    bin_info->reg0_offset);

	
	interval = bin_info->reg_interval;
	shift = jemalloc_ffs(interval) - 1;
	diff >>= shift;
	interval >>= shift;

	if (interval == 1) {
		
		regind = diff;
	} else {
		













#define	SIZE_INV_SHIFT	((sizeof(unsigned) << 3) - LG_RUN_MAXREGS)
#define	SIZE_INV(s)	(((1U << SIZE_INV_SHIFT) / (s)) + 1)
		static const unsigned interval_invs[] = {
		    SIZE_INV(3),
		    SIZE_INV(4), SIZE_INV(5), SIZE_INV(6), SIZE_INV(7),
		    SIZE_INV(8), SIZE_INV(9), SIZE_INV(10), SIZE_INV(11),
		    SIZE_INV(12), SIZE_INV(13), SIZE_INV(14), SIZE_INV(15),
		    SIZE_INV(16), SIZE_INV(17), SIZE_INV(18), SIZE_INV(19),
		    SIZE_INV(20), SIZE_INV(21), SIZE_INV(22), SIZE_INV(23),
		    SIZE_INV(24), SIZE_INV(25), SIZE_INV(26), SIZE_INV(27),
		    SIZE_INV(28), SIZE_INV(29), SIZE_INV(30), SIZE_INV(31)
		};

		if (likely(interval <= ((sizeof(interval_invs) /
		    sizeof(unsigned)) + 2))) {
			regind = (diff * interval_invs[interval - 3]) >>
			    SIZE_INV_SHIFT;
		} else
			regind = diff / interval;
#undef SIZE_INV
#undef SIZE_INV_SHIFT
	}
	assert(diff == regind * interval);
	assert(regind < bin_info->nregs);

	return (regind);
}

JEMALLOC_INLINE prof_tctx_t *
arena_prof_tctx_get(const void *ptr)
{
	prof_tctx_t *ret;
	arena_chunk_t *chunk;
	size_t pageind, mapbits;

	cassert(config_prof);
	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	mapbits = arena_mapbits_get(chunk, pageind);
	assert((mapbits & CHUNK_MAP_ALLOCATED) != 0);
	if (likely((mapbits & CHUNK_MAP_LARGE) == 0))
		ret = (prof_tctx_t *)(uintptr_t)1U;
	else
		ret = arena_miscelm_get(chunk, pageind)->prof_tctx;

	return (ret);
}

JEMALLOC_INLINE void
arena_prof_tctx_set(const void *ptr, prof_tctx_t *tctx)
{
	arena_chunk_t *chunk;
	size_t pageind;

	cassert(config_prof);
	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	assert(arena_mapbits_allocated_get(chunk, pageind) != 0);

	if (unlikely(arena_mapbits_large_get(chunk, pageind) != 0))
		arena_miscelm_get(chunk, pageind)->prof_tctx = tctx;
}

JEMALLOC_ALWAYS_INLINE void *
arena_malloc(tsd_t *tsd, arena_t *arena, size_t size, bool zero,
    bool try_tcache)
{
	tcache_t *tcache;

	assert(size != 0);
	assert(size <= arena_maxclass);

	if (likely(size <= SMALL_MAXCLASS)) {
		if (likely(try_tcache) && likely((tcache = tcache_get(tsd,
		    true)) != NULL))
			return (tcache_alloc_small(tcache, size, zero));
		else {
			arena = arena_choose(tsd, arena);
			if (unlikely(arena == NULL))
				return (NULL);
			return (arena_malloc_small(arena, size, zero));
		}
	} else {
		



		if (try_tcache && size <= tcache_maxclass && likely((tcache =
		    tcache_get(tsd, true)) != NULL))
			return (tcache_alloc_large(tcache, size, zero));
		else {
			arena = arena_choose(tsd, arena);
			if (unlikely(arena == NULL))
				return (NULL);
			return (arena_malloc_large(arena, size, zero));
		}
	}
}


JEMALLOC_ALWAYS_INLINE size_t
arena_salloc(const void *ptr, bool demote)
{
	size_t ret;
	arena_chunk_t *chunk;
	size_t pageind;
	index_t binind;

	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	assert(arena_mapbits_allocated_get(chunk, pageind) != 0);
	binind = arena_mapbits_binind_get(chunk, pageind);
	if (unlikely(binind == BININD_INVALID || (config_prof && !demote &&
	    arena_mapbits_large_get(chunk, pageind) != 0))) {
		




		assert(((uintptr_t)ptr & PAGE_MASK) == 0);
		ret = arena_mapbits_large_size_get(chunk, pageind);
		assert(ret != 0);
		assert(pageind + (ret>>LG_PAGE) <= chunk_npages);
		assert(arena_mapbits_dirty_get(chunk, pageind) ==
		    arena_mapbits_dirty_get(chunk, pageind+(ret>>LG_PAGE)-1));
	} else {
		
		assert(arena_mapbits_large_get(chunk, pageind) != 0 ||
		    arena_ptr_small_binind_get(ptr, arena_mapbits_get(chunk,
		    pageind)) == binind);
		ret = index2size(binind);
	}

	return (ret);
}

JEMALLOC_ALWAYS_INLINE void
arena_dalloc(tsd_t *tsd, arena_chunk_t *chunk, void *ptr, bool try_tcache)
{
	size_t pageind, mapbits;
	tcache_t *tcache;

	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	mapbits = arena_mapbits_get(chunk, pageind);
	assert(arena_mapbits_allocated_get(chunk, pageind) != 0);
	if (likely((mapbits & CHUNK_MAP_LARGE) == 0)) {
		
		if (likely(try_tcache) && likely((tcache = tcache_get(tsd,
		    false)) != NULL)) {
			index_t binind = arena_ptr_small_binind_get(ptr,
			    mapbits);
			tcache_dalloc_small(tcache, ptr, binind);
		} else
			arena_dalloc_small(chunk->arena, chunk, ptr, pageind);
	} else {
		size_t size = arena_mapbits_large_size_get(chunk, pageind);

		assert(((uintptr_t)ptr & PAGE_MASK) == 0);

		if (try_tcache && size <= tcache_maxclass && likely((tcache =
		    tcache_get(tsd, false)) != NULL))
			tcache_dalloc_large(tcache, ptr, size);
		else
			arena_dalloc_large(chunk->arena, chunk, ptr);
	}
}

JEMALLOC_ALWAYS_INLINE void
arena_sdalloc(tsd_t *tsd, arena_chunk_t *chunk, void *ptr, size_t size,
    bool try_tcache)
{
	tcache_t *tcache;

	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	if (config_prof && opt_prof) {
		size_t pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
		assert(arena_mapbits_allocated_get(chunk, pageind) != 0);
		if (arena_mapbits_large_get(chunk, pageind) != 0) {
			
			assert(((uintptr_t)ptr & PAGE_MASK) == 0);
			size = arena_mapbits_large_size_get(chunk, pageind);
		}
	}
	assert(s2u(size) == s2u(arena_salloc(ptr, false)));

	if (likely(size <= SMALL_MAXCLASS)) {
		
		if (likely(try_tcache) && likely((tcache = tcache_get(tsd,
		    false)) != NULL)) {
			index_t binind = size2index(size);
			tcache_dalloc_small(tcache, ptr, binind);
		} else {
			size_t pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >>
			    LG_PAGE;
			arena_dalloc_small(chunk->arena, chunk, ptr, pageind);
		}
	} else {
		assert(((uintptr_t)ptr & PAGE_MASK) == 0);

		if (try_tcache && size <= tcache_maxclass && (tcache =
		    tcache_get(tsd, false)) != NULL)
			tcache_dalloc_large(tcache, ptr, size);
		else
			arena_dalloc_large(chunk->arena, chunk, ptr);
	}
}
#  endif 
#endif

#endif 

