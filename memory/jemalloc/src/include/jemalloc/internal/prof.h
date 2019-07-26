
#ifdef JEMALLOC_H_TYPES

typedef struct prof_bt_s prof_bt_t;
typedef struct prof_cnt_s prof_cnt_t;
typedef struct prof_thr_cnt_s prof_thr_cnt_t;
typedef struct prof_ctx_s prof_ctx_t;
typedef struct prof_tdata_s prof_tdata_t;


#ifdef JEMALLOC_PROF
#  define PROF_PREFIX_DEFAULT		"jeprof"
#else
#  define PROF_PREFIX_DEFAULT		""
#endif
#define	LG_PROF_SAMPLE_DEFAULT		19
#define	LG_PROF_INTERVAL_DEFAULT	-1






#define	PROF_BT_MAX			128


#define	PROF_TCMAX			1024


#define	PROF_CKH_MINITEMS		64


#define	PROF_DUMP_BUFSIZE		65536


#define	PROF_PRINTF_BUFSIZE		128





#define	PROF_NCTX_LOCKS			1024





#define	PROF_TDATA_STATE_REINCARNATED	((prof_tdata_t *)(uintptr_t)1)
#define	PROF_TDATA_STATE_PURGATORY	((prof_tdata_t *)(uintptr_t)2)
#define	PROF_TDATA_STATE_MAX		PROF_TDATA_STATE_PURGATORY

#endif 

#ifdef JEMALLOC_H_STRUCTS

struct prof_bt_s {
	
	void		**vec;
	unsigned	len;
};

#ifdef JEMALLOC_PROF_LIBGCC

typedef struct {
	prof_bt_t	*bt;
	unsigned	nignore;
	unsigned	max;
} prof_unwind_data_t;
#endif

struct prof_cnt_s {
	








	int64_t		curobjs;
	int64_t		curbytes;
	uint64_t	accumobjs;
	uint64_t	accumbytes;
};

struct prof_thr_cnt_s {
	
	ql_elm(prof_thr_cnt_t)	cnts_link;

	
	ql_elm(prof_thr_cnt_t)	lru_link;

	






	prof_ctx_t		*ctx;

	














	unsigned		epoch;

	
	prof_cnt_t		cnts;
};

struct prof_ctx_s {
	
	prof_bt_t		*bt;

	
	malloc_mutex_t		*lock;

	








	unsigned		nlimbo;

	
	prof_cnt_t		cnt_summed;

	
	prof_cnt_t		cnt_merged;

	



	ql_head(prof_thr_cnt_t)	cnts_ql;

	
	ql_elm(prof_ctx_t)	dump_link;
};
typedef ql_head(prof_ctx_t) prof_ctx_list_t;

struct prof_tdata_s {
	









	ckh_t			bt2cnt;

	
	ql_head(prof_thr_cnt_t)	lru_ql;

	
	void			**vec;

	
	uint64_t		prng_state;
	uint64_t		threshold;
	uint64_t		accum;

	
	bool			enq;
	bool			enq_idump;
	bool			enq_gdump;
};

#endif 

#ifdef JEMALLOC_H_EXTERNS

extern bool	opt_prof;






extern bool	opt_prof_active;
extern size_t	opt_lg_prof_sample;   
extern ssize_t	opt_lg_prof_interval; 
extern bool	opt_prof_gdump;       
extern bool	opt_prof_final;       
extern bool	opt_prof_leak;        
extern bool	opt_prof_accum;       
extern char	opt_prof_prefix[
    
#ifdef JEMALLOC_PROF
    PATH_MAX +
#endif
    1];








extern uint64_t	prof_interval;





extern bool	prof_promote;

void	bt_init(prof_bt_t *bt, void **vec);
void	prof_backtrace(prof_bt_t *bt, unsigned nignore);
prof_thr_cnt_t	*prof_lookup(prof_bt_t *bt);
#ifdef JEMALLOC_JET
size_t	prof_bt_count(void);
typedef int (prof_dump_open_t)(bool, const char *);
extern prof_dump_open_t *prof_dump_open;
#endif
void	prof_idump(void);
bool	prof_mdump(const char *filename);
void	prof_gdump(void);
prof_tdata_t	*prof_tdata_init(void);
void	prof_tdata_cleanup(void *arg);
void	prof_boot0(void);
void	prof_boot1(void);
bool	prof_boot2(void);
void	prof_prefork(void);
void	prof_postfork_parent(void);
void	prof_postfork_child(void);

#endif 

#ifdef JEMALLOC_H_INLINES

