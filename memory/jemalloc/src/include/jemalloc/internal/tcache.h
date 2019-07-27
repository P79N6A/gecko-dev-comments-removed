
#ifdef JEMALLOC_H_TYPES

typedef struct tcache_bin_info_s tcache_bin_info_t;
typedef struct tcache_bin_s tcache_bin_t;
typedef struct tcache_s tcache_t;






#define	TCACHE_STATE_DISABLED		((tcache_t *)(uintptr_t)1)
#define	TCACHE_STATE_REINCARNATED	((tcache_t *)(uintptr_t)2)
#define	TCACHE_STATE_PURGATORY		((tcache_t *)(uintptr_t)3)
#define	TCACHE_STATE_MAX		TCACHE_STATE_PURGATORY








#define	TCACHE_NSLOTS_SMALL_MAX		200


#define	TCACHE_NSLOTS_LARGE		20


#define	LG_TCACHE_MAXCLASS_DEFAULT	15






#define	TCACHE_GC_SWEEP			8192


#define	TCACHE_GC_INCR							\
    ((TCACHE_GC_SWEEP / NBINS) + ((TCACHE_GC_SWEEP / NBINS == 0) ? 0 : 1))

#endif 

#ifdef JEMALLOC_H_STRUCTS

typedef enum {
	tcache_enabled_false   = 0, 
	tcache_enabled_true    = 1,
	tcache_enabled_default = 2
} tcache_enabled_t;





struct tcache_bin_info_s {
	unsigned	ncached_max;	
};

struct tcache_bin_s {
	tcache_bin_stats_t tstats;
	int		low_water;	
	unsigned	lg_fill_div;	
	unsigned	ncached;	
	void		**avail;	
};

struct tcache_s {
	ql_elm(tcache_t) link;		
	uint64_t	prof_accumbytes;
	arena_t		*arena;		
	unsigned	ev_cnt;		
	index_t		next_gc_bin;	
	tcache_bin_t	tbins[1];	
	





};

#endif 

#ifdef JEMALLOC_H_EXTERNS

extern bool	opt_tcache;
extern ssize_t	opt_lg_tcache_max;

extern tcache_bin_info_t	*tcache_bin_info;





extern size_t			nhbins;


extern size_t			tcache_maxclass;

size_t	tcache_salloc(const void *ptr);
void	tcache_event_hard(tcache_t *tcache);
void	*tcache_alloc_small_hard(tcache_t *tcache, tcache_bin_t *tbin,
    index_t binind);
void	tcache_bin_flush_small(tcache_bin_t *tbin, index_t binind, unsigned rem,
    tcache_t *tcache);
void	tcache_bin_flush_large(tcache_bin_t *tbin, index_t binind, unsigned rem,
    tcache_t *tcache);
void	tcache_arena_associate(tcache_t *tcache, arena_t *arena);
void	tcache_arena_reassociate(tcache_t *tcache, arena_t *arena);
void	tcache_arena_dissociate(tcache_t *tcache);
tcache_t *tcache_get_hard(tsd_t *tsd);
tcache_t *tcache_create(tsd_t *tsd, arena_t *arena);
void	tcache_cleanup(tsd_t *tsd);
void	tcache_enabled_cleanup(tsd_t *tsd);
void	tcache_stats_merge(tcache_t *tcache, arena_t *arena);
bool	tcache_boot(void);

#endif 

#ifdef JEMALLOC_H_INLINES

#ifndef JEMALLOC_ENABLE_INLINE
void	tcache_event(tcache_t *tcache);
void	tcache_flush(void);
bool	tcache_enabled_get(void);
tcache_t *tcache_get(tsd_t *tsd, bool create);
void	tcache_enabled_set(bool enabled);
void	*tcache_alloc_easy(tcache_bin_t *tbin);
void	*tcache_alloc_small(tcache_t *tcache, size_t size, bool zero);
void	*tcache_alloc_large(tcache_t *tcache, size_t size, bool zero);
void	tcache_dalloc_small(tcache_t *tcache, void *ptr, index_t binind);
void	tcache_dalloc_large(tcache_t *tcache, void *ptr, size_t size);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_TCACHE_C_))
JEMALLOC_INLINE void
tcache_flush(void)
{
	tsd_t *tsd;

	cassert(config_tcache);

	tsd = tsd_fetch();
	tcache_cleanup(tsd);
}

JEMALLOC_INLINE bool
tcache_enabled_get(void)
{
	tsd_t *tsd;
	tcache_enabled_t tcache_enabled;

	cassert(config_tcache);

	tsd = tsd_fetch();
	tcache_enabled = tsd_tcache_enabled_get(tsd);
	if (tcache_enabled == tcache_enabled_default) {
		tcache_enabled = (tcache_enabled_t)opt_tcache;
		tsd_tcache_enabled_set(tsd, tcache_enabled);
	}

	return ((bool)tcache_enabled);
}

JEMALLOC_INLINE void
tcache_enabled_set(bool enabled)
{
	tsd_t *tsd;
	tcache_enabled_t tcache_enabled;

	cassert(config_tcache);

	tsd = tsd_fetch();

	tcache_enabled = (tcache_enabled_t)enabled;
	tsd_tcache_enabled_set(tsd, tcache_enabled);

	if (!enabled)
		tcache_cleanup(tsd);
}

JEMALLOC_ALWAYS_INLINE tcache_t *
tcache_get(tsd_t *tsd, bool create)
{
	tcache_t *tcache;

	if (!config_tcache)
		return (NULL);

	tcache = tsd_tcache_get(tsd);
	if (!create)
		return (tcache);
	if (unlikely(tcache == NULL) && tsd_nominal(tsd)) {
		tcache = tcache_get_hard(tsd);
		tsd_tcache_set(tsd, tcache);
	}

	return (tcache);
}

