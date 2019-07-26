
#ifdef JEMALLOC_H_TYPES


















#define	RUN_BFP			12

#define	RUN_MAX_OVRHD		0x0000003dU
#define	RUN_MAX_OVRHD_RELAX	0x00001800U


#define	LG_RUN_MAXREGS		11
#define	RUN_MAXREGS		(1U << LG_RUN_MAXREGS)





#define	REDZONE_MINSIZE		16









#define	LG_DIRTY_MULT_DEFAULT	3

typedef struct arena_chunk_map_s arena_chunk_map_t;
typedef struct arena_chunk_s arena_chunk_t;
typedef struct arena_run_s arena_run_t;
typedef struct arena_bin_info_s arena_bin_info_t;
typedef struct arena_bin_s arena_bin_t;
typedef struct arena_s arena_t;

#endif 

#ifdef JEMALLOC_H_STRUCTS


struct arena_chunk_map_s {
#ifndef JEMALLOC_PROF
	





	union {
#endif
	union {
		






		rb_node(arena_chunk_map_t)	rb_link;
		





		ql_elm(arena_chunk_map_t)	ql_link;
	}				u;

	
	prof_ctx_t			*prof_ctx;
#ifndef JEMALLOC_PROF
	}; 
#endif

	





















































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
typedef rb_tree(arena_chunk_map_t) arena_avail_tree_t;
typedef rb_tree(arena_chunk_map_t) arena_run_tree_t;
typedef ql_head(arena_chunk_map_t) arena_chunk_mapelms_t;


struct arena_chunk_s {
	
	arena_t			*arena;

	
	rb_node(arena_chunk_t)	dirty_link;

	
	size_t			ndirty;

	
	size_t			nruns_avail;

	






	size_t			nruns_adjac;

	





	arena_chunk_map_t	map[1]; 
};
typedef rb_tree(arena_chunk_t) arena_chunk_tree_t;

struct arena_run_s {
	
	arena_bin_t	*bin;

	
	uint32_t	nextind;

	
	unsigned	nfree;
};







































struct arena_bin_info_s {
	
	size_t		reg_size;

	
	size_t		redzone_size;

	
	size_t		reg_interval;

	
	size_t		run_size;

	
	uint32_t	nregs;

	



	uint32_t	bitmap_offset;

	



	bitmap_info_t	bitmap_info;

	



	uint32_t	ctx0_offset;

	
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

	
	arena_chunk_tree_t	chunks_dirty;

	









	arena_chunk_t		*spare;

	
	size_t			nactive;

	





	size_t			ndirty;

	





	size_t			npurgatory;

	



	arena_avail_tree_t	runs_avail;

	
	arena_bin_t		bins[NBINS];
};

#endif 

#ifdef JEMALLOC_H_EXTERNS

extern ssize_t	opt_lg_dirty_mult;





extern uint8_t const	small_size2bin[];
#define	SMALL_SIZE2BIN(s)	(small_size2bin[(s-1) >> LG_TINY_MIN])

extern arena_bin_info_t	arena_bin_info[NBINS];


#define			nlclasses (chunk_npages - map_bias)

void	arena_purge_all(arena_t *arena);
void	arena_tcache_fill_small(arena_t *arena, tcache_bin_t *tbin,
    size_t binind, uint64_t prof_accumbytes);
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
void	arena_dalloc_bin_locked(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    arena_chunk_map_t *mapelm);
void	arena_dalloc_bin(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t pageind, arena_chunk_map_t *mapelm);
void	arena_dalloc_small(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t pageind);
#ifdef JEMALLOC_JET
typedef void (arena_dalloc_junk_large_t)(void *, size_t);
extern arena_dalloc_junk_large_t *arena_dalloc_junk_large;
#endif
void	arena_dalloc_large_locked(arena_t *arena, arena_chunk_t *chunk,
    void *ptr);
void	arena_dalloc_large(arena_t *arena, arena_chunk_t *chunk, void *ptr);
#ifdef JEMALLOC_JET
typedef void (arena_ralloc_junk_large_t)(void *, size_t, size_t);
extern arena_ralloc_junk_large_t *arena_ralloc_junk_large;
#endif
bool	arena_ralloc_no_move(void *ptr, size_t oldsize, size_t size,
    size_t extra, bool zero);
void	*arena_ralloc(arena_t *arena, void *ptr, size_t oldsize, size_t size,
    size_t extra, size_t alignment, bool zero, bool try_tcache_alloc,
    bool try_tcache_dalloc);
dss_prec_t	arena_dss_prec_get(arena_t *arena);
void	arena_dss_prec_set(arena_t *arena, dss_prec_t dss_prec);
void	arena_stats_merge(arena_t *arena, const char **dss, size_t *nactive,
    size_t *ndirty, arena_stats_t *astats, malloc_bin_stats_t *bstats,
    malloc_large_stats_t *lstats);
bool	arena_new(arena_t *arena, unsigned ind);
void	arena_boot(void);
void	arena_prefork(arena_t *arena);
void	arena_postfork_parent(arena_t *arena);
void	arena_postfork_child(arena_t *arena);

#endif 

#ifdef JEMALLOC_H_INLINES

#ifndef JEMALLOC_ENABLE_INLINE
arena_chunk_map_t	*arena_mapp_get(arena_chunk_t *chunk, size_t pageind);
size_t	*arena_mapbitsp_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbitsp_read(size_t *mapbitsp);
size_t	arena_mapbits_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_unallocated_size_get(arena_chunk_t *chunk,
    size_t pageind);
size_t	arena_mapbits_large_size_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_small_runind_get(arena_chunk_t *chunk, size_t pageind);
size_t	arena_mapbits_binind_get(arena_chunk_t *chunk, size_t pageind);
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
    size_t binind);