#define	PROF_ALLOC_PREP(nignore, size, ret) do {			\
	prof_tdata_t *prof_tdata;					\
	prof_bt_t bt;							\
									\
	assert(size == s2u(size));					\
									\
	prof_tdata = prof_tdata_get(true);				\
	if ((uintptr_t)prof_tdata <= (uintptr_t)PROF_TDATA_STATE_MAX) {	\
		if (prof_tdata != NULL)					\
			ret = (prof_thr_cnt_t *)(uintptr_t)1U;		\
		else							\
			ret = NULL;					\
		break;							\
	}								\
									\
	if (opt_prof_active == false) {					\
		/* Sampling is currently inactive, so avoid sampling. */\
		ret = (prof_thr_cnt_t *)(uintptr_t)1U;			\
	} else if (opt_lg_prof_sample == 0) {				\
		/* Don't bother with sampling logic, since sampling   */\
		/* interval is 1.                                     */\
		bt_init(&bt, prof_tdata->vec);				\
		prof_backtrace(&bt, nignore);				\
		ret = prof_lookup(&bt);					\
	} else {							\
		if (prof_tdata->threshold == 0) {			\
			/* Initialize.  Seed the prng differently for */\
			/* each thread.                               */\
			prof_tdata->prng_state =			\
			    (uint64_t)(uintptr_t)&size;			\
			prof_sample_threshold_update(prof_tdata);	\
		}							\
									\
		/* Determine whether to capture a backtrace based on  */\
		/* whether size is enough for prof_accum to reach     */\
		/* prof_tdata->threshold.  However, delay updating    */\
		/* these variables until prof_{m,re}alloc(), because  */\
		/* we don't know for sure that the allocation will    */\
		/* succeed.                                           */\
		/*                                                    */\
		/* Use subtraction rather than addition to avoid      */\
		/* potential integer overflow.                        */\
		if (size >= prof_tdata->threshold -			\
		    prof_tdata->accum) {				\
			bt_init(&bt, prof_tdata->vec);			\
			prof_backtrace(&bt, nignore);			\
			ret = prof_lookup(&bt);				\
		} else							\
			ret = (prof_thr_cnt_t *)(uintptr_t)1U;		\
	}								\
} while (0)

#ifndef JEMALLOC_ENABLE_INLINE
malloc_tsd_protos(JEMALLOC_ATTR(unused), prof_tdata, prof_tdata_t *)

prof_tdata_t	*prof_tdata_get(bool create);
void	prof_sample_threshold_update(prof_tdata_t *prof_tdata);
prof_ctx_t	*prof_ctx_get(const void *ptr);
void	prof_ctx_set(const void *ptr, size_t usize, prof_ctx_t *ctx);
bool	prof_sample_accum_update(size_t size);
void	prof_malloc(const void *ptr, size_t usize, prof_thr_cnt_t *cnt);
void	prof_realloc(const void *ptr, size_t usize, prof_thr_cnt_t *cnt,
    size_t old_usize, prof_ctx_t *old_ctx);
void	prof_free(const void *ptr, size_t size);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_PROF_C_))

malloc_tsd_externs(prof_tdata, prof_tdata_t *)
malloc_tsd_funcs(JEMALLOC_INLINE, prof_tdata, prof_tdata_t *, NULL,
    prof_tdata_cleanup)

JEMALLOC_INLINE prof_tdata_t *
prof_tdata_get(bool create)
{
	prof_tdata_t *prof_tdata;

	cassert(config_prof);

	prof_tdata = *prof_tdata_tsd_get();
	if (create && prof_tdata == NULL)
		prof_tdata = prof_tdata_init();

	return (prof_tdata);
}

JEMALLOC_INLINE void
prof_sample_threshold_update(prof_tdata_t *prof_tdata)
{
	












#ifdef JEMALLOC_PROF
	uint64_t r;
	double u;

	cassert(config_prof);

	

















	prng64(r, 53, prof_tdata->prng_state,
	    UINT64_C(6364136223846793005), UINT64_C(1442695040888963407));
	u = (double)r * (1.0/9007199254740992.0L);
	prof_tdata->threshold = (uint64_t)(log(u) /
	    log(1.0 - (1.0 / (double)((uint64_t)1U << opt_lg_prof_sample))))
	    + (uint64_t)1U;
#endif
}

JEMALLOC_INLINE prof_ctx_t *
prof_ctx_get(const void *ptr)
{
	prof_ctx_t *ret;
	arena_chunk_t *chunk;

	cassert(config_prof);
	assert(ptr != NULL);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr) {
		
		ret = arena_prof_ctx_get(ptr);
	} else
		ret = huge_prof_ctx_get(ptr);

	return (ret);
}

JEMALLOC_INLINE void
prof_ctx_set(const void *ptr, size_t usize, prof_ctx_t *ctx)
{
	arena_chunk_t *chunk;

	cassert(config_prof);
	assert(ptr != NULL);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr) {
		
		arena_prof_ctx_set(ptr, usize, ctx);
	} else
		huge_prof_ctx_set(ptr, ctx);
}