JEMALLOC_ALWAYS_INLINE void
tcache_event(tcache_t *tcache)
{

	if (TCACHE_GC_INCR == 0)
		return;

	tcache->ev_cnt++;
	assert(tcache->ev_cnt <= TCACHE_GC_INCR);
	if (unlikely(tcache->ev_cnt == TCACHE_GC_INCR))
		tcache_event_hard(tcache);
}

JEMALLOC_ALWAYS_INLINE void *
tcache_alloc_easy(tcache_bin_t *tbin)
{
	void *ret;

	if (unlikely(tbin->ncached == 0)) {
		tbin->low_water = -1;
		return (NULL);
	}
	tbin->ncached--;
	if (unlikely((int)tbin->ncached < tbin->low_water))
		tbin->low_water = tbin->ncached;
	ret = tbin->avail[tbin->ncached];
	return (ret);
}

JEMALLOC_ALWAYS_INLINE void *
tcache_alloc_small(tcache_t *tcache, size_t size, bool zero)
{
	void *ret;
	index_t binind;
	size_t usize;
	tcache_bin_t *tbin;

	binind = size2index(size);
	assert(binind < NBINS);
	tbin = &tcache->tbins[binind];
	usize = index2size(binind);
	ret = tcache_alloc_easy(tbin);
	if (unlikely(ret == NULL)) {
		ret = tcache_alloc_small_hard(tcache, tbin, binind);
		if (ret == NULL)
			return (NULL);
	}
	assert(tcache_salloc(ret) == usize);

	if (likely(!zero)) {
		if (config_fill) {
			if (unlikely(opt_junk_alloc)) {
				arena_alloc_junk_small(ret,
				    &arena_bin_info[binind], false);
			} else if (unlikely(opt_zero))
				memset(ret, 0, usize);
		}
	} else {
		if (config_fill && unlikely(opt_junk_alloc)) {
			arena_alloc_junk_small(ret, &arena_bin_info[binind],
			    true);
		}
		memset(ret, 0, usize);
	}

	if (config_stats)
		tbin->tstats.nrequests++;
	if (config_prof)
		tcache->prof_accumbytes += usize;
	tcache_event(tcache);
	return (ret);
}

JEMALLOC_ALWAYS_INLINE void *
tcache_alloc_large(tcache_t *tcache, size_t size, bool zero)
{
	void *ret;
	index_t binind;
	size_t usize;
	tcache_bin_t *tbin;

	binind = size2index(size);
	usize = index2size(binind);
	assert(usize <= tcache_maxclass);
	assert(binind < nhbins);
	tbin = &tcache->tbins[binind];
	ret = tcache_alloc_easy(tbin);
	if (unlikely(ret == NULL)) {
		



		ret = arena_malloc_large(tcache->arena, usize, zero);
		if (ret == NULL)
			return (NULL);
	} else {
		if (config_prof && usize == LARGE_MINCLASS) {
			arena_chunk_t *chunk =
			    (arena_chunk_t *)CHUNK_ADDR2BASE(ret);
			size_t pageind = (((uintptr_t)ret - (uintptr_t)chunk) >>
			    LG_PAGE);
			arena_mapbits_large_binind_set(chunk, pageind,
			    BININD_INVALID);
		}
		if (likely(!zero)) {
			if (config_fill) {
				if (unlikely(opt_junk_alloc))
					memset(ret, 0xa5, usize);
				else if (unlikely(opt_zero))
					memset(ret, 0, usize);
			}
		} else
			memset(ret, 0, usize);

		if (config_stats)
			tbin->tstats.nrequests++;
		if (config_prof)
			tcache->prof_accumbytes += usize;
	}

	tcache_event(tcache);
	return (ret);
}

JEMALLOC_ALWAYS_INLINE void
tcache_dalloc_small(tcache_t *tcache, void *ptr, index_t binind)
{
	tcache_bin_t *tbin;
	tcache_bin_info_t *tbin_info;

	assert(tcache_salloc(ptr) <= SMALL_MAXCLASS);

	if (config_fill && unlikely(opt_junk_free))
		arena_dalloc_junk_small(ptr, &arena_bin_info[binind]);

	tbin = &tcache->tbins[binind];
	tbin_info = &tcache_bin_info[binind];
	if (unlikely(tbin->ncached == tbin_info->ncached_max)) {
		tcache_bin_flush_small(tbin, binind, (tbin_info->ncached_max >>
		    1), tcache);
	}
	assert(tbin->ncached < tbin_info->ncached_max);
	tbin->avail[tbin->ncached] = ptr;
	tbin->ncached++;

	tcache_event(tcache);
}

JEMALLOC_ALWAYS_INLINE void
tcache_dalloc_large(tcache_t *tcache, void *ptr, size_t size)
{
	index_t binind;
	tcache_bin_t *tbin;
	tcache_bin_info_t *tbin_info;

	assert((size & PAGE_MASK) == 0);
	assert(tcache_salloc(ptr) > SMALL_MAXCLASS);
	assert(tcache_salloc(ptr) <= tcache_maxclass);

	binind = size2index(size);

	if (config_fill && unlikely(opt_junk_free))
		arena_dalloc_junk_large(ptr, size);

	tbin = &tcache->tbins[binind];
	tbin_info = &tcache_bin_info[binind];
	if (unlikely(tbin->ncached == tbin_info->ncached_max)) {
		tcache_bin_flush_large(tbin, binind, (tbin_info->ncached_max >>
		    1), tcache);
	}
	assert(tbin->ncached < tbin_info->ncached_max);
	tbin->avail[tbin->ncached] = ptr;
	tbin->ncached++;

	tcache_event(tcache);
}
#endif

#endif 