void	arena_mapbits_small_set(arena_chunk_t *chunk, size_t pageind,
    size_t runind, size_t binind, size_t flags);
void	arena_mapbits_unzeroed_set(arena_chunk_t *chunk, size_t pageind,
    size_t unzeroed);
bool	arena_prof_accum_impl(arena_t *arena, uint64_t accumbytes);
bool	arena_prof_accum_locked(arena_t *arena, uint64_t accumbytes);
bool	arena_prof_accum(arena_t *arena, uint64_t accumbytes);
size_t	arena_ptr_small_binind_get(const void *ptr, size_t mapbits);
size_t	arena_bin_index(arena_t *arena, arena_bin_t *bin);
unsigned	arena_run_regind(arena_run_t *run, arena_bin_info_t *bin_info,
    const void *ptr);
prof_ctx_t	*arena_prof_ctx_get(const void *ptr);
void	arena_prof_ctx_set(const void *ptr, size_t usize, prof_ctx_t *ctx);
void	*arena_malloc(arena_t *arena, size_t size, bool zero, bool try_tcache);
size_t	arena_salloc(const void *ptr, bool demote);
void	arena_dalloc(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    bool try_tcache);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_ARENA_C_))
#  ifdef JEMALLOC_ARENA_INLINE_A
JEMALLOC_ALWAYS_INLINE arena_chunk_map_t *
arena_mapp_get(arena_chunk_t *chunk, size_t pageind)
{

	assert(pageind >= map_bias);
	assert(pageind < chunk_npages);

	return (&chunk->map[pageind-map_bias]);
}

JEMALLOC_ALWAYS_INLINE size_t *
arena_mapbitsp_get(arena_chunk_t *chunk, size_t pageind)
{

	return (&arena_mapp_get(chunk, pageind)->bits);
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

JEMALLOC_ALWAYS_INLINE size_t
arena_mapbits_binind_get(arena_chunk_t *chunk, size_t pageind)
{
	size_t mapbits;
	size_t binind;

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
    size_t binind)
{
	size_t *mapbitsp = arena_mapbitsp_get(chunk, pageind);
	size_t mapbits = arena_mapbitsp_read(mapbitsp);

	assert(binind <= BININD_INVALID);
	assert(arena_mapbits_large_size_get(chunk, pageind) == PAGE);
	arena_mapbitsp_write(mapbitsp, (mapbits & ~CHUNK_MAP_BININD_MASK) |
	    (binind << CHUNK_MAP_BININD_SHIFT));
}

JEMALLOC_ALWAYS_INLINE void
arena_mapbits_small_set(arena_chunk_t *chunk, size_t pageind, size_t runind,
    size_t binind, size_t flags)
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

	if (prof_interval == 0)
		return (false);
	return (arena_prof_accum_impl(arena, accumbytes));
}

JEMALLOC_INLINE bool
arena_prof_accum(arena_t *arena, uint64_t accumbytes)
{

	cassert(config_prof);

	if (prof_interval == 0)
		return (false);

	{
		bool ret;

		malloc_mutex_lock(&arena->lock);
		ret = arena_prof_accum_impl(arena, accumbytes);
		malloc_mutex_unlock(&arena->lock);
		return (ret);
	}
}