JEMALLOC_INLINE bool
prof_sample_accum_update(size_t size)
{
	prof_tdata_t *prof_tdata;

	cassert(config_prof);
	
	assert(opt_lg_prof_sample != 0);

	prof_tdata = prof_tdata_get(false);
	if ((uintptr_t)prof_tdata <= (uintptr_t)PROF_TDATA_STATE_MAX)
		return (true);

	
	if (size >= prof_tdata->threshold - prof_tdata->accum) {
		prof_tdata->accum -= (prof_tdata->threshold - size);
		
		prof_sample_threshold_update(prof_tdata);
		while (prof_tdata->accum >= prof_tdata->threshold) {
			prof_tdata->accum -= prof_tdata->threshold;
			prof_sample_threshold_update(prof_tdata);
		}
		return (false);
	} else {
		prof_tdata->accum += size;
		return (true);
	}
}

JEMALLOC_INLINE void
prof_malloc(const void *ptr, size_t usize, prof_thr_cnt_t *cnt)
{

	cassert(config_prof);
	assert(ptr != NULL);
	assert(usize == isalloc(ptr, true));

	if (opt_lg_prof_sample != 0) {
		if (prof_sample_accum_update(usize)) {
			






			assert((uintptr_t)cnt == (uintptr_t)1U);
		}
	}

	if ((uintptr_t)cnt > (uintptr_t)1U) {
		prof_ctx_set(ptr, usize, cnt->ctx);

		cnt->epoch++;
		
		mb_write();
		
		cnt->cnts.curobjs++;
		cnt->cnts.curbytes += usize;
		if (opt_prof_accum) {
			cnt->cnts.accumobjs++;
			cnt->cnts.accumbytes += usize;
		}
		
		mb_write();
		
		cnt->epoch++;
		
		mb_write();
		
	} else
		prof_ctx_set(ptr, usize, (prof_ctx_t *)(uintptr_t)1U);
}

JEMALLOC_INLINE void
prof_realloc(const void *ptr, size_t usize, prof_thr_cnt_t *cnt,
    size_t old_usize, prof_ctx_t *old_ctx)
{
	prof_thr_cnt_t *told_cnt;

	cassert(config_prof);
	assert(ptr != NULL || (uintptr_t)cnt <= (uintptr_t)1U);

	if (ptr != NULL) {
		assert(usize == isalloc(ptr, true));
		if (opt_lg_prof_sample != 0) {
			if (prof_sample_accum_update(usize)) {
				







				cnt = (prof_thr_cnt_t *)(uintptr_t)1U;
			}
		}
	}

	if ((uintptr_t)old_ctx > (uintptr_t)1U) {
		told_cnt = prof_lookup(old_ctx->bt);
		if (told_cnt == NULL) {
			



			malloc_mutex_lock(old_ctx->lock);
			old_ctx->cnt_merged.curobjs--;
			old_ctx->cnt_merged.curbytes -= old_usize;
			malloc_mutex_unlock(old_ctx->lock);
			told_cnt = (prof_thr_cnt_t *)(uintptr_t)1U;
		}
	} else
		told_cnt = (prof_thr_cnt_t *)(uintptr_t)1U;

	if ((uintptr_t)told_cnt > (uintptr_t)1U)
		told_cnt->epoch++;
	if ((uintptr_t)cnt > (uintptr_t)1U) {
		prof_ctx_set(ptr, usize, cnt->ctx);
		cnt->epoch++;
	} else if (ptr != NULL)
		prof_ctx_set(ptr, usize, (prof_ctx_t *)(uintptr_t)1U);
	
	mb_write();
	
	if ((uintptr_t)told_cnt > (uintptr_t)1U) {
		told_cnt->cnts.curobjs--;
		told_cnt->cnts.curbytes -= old_usize;
	}
	if ((uintptr_t)cnt > (uintptr_t)1U) {
		cnt->cnts.curobjs++;
		cnt->cnts.curbytes += usize;
		if (opt_prof_accum) {
			cnt->cnts.accumobjs++;
			cnt->cnts.accumbytes += usize;
		}
	}
	
	mb_write();
	
	if ((uintptr_t)told_cnt > (uintptr_t)1U)
		told_cnt->epoch++;
	if ((uintptr_t)cnt > (uintptr_t)1U)
		cnt->epoch++;
	
	mb_write(); 
}

JEMALLOC_INLINE void
prof_free(const void *ptr, size_t size)
{
	prof_ctx_t *ctx = prof_ctx_get(ptr);

	cassert(config_prof);

	if ((uintptr_t)ctx > (uintptr_t)1) {
		prof_thr_cnt_t *tcnt;
		assert(size == isalloc(ptr, true));
		tcnt = prof_lookup(ctx->bt);

		if (tcnt != NULL) {
			tcnt->epoch++;
			
			mb_write();
			
			tcnt->cnts.curobjs--;
			tcnt->cnts.curbytes -= size;
			
			mb_write();
			
			tcnt->epoch++;
			
			mb_write();
			
		} else {
			



			malloc_mutex_lock(ctx->lock);
			ctx->cnt_merged.curobjs--;
			ctx->cnt_merged.curbytes -= size;
			malloc_mutex_unlock(ctx->lock);
		}
	}
}
#endif

#endif 