JEMALLOC_ALWAYS_INLINE size_t
arena_ptr_small_binind_get(const void *ptr, size_t mapbits)
{
	size_t binind;

	binind = (mapbits & CHUNK_MAP_BININD_MASK) >> CHUNK_MAP_BININD_SHIFT;

	if (config_debug) {
		arena_chunk_t *chunk;
		arena_t *arena;
		size_t pageind;
		size_t actual_mapbits;
		arena_run_t *run;
		arena_bin_t *bin;
		size_t actual_binind;
		arena_bin_info_t *bin_info;

		assert(binind != BININD_INVALID);
		assert(binind < NBINS);
		chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		arena = chunk->arena;
		pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
		actual_mapbits = arena_mapbits_get(chunk, pageind);
		assert(mapbits == actual_mapbits);
		assert(arena_mapbits_large_get(chunk, pageind) == 0);
		assert(arena_mapbits_allocated_get(chunk, pageind) != 0);
		run = (arena_run_t *)((uintptr_t)chunk + (uintptr_t)((pageind -
		    (actual_mapbits >> LG_PAGE)) << LG_PAGE));
		bin = run->bin;
		actual_binind = bin - arena->bins;
		assert(binind == actual_binind);
		bin_info = &arena_bin_info[actual_binind];
		assert(((uintptr_t)ptr - ((uintptr_t)run +
		    (uintptr_t)bin_info->reg0_offset)) % bin_info->reg_interval
		    == 0);
	}

	return (binind);
}
#  endif 

#  ifdef JEMALLOC_ARENA_INLINE_B
JEMALLOC_INLINE size_t
arena_bin_index(arena_t *arena, arena_bin_t *bin)
{
	size_t binind = bin - arena->bins;
	assert(binind < NBINS);
	return (binind);
}

JEMALLOC_INLINE unsigned
arena_run_regind(arena_run_t *run, arena_bin_info_t *bin_info, const void *ptr)
{
	unsigned shift, diff, regind;
	size_t interval;

	



	assert((uintptr_t)ptr >= (uintptr_t)run +
	    (uintptr_t)bin_info->reg0_offset);

	



	diff = (unsigned)((uintptr_t)ptr - (uintptr_t)run -
	    bin_info->reg0_offset);

	
	interval = bin_info->reg_interval;
	shift = ffs(interval) - 1;
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

		if (interval <= ((sizeof(interval_invs) / sizeof(unsigned)) +
		    2)) {
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

JEMALLOC_INLINE prof_ctx_t *
arena_prof_ctx_get(const void *ptr)
{
	prof_ctx_t *ret;
	arena_chunk_t *chunk;
	size_t pageind, mapbits;

	cassert(config_prof);
	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	mapbits = arena_mapbits_get(chunk, pageind);
	assert((mapbits & CHUNK_MAP_ALLOCATED) != 0);
	if ((mapbits & CHUNK_MAP_LARGE) == 0) {
		if (prof_promote)
			ret = (prof_ctx_t *)(uintptr_t)1U;
		else {
			arena_run_t *run = (arena_run_t *)((uintptr_t)chunk +
			    (uintptr_t)((pageind - (mapbits >> LG_PAGE)) <<
			    LG_PAGE));
			size_t binind = arena_ptr_small_binind_get(ptr,
			    mapbits);
			arena_bin_info_t *bin_info = &arena_bin_info[binind];
			unsigned regind;

			regind = arena_run_regind(run, bin_info, ptr);
			ret = *(prof_ctx_t **)((uintptr_t)run +
			    bin_info->ctx0_offset + (regind *
			    sizeof(prof_ctx_t *)));
		}
	} else
		ret = arena_mapp_get(chunk, pageind)->prof_ctx;

	return (ret);
}

JEMALLOC_INLINE void
arena_prof_ctx_set(const void *ptr, size_t usize, prof_ctx_t *ctx)
{
	arena_chunk_t *chunk;
	size_t pageind;

	cassert(config_prof);
	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	assert(arena_mapbits_allocated_get(chunk, pageind) != 0);

	if (usize > SMALL_MAXCLASS || (prof_promote &&
	    ((uintptr_t)ctx != (uintptr_t)1U || arena_mapbits_large_get(chunk,
	    pageind) != 0))) {
		assert(arena_mapbits_large_get(chunk, pageind) != 0);
		arena_mapp_get(chunk, pageind)->prof_ctx = ctx;
	} else {
		assert(arena_mapbits_large_get(chunk, pageind) == 0);
		if (prof_promote == false) {
			size_t mapbits = arena_mapbits_get(chunk, pageind);
			arena_run_t *run = (arena_run_t *)((uintptr_t)chunk +
			    (uintptr_t)((pageind - (mapbits >> LG_PAGE)) <<
			    LG_PAGE));
			size_t binind;
			arena_bin_info_t *bin_info;
			unsigned regind;

			binind = arena_ptr_small_binind_get(ptr, mapbits);
			bin_info = &arena_bin_info[binind];
			regind = arena_run_regind(run, bin_info, ptr);

			*((prof_ctx_t **)((uintptr_t)run +
			    bin_info->ctx0_offset + (regind * sizeof(prof_ctx_t
			    *)))) = ctx;
		}
	}
}

JEMALLOC_ALWAYS_INLINE void *
arena_malloc(arena_t *arena, size_t size, bool zero, bool try_tcache)
{
	tcache_t *tcache;

	assert(size != 0);
	assert(size <= arena_maxclass);

	if (size <= SMALL_MAXCLASS) {
		if (try_tcache && (tcache = tcache_get(true)) != NULL)
			return (tcache_alloc_small(tcache, size, zero));
		else {
			return (arena_malloc_small(choose_arena(arena), size,
			    zero));
		}
	} else {
		



		if (try_tcache && size <= tcache_maxclass && (tcache =
		    tcache_get(true)) != NULL)
			return (tcache_alloc_large(tcache, size, zero));
		else {
			return (arena_malloc_large(choose_arena(arena), size,
			    zero));
		}
	}
}


JEMALLOC_ALWAYS_INLINE size_t
arena_salloc(const void *ptr, bool demote)
{
	size_t ret;
	arena_chunk_t *chunk;
	size_t pageind, binind;

	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	assert(arena_mapbits_allocated_get(chunk, pageind) != 0);
	binind = arena_mapbits_binind_get(chunk, pageind);
	if (binind == BININD_INVALID || (config_prof && demote == false &&
	    prof_promote && arena_mapbits_large_get(chunk, pageind) != 0)) {
		





		assert(((uintptr_t)ptr & PAGE_MASK) == 0);
		ret = arena_mapbits_large_size_get(chunk, pageind);
		assert(ret != 0);
		assert(pageind + (ret>>LG_PAGE) <= chunk_npages);
		assert(ret == PAGE || arena_mapbits_large_size_get(chunk,
		    pageind+(ret>>LG_PAGE)-1) == 0);
		assert(binind == arena_mapbits_binind_get(chunk,
		    pageind+(ret>>LG_PAGE)-1));
		assert(arena_mapbits_dirty_get(chunk, pageind) ==
		    arena_mapbits_dirty_get(chunk, pageind+(ret>>LG_PAGE)-1));
	} else {
		



		assert(arena_mapbits_large_get(chunk, pageind) != 0 ||
		    arena_ptr_small_binind_get(ptr, arena_mapbits_get(chunk,
		    pageind)) == binind);
		ret = arena_bin_info[binind].reg_size;
	}

	return (ret);
}

JEMALLOC_ALWAYS_INLINE void
arena_dalloc(arena_t *arena, arena_chunk_t *chunk, void *ptr, bool try_tcache)
{
	size_t pageind, mapbits;
	tcache_t *tcache;

	assert(arena != NULL);
	assert(chunk->arena == arena);
	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> LG_PAGE;
	mapbits = arena_mapbits_get(chunk, pageind);
	assert(arena_mapbits_allocated_get(chunk, pageind) != 0);
	if ((mapbits & CHUNK_MAP_LARGE) == 0) {
		
		if (try_tcache && (tcache = tcache_get(false)) != NULL) {
			size_t binind;

			binind = arena_ptr_small_binind_get(ptr, mapbits);
			tcache_dalloc_small(tcache, ptr, binind);
		} else
			arena_dalloc_small(arena, chunk, ptr, pageind);
	} else {
		size_t size = arena_mapbits_large_size_get(chunk, pageind);

		assert(((uintptr_t)ptr & PAGE_MASK) == 0);

		if (try_tcache && size <= tcache_maxclass && (tcache =
		    tcache_get(false)) != NULL) {
			tcache_dalloc_large(tcache, ptr, size);
		} else
			arena_dalloc_large(arena, chunk, ptr);
	}
}
#  endif 
#endif

#endif 

