




































































































#ifndef MOZ_MEMORY_DEBUG
#  define	MALLOC_PRODUCTION
#endif

#ifndef MALLOC_PRODUCTION
   



#  define MALLOC_DEBUG


#  define MALLOC_STATS


#  define MALLOC_FILL


#  define MALLOC_UTRACE


#  define MALLOC_XMALLOC


#  define MALLOC_SYSV
#endif





















#if (!defined(MOZ_MEMORY_DARWIN) && !defined(MOZ_MEMORY_WINDOWS))
#define	MALLOC_DSS
#endif

#ifdef MOZ_MEMORY_LINUX
#define	_GNU_SOURCE
#define	issetugid() 0
#if 0 
#  define MALLOC_DECOMMIT




#  undef MALLOC_DSS
#endif
#endif

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MOZ_MEMORY_WINDOWS
#include <cruntime.h>
#include <internal.h>
#include <windows.h>
#include <io.h>
#include "tree.h"

#pragma warning( disable: 4267 4996 4146 )

#define	bool BOOL
#define	false FALSE
#define	true TRUE
#define	inline __inline
#define	SIZE_T_MAX SIZE_MAX
#define	STDERR_FILENO 2
#define	PATH_MAX MAX_PATH
#define	vsnprintf _vsnprintf
#define	assert(f)

static unsigned long tlsIndex = 0xffffffff;

#define	__thread
#define	_pthread_self() __threadid()
#define	issetugid() 0


#pragma intrinsic(_BitScanForward)
static __forceinline int
ffs(int x)
{
	unsigned long i;

	if (_BitScanForward(&i, x) != 0)
		return (i + 1);

	return (0);
}


static char mozillaMallocOptionsBuf[64];

#define	getenv xgetenv
static char *
getenv(const char *name)
{

	if (GetEnvironmentVariableA(name, (LPSTR)&mozillaMallocOptionsBuf,
		    sizeof(mozillaMallocOptionsBuf)) > 0)
		return (mozillaMallocOptionsBuf);

	return (NULL);
}

typedef unsigned char uint8_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long uintmax_t;

#define	MALLOC_DECOMMIT
#endif

#ifndef MOZ_MEMORY_WINDOWS
#include <sys/cdefs.h>
#ifndef __DECONST
#  define __DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif
#ifndef MOZ_MEMORY
__FBSDID("$FreeBSD: src/lib/libc/stdlib/malloc.c,v 1.162 2008/02/06 02:59:54 jasone Exp $");
#include "libc_private.h"
#ifdef MALLOC_DEBUG
#  define _LOCK_DEBUG
#endif
#include "spinlock.h"
#include "namespace.h"
#endif
#include <sys/mman.h>
#ifndef MADV_FREE
#  define MADV_FREE	MADV_DONTNEED
#endif
#include <sys/param.h>
#ifndef MOZ_MEMORY
#include <sys/stddef.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include "tree.h"
#ifndef MOZ_MEMORY
#include <sys/tree.h>
#endif
#include <sys/uio.h>
#ifndef MOZ_MEMORY
#include <sys/ktrace.h> 

#include <machine/atomic.h>
#include <machine/cpufunc.h>
#include <machine/vmparam.h>
#endif

#include <errno.h>
#include <limits.h>
#ifndef SIZE_T_MAX
#  define SIZE_T_MAX	SIZE_MAX
#endif
#include <pthread.h>
#ifdef MOZ_MEMORY_DARWIN
#define _pthread_self pthread_self
#define _pthread_mutex_init pthread_mutex_init
#define _pthread_mutex_trylock pthread_mutex_trylock
#define _pthread_mutex_lock pthread_mutex_lock
#define _pthread_mutex_unlock pthread_mutex_unlock
#endif
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef MOZ_MEMORY_DARWIN
#include <strings.h>
#endif
#include <unistd.h>

#ifdef MOZ_MEMORY_DARWIN
#include <libkern/OSAtomic.h>
#include <mach/mach_error.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <malloc/malloc.h>
#endif

#ifndef MOZ_MEMORY
#include "un-namespace.h"
#endif

#endif

#ifdef MOZ_MEMORY_DARWIN
static const bool __isthreaded = true;
#endif

#define __DECONST(type, var) ((type)(uintptr_t)(const void *)(var))

#ifdef MALLOC_DEBUG
#  ifdef NDEBUG
#    undef NDEBUG
#  endif
#else
#  ifndef NDEBUG
#    define NDEBUG
#  endif
#endif
#ifndef MOZ_MEMORY_WINDOWS
#include <assert.h>
#endif

#ifdef MALLOC_DEBUG
   
#ifdef inline
#undef inline
#endif

#  define inline
#endif

#ifndef MOZ_MEMORY_WINDOWS
#define	VISIBLE __attribute__((visibility("default")))
#else
#define	VISIBLE
#endif


#define	STRERROR_BUF		64


#  define QUANTUM_2POW_MIN      4
#ifdef MOZ_MEMORY_SIZEOF_PTR_2POW
#  define SIZEOF_PTR_2POW		MOZ_MEMORY_SIZEOF_PTR_2POW
#else
#  define SIZEOF_PTR_2POW       2
#endif
#define PIC
#ifndef MOZ_MEMORY_DARWIN
static const bool __isthreaded = true;
#else
#  define NO_TLS
#endif
#if 0
#ifdef __i386__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	2
#  define CPU_SPINWAIT		__asm__ volatile("pause")
#endif
#ifdef __ia64__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#endif
#ifdef __alpha__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#  define NO_TLS
#endif
#ifdef __sparc64__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#  define NO_TLS
#endif
#ifdef __amd64__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	3
#  define CPU_SPINWAIT		__asm__ volatile("pause")
#endif
#ifdef __arm__
#  define QUANTUM_2POW_MIN	3
#  define SIZEOF_PTR_2POW	2
#  define NO_TLS
#endif
#ifdef __powerpc__
#  define QUANTUM_2POW_MIN	4
#  define SIZEOF_PTR_2POW	2
#endif
#endif

#define	SIZEOF_PTR		(1U << SIZEOF_PTR_2POW)


#ifndef SIZEOF_INT_2POW
#  define SIZEOF_INT_2POW	2
#endif


#if (!defined(PIC) && !defined(NO_TLS))
#  define NO_TLS
#endif

#ifdef NO_TLS
   
#  ifdef MALLOC_BALANCE
#    undef MALLOC_BALANCE
#  endif
   
#  ifdef MALLOC_LAZY_FREE
#    undef MALLOC_LAZY_FREE
#  endif
#endif





#define	CHUNK_2POW_DEFAULT	20


#define	DIRTY_MAX_DEFAULT	(1U << 9)






#define	CACHELINE_2POW		6
#define	CACHELINE		((size_t)(1U << CACHELINE_2POW))


#define	TINY_MIN_2POW		1






#define	SMALL_MAX_2POW_DEFAULT	9
#define	SMALL_MAX_DEFAULT	(1U << SMALL_MAX_2POW_DEFAULT)

















#define	RUN_BFP			12

#define	RUN_MAX_OVRHD		0x0000003dU
#define	RUN_MAX_OVRHD_RELAX	0x00001800U






#define	RUN_MAX_SMALL_2POW	15
#define	RUN_MAX_SMALL		(1U << RUN_MAX_SMALL_2POW)

#ifdef MALLOC_LAZY_FREE
   
#  define LAZY_FREE_2POW_DEFAULT 8
   






#  define LAZY_FREE_NPROBES	5
#endif






#ifndef CPU_SPINWAIT
#  define CPU_SPINWAIT
#endif






#define	SPIN_LIMIT_2POW		11






#define	BLOCK_COST_2POW		4

#ifdef MALLOC_BALANCE
   







#  define BALANCE_ALPHA_INV_2POW	9

   



#  define BALANCE_THRESHOLD_DEFAULT	(1U << (SPIN_LIMIT_2POW-4))
#endif








#if defined(MOZ_MEMORY_WINDOWS)
#define malloc_mutex_t CRITICAL_SECTION
#define malloc_spinlock_t CRITICAL_SECTION
#elif defined(MOZ_MEMORY_DARWIN)
typedef struct {
	OSSpinLock	lock;
} malloc_mutex_t;
typedef struct {
	OSSpinLock	lock;
} malloc_spinlock_t;
#elif defined(MOZ_MEMORY)
typedef pthread_mutex_t malloc_mutex_t;
typedef pthread_mutex_t malloc_spinlock_t;
#else

typedef struct {
	spinlock_t	lock;
} malloc_mutex_t;
typedef malloc_spinlock_t malloc_mutex_t;
#endif


static bool malloc_initialized = false;

#if defined(MOZ_MEMORY_WINDOWS)

#elif defined(MOZ_MEMORY_DARWIN)
static malloc_mutex_t init_lock = {OS_SPINLOCK_INIT};
#elif defined(MOZ_MEMORY_LINUX)
static malloc_mutex_t init_lock = PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;
#elif defined(MOZ_MEMORY)
static malloc_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
#else
static malloc_mutex_t init_lock = {_SPINLOCK_INITIALIZER};
#endif






#ifdef MALLOC_STATS

typedef struct malloc_bin_stats_s malloc_bin_stats_t;
struct malloc_bin_stats_s {
	



	uint64_t	nrequests;

	
	uint64_t	nruns;

	



	uint64_t	reruns;

	
	unsigned long	highruns;

	
	unsigned long	curruns;
};

typedef struct arena_stats_s arena_stats_t;
struct arena_stats_s {
	
	size_t		mapped;

	




	uint64_t	npurge;
	uint64_t	nmadvise;
	uint64_t	purged;
#ifdef MALLOC_DECOMMIT
	



	uint64_t	ndecommit;
	uint64_t	ncommit;
	uint64_t	decommitted;
#endif

	
	size_t		allocated_small;
	uint64_t	nmalloc_small;
	uint64_t	ndalloc_small;

	size_t		allocated_large;
	uint64_t	nmalloc_large;
	uint64_t	ndalloc_large;

#ifdef MALLOC_BALANCE
	
	uint64_t	nbalance;
#endif
};

typedef struct chunk_stats_s chunk_stats_t;
struct chunk_stats_s {
	
	uint64_t	nchunks;

	
	unsigned long	highchunks;

	




	unsigned long	curchunks;
};

#endif 







typedef struct extent_node_s extent_node_t;
struct extent_node_s {
	
	RB_ENTRY(extent_node_s) link_szad;

	
	RB_ENTRY(extent_node_s) link_ad;

	
	void	*addr;

	
	size_t	size;
};
typedef struct extent_tree_szad_s extent_tree_szad_t;
RB_HEAD(extent_tree_szad_s, extent_node_s);
typedef struct extent_tree_ad_s extent_tree_ad_t;
RB_HEAD(extent_tree_ad_s, extent_node_s);






typedef struct arena_s arena_t;
typedef struct arena_bin_s arena_bin_t;





typedef uint8_t arena_chunk_map_t;
#define	CHUNK_MAP_UNTOUCHED	0x80U
#define	CHUNK_MAP_DIRTY		0x40U
#define	CHUNK_MAP_LARGE		0x20U
#ifdef MALLOC_DECOMMIT
#define	CHUNK_MAP_DECOMMITTED	0x10U
#define	CHUNK_MAP_POS_MASK	0x0fU
#else
#define	CHUNK_MAP_POS_MASK	0x1fU
#endif


typedef struct arena_chunk_s arena_chunk_t;
struct arena_chunk_s {
	
	arena_t		*arena;

	
	RB_ENTRY(arena_chunk_s) link;

	



	size_t		pages_used;

	
	size_t		ndirty;

	



	extent_tree_ad_t nodes;
	extent_node_t	*nodes_past;

	




	arena_chunk_map_t map[1]; 
};
typedef struct arena_chunk_tree_s arena_chunk_tree_t;
RB_HEAD(arena_chunk_tree_s, arena_chunk_s);

typedef struct arena_run_s arena_run_t;
struct arena_run_s {
	
	RB_ENTRY(arena_run_s) link;

#ifdef MALLOC_DEBUG
	uint32_t	magic;
#  define ARENA_RUN_MAGIC 0x384adf93
#endif

	
	arena_bin_t	*bin;

	
	unsigned	regs_minelm;

	
	unsigned	nfree;

	
	unsigned	regs_mask[1]; 
};
typedef struct arena_run_tree_s arena_run_tree_t;
RB_HEAD(arena_run_tree_s, arena_run_s);

struct arena_bin_s {
	



	arena_run_t	*runcur;

	






	arena_run_tree_t runs;

	
	size_t		reg_size;

	
	size_t		run_size;

	
	uint32_t	nregs;

	
	uint32_t	regs_mask_nelms;

	
	uint32_t	reg0_offset;

#ifdef MALLOC_STATS
	
	malloc_bin_stats_t stats;
#endif
};

struct arena_s {
#ifdef MALLOC_DEBUG
	uint32_t		magic;
#  define ARENA_MAGIC 0x947d3d24
#endif

	
#ifdef MOZ_MEMORY
	malloc_spinlock_t	lock;
#else
	pthread_mutex_t		lock;
#endif

#ifdef MALLOC_STATS
	arena_stats_t		stats;
#endif

	


	arena_chunk_tree_t	chunks;

	









	arena_chunk_t		*spare;

	





	size_t			ndirty;

	




	extent_tree_szad_t	runs_avail_szad;
	extent_tree_ad_t	runs_avail_ad;

	
	extent_tree_ad_t	runs_alloced_ad;

#ifdef MALLOC_BALANCE
	



	uint32_t		contention;
#endif

#ifdef MALLOC_LAZY_FREE
	





	void			**free_cache;
#endif

	






















	arena_bin_t		bins[1]; 
};







static unsigned		ncpus;


static size_t		pagesize;
static size_t		pagesize_mask;
static size_t		pagesize_2pow;


static size_t		bin_maxclass; 
static unsigned		ntbins; 
static unsigned		nqbins; 
static unsigned		nsbins; 
static size_t		small_min;
static size_t		small_max;


static size_t		quantum;
static size_t		quantum_mask; 


static size_t		chunksize;
static size_t		chunksize_mask; 
static size_t		chunk_npages;
static size_t		arena_chunk_header_npages;
static size_t		arena_maxclass; 







static malloc_mutex_t	huge_mtx;


static extent_tree_ad_t	huge;

#ifdef MALLOC_DSS




static malloc_mutex_t	dss_mtx;

static void		*dss_base;

static void		*dss_prev;

static void		*dss_max;







static extent_tree_szad_t dss_chunks_szad;
static extent_tree_ad_t	dss_chunks_ad;
#endif

#ifdef MALLOC_STATS

static uint64_t		huge_nmalloc;
static uint64_t		huge_ndalloc;
static size_t		huge_allocated;
#endif











static void		*base_pages;
static void		*base_next_addr;
static void		*base_past_addr; 
static extent_node_t	*base_nodes;
static malloc_mutex_t	base_mtx;
#ifdef MALLOC_STATS
static size_t		base_mapped;
#endif










static arena_t		**arenas;
static unsigned		narenas;
#ifndef NO_TLS
#  ifdef MALLOC_BALANCE
static unsigned		narenas_2pow;
#  else
static unsigned		next_arena;
#  endif
#endif
#ifdef MOZ_MEMORY
static malloc_spinlock_t arenas_lock; 
#else
static pthread_mutex_t arenas_lock; 
#endif

#ifndef NO_TLS




#ifndef MOZ_MEMORY_WINDOWS
static __thread arena_t	*arenas_map;
#endif
#endif

#ifdef MALLOC_STATS

static chunk_stats_t	stats_chunks;
#endif





const char	*_malloc_options
#ifdef MOZ_MEMORY_WINDOWS
= "A10n2F"
#elif (defined(MOZ_MEMORY_DARWIN))
= "AP10n"
#elif (defined(MOZ_MEMORY_LINUX))
= "A10n2F"
#endif
;

#ifndef MALLOC_PRODUCTION
static bool	opt_abort = true;
#ifdef MALLOC_FILL
static bool	opt_junk = true;
#endif
#else
static bool	opt_abort = false;
#ifdef MALLOC_FILL
static bool	opt_junk = false;
#endif
#endif
#ifdef MALLOC_DSS
static bool	opt_dss = true;
static bool	opt_mmap = true;
#endif
static size_t	opt_dirty_max = DIRTY_MAX_DEFAULT;
#ifdef MALLOC_LAZY_FREE
static int	opt_lazy_free_2pow = LAZY_FREE_2POW_DEFAULT;
#endif
#ifdef MALLOC_BALANCE
static uint64_t	opt_balance_threshold = BALANCE_THRESHOLD_DEFAULT;
#endif
static bool	opt_print_stats = false;
static size_t	opt_quantum_2pow = QUANTUM_2POW_MIN;
static size_t	opt_small_max_2pow = SMALL_MAX_2POW_DEFAULT;
static size_t	opt_chunk_2pow = CHUNK_2POW_DEFAULT;
#ifdef MALLOC_UTRACE
static bool	opt_utrace = false;
#endif
#ifdef MALLOC_SYSV
static bool	opt_sysv = false;
#endif
#ifdef MALLOC_XMALLOC
static bool	opt_xmalloc = false;
#endif
#ifdef MALLOC_FILL
static bool	opt_zero = false;
#endif
static int	opt_narenas_lshift = 0;

#ifdef MALLOC_UTRACE
typedef struct {
	void	*p;
	size_t	s;
	void	*r;
} malloc_utrace_t;

#define	UTRACE(a, b, c)							\
	if (opt_utrace) {						\
		malloc_utrace_t ut;					\
		ut.p = (a);						\
		ut.s = (b);						\
		ut.r = (c);						\
		utrace(&ut, sizeof(ut));				\
	}
#else
#define	UTRACE(a, b, c)
#endif






static bool	malloc_mutex_init(malloc_mutex_t *mutex);
static bool	malloc_spin_init(malloc_spinlock_t *lock);
static void	wrtmessage(const char *p1, const char *p2, const char *p3,
		const char *p4);
#ifdef MALLOC_STATS
#ifdef MOZ_MEMORY_DARWIN

#define malloc_printf xmalloc_printf
#endif
static void	malloc_printf(const char *format, ...);
#endif
static char	*umax2s(uintmax_t x, char *s);
#ifdef MALLOC_DSS
static bool	base_pages_alloc_dss(size_t minsize);
#endif
static bool	base_pages_alloc_mmap(size_t minsize);
static bool	base_pages_alloc(size_t minsize);
static void	*base_alloc(size_t size);
static void	*base_calloc(size_t number, size_t size);
static extent_node_t *base_node_alloc(void);
static void	base_node_dealloc(extent_node_t *node);
#ifdef MALLOC_STATS
static void	stats_print(arena_t *arena);
#endif
static void	*pages_map(void *addr, size_t size);
static void	pages_unmap(void *addr, size_t size);
#ifdef MALLOC_DSS
static void	*chunk_alloc_dss(size_t size);
static void	*chunk_recycle_dss(size_t size, bool zero);
#endif
static void	*chunk_alloc_mmap(size_t size);
static void	*chunk_alloc(size_t size, bool zero);
#ifdef MALLOC_DSS
static extent_node_t *chunk_dealloc_dss_record(void *chunk, size_t size);
static bool	chunk_dealloc_dss(void *chunk, size_t size);
#endif
static void	chunk_dealloc_mmap(void *chunk, size_t size);
static void	chunk_dealloc(void *chunk, size_t size);
#ifndef NO_TLS
static arena_t	*choose_arena_hard(void);
#endif
static extent_node_t *arena_chunk_node_alloc(arena_chunk_t *chunk);
static void	arena_chunk_node_dealloc(arena_chunk_t *chunk,
    extent_node_t *node);
static void	arena_run_split(arena_t *arena, arena_run_t *run, size_t size,
    bool small, bool zero);
static arena_chunk_t *arena_chunk_alloc(arena_t *arena);
static void	arena_chunk_dealloc(arena_t *arena, arena_chunk_t *chunk);
static arena_run_t *arena_run_alloc(arena_t *arena, size_t size, bool small,
    bool zero);
static void	arena_purge(arena_t *arena);
static void	arena_run_dalloc(arena_t *arena, arena_run_t *run, bool dirty);
static void	arena_run_trim_head(arena_t *arena, arena_chunk_t *chunk,
    extent_node_t *nodeB, arena_run_t *run, size_t oldsize, size_t newsize);
static void	arena_run_trim_tail(arena_t *arena, arena_chunk_t *chunk,
    extent_node_t *nodeA, arena_run_t *run, size_t oldsize, size_t newsize,
    bool dirty);
static arena_run_t *arena_bin_nonfull_run_get(arena_t *arena, arena_bin_t *bin);
static void *arena_bin_malloc_hard(arena_t *arena, arena_bin_t *bin);
static size_t arena_bin_run_size_calc(arena_bin_t *bin, size_t min_run_size);
#ifdef MALLOC_BALANCE
static void	arena_lock_balance_hard(arena_t *arena);
#endif
static void	*arena_malloc_large(arena_t *arena, size_t size, bool zero);
static void	*arena_palloc(arena_t *arena, size_t alignment, size_t size,
    size_t alloc_size);
static size_t	arena_salloc(const void *ptr);
#ifdef MALLOC_LAZY_FREE
static void	arena_dalloc_lazy_hard(arena_t *arena, arena_chunk_t *chunk,
    void *ptr, size_t pageind, arena_chunk_map_t *mapelm);
#endif
static void	arena_dalloc_large(arena_t *arena, arena_chunk_t *chunk,
    void *ptr);
static void	arena_ralloc_large_shrink(arena_t *arena, arena_chunk_t *chunk,
    void *ptr, size_t size, size_t oldsize);
static bool	arena_ralloc_large_grow(arena_t *arena, arena_chunk_t *chunk,
    void *ptr, size_t size, size_t oldsize);
static bool	arena_ralloc_large(void *ptr, size_t size, size_t oldsize);
static void	*arena_ralloc(void *ptr, size_t size, size_t oldsize);
static bool	arena_new(arena_t *arena);
static arena_t	*arenas_extend(unsigned ind);
static void	*huge_malloc(size_t size, bool zero);
static void	*huge_palloc(size_t alignment, size_t size);
static void	*huge_ralloc(void *ptr, size_t size, size_t oldsize);
static void	huge_dalloc(void *ptr);
static void	malloc_print_stats(void);
#ifndef MOZ_MEMORY_WINDOWS
static
#endif
bool		malloc_init_hard(void);











static bool
malloc_mutex_init(malloc_mutex_t *mutex)
{
#if defined(MOZ_MEMORY_WINDOWS)
	if (__isthreaded)
		if (! __crtInitCritSecAndSpinCount(mutex, _CRT_SPINCOUNT))
			return (true);
#elif defined(MOZ_MEMORY_DARWIN)
	mutex->lock = OS_SPINLOCK_INIT;
#elif defined(MOZ_MEMORY_LINUX)
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) != 0)
		return (true);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	if (pthread_mutex_init(mutex, &attr) != 0) {
		pthread_mutexattr_destroy(&attr);
		return (true);
	}
	pthread_mutexattr_destroy(&attr);
#elif defined(MOZ_MEMORY)
	if (pthread_mutex_init(mutex, NULL) != 0)
		return (true);
#else
	static const spinlock_t lock = _SPINLOCK_INITIALIZER;

	mutex->lock = lock;
#endif
	return (false);
}

static inline void
malloc_mutex_lock(malloc_mutex_t *mutex)
{

#if defined(MOZ_MEMORY_WINDOWS)
	EnterCriticalSection(mutex);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockLock(&mutex->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_lock(mutex);
#else
	if (__isthreaded)
		_SPINLOCK(&mutex->lock);
#endif
}

static inline void
malloc_mutex_unlock(malloc_mutex_t *mutex)
{

#if defined(MOZ_MEMORY_WINDOWS)
	LeaveCriticalSection(mutex);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockUnlock(&mutex->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_unlock(mutex);
#else
	if (__isthreaded)
		_SPINUNLOCK(&mutex->lock);
#endif
}

static bool
malloc_spin_init(malloc_spinlock_t *lock)
{
#if defined(MOZ_MEMORY_WINDOWS)
	if (__isthreaded)
		if (! __crtInitCritSecAndSpinCount(lock, _CRT_SPINCOUNT))
			return (true);
#elif defined(MOZ_MEMORY_DARWIN)
	lock->lock = OS_SPINLOCK_INIT;
#elif defined(MOZ_MEMORY_LINUX)
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) != 0)
		return (true);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	if (pthread_mutex_init(lock, &attr) != 0) {
		pthread_mutexattr_destroy(&attr);
		return (true);
	}
	pthread_mutexattr_destroy(&attr);
#elif defined(MOZ_MEMORY)
	if (pthread_mutex_init(lock, NULL) != 0)
		return (true);
#else
	lock->lock = _SPINLOCK_INITIALIZER;
#endif
	return (false);
}

static inline void
malloc_spin_lock(malloc_spinlock_t *lock)
{

#if defined(MOZ_MEMORY_WINDOWS)
	EnterCriticalSection(lock);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockLock(&lock->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_lock(lock);
#else
	if (__isthreaded)
		_SPINLOCK(&lock->lock);
#endif
}

static inline void
malloc_spin_unlock(malloc_spinlock_t *lock)
{
#if defined(MOZ_MEMORY_WINDOWS)
	LeaveCriticalSection(lock);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockUnlock(&lock->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_unlock(lock);
#else
	if (__isthreaded)
		_SPINUNLOCK(&lock->lock);
#endif
}











#if defined(MOZ_MEMORY) && !defined(MOZ_MEMORY_DARWIN)
#  define	malloc_spin_init	malloc_mutex_init
#  define	malloc_spin_lock	malloc_mutex_lock
#  define	malloc_spin_unlock	malloc_mutex_unlock
#endif

#ifndef MOZ_MEMORY




int	_pthread_mutex_init_calloc_cb(pthread_mutex_t *mutex,
    void *(calloc_cb)(size_t, size_t));

__weak_reference(_pthread_mutex_init_calloc_cb_stub,
    _pthread_mutex_init_calloc_cb);

int
_pthread_mutex_init_calloc_cb_stub(pthread_mutex_t *mutex,
    void *(calloc_cb)(size_t, size_t))
{

	return (0);
}

static bool
malloc_spin_init(pthread_mutex_t *lock)
{

	if (_pthread_mutex_init_calloc_cb(lock, base_calloc) != 0)
		return (true);

	return (false);
}

static inline unsigned
malloc_spin_lock(pthread_mutex_t *lock)
{
	unsigned ret = 0;

	if (__isthreaded) {
		if (_pthread_mutex_trylock(lock) != 0) {
			unsigned i;
			volatile unsigned j;

			
			for (i = 1; i <= SPIN_LIMIT_2POW; i++) {
				for (j = 0; j < (1U << i); j++)
					ret++;

				CPU_SPINWAIT;
				if (_pthread_mutex_trylock(lock) == 0)
					return (ret);
			}

			




			_pthread_mutex_lock(lock);
			assert((ret << BLOCK_COST_2POW) != 0);
			return (ret << BLOCK_COST_2POW);
		}
	}

	return (ret);
}

static inline void
malloc_spin_unlock(pthread_mutex_t *lock)
{

	if (__isthreaded)
		_pthread_mutex_unlock(lock);
}
#endif











#define	CHUNK_ADDR2BASE(a)						\
	((void *)((uintptr_t)(a) & ~chunksize_mask))


#define	CHUNK_ADDR2OFFSET(a)						\
	((size_t)((uintptr_t)(a) & chunksize_mask))


#define	CHUNK_CEILING(s)						\
	(((s) + chunksize_mask) & ~chunksize_mask)


#define	CACHELINE_CEILING(s)						\
	(((s) + (CACHELINE - 1)) & ~(CACHELINE - 1))


#define	QUANTUM_CEILING(a)						\
	(((a) + quantum_mask) & ~quantum_mask)


#define	PAGE_CEILING(s)							\
	(((s) + pagesize_mask) & ~pagesize_mask)


static inline size_t
pow2_ceil(size_t x)
{

	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
#if (SIZEOF_PTR == 8)
	x |= x >> 32;
#endif
	x++;
	return (x);
}

#if (defined(MALLOC_LAZY_FREE) || defined(MALLOC_BALANCE))


















#  define PRN_DEFINE(suffix, var, a, c)					\
static inline void							\
sprn_##suffix(uint32_t seed)						\
{									\
	var = seed;							\
}									\
									\
static inline uint32_t							\
prn_##suffix(uint32_t lg_range)						\
{									\
	uint32_t ret, x;						\
									\
	assert(lg_range > 0);						\
	assert(lg_range <= 32);						\
									\
	x = (var * (a)) + (c);						\
	var = x;							\
	ret = x >> (32 - lg_range);					\
									\
	return (ret);							\
}
#  define SPRN(suffix, seed)	sprn_##suffix(seed)
#  define PRN(suffix, lg_range)	prn_##suffix(lg_range)
#endif






#ifdef MALLOC_LAZY_FREE

static __thread uint32_t lazy_free_x;
PRN_DEFINE(lazy_free, lazy_free_x, 12345, 12347)
#endif

#ifdef MALLOC_BALANCE

static __thread uint32_t balance_x;
PRN_DEFINE(balance, balance_x, 1297, 1301)
#endif

#ifdef MALLOC_UTRACE
static int
utrace(const void *addr, size_t len)
{
	malloc_utrace_t *ut = (malloc_utrace_t *)addr;

	assert(len == sizeof(malloc_utrace_t));

	if (ut->p == NULL && ut->s == 0 && ut->r == NULL)
		malloc_printf("%d x USER malloc_init()\n", getpid());
	else if (ut->p == NULL && ut->r != NULL) {
		malloc_printf("%d x USER %p = malloc(%zu)\n", getpid(), ut->r,
		    ut->s);
	} else if (ut->p != NULL && ut->r != NULL) {
		malloc_printf("%d x USER %p = realloc(%p, %zu)\n", getpid(),
		    ut->r, ut->p, ut->s);
	} else
		malloc_printf("%d x USER free(%p)\n", getpid(), ut->p);

	return (0);
}
#endif

static inline const char *
_getprogname(void)
{

	return ("<jemalloc>");
}

static void
wrtmessage(const char *p1, const char *p2, const char *p3, const char *p4)
{
#if defined(MOZ_MEMORY) && !defined(MOZ_MEMORY_WINDOWS)
#define	_write	write
#endif
	_write(STDERR_FILENO, p1, (unsigned int) strlen(p1));
	_write(STDERR_FILENO, p2, (unsigned int) strlen(p2));
	_write(STDERR_FILENO, p3, (unsigned int) strlen(p3));
	_write(STDERR_FILENO, p4, (unsigned int) strlen(p4));
}

#define _malloc_message malloc_message

void	(*_malloc_message)(const char *p1, const char *p2, const char *p3,
	    const char *p4) = wrtmessage;

#ifdef MALLOC_STATS



static void
malloc_printf(const char *format, ...)
{
	char buf[4096];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	_malloc_message(buf, "", "", "");
}
#endif







#define	UMAX2S_BUFSIZE	21
static char *
umax2s(uintmax_t x, char *s)
{
	unsigned i;

	
	assert(sizeof(uintmax_t) <= 8);

	i = UMAX2S_BUFSIZE - 1;
	s[i] = '\0';
	do {
		i--;
		s[i] = "0123456789"[x % 10];
		x /= 10;
	} while (x > 0);

	return (&s[i]);
}



#ifdef MALLOC_DSS
static bool
base_pages_alloc_dss(size_t minsize)
{

	



	malloc_mutex_lock(&dss_mtx);
	if (dss_prev != (void *)-1) {
		intptr_t incr;
		size_t csize = CHUNK_CEILING(minsize);

		do {
			
			dss_max = sbrk(0);

			




			incr = (intptr_t)chunksize
			    - (intptr_t)CHUNK_ADDR2OFFSET(dss_max);
			assert(incr >= 0);
			if ((size_t)incr < minsize)
				incr += csize;

			dss_prev = sbrk(incr);
			if (dss_prev == dss_max) {
				
				dss_max = (void *)((intptr_t)dss_prev + incr);
				base_pages = dss_prev;
				base_next_addr = base_pages;
				base_past_addr = dss_max;
#ifdef MALLOC_STATS
				base_mapped += incr;
#endif
				malloc_mutex_unlock(&dss_mtx);
				return (false);
			}
		} while (dss_prev != (void *)-1);
	}
	malloc_mutex_unlock(&dss_mtx);

	return (true);
}
#endif

static bool
base_pages_alloc_mmap(size_t minsize)
{
	size_t csize;

	assert(minsize != 0);
	csize = PAGE_CEILING(minsize);
	base_pages = pages_map(NULL, csize);
	if (base_pages == NULL)
		return (true);
	base_next_addr = base_pages;
	base_past_addr = (void *)((uintptr_t)base_pages + csize);
#ifdef MALLOC_STATS
	base_mapped += csize;
#endif

	return (false);
}

static bool
base_pages_alloc(size_t minsize)
{

#ifdef MALLOC_DSS
	if (opt_dss) {
		if (base_pages_alloc_dss(minsize) == false)
			return (false);
	}

	if (opt_mmap && minsize != 0)
#endif
	{
		if (base_pages_alloc_mmap(minsize) == false)
			return (false);
	}

	return (true);
}

static void *
base_alloc(size_t size)
{
	void *ret;
	size_t csize;

	
	csize = CACHELINE_CEILING(size);

	malloc_mutex_lock(&base_mtx);
	
	if ((uintptr_t)base_next_addr + csize > (uintptr_t)base_past_addr) {
		if (base_pages_alloc(csize))
			return (NULL);
	}
	
	ret = base_next_addr;
	base_next_addr = (void *)((uintptr_t)base_next_addr + csize);
	malloc_mutex_unlock(&base_mtx);

	return (ret);
}

static void *
base_calloc(size_t number, size_t size)
{
	void *ret;

	ret = base_alloc(number * size);
	memset(ret, 0, number * size);

	return (ret);
}

static extent_node_t *
base_node_alloc(void)
{
	extent_node_t *ret;

	malloc_mutex_lock(&base_mtx);
	if (base_nodes != NULL) {
		ret = base_nodes;
		base_nodes = *(extent_node_t **)ret;
		malloc_mutex_unlock(&base_mtx);
	} else {
		malloc_mutex_unlock(&base_mtx);
		ret = (extent_node_t *)base_alloc(sizeof(extent_node_t));
	}

	return (ret);
}

static void
base_node_dealloc(extent_node_t *node)
{

	malloc_mutex_lock(&base_mtx);
	*(extent_node_t **)node = base_nodes;
	base_nodes = node;
	malloc_mutex_unlock(&base_mtx);
}



#ifdef MALLOC_STATS
static void
stats_print(arena_t *arena)
{
	unsigned i, gap_start;

#ifdef MOZ_MEMORY_WINDOWS
	malloc_printf("dirty: %Iu page%s dirty, %I64u sweep%s,"
	    " %I64u madvise%s, %I64u page%s purged\n",
	    arena->ndirty, arena->ndirty == 1 ? "" : "s",
	    arena->stats.npurge, arena->stats.npurge == 1 ? "" : "s",
	    arena->stats.nmadvise, arena->stats.nmadvise == 1 ? "" : "s",
	    arena->stats.purged, arena->stats.purged == 1 ? "" : "s");
#  ifdef MALLOC_DECOMMIT
	malloc_printf("decommit: %I64u decommit%s, %I64u commit%s,"
	    " %I64u page%s decommitted\n",
	    arena->stats.ndecommit, (arena->stats.ndecommit == 1) ? "" : "s",
	    arena->stats.ncommit, (arena->stats.ncommit == 1) ? "" : "s",
	    arena->stats.decommitted,
	    (arena->stats.decommitted == 1) ? "" : "s");
#  endif

	malloc_printf("            allocated      nmalloc      ndalloc\n");
	malloc_printf("small:   %12Iu %12I64u %12I64u\n",
	    arena->stats.allocated_small, arena->stats.nmalloc_small,
	    arena->stats.ndalloc_small);
	malloc_printf("large:   %12Iu %12I64u %12I64u\n",
	    arena->stats.allocated_large, arena->stats.nmalloc_large,
	    arena->stats.ndalloc_large);
	malloc_printf("total:   %12Iu %12I64u %12I64u\n",
	    arena->stats.allocated_small + arena->stats.allocated_large,
	    arena->stats.nmalloc_small + arena->stats.nmalloc_large,
	    arena->stats.ndalloc_small + arena->stats.ndalloc_large);
	malloc_printf("mapped:  %12Iu\n", arena->stats.mapped);
#else
	malloc_printf("dirty: %zu page%s dirty, %llu sweep%s,"
	    " %llu madvise%s, %llu page%s purged\n",
	    arena->ndirty, arena->ndirty == 1 ? "" : "s",
	    arena->stats.npurge, arena->stats.npurge == 1 ? "" : "s",
	    arena->stats.nmadvise, arena->stats.nmadvise == 1 ? "" : "s",
	    arena->stats.purged, arena->stats.purged == 1 ? "" : "s");
#  ifdef MALLOC_DECOMMIT
	malloc_printf("decommit: %llu decommit%s, %llu commit%s,"
	    " %llu page%s decommitted\n",
	    arena->stats.ndecommit, (arena->stats.ndecommit == 1) ? "" : "s",
	    arena->stats.ncommit, (arena->stats.ncommit == 1) ? "" : "s",
	    arena->stats.decommitted,
	    (arena->stats.decommitted == 1) ? "" : "s");
#  endif

	malloc_printf("            allocated      nmalloc      ndalloc\n");
	malloc_printf("small:   %12zu %12llu %12llu\n",
	    arena->stats.allocated_small, arena->stats.nmalloc_small,
	    arena->stats.ndalloc_small);
	malloc_printf("large:   %12zu %12llu %12llu\n",
	    arena->stats.allocated_large, arena->stats.nmalloc_large,
	    arena->stats.ndalloc_large);
	malloc_printf("total:   %12zu %12llu %12llu\n",
	    arena->stats.allocated_small + arena->stats.allocated_large,
	    arena->stats.nmalloc_small + arena->stats.nmalloc_large,
	    arena->stats.ndalloc_small + arena->stats.ndalloc_large);
	malloc_printf("mapped:  %12zu\n", arena->stats.mapped);
#endif
	malloc_printf("bins:     bin   size regs pgs  requests   newruns"
	    "    reruns maxruns curruns\n");
	for (i = 0, gap_start = UINT_MAX; i < ntbins + nqbins + nsbins; i++) {
		if (arena->bins[i].stats.nrequests == 0) {
			if (gap_start == UINT_MAX)
				gap_start = i;
		} else {
			if (gap_start != UINT_MAX) {
				if (i > gap_start + 1) {
					
					malloc_printf("[%u..%u]\n",
					    gap_start, i - 1);
				} else {
					
					malloc_printf("[%u]\n", gap_start);
				}
				gap_start = UINT_MAX;
			}
			malloc_printf(
#if defined(MOZ_MEMORY_WINDOWS)
			    "%13u %1s %4u %4u %3u %9I64u %9I64u"
			    " %9I64u %7u %7u\n",
#else
			    "%13u %1s %4u %4u %3u %9llu %9llu"
			    " %9llu %7lu %7lu\n",
#endif
			    i,
			    i < ntbins ? "T" : i < ntbins + nqbins ? "Q" : "S",
			    arena->bins[i].reg_size,
			    arena->bins[i].nregs,
			    arena->bins[i].run_size >> pagesize_2pow,
			    arena->bins[i].stats.nrequests,
			    arena->bins[i].stats.nruns,
			    arena->bins[i].stats.reruns,
			    arena->bins[i].stats.highruns,
			    arena->bins[i].stats.curruns);
		}
	}
	if (gap_start != UINT_MAX) {
		if (i > gap_start + 1) {
			
			malloc_printf("[%u..%u]\n", gap_start, i - 1);
		} else {
			
			malloc_printf("[%u]\n", gap_start);
		}
	}
}
#endif









static inline int
extent_szad_comp(extent_node_t *a, extent_node_t *b)
{
	int ret;
	size_t a_size = a->size;
	size_t b_size = b->size;

	ret = (a_size > b_size) - (a_size < b_size);
	if (ret == 0) {
		uintptr_t a_addr = (uintptr_t)a->addr;
		uintptr_t b_addr = (uintptr_t)b->addr;

		ret = (a_addr > b_addr) - (a_addr < b_addr);
	}

	return (ret);
}


RB_GENERATE_STATIC(extent_tree_szad_s, extent_node_s, link_szad,
    extent_szad_comp)

static inline int
extent_ad_comp(extent_node_t *a, extent_node_t *b)
{
	uintptr_t a_addr = (uintptr_t)a->addr;
	uintptr_t b_addr = (uintptr_t)b->addr;

	return ((a_addr > b_addr) - (a_addr < b_addr));
}


RB_GENERATE_STATIC(extent_tree_ad_s, extent_node_s, link_ad, extent_ad_comp)









#ifdef MOZ_MEMORY_WINDOWS
static void *
pages_map(void *addr, size_t size)
{
	void *ret;

	ret = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE,
	    PAGE_READWRITE);

	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{

	if (VirtualFree(addr, 0, MEM_RELEASE) == 0) {
		_malloc_message(_getprogname(),
		    ": (malloc) Error in VirtualFree()\n", "", "");
		if (opt_abort)
			abort();
	}
}
#elif (defined(MOZ_MEMORY_DARWIN))
static void *
pages_map(void *addr, size_t size)
{
	void *ret;
	kern_return_t err;
	int flags;

	if (addr != NULL) {
		ret = addr;
		flags = 0;
	} else
		flags = VM_FLAGS_ANYWHERE;

	err = vm_allocate((vm_map_t)mach_task_self(), (vm_address_t *)&ret,
	    (vm_size_t)size, flags);
	if (err != KERN_SUCCESS)
		ret = NULL;

	assert(ret == NULL || (addr == NULL && ret != addr)
	    || (addr != NULL && ret == addr));
	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{
	kern_return_t err;

	err = vm_deallocate((vm_map_t)mach_task_self(), (vm_address_t)addr,
	    (vm_size_t)size);
	if (err != KERN_SUCCESS) {
		malloc_message(_getprogname(),
		    ": (malloc) Error in vm_deallocate(): ",
		    mach_error_string(err), "\n");
		if (opt_abort)
			abort();
	}
}

#define	VM_COPY_MIN (pagesize << 5)
static inline void
pages_copy(void *dest, const void *src, size_t n)
{

	assert((void *)((uintptr_t)dest & ~pagesize_mask) == dest);
	assert(n >= VM_COPY_MIN);
	assert((void *)((uintptr_t)src & ~pagesize_mask) == src);

	vm_copy(mach_task_self(), (vm_address_t)src, (vm_size_t)n,
	    (vm_address_t)dest);
}
#else 
static void *
pages_map(void *addr, size_t size)
{
	void *ret;

	



	ret = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
	    -1, 0);
	assert(ret != NULL);

	if (ret == MAP_FAILED)
		ret = NULL;
	else if (addr != NULL && ret != addr) {
		


		if (munmap(ret, size) == -1) {
			char buf[STRERROR_BUF];

			strerror_r(errno, buf, sizeof(buf));
			_malloc_message(_getprogname(),
			    ": (malloc) Error in munmap(): ", buf, "\n");
			if (opt_abort)
				abort();
		}
		ret = NULL;
	}

	assert(ret == NULL || (addr == NULL && ret != addr)
	    || (addr != NULL && ret == addr));
	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{

	if (munmap(addr, size) == -1) {
		char buf[STRERROR_BUF];

		strerror_r(errno, buf, sizeof(buf));
		_malloc_message(_getprogname(),
		    ": (malloc) Error in munmap(): ", buf, "\n");
		if (opt_abort)
			abort();
	}
}
#endif

#ifdef MALLOC_DECOMMIT
static inline void
pages_decommit(void *addr, size_t size)
{

#ifdef MOZ_MEMORY_WINDOWS
	VirtualFree(addr, size, MEM_DECOMMIT);
#else
	if (mmap(addr, size, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1,
	    0) == MAP_FAILED)
		abort();
#endif
}

static inline void
pages_commit(void *addr, size_t size)
{

#  ifdef MOZ_MEMORY_WINDOWS
	VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
#  else
	if (mmap(addr, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE |
	    MAP_ANON, -1, 0) == MAP_FAILED)
		abort();
#  endif
}
#endif

#ifdef MALLOC_DSS
static void *
chunk_alloc_dss(size_t size)
{

	malloc_mutex_lock(&dss_mtx);
	if (dss_prev != (void *)-1) {
		intptr_t incr;

		




		do {
			void *ret;

			
			dss_max = sbrk(0);

			



			incr = (intptr_t)size
			    - (intptr_t)CHUNK_ADDR2OFFSET(dss_max);
			if (incr == (intptr_t)size)
				ret = dss_max;
			else {
				ret = (void *)((intptr_t)dss_max + incr);
				incr += size;
			}

			dss_prev = sbrk(incr);
			if (dss_prev == dss_max) {
				
				dss_max = (void *)((intptr_t)dss_prev + incr);
				malloc_mutex_unlock(&dss_mtx);
				return (ret);
			}
		} while (dss_prev != (void *)-1);
	}
	malloc_mutex_unlock(&dss_mtx);

	return (NULL);
}

static void *
chunk_recycle_dss(size_t size, bool zero)
{
	extent_node_t *node, key;

	key.addr = NULL;
	key.size = size;
	malloc_mutex_lock(&dss_mtx);
	node = RB_NFIND(extent_tree_szad_s, &dss_chunks_szad, &key);
	if (node != NULL) {
		void *ret = node->addr;

		
		RB_REMOVE(extent_tree_szad_s, &dss_chunks_szad, node);
		if (node->size == size) {
			RB_REMOVE(extent_tree_ad_s, &dss_chunks_ad, node);
			base_node_dealloc(node);
		} else {
			




			assert(node->size > size);
			node->addr = (void *)((uintptr_t)node->addr + size);
			node->size -= size;
			RB_INSERT(extent_tree_szad_s, &dss_chunks_szad, node);
		}
		malloc_mutex_unlock(&dss_mtx);

		if (zero)
			memset(ret, 0, size);
		return (ret);
	}
	malloc_mutex_unlock(&dss_mtx);

	return (NULL);
}
#endif

#ifdef MOZ_MEMORY_WINDOWS
static inline void *
chunk_alloc_mmap(size_t size)
{
	void *ret;
	size_t offset;

	






	ret = pages_map(NULL, size);
	if (ret == NULL)
		return (NULL);

	offset = CHUNK_ADDR2OFFSET(ret);
	if (offset != 0) {
		
		pages_unmap(ret, size);
		ret = pages_map((void *)((uintptr_t)ret + size - offset), size);
		while (ret == NULL) {
			



			ret = pages_map(NULL, size + chunksize);
			if (ret == NULL)
				return (NULL);
			



			offset = CHUNK_ADDR2OFFSET(ret);
			pages_unmap(ret, size + chunksize);
			if (offset == 0)
				ret = pages_map(ret, size);
			else {
				ret = pages_map((void *)((uintptr_t)ret +
				    chunksize - offset), size);
			}
			



		}
	}

	return (ret);
}
#else
static inline void *
chunk_alloc_mmap(size_t size)
{
	void *ret;
	size_t offset;

	


















	ret = pages_map(NULL, size);
	if (ret == NULL)
		return (NULL);

	offset = CHUNK_ADDR2OFFSET(ret);
	if (offset != 0) {
		
		if (pages_map((void *)((uintptr_t)ret + size),
		    chunksize - offset) == NULL) {
			



			pages_unmap(ret, size);

			
			if (size + chunksize <= size)
				return NULL;

			ret = pages_map(NULL, size + chunksize);
			if (ret == NULL)
				return (NULL);

			
			offset = CHUNK_ADDR2OFFSET(ret);
			if (offset != 0) {
				
				pages_unmap(ret, chunksize - offset);

				ret = (void *)((uintptr_t)ret +
				    (chunksize - offset));

				
				pages_unmap((void *)((uintptr_t)ret + size),
				    offset);
			} else {
				
				pages_unmap((void *)((uintptr_t)ret + size),
				    chunksize);
			}
		} else {
			
			pages_unmap(ret, chunksize - offset);
			ret = (void *)((uintptr_t)ret + (chunksize - offset));
		}
	}

	return (ret);
}
#endif

static void *
chunk_alloc(size_t size, bool zero)
{
	void *ret;

	assert(size != 0);
	assert((size & chunksize_mask) == 0);

#ifdef MALLOC_DSS
	if (opt_dss) {
		ret = chunk_recycle_dss(size, zero);
		if (ret != NULL) {
			goto RETURN;
		}

		ret = chunk_alloc_dss(size);
		if (ret != NULL)
			goto RETURN;
	}

	if (opt_mmap)
#endif
	{
		ret = chunk_alloc_mmap(size);
		if (ret != NULL)
			goto RETURN;
	}

	
	ret = NULL;
RETURN:
#ifdef MALLOC_STATS
	if (ret != NULL) {
		stats_chunks.nchunks += (size / chunksize);
		stats_chunks.curchunks += (size / chunksize);
	}
	if (stats_chunks.curchunks > stats_chunks.highchunks)
		stats_chunks.highchunks = stats_chunks.curchunks;
#endif

	assert(CHUNK_ADDR2BASE(ret) == ret);
	return (ret);
}

#ifdef MALLOC_DSS
static extent_node_t *
chunk_dealloc_dss_record(void *chunk, size_t size)
{
	extent_node_t *node, *prev, key;

	key.addr = (void *)((uintptr_t)chunk + size);
	node = RB_NFIND(extent_tree_ad_s, &dss_chunks_ad, &key);
	
	if (node != NULL && node->addr == key.addr) {
		




		RB_REMOVE(extent_tree_szad_s, &dss_chunks_szad, node);
		node->addr = chunk;
		node->size += size;
		RB_INSERT(extent_tree_szad_s, &dss_chunks_szad, node);
	} else {
		




		malloc_mutex_unlock(&dss_mtx);
		node = base_node_alloc();
		malloc_mutex_lock(&dss_mtx);
		if (node == NULL)
			return (NULL);
		node->addr = chunk;
		node->size = size;
		RB_INSERT(extent_tree_ad_s, &dss_chunks_ad, node);
		RB_INSERT(extent_tree_szad_s, &dss_chunks_szad, node);
	}

	
	prev = RB_PREV(extent_tree_ad_s, &dss_chunks_ad, node);
	if (prev != NULL && (void *)((uintptr_t)prev->addr + prev->size) ==
	    chunk) {
		




		RB_REMOVE(extent_tree_szad_s, &dss_chunks_szad, prev);
		RB_REMOVE(extent_tree_ad_s, &dss_chunks_ad, prev);

		RB_REMOVE(extent_tree_szad_s, &dss_chunks_szad, node);
		node->addr = prev->addr;
		node->size += prev->size;
		RB_INSERT(extent_tree_szad_s, &dss_chunks_szad, node);

		base_node_dealloc(prev);
	}

	return (node);
}

static bool
chunk_dealloc_dss(void *chunk, size_t size)
{

	malloc_mutex_lock(&dss_mtx);
	if ((uintptr_t)chunk >= (uintptr_t)dss_base
	    && (uintptr_t)chunk < (uintptr_t)dss_max) {
		extent_node_t *node;

		
		node = chunk_dealloc_dss_record(chunk, size);
		if (node != NULL) {
			chunk = node->addr;
			size = node->size;
		}

		
		dss_max = sbrk(0);

		






		if ((void *)((uintptr_t)chunk + size) == dss_max
		    && (dss_prev = sbrk(-(intptr_t)size)) == dss_max) {
			
			dss_max = (void *)((intptr_t)dss_prev - (intptr_t)size);

			if (node != NULL) {
				RB_REMOVE(extent_tree_szad_s, &dss_chunks_szad,
				    node);
				RB_REMOVE(extent_tree_ad_s, &dss_chunks_ad,
				    node);
				base_node_dealloc(node);
			}
			malloc_mutex_unlock(&dss_mtx);
		} else {
			malloc_mutex_unlock(&dss_mtx);
#ifdef MOZ_MEMORY_WINDOWS
			VirtualAlloc(chunk, size, MEM_RESET, PAGE_READWRITE);
#elif (defined(MOZ_MEMORY_DARWIN))
			mmap(chunk, size, PROT_READ | PROT_WRITE, MAP_PRIVATE
			    | MAP_ANON | MAP_FIXED, -1, 0);
#else
			madvise(chunk, size, MADV_FREE);
#endif
		}

		return (false);
	}
	malloc_mutex_unlock(&dss_mtx);

	return (true);
}
#endif

static void
chunk_dealloc_mmap(void *chunk, size_t size)
{

	pages_unmap(chunk, size);
}

static void
chunk_dealloc(void *chunk, size_t size)
{

	assert(chunk != NULL);
	assert(CHUNK_ADDR2BASE(chunk) == chunk);
	assert(size != 0);
	assert((size & chunksize_mask) == 0);

#ifdef MALLOC_STATS
	stats_chunks.curchunks -= (size / chunksize);
#endif

#ifdef MALLOC_DSS
	if (opt_dss) {
		if (chunk_dealloc_dss(chunk, size) == false)
			return;
	}

	if (opt_mmap)
#endif
		chunk_dealloc_mmap(chunk, size);
}













static inline arena_t *
choose_arena(void)
{
	arena_t *ret;

	




#ifndef NO_TLS
	if (__isthreaded == false) {
	    
	    return (arenas[0]);
	}

#  ifdef MOZ_MEMORY_WINDOWS
	ret = TlsGetValue(tlsIndex);
#  else
	ret = arenas_map;
#  endif

	if (ret == NULL) {
		ret = choose_arena_hard();
		assert(ret != NULL);
	}
#else
	if (__isthreaded && narenas > 1) {
		unsigned long ind;

		







		ind = (unsigned long) _pthread_self() % narenas;

		












		ret = arenas[ind];
		if (ret == NULL) {
			



			malloc_spin_lock(&arenas_lock);
			if (arenas[ind] == NULL)
				ret = arenas_extend((unsigned)ind);
			else
				ret = arenas[ind];
			malloc_spin_unlock(&arenas_lock);
		}
	} else
		ret = arenas[0];
#endif

	assert(ret != NULL);
	return (ret);
}

#ifndef NO_TLS




static arena_t *
choose_arena_hard(void)
{
	arena_t *ret;

	assert(__isthreaded);

#ifdef MALLOC_LAZY_FREE
	









	SPRN(lazy_free, (uint32_t)(uintptr_t)(_pthread_self()));
#endif

#ifdef MALLOC_BALANCE
	





	SPRN(balance, (uint32_t)(uintptr_t)(_pthread_self()));
#endif

	if (narenas > 1) {
#ifdef MALLOC_BALANCE
		unsigned ind;

		ind = PRN(balance, narenas_2pow);
		if ((ret = arenas[ind]) == NULL) {
			malloc_spin_lock(&arenas_lock);
			if ((ret = arenas[ind]) == NULL)
				ret = arenas_extend(ind);
			malloc_spin_unlock(&arenas_lock);
		}
#else
		malloc_spin_lock(&arenas_lock);
		if ((ret = arenas[next_arena]) == NULL)
			ret = arenas_extend(next_arena);
		next_arena = (next_arena + 1) % narenas;
		malloc_spin_unlock(&arenas_lock);
#endif
	} else
		ret = arenas[0];

#ifdef MOZ_MEMORY_WINDOWS
	TlsSetValue(tlsIndex, ret);
#else
	arenas_map = ret;
#endif

	return (ret);
}
#endif

static inline int
arena_chunk_comp(arena_chunk_t *a, arena_chunk_t *b)
{
	uintptr_t a_chunk = (uintptr_t)a;
	uintptr_t b_chunk = (uintptr_t)b;

	assert(a != NULL);
	assert(b != NULL);

	return ((a_chunk > b_chunk) - (a_chunk < b_chunk));
}


RB_GENERATE_STATIC(arena_chunk_tree_s, arena_chunk_s, link, arena_chunk_comp)

static inline int
arena_run_comp(arena_run_t *a, arena_run_t *b)
{
	uintptr_t a_run = (uintptr_t)a;
	uintptr_t b_run = (uintptr_t)b;

	assert(a != NULL);
	assert(b != NULL);

	return ((a_run > b_run) - (a_run < b_run));
}


RB_GENERATE_STATIC(arena_run_tree_s, arena_run_s, link, arena_run_comp)

static extent_node_t *
arena_chunk_node_alloc(arena_chunk_t *chunk)
{
	extent_node_t *ret;

	ret = RB_MIN(extent_tree_ad_s, &chunk->nodes);
	if (ret != NULL)
		RB_REMOVE(extent_tree_ad_s, &chunk->nodes, ret);
	else {
		ret = chunk->nodes_past;
		chunk->nodes_past = (extent_node_t *)
		    ((uintptr_t)chunk->nodes_past + sizeof(extent_node_t));
		assert((uintptr_t)ret + sizeof(extent_node_t) <=
		    (uintptr_t)chunk + (arena_chunk_header_npages <<
		    pagesize_2pow));
	}

	return (ret);
}

static void
arena_chunk_node_dealloc(arena_chunk_t *chunk, extent_node_t *node)
{

	node->addr = (void *)node;
	RB_INSERT(extent_tree_ad_s, &chunk->nodes, node);
}

static inline void *
arena_run_reg_alloc(arena_run_t *run, arena_bin_t *bin)
{
	void *ret;
	unsigned i, mask, bit, regind;

	assert(run->magic == ARENA_RUN_MAGIC);
	assert(run->regs_minelm < bin->regs_mask_nelms);

	




	i = run->regs_minelm;
	mask = run->regs_mask[i];
	if (mask != 0) {
		
		bit = ffs((int)mask) - 1;

		regind = ((i << (SIZEOF_INT_2POW + 3)) + bit);
		assert(regind < bin->nregs);
		ret = (void *)(((uintptr_t)run) + bin->reg0_offset
		    + (bin->reg_size * regind));

		
		mask ^= (1U << bit);
		run->regs_mask[i] = mask;

		return (ret);
	}

	for (i++; i < bin->regs_mask_nelms; i++) {
		mask = run->regs_mask[i];
		if (mask != 0) {
			
			bit = ffs((int)mask) - 1;

			regind = ((i << (SIZEOF_INT_2POW + 3)) + bit);
			assert(regind < bin->nregs);
			ret = (void *)(((uintptr_t)run) + bin->reg0_offset
			    + (bin->reg_size * regind));

			
			mask ^= (1U << bit);
			run->regs_mask[i] = mask;

			



			run->regs_minelm = i; 

			return (ret);
		}
	}
	
	assert(0);
	return (NULL);
}

static inline void
arena_run_reg_dalloc(arena_run_t *run, arena_bin_t *bin, void *ptr, size_t size)
{
	









#define	SIZE_INV_SHIFT 21
#define	SIZE_INV(s) (((1U << SIZE_INV_SHIFT) / (s << QUANTUM_2POW_MIN)) + 1)
	static const unsigned size_invs[] = {
	    SIZE_INV(3),
	    SIZE_INV(4), SIZE_INV(5), SIZE_INV(6), SIZE_INV(7),
	    SIZE_INV(8), SIZE_INV(9), SIZE_INV(10), SIZE_INV(11),
	    SIZE_INV(12),SIZE_INV(13), SIZE_INV(14), SIZE_INV(15),
	    SIZE_INV(16),SIZE_INV(17), SIZE_INV(18), SIZE_INV(19),
	    SIZE_INV(20),SIZE_INV(21), SIZE_INV(22), SIZE_INV(23),
	    SIZE_INV(24),SIZE_INV(25), SIZE_INV(26), SIZE_INV(27),
	    SIZE_INV(28),SIZE_INV(29), SIZE_INV(30), SIZE_INV(31)
#if (QUANTUM_2POW_MIN < 4)
	    ,
	    SIZE_INV(32), SIZE_INV(33), SIZE_INV(34), SIZE_INV(35),
	    SIZE_INV(36), SIZE_INV(37), SIZE_INV(38), SIZE_INV(39),
	    SIZE_INV(40), SIZE_INV(41), SIZE_INV(42), SIZE_INV(43),
	    SIZE_INV(44), SIZE_INV(45), SIZE_INV(46), SIZE_INV(47),
	    SIZE_INV(48), SIZE_INV(49), SIZE_INV(50), SIZE_INV(51),
	    SIZE_INV(52), SIZE_INV(53), SIZE_INV(54), SIZE_INV(55),
	    SIZE_INV(56), SIZE_INV(57), SIZE_INV(58), SIZE_INV(59),
	    SIZE_INV(60), SIZE_INV(61), SIZE_INV(62), SIZE_INV(63)
#endif
	};
	unsigned diff, regind, elm, bit;

	assert(run->magic == ARENA_RUN_MAGIC);
	assert(((sizeof(size_invs)) / sizeof(unsigned)) + 3
	    >= (SMALL_MAX_DEFAULT >> QUANTUM_2POW_MIN));

	



	diff = (unsigned)((uintptr_t)ptr - (uintptr_t)run - bin->reg0_offset);
	if ((size & (size - 1)) == 0) {
		





		static const unsigned char log2_table[] = {
		    0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7
		};

		if (size <= 128)
			regind = (diff >> log2_table[size - 1]);
		else if (size <= 32768)
			regind = diff >> (8 + log2_table[(size >> 8) - 1]);
		else {
			



			regind = diff / size;
		}
	} else if (size <= ((sizeof(size_invs) / sizeof(unsigned))
	    << QUANTUM_2POW_MIN) + 2) {
		regind = size_invs[(size >> QUANTUM_2POW_MIN) - 3] * diff;
		regind >>= SIZE_INV_SHIFT;
	} else {
		





		regind = diff / size;
	};
	assert(diff == regind * size);
	assert(regind < bin->nregs);

	elm = regind >> (SIZEOF_INT_2POW + 3);
	if (elm < run->regs_minelm)
		run->regs_minelm = elm;
	bit = regind - (elm << (SIZEOF_INT_2POW + 3));
	assert((run->regs_mask[elm] & (1U << bit)) == 0);
	run->regs_mask[elm] |= (1U << bit);
#undef SIZE_INV
#undef SIZE_INV_SHIFT
}

static void
arena_run_split(arena_t *arena, arena_run_t *run, size_t size, bool small,
    bool zero)
{
	arena_chunk_t *chunk;
	size_t run_ind, total_pages, need_pages, rem_pages, i;
	extent_node_t *nodeA, *nodeB, key;

	
	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(run);
	nodeA = arena_chunk_node_alloc(chunk);
	nodeA->addr = run;
	nodeA->size = size;
	RB_INSERT(extent_tree_ad_s, &arena->runs_alloced_ad, nodeA);

	key.addr = run;
	nodeB = RB_FIND(extent_tree_ad_s, &arena->runs_avail_ad, &key);
	assert(nodeB != NULL);

	run_ind = (unsigned)(((uintptr_t)run - (uintptr_t)chunk)
	    >> pagesize_2pow);
	total_pages = nodeB->size >> pagesize_2pow;
	need_pages = (size >> pagesize_2pow);
	assert(need_pages > 0);
	assert(need_pages <= total_pages);
	assert(need_pages <= CHUNK_MAP_POS_MASK || small == false);
	rem_pages = total_pages - need_pages;

	for (i = 0; i < need_pages; i++) {
#ifdef MALLOC_DECOMMIT
		





		if (chunk->map[run_ind + i] & CHUNK_MAP_DECOMMITTED) {
			size_t j;

			




			for (j = 0; i + j < need_pages && (chunk->map[run_ind +
			    i + j] & CHUNK_MAP_DECOMMITTED); j++) {
				chunk->map[run_ind + i + j] ^=
				    CHUNK_MAP_DECOMMITTED;
			}

			pages_commit((void *)((uintptr_t)chunk + ((run_ind + i)
			    << pagesize_2pow)), (j << pagesize_2pow));
#  ifdef MALLOC_STATS
			arena->stats.ncommit++;
#  endif
		}
#endif

		
		if (zero) {
			if ((chunk->map[run_ind + i] & CHUNK_MAP_UNTOUCHED)
			    == 0) {
				memset((void *)((uintptr_t)chunk + ((run_ind
				    + i) << pagesize_2pow)), 0, pagesize);
				
			}
		}

		
		if (chunk->map[run_ind + i] & CHUNK_MAP_DIRTY) {
			chunk->ndirty--;
			arena->ndirty--;
		}

		
		if (small)
			chunk->map[run_ind + i] = (uint8_t)i;
		else
			chunk->map[run_ind + i] = CHUNK_MAP_LARGE;
	}

	
	RB_REMOVE(extent_tree_szad_s, &arena->runs_avail_szad, nodeB);
	if (rem_pages > 0) {
		



		nodeB->addr = (void *)((uintptr_t)nodeB->addr + size);
		nodeB->size -= size;
		RB_INSERT(extent_tree_szad_s, &arena->runs_avail_szad, nodeB);
	} else {
		
		RB_REMOVE(extent_tree_ad_s, &arena->runs_avail_ad, nodeB);
		arena_chunk_node_dealloc(chunk, nodeB);
	}

	chunk->pages_used += need_pages;
}

static arena_chunk_t *
arena_chunk_alloc(arena_t *arena)
{
	arena_chunk_t *chunk;
	extent_node_t *node;

	if (arena->spare != NULL) {
		chunk = arena->spare;
		arena->spare = NULL;
	} else {
		chunk = (arena_chunk_t *)chunk_alloc(chunksize, true);
		if (chunk == NULL)
			return (NULL);
#ifdef MALLOC_STATS
		arena->stats.mapped += chunksize;
#endif

		chunk->arena = arena;

		RB_INSERT(arena_chunk_tree_s, &arena->chunks, chunk);

		



		chunk->pages_used = 0;
		chunk->ndirty = 0;

		



		memset(chunk->map, (CHUNK_MAP_LARGE | CHUNK_MAP_POS_MASK),
		    arena_chunk_header_npages);
		memset(&chunk->map[arena_chunk_header_npages],
		    (CHUNK_MAP_UNTOUCHED
#ifdef MALLOC_DECOMMIT
		    | CHUNK_MAP_DECOMMITTED
#endif
		    ), (chunk_npages -
		    arena_chunk_header_npages));

		
		RB_INIT(&chunk->nodes);
		chunk->nodes_past = (extent_node_t *)QUANTUM_CEILING(
		    (uintptr_t)&chunk->map[chunk_npages]);

#ifdef MALLOC_DECOMMIT
		




		pages_decommit((void *)((uintptr_t)chunk +
		    (arena_chunk_header_npages << pagesize_2pow)),
		    ((chunk_npages - arena_chunk_header_npages) <<
		    pagesize_2pow));
#  ifdef MALLOC_STATS
		arena->stats.ndecommit++;
		arena->stats.decommitted += (chunk_npages -
		    arena_chunk_header_npages);
#  endif
#endif
	}

	
	node = arena_chunk_node_alloc(chunk);
	node->addr = (void *)((uintptr_t)chunk + (arena_chunk_header_npages <<
	    pagesize_2pow));
	node->size = chunksize - (arena_chunk_header_npages << pagesize_2pow);
	RB_INSERT(extent_tree_szad_s, &arena->runs_avail_szad, node);
	RB_INSERT(extent_tree_ad_s, &arena->runs_avail_ad, node);

	return (chunk);
}

static void
arena_chunk_dealloc(arena_t *arena, arena_chunk_t *chunk)
{
	extent_node_t *node, key;

	if (arena->spare != NULL) {
		RB_REMOVE(arena_chunk_tree_s, &chunk->arena->chunks,
		    arena->spare);
		arena->ndirty -= arena->spare->ndirty;
		chunk_dealloc((void *)arena->spare, chunksize);
#ifdef MALLOC_STATS
		arena->stats.mapped -= chunksize;
#endif
	}

	





	key.addr = (void *)((uintptr_t)chunk + (arena_chunk_header_npages <<
	    pagesize_2pow));
	node = RB_FIND(extent_tree_ad_s, &arena->runs_avail_ad, &key);
	assert(node != NULL);
	RB_REMOVE(extent_tree_szad_s, &arena->runs_avail_szad, node);
	RB_REMOVE(extent_tree_ad_s, &arena->runs_avail_ad, node);
	arena_chunk_node_dealloc(chunk, node);

	arena->spare = chunk;
}

static arena_run_t *
arena_run_alloc(arena_t *arena, size_t size, bool small, bool zero)
{
	arena_chunk_t *chunk;
	arena_run_t *run;
	extent_node_t *node, key;

	assert(size <= (chunksize - (arena_chunk_header_npages <<
	    pagesize_2pow)));
	assert((size & pagesize_mask) == 0);

	
	key.addr = NULL;
	key.size = size;
	node = RB_NFIND(extent_tree_szad_s, &arena->runs_avail_szad, &key);
	if (node != NULL) {
		run = (arena_run_t *)node->addr;
		arena_run_split(arena, run, size, small, zero);
		return (run);
	}

	


	chunk = arena_chunk_alloc(arena);
	if (chunk == NULL)
		return (NULL);
	run = (arena_run_t *)((uintptr_t)chunk + (arena_chunk_header_npages <<
	    pagesize_2pow));
	
	arena_run_split(arena, run, size, small, zero);
	return (run);
}

static void
arena_purge(arena_t *arena)
{
	arena_chunk_t *chunk;
#ifdef MALLOC_DEBUG
	size_t ndirty;

	ndirty = 0;
	RB_FOREACH(chunk, arena_chunk_tree_s, &arena->chunks) {
		ndirty += chunk->ndirty;
	}
	assert(ndirty == arena->ndirty);
#endif
	assert(arena->ndirty > opt_dirty_max);

#ifdef MALLOC_STATS
	arena->stats.npurge++;
#endif

	



	RB_FOREACH_REVERSE(chunk, arena_chunk_tree_s, &arena->chunks) {
		if (chunk->ndirty > 0) {
			size_t i;

			for (i = chunk_npages - 1; i >=
			    arena_chunk_header_npages; i--) {
				if (chunk->map[i] & CHUNK_MAP_DIRTY) {
					size_t npages;

					chunk->map[i] = (CHUNK_MAP_LARGE |
#ifdef MALLOC_DECOMMIT
					    CHUNK_MAP_DECOMMITTED |
#endif
					    CHUNK_MAP_POS_MASK);
					chunk->ndirty--;
					arena->ndirty--;
					
					for (npages = 1; i >
					    arena_chunk_header_npages &&
					    (chunk->map[i - 1] &
					    CHUNK_MAP_DIRTY); npages++) {
						i--;
						chunk->map[i] = (CHUNK_MAP_LARGE
#ifdef MALLOC_DECOMMIT
						    | CHUNK_MAP_DECOMMITTED
#endif
						    | CHUNK_MAP_POS_MASK);
						chunk->ndirty--;
						arena->ndirty--;
					}

#ifdef MALLOC_DECOMMIT
					pages_decommit((void *)((uintptr_t)
					    chunk + (i << pagesize_2pow)),
					    (npages << pagesize_2pow));
#  ifdef MALLOC_STATS
					arena->stats.ndecommit++;
					arena->stats.decommitted += npages;
#  endif
#else
					madvise((void *)((uintptr_t)chunk + (i
					    << pagesize_2pow)), pagesize *
					    npages, MADV_FREE);
#endif
#ifdef MALLOC_STATS
					arena->stats.nmadvise++;
					arena->stats.purged += npages;
#endif
				}
			}
		}
	}
}

static void
arena_run_dalloc(arena_t *arena, arena_run_t *run, bool dirty)
{
	arena_chunk_t *chunk;
	extent_node_t *nodeA, *nodeB, *nodeC, key;
	size_t size, run_ind, run_pages;

	
	key.addr = run;
	nodeB = RB_FIND(extent_tree_ad_s, &arena->runs_alloced_ad, &key);
	assert(nodeB != NULL);
	RB_REMOVE(extent_tree_ad_s, &arena->runs_alloced_ad, nodeB);
	size = nodeB->size;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(run);
	run_ind = (unsigned)(((uintptr_t)run - (uintptr_t)chunk)
	    >> pagesize_2pow);
	assert(run_ind >= arena_chunk_header_npages);
	assert(run_ind < (chunksize >> pagesize_2pow));
	run_pages = (size >> pagesize_2pow);

	
	chunk->pages_used -= run_pages;

	if (dirty) {
		size_t i;

		for (i = 0; i < run_pages; i++) {
			assert((chunk->map[run_ind + i] & CHUNK_MAP_DIRTY) ==
			    0);
			chunk->map[run_ind + i] |= CHUNK_MAP_DIRTY;
			chunk->ndirty++;
			arena->ndirty++;
		}
	}
#ifdef MALLOC_DEBUG
	
	{
		size_t i;

		for (i = 0; i < run_pages; i++) {
			chunk->map[run_ind + i] |= (CHUNK_MAP_LARGE |
			    CHUNK_MAP_POS_MASK);
		}
	}
#endif

	
	key.addr = (void *)((uintptr_t)run + size);
	nodeC = RB_NFIND(extent_tree_ad_s, &arena->runs_avail_ad, &key);
	if (nodeC != NULL && nodeC->addr == key.addr) {
		




		RB_REMOVE(extent_tree_szad_s, &arena->runs_avail_szad, nodeC);
		nodeC->addr = (void *)run;
		nodeC->size += size;
		RB_INSERT(extent_tree_szad_s, &arena->runs_avail_szad, nodeC);
		arena_chunk_node_dealloc(chunk, nodeB);
		nodeB = nodeC;
	} else {
		


		RB_INSERT(extent_tree_szad_s, &arena->runs_avail_szad, nodeB);
		RB_INSERT(extent_tree_ad_s, &arena->runs_avail_ad, nodeB);
	}

	
	nodeA = RB_PREV(extent_tree_ad_s, &arena->runs_avail_ad, nodeB);
	if (nodeA != NULL && (void *)((uintptr_t)nodeA->addr + nodeA->size) ==
	    (void *)run) {
		




		RB_REMOVE(extent_tree_szad_s, &arena->runs_avail_szad, nodeA);
		RB_REMOVE(extent_tree_ad_s, &arena->runs_avail_ad, nodeA);

		RB_REMOVE(extent_tree_szad_s, &arena->runs_avail_szad, nodeB);
		nodeB->addr = nodeA->addr;
		nodeB->size += nodeA->size;
		RB_INSERT(extent_tree_szad_s, &arena->runs_avail_szad, nodeB);

		arena_chunk_node_dealloc(chunk, nodeA);
	}

	
	if (chunk->pages_used == 0)
		arena_chunk_dealloc(arena, chunk);

	
	if (arena->ndirty > opt_dirty_max)
		arena_purge(arena);
}

static void
arena_run_trim_head(arena_t *arena, arena_chunk_t *chunk, extent_node_t *nodeB,
    arena_run_t *run, size_t oldsize, size_t newsize)
{
	extent_node_t *nodeA;

	assert(nodeB->addr == run);
	assert(nodeB->size == oldsize);
	assert(oldsize > newsize);

	



	nodeB->addr = (void *)((uintptr_t)run + (oldsize - newsize));
	nodeB->size = newsize;

	



	nodeA = arena_chunk_node_alloc(chunk);
	nodeA->addr = (void *)run;
	nodeA->size = oldsize - newsize;
	RB_INSERT(extent_tree_ad_s, &arena->runs_alloced_ad, nodeA);

	arena_run_dalloc(arena, (arena_run_t *)run, false);
}

static void
arena_run_trim_tail(arena_t *arena, arena_chunk_t *chunk, extent_node_t *nodeA,
    arena_run_t *run, size_t oldsize, size_t newsize, bool dirty)
{
	extent_node_t *nodeB;

	assert(nodeA->addr == run);
	assert(nodeA->size == oldsize);
	assert(oldsize > newsize);

	



	nodeA->size = newsize;

	



	nodeB = arena_chunk_node_alloc(chunk);
	nodeB->addr = (void *)((uintptr_t)run + newsize);
	nodeB->size = oldsize - newsize;
	RB_INSERT(extent_tree_ad_s, &arena->runs_alloced_ad, nodeB);

	arena_run_dalloc(arena, (arena_run_t *)((uintptr_t)run + newsize),
	    dirty);
}

static arena_run_t *
arena_bin_nonfull_run_get(arena_t *arena, arena_bin_t *bin)
{
	arena_run_t *run;
	unsigned i, remainder;

	
	if ((run = RB_MIN(arena_run_tree_s, &bin->runs)) != NULL) {
		
		RB_REMOVE(arena_run_tree_s, &bin->runs, run);
#ifdef MALLOC_STATS
		bin->stats.reruns++;
#endif
		return (run);
	}
	

	
	run = arena_run_alloc(arena, bin->run_size, true, false);
	if (run == NULL)
		return (NULL);

	
	run->bin = bin;

	for (i = 0; i < bin->regs_mask_nelms; i++)
		run->regs_mask[i] = UINT_MAX;
	remainder = bin->nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1);
	if (remainder != 0) {
		
		run->regs_mask[i] = (UINT_MAX >> ((1U << (SIZEOF_INT_2POW + 3))
		    - remainder));
	}

	run->regs_minelm = 0;

	run->nfree = bin->nregs;
#ifdef MALLOC_DEBUG
	run->magic = ARENA_RUN_MAGIC;
#endif

#ifdef MALLOC_STATS
	bin->stats.nruns++;
	bin->stats.curruns++;
	if (bin->stats.curruns > bin->stats.highruns)
		bin->stats.highruns = bin->stats.curruns;
#endif
	return (run);
}


static inline void *
arena_bin_malloc_easy(arena_t *arena, arena_bin_t *bin, arena_run_t *run)
{
	void *ret;

	assert(run->magic == ARENA_RUN_MAGIC);
	assert(run->nfree > 0);

	ret = arena_run_reg_alloc(run, bin);
	assert(ret != NULL);
	run->nfree--;

	return (ret);
}


static void *
arena_bin_malloc_hard(arena_t *arena, arena_bin_t *bin)
{

	bin->runcur = arena_bin_nonfull_run_get(arena, bin);
	if (bin->runcur == NULL)
		return (NULL);
	assert(bin->runcur->magic == ARENA_RUN_MAGIC);
	assert(bin->runcur->nfree > 0);

	return (arena_bin_malloc_easy(arena, bin, bin->runcur));
}












static size_t
arena_bin_run_size_calc(arena_bin_t *bin, size_t min_run_size)
{
	size_t try_run_size, good_run_size;
	unsigned good_nregs, good_mask_nelms, good_reg0_offset;
	unsigned try_nregs, try_mask_nelms, try_reg0_offset;

	assert(min_run_size >= pagesize);
	assert(min_run_size <= arena_maxclass);
	assert(min_run_size <= RUN_MAX_SMALL);

	









	try_run_size = min_run_size;
	try_nregs = ((try_run_size - sizeof(arena_run_t)) / bin->reg_size)
	    + 1; 
	do {
		try_nregs--;
		try_mask_nelms = (try_nregs >> (SIZEOF_INT_2POW + 3)) +
		    ((try_nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1)) ? 1 : 0);
		try_reg0_offset = try_run_size - (try_nregs * bin->reg_size);
	} while (sizeof(arena_run_t) + (sizeof(unsigned) * (try_mask_nelms - 1))
	    > try_reg0_offset);

	
	do {
		


		good_run_size = try_run_size;
		good_nregs = try_nregs;
		good_mask_nelms = try_mask_nelms;
		good_reg0_offset = try_reg0_offset;

		
		try_run_size += pagesize;
		try_nregs = ((try_run_size - sizeof(arena_run_t)) /
		    bin->reg_size) + 1; 
		do {
			try_nregs--;
			try_mask_nelms = (try_nregs >> (SIZEOF_INT_2POW + 3)) +
			    ((try_nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1)) ?
			    1 : 0);
			try_reg0_offset = try_run_size - (try_nregs *
			    bin->reg_size);
		} while (sizeof(arena_run_t) + (sizeof(unsigned) *
		    (try_mask_nelms - 1)) > try_reg0_offset);
	} while (try_run_size <= arena_maxclass && try_run_size <= RUN_MAX_SMALL
	    && RUN_MAX_OVRHD * (bin->reg_size << 3) > RUN_MAX_OVRHD_RELAX
	    && (try_reg0_offset << RUN_BFP) > RUN_MAX_OVRHD * try_run_size);

	assert(sizeof(arena_run_t) + (sizeof(unsigned) * (good_mask_nelms - 1))
	    <= good_reg0_offset);
	assert((good_mask_nelms << (SIZEOF_INT_2POW + 3)) >= good_nregs);

	
	bin->run_size = good_run_size;
	bin->nregs = good_nregs;
	bin->regs_mask_nelms = good_mask_nelms;
	bin->reg0_offset = good_reg0_offset;

	return (good_run_size);
}

#ifdef MALLOC_BALANCE
static inline void
arena_lock_balance(arena_t *arena)
{
	unsigned contention;

	contention = malloc_spin_lock(&arena->lock);
	if (narenas > 1) {
		




		arena->contention = (((uint64_t)arena->contention
		    * (uint64_t)((1U << BALANCE_ALPHA_INV_2POW)-1))
		    + (uint64_t)contention) >> BALANCE_ALPHA_INV_2POW;
		if (arena->contention >= opt_balance_threshold)
			arena_lock_balance_hard(arena);
	}
}

static void
arena_lock_balance_hard(arena_t *arena)
{
	uint32_t ind;

	arena->contention = 0;
#ifdef MALLOC_STATS
	arena->stats.nbalance++;
#endif
	ind = PRN(balance, narenas_2pow);
	if (arenas[ind] != NULL) {
#ifdef MOZ_MEMORY_WINDOWS
		TlsSetValue(tlsIndex, arenas[ind]);
#else
		arenas_map = arenas[ind];
#endif
	} else {
		malloc_spin_lock(&arenas_lock);
		if (arenas[ind] != NULL) {
#ifdef MOZ_MEMORY_WINDOWS
			TlsSetValue(tlsIndex, arenas[ind]);
#else
			arenas_map = arenas[ind];
#endif
		} else {
#ifdef MOZ_MEMORY_WINDOWS
			TlsSetValue(tlsIndex, arenas_extend(ind));
#else
			arenas_map = arenas_extend(ind);
#endif
		}
		malloc_spin_unlock(&arenas_lock);
	}
}
#endif

static inline void *
arena_malloc_small(arena_t *arena, size_t size, bool zero)
{
	void *ret;
	arena_bin_t *bin;
	arena_run_t *run;

	if (size < small_min) {
		
		size = pow2_ceil(size);
		bin = &arena->bins[ffs((int)(size >> (TINY_MIN_2POW +
		    1)))];
#if (!defined(NDEBUG) || defined(MALLOC_STATS))
		




		if (size < (1U << TINY_MIN_2POW))
			size = (1U << TINY_MIN_2POW);
#endif
	} else if (size <= small_max) {
		
		size = QUANTUM_CEILING(size);
		bin = &arena->bins[ntbins + (size >> opt_quantum_2pow)
		    - 1];
	} else {
		
		size = pow2_ceil(size);
		bin = &arena->bins[ntbins + nqbins
		    + (ffs((int)(size >> opt_small_max_2pow)) - 2)];
	}
	assert(size == bin->reg_size);

#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	if ((run = bin->runcur) != NULL && run->nfree > 0)
		ret = arena_bin_malloc_easy(arena, bin, run);
	else
		ret = arena_bin_malloc_hard(arena, bin);

	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}

#ifdef MALLOC_STATS
	bin->stats.nrequests++;
	arena->stats.nmalloc_small++;
	arena->stats.allocated_small += size;
#endif
	malloc_spin_unlock(&arena->lock);

	if (zero == false) {
#ifdef MALLOC_FILL
		if (opt_junk)
			memset(ret, 0xa5, size);
		else if (opt_zero)
			memset(ret, 0, size);
#endif
	} else
		memset(ret, 0, size);

	return (ret);
}

static void *
arena_malloc_large(arena_t *arena, size_t size, bool zero)
{
	void *ret;

	
	size = PAGE_CEILING(size);
#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	ret = (void *)arena_run_alloc(arena, size, false, zero);
	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}
#ifdef MALLOC_STATS
	arena->stats.nmalloc_large++;
	arena->stats.allocated_large += size;
#endif
	malloc_spin_unlock(&arena->lock);

	if (zero == false) {
#ifdef MALLOC_FILL
		if (opt_junk)
			memset(ret, 0xa5, size);
		else if (opt_zero)
			memset(ret, 0, size);
#endif
	}

	return (ret);
}

static inline void *
arena_malloc(arena_t *arena, size_t size, bool zero)
{

	assert(arena != NULL);
	assert(arena->magic == ARENA_MAGIC);
	assert(size != 0);
	assert(QUANTUM_CEILING(size) <= arena_maxclass);

	if (size <= bin_maxclass) {
		return (arena_malloc_small(arena, size, zero));
	} else
		return (arena_malloc_large(arena, size, zero));
}

static inline void *
imalloc(size_t size)
{

	assert(size != 0);

	if (size <= arena_maxclass)
		return (arena_malloc(choose_arena(), size, false));
	else
		return (huge_malloc(size, false));
}

static inline void *
icalloc(size_t size)
{

	if (size <= arena_maxclass)
		return (arena_malloc(choose_arena(), size, true));
	else
		return (huge_malloc(size, true));
}


static void *
arena_palloc(arena_t *arena, size_t alignment, size_t size, size_t alloc_size)
{
	void *ret;
	size_t offset;
	arena_chunk_t *chunk;
	extent_node_t *node, key;

	assert((size & pagesize_mask) == 0);
	assert((alignment & pagesize_mask) == 0);

#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	ret = (void *)arena_run_alloc(arena, alloc_size, false, false);
	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ret);

	offset = (uintptr_t)ret & (alignment - 1);
	assert((offset & pagesize_mask) == 0);
	assert(offset < alloc_size);
	if (offset == 0) {
		



		key.addr = ret;
		node = RB_FIND(extent_tree_ad_s, &arena->runs_alloced_ad, &key);
		assert(node != NULL);

		arena_run_trim_tail(arena, chunk, node, ret, alloc_size, size,
		    false);
	} else {
		size_t leadsize, trailsize;

		



		key.addr = ret;
		node = RB_FIND(extent_tree_ad_s, &arena->runs_alloced_ad, &key);
		assert(node != NULL);

		leadsize = alignment - offset;
		if (leadsize > 0) {
			arena_run_trim_head(arena, chunk, node, ret, alloc_size,
			    alloc_size - leadsize);
			ret = (void *)((uintptr_t)ret + leadsize);
		}

		trailsize = alloc_size - leadsize - size;
		if (trailsize != 0) {
			
			assert(trailsize < alloc_size);
			arena_run_trim_tail(arena, chunk, node, ret, size +
			    trailsize, size, false);
		}
	}

#ifdef MALLOC_STATS
	arena->stats.nmalloc_large++;
	arena->stats.allocated_large += size;
#endif
	malloc_spin_unlock(&arena->lock);

#ifdef MALLOC_FILL
	if (opt_junk)
		memset(ret, 0xa5, size);
	else if (opt_zero)
		memset(ret, 0, size);
#endif
	return (ret);
}

static inline void *
ipalloc(size_t alignment, size_t size)
{
	void *ret;
	size_t ceil_size;

	

















	ceil_size = (size + (alignment - 1)) & (-alignment);
	



	if (ceil_size < size) {
		
		return (NULL);
	}

	if (ceil_size <= pagesize || (alignment <= pagesize
	    && ceil_size <= arena_maxclass))
		ret = arena_malloc(choose_arena(), ceil_size, false);
	else {
		size_t run_size;

		



		alignment = PAGE_CEILING(alignment);
		ceil_size = PAGE_CEILING(size);
		











		if (ceil_size < size || ceil_size + alignment < ceil_size) {
			
			return (NULL);
		}

		



		if (ceil_size >= alignment)
			run_size = ceil_size + alignment - pagesize;
		else {
			








			run_size = (alignment << 1) - pagesize;
		}

		if (run_size <= arena_maxclass) {
			ret = arena_palloc(choose_arena(), alignment, ceil_size,
			    run_size);
		} else if (alignment <= chunksize)
			ret = huge_malloc(ceil_size, false);
		else
			ret = huge_palloc(alignment, ceil_size);
	}

	assert(((uintptr_t)ret & (alignment - 1)) == 0);
	return (ret);
}


static size_t
arena_salloc(const void *ptr)
{
	size_t ret;
	arena_chunk_t *chunk;
	arena_chunk_map_t mapelm;
	size_t pageind;

	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = (((uintptr_t)ptr - (uintptr_t)chunk) >> pagesize_2pow);
	mapelm = chunk->map[pageind];
	if ((mapelm & CHUNK_MAP_LARGE) == 0) {
		arena_run_t *run;

		
		pageind -= (mapelm & CHUNK_MAP_POS_MASK);
		run = (arena_run_t *)((uintptr_t)chunk + (pageind <<
		    pagesize_2pow));
		assert(run->magic == ARENA_RUN_MAGIC);
		ret = run->bin->reg_size;
	} else {
		arena_t *arena = chunk->arena;
		extent_node_t *node, key;

		
		assert((mapelm & CHUNK_MAP_POS_MASK) == 0);
		arena = chunk->arena;
		malloc_spin_lock(&arena->lock);
		key.addr = (void *)ptr;
		node = RB_FIND(extent_tree_ad_s, &arena->runs_alloced_ad, &key);
		assert(node != NULL);
		ret = node->size;
		malloc_spin_unlock(&arena->lock);
	}

	return (ret);
}

static inline size_t
isalloc(const void *ptr)
{
	size_t ret;
	arena_chunk_t *chunk;

	assert(ptr != NULL);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr) {
		
		assert(chunk->arena->magic == ARENA_MAGIC);

		ret = arena_salloc(ptr);
	} else {
		extent_node_t *node, key;

		

		malloc_mutex_lock(&huge_mtx);

		
		key.addr = __DECONST(void *, ptr);
		node = RB_FIND(extent_tree_ad_s, &huge, &key);
		assert(node != NULL);

		ret = node->size;

		malloc_mutex_unlock(&huge_mtx);
	}

	return (ret);
}

static inline void
arena_dalloc_small(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t pageind, arena_chunk_map_t mapelm)
{
	arena_run_t *run;
	arena_bin_t *bin;
	size_t size;

	pageind -= (mapelm & CHUNK_MAP_POS_MASK);

	run = (arena_run_t *)((uintptr_t)chunk + (pageind << pagesize_2pow));
	assert(run->magic == ARENA_RUN_MAGIC);
	bin = run->bin;
	size = bin->reg_size;

#ifdef MALLOC_FILL
	if (opt_junk)
		memset(ptr, 0x5a, size);
#endif

	arena_run_reg_dalloc(run, bin, ptr, size);
	run->nfree++;

	if (run->nfree == bin->nregs) {
		
		if (run == bin->runcur)
			bin->runcur = NULL;
		else if (bin->nregs != 1) {
			




			RB_REMOVE(arena_run_tree_s, &bin->runs, run);
		}
#ifdef MALLOC_DEBUG
		run->magic = 0;
#endif
		arena_run_dalloc(arena, run, true);
#ifdef MALLOC_STATS
		bin->stats.curruns--;
#endif
	} else if (run->nfree == 1 && run != bin->runcur) {
		



		if (bin->runcur == NULL)
			bin->runcur = run;
		else if ((uintptr_t)run < (uintptr_t)bin->runcur) {
			
			if (bin->runcur->nfree > 0) {
				
				RB_INSERT(arena_run_tree_s, &bin->runs,
				    bin->runcur);
			}
			bin->runcur = run;
		} else
			RB_INSERT(arena_run_tree_s, &bin->runs, run);
	}
#ifdef MALLOC_STATS
	arena->stats.allocated_small -= size;
	arena->stats.ndalloc_small++;
#endif
}

#ifdef MALLOC_LAZY_FREE
static inline void
arena_dalloc_lazy(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t pageind, arena_chunk_map_t *mapelm)
{
	void **free_cache = arena->free_cache;
	unsigned i, slot;

	if (__isthreaded == false || opt_lazy_free_2pow < 0) {
		malloc_spin_lock(&arena->lock);
		arena_dalloc_small(arena, chunk, ptr, pageind, *mapelm);
		malloc_spin_unlock(&arena->lock);
		return;
	}

	for (i = 0; i < LAZY_FREE_NPROBES; i++) {
		slot = PRN(lazy_free, opt_lazy_free_2pow);
		if (atomic_cmpset_ptr((uintptr_t *)&free_cache[slot],
		    (uintptr_t)NULL, (uintptr_t)ptr)) {
			return;
		}
	}

	arena_dalloc_lazy_hard(arena, chunk, ptr, pageind, mapelm);
}

static void
arena_dalloc_lazy_hard(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t pageind, arena_chunk_map_t *mapelm)
{
	void **free_cache = arena->free_cache;
	unsigned i, slot;

	malloc_spin_lock(&arena->lock);
	arena_dalloc_small(arena, chunk, ptr, pageind, *mapelm);

	





	if ((ptr = (void *)atomic_readandclear_ptr(
	    (uintptr_t *)&free_cache[slot]))
	    != NULL) {
		unsigned lazy_free_mask;
		
		







		
		chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		pageind = (((uintptr_t)ptr - (uintptr_t)chunk) >>
		    pagesize_2pow);
		mapelm = &chunk->map[pageind];
		arena_dalloc_small(arena, chunk, ptr, pageind, *mapelm);

		
		lazy_free_mask = (1U << opt_lazy_free_2pow) - 1;
		for (i = (slot + 1) & lazy_free_mask;
		     i != slot;
		     i = (i + 1) & lazy_free_mask) {
			ptr = (void *)atomic_readandclear_ptr(
			    (uintptr_t *)&free_cache[i]);
			if (ptr != NULL) {
				chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
				pageind = (((uintptr_t)ptr - (uintptr_t)chunk)
				    >> pagesize_2pow);
				mapelm = &chunk->map[pageind];
				arena_dalloc_small(arena, chunk, ptr, pageind,
				    *mapelm);
			}
		}
	}

	malloc_spin_unlock(&arena->lock);
}
#endif

static void
arena_dalloc_large(arena_t *arena, arena_chunk_t *chunk, void *ptr)
{
	
	malloc_spin_lock(&arena->lock);

#ifdef MALLOC_FILL
#ifndef MALLOC_STATS
	if (opt_junk)
#endif
#endif
	{
		extent_node_t *node, key;
		size_t size;

		key.addr = ptr;
		node = RB_FIND(extent_tree_ad_s,
		    &arena->runs_alloced_ad, &key);
		assert(node != NULL);
		size = node->size;
#ifdef MALLOC_FILL
#ifdef MALLOC_STATS
		if (opt_junk)
#endif
			memset(ptr, 0x5a, size);
#endif
#ifdef MALLOC_STATS
		arena->stats.allocated_large -= size;
#endif
	}
#ifdef MALLOC_STATS
	arena->stats.ndalloc_large++;
#endif

	arena_run_dalloc(arena, (arena_run_t *)ptr, true);
	malloc_spin_unlock(&arena->lock);
}

static inline void
arena_dalloc(arena_t *arena, arena_chunk_t *chunk, void *ptr)
{
	size_t pageind;
	arena_chunk_map_t *mapelm;

	assert(arena != NULL);
	assert(arena->magic == ARENA_MAGIC);
	assert(chunk->arena == arena);
	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	pageind = (((uintptr_t)ptr - (uintptr_t)chunk) >> pagesize_2pow);
	mapelm = &chunk->map[pageind];
	if ((*mapelm & CHUNK_MAP_LARGE) == 0) {
		
#ifdef MALLOC_LAZY_FREE
		arena_dalloc_lazy(arena, chunk, ptr, pageind, mapelm);
#else
		malloc_spin_lock(&arena->lock);
		arena_dalloc_small(arena, chunk, ptr, pageind, *mapelm);
		malloc_spin_unlock(&arena->lock);
#endif
	} else {
		assert((*mapelm & CHUNK_MAP_POS_MASK) == 0);
		arena_dalloc_large(arena, chunk, ptr);
	}
}

static inline void
idalloc(void *ptr)
{
	arena_chunk_t *chunk;

	assert(ptr != NULL);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr)
		arena_dalloc(chunk->arena, chunk, ptr);
	else
		huge_dalloc(ptr);
}

static void
arena_ralloc_large_shrink(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t size, size_t oldsize)
{
	extent_node_t *node, key;

	assert(size < oldsize);

	



	key.addr = (void *)((uintptr_t)ptr);
#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	node = RB_FIND(extent_tree_ad_s, &arena->runs_alloced_ad, &key);
	assert(node != NULL);
	arena_run_trim_tail(arena, chunk, node, (arena_run_t *)ptr, oldsize,
	    size, true);
#ifdef MALLOC_STATS
	arena->stats.allocated_large -= oldsize - size;
#endif
	malloc_spin_unlock(&arena->lock);
}

static bool
arena_ralloc_large_grow(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t size, size_t oldsize)
{
	extent_node_t *nodeC, key;

	
	assert(size > oldsize);
	key.addr = (void *)((uintptr_t)ptr + oldsize);
#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	nodeC = RB_FIND(extent_tree_ad_s, &arena->runs_avail_ad, &key);
	if (nodeC != NULL && oldsize + nodeC->size >= size) {
		extent_node_t *nodeA, *nodeB;

		






		arena_run_split(arena, (arena_run_t *)nodeC->addr, size -
		    oldsize, false, false);

		key.addr = ptr;
		nodeA = RB_FIND(extent_tree_ad_s, &arena->runs_alloced_ad,
		    &key);
		assert(nodeA != NULL);

		key.addr = (void *)((uintptr_t)ptr + oldsize);
		nodeB = RB_FIND(extent_tree_ad_s, &arena->runs_alloced_ad,
		    &key);
		assert(nodeB != NULL);

		nodeA->size += nodeB->size;

		RB_REMOVE(extent_tree_ad_s, &arena->runs_alloced_ad, nodeB);
		arena_chunk_node_dealloc(chunk, nodeB);

#ifdef MALLOC_STATS
		arena->stats.allocated_large += size - oldsize;
#endif
		malloc_spin_unlock(&arena->lock);
		return (false);
	}
	malloc_spin_unlock(&arena->lock);

	return (true);
}





static bool
arena_ralloc_large(void *ptr, size_t size, size_t oldsize)
{
	size_t psize;

	psize = PAGE_CEILING(size);
	if (psize == oldsize) {
		
#ifdef MALLOC_FILL
		if (opt_junk && size < oldsize) {
			memset((void *)((uintptr_t)ptr + size), 0x5a, oldsize -
			    size);
		}
#endif
		return (false);
	} else {
		arena_chunk_t *chunk;
		arena_t *arena;

		chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		arena = chunk->arena;
		assert(arena->magic == ARENA_MAGIC);

		if (psize < oldsize) {
#ifdef MALLOC_FILL
			
			if (opt_junk) {
				memset((void *)((uintptr_t)ptr + size), 0x5a,
				    oldsize - size);
			}
#endif
			arena_ralloc_large_shrink(arena, chunk, ptr, psize,
			    oldsize);
			return (false);
		} else {
			bool ret = arena_ralloc_large_grow(arena, chunk, ptr,
			    psize, oldsize);
#ifdef MALLOC_FILL
			if (ret == false && opt_zero) {
				memset((void *)((uintptr_t)ptr + oldsize), 0,
				    size - oldsize);
			}
#endif
			return (ret);
		}
	}
}

static void *
arena_ralloc(void *ptr, size_t size, size_t oldsize)
{
	void *ret;
	size_t copysize;

	
	if (size < small_min) {
		if (oldsize < small_min &&
		    ffs((int)(pow2_ceil(size) >> (TINY_MIN_2POW + 1)))
		    == ffs((int)(pow2_ceil(oldsize) >> (TINY_MIN_2POW + 1))))
			goto IN_PLACE; 
	} else if (size <= small_max) {
		if (oldsize >= small_min && oldsize <= small_max &&
		    (QUANTUM_CEILING(size) >> opt_quantum_2pow)
		    == (QUANTUM_CEILING(oldsize) >> opt_quantum_2pow))
			goto IN_PLACE; 
	} else if (size <= bin_maxclass) {
		if (oldsize > small_max && oldsize <= bin_maxclass &&
		    pow2_ceil(size) == pow2_ceil(oldsize))
			goto IN_PLACE; 
	} else if (oldsize > bin_maxclass && oldsize <= arena_maxclass) {
		assert(size > bin_maxclass);
		if (arena_ralloc_large(ptr, size, oldsize) == false)
			return (ptr);
	}

	




	ret = arena_malloc(choose_arena(), size, false);
	if (ret == NULL)
		return (NULL);

	
	copysize = (size < oldsize) ? size : oldsize;
#ifdef VM_COPY_MIN
	if (copysize >= VM_COPY_MIN)
		pages_copy(ret, ptr, copysize);
	else
#endif
		memcpy(ret, ptr, copysize);
	idalloc(ptr);
	return (ret);
IN_PLACE:
#ifdef MALLOC_FILL
	if (opt_junk && size < oldsize)
		memset((void *)((uintptr_t)ptr + size), 0x5a, oldsize - size);
	else if (opt_zero && size > oldsize)
		memset((void *)((uintptr_t)ptr + oldsize), 0, size - oldsize);
#endif
	return (ptr);
}

static inline void *
iralloc(void *ptr, size_t size)
{
	size_t oldsize;

	assert(ptr != NULL);
	assert(size != 0);

	oldsize = isalloc(ptr);

	if (size <= arena_maxclass)
		return (arena_ralloc(ptr, size, oldsize));
	else
		return (huge_ralloc(ptr, size, oldsize));
}

static bool
arena_new(arena_t *arena)
{
	unsigned i;
	arena_bin_t *bin;
	size_t pow2_size, prev_run_size;

	if (malloc_spin_init(&arena->lock))
		return (true);

#ifdef MALLOC_STATS
	memset(&arena->stats, 0, sizeof(arena_stats_t));
#endif

	
	RB_INIT(&arena->chunks);
	arena->spare = NULL;

	arena->ndirty = 0;

	RB_INIT(&arena->runs_avail_szad);
	RB_INIT(&arena->runs_avail_ad);
	RB_INIT(&arena->runs_alloced_ad);

#ifdef MALLOC_BALANCE
	arena->contention = 0;
#endif
#ifdef MALLOC_LAZY_FREE
	if (opt_lazy_free_2pow >= 0) {
		arena->free_cache = (void **) base_calloc(1, sizeof(void *)
		    * (1U << opt_lazy_free_2pow));
		if (arena->free_cache == NULL)
			return (true);
	} else
		arena->free_cache = NULL;
#endif

	
	prev_run_size = pagesize;

	
	for (i = 0; i < ntbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		RB_INIT(&bin->runs);

		bin->reg_size = (1U << (TINY_MIN_2POW + i));

		prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);

#ifdef MALLOC_STATS
		memset(&bin->stats, 0, sizeof(malloc_bin_stats_t));
#endif
	}

	
	for (; i < ntbins + nqbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		RB_INIT(&bin->runs);

		bin->reg_size = quantum * (i - ntbins + 1);

		pow2_size = pow2_ceil(quantum * (i - ntbins + 1));
		prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);

#ifdef MALLOC_STATS
		memset(&bin->stats, 0, sizeof(malloc_bin_stats_t));
#endif
	}

	
	for (; i < ntbins + nqbins + nsbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		RB_INIT(&bin->runs);

		bin->reg_size = (small_max << (i - (ntbins + nqbins) + 1));

		prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);

#ifdef MALLOC_STATS
		memset(&bin->stats, 0, sizeof(malloc_bin_stats_t));
#endif
	}

#ifdef MALLOC_DEBUG
	arena->magic = ARENA_MAGIC;
#endif

	return (false);
}


static arena_t *
arenas_extend(unsigned ind)
{
	arena_t *ret;

	
	ret = (arena_t *)base_alloc(sizeof(arena_t)
	    + (sizeof(arena_bin_t) * (ntbins + nqbins + nsbins - 1)));
	if (ret != NULL && arena_new(ret) == false) {
		arenas[ind] = ret;
		return (ret);
	}
	

	





	_malloc_message(_getprogname(),
	    ": (malloc) Error initializing arena\n", "", "");
	if (opt_abort)
		abort();

	return (arenas[0]);
}









static void *
huge_malloc(size_t size, bool zero)
{
	void *ret;
	size_t csize;
	extent_node_t *node;

	

	csize = CHUNK_CEILING(size);
	if (csize == 0) {
		
		return (NULL);
	}

	
	node = base_node_alloc();
	if (node == NULL)
		return (NULL);

	ret = chunk_alloc(csize, zero);
	if (ret == NULL) {
		base_node_dealloc(node);
		return (NULL);
	}

	
	node->addr = ret;
	node->size = csize;

	malloc_mutex_lock(&huge_mtx);
	RB_INSERT(extent_tree_ad_s, &huge, node);
#ifdef MALLOC_STATS
	huge_nmalloc++;
	huge_allocated += csize;
#endif
	malloc_mutex_unlock(&huge_mtx);

#ifdef MALLOC_FILL
	if (zero == false) {
		if (opt_junk)
			memset(ret, 0xa5, csize);
		else if (opt_zero)
			memset(ret, 0, csize);
	}
#endif

	return (ret);
}


static void *
huge_palloc(size_t alignment, size_t size)
{
	void *ret;
	size_t alloc_size, chunk_size, offset;
	extent_node_t *node;

	







	assert(alignment >= chunksize);

	chunk_size = CHUNK_CEILING(size);

	if (size >= alignment)
		alloc_size = chunk_size + alignment - chunksize;
	else
		alloc_size = (alignment << 1) - chunksize;

	
	node = base_node_alloc();
	if (node == NULL)
		return (NULL);

	ret = chunk_alloc(alloc_size, false);
	if (ret == NULL) {
		base_node_dealloc(node);
		return (NULL);
	}

	offset = (uintptr_t)ret & (alignment - 1);
	assert((offset & chunksize_mask) == 0);
	assert(offset < alloc_size);
	if (offset == 0) {
		
		chunk_dealloc((void *)((uintptr_t)ret + chunk_size), alloc_size
		    - chunk_size);
	} else {
		size_t trailsize;

		
		chunk_dealloc(ret, alignment - offset);

		ret = (void *)((uintptr_t)ret + (alignment - offset));

		trailsize = alloc_size - (alignment - offset) - chunk_size;
		if (trailsize != 0) {
		    
		    assert(trailsize < alloc_size);
		    chunk_dealloc((void *)((uintptr_t)ret + chunk_size),
			trailsize);
		}
	}

	
	node->addr = ret;
	node->size = chunk_size;

	malloc_mutex_lock(&huge_mtx);
	RB_INSERT(extent_tree_ad_s, &huge, node);
#ifdef MALLOC_STATS
	huge_nmalloc++;
	huge_allocated += chunk_size;
#endif
	malloc_mutex_unlock(&huge_mtx);

#ifdef MALLOC_FILL
	if (opt_junk)
		memset(ret, 0xa5, chunk_size);
	else if (opt_zero)
		memset(ret, 0, chunk_size);
#endif

	return (ret);
}

static void *
huge_ralloc(void *ptr, size_t size, size_t oldsize)
{
	void *ret;
	size_t copysize;

	
	if (oldsize > arena_maxclass &&
	    CHUNK_CEILING(size) == CHUNK_CEILING(oldsize)) {
#ifdef MALLOC_FILL
		if (opt_junk && size < oldsize) {
			memset((void *)((uintptr_t)ptr + size), 0x5a, oldsize
			    - size);
		} else if (opt_zero && size > oldsize) {
			memset((void *)((uintptr_t)ptr + oldsize), 0, size
			    - oldsize);
		}
#endif
		return (ptr);
	}

	




	ret = huge_malloc(size, false);
	if (ret == NULL)
		return (NULL);

	copysize = (size < oldsize) ? size : oldsize;
#ifdef VM_COPY_MIN
	if (copysize >= VM_COPY_MIN)
		pages_copy(ret, ptr, copysize);
	else
#endif
		memcpy(ret, ptr, copysize);
	idalloc(ptr);
	return (ret);
}

static void
huge_dalloc(void *ptr)
{
	extent_node_t *node, key;

	malloc_mutex_lock(&huge_mtx);

	
	key.addr = ptr;
	node = RB_FIND(extent_tree_ad_s, &huge, &key);
	assert(node != NULL);
	assert(node->addr == ptr);
	RB_REMOVE(extent_tree_ad_s, &huge, node);

#ifdef MALLOC_STATS
	huge_ndalloc++;
	huge_allocated -= node->size;
#endif

	malloc_mutex_unlock(&huge_mtx);

	
#ifdef MALLOC_DSS
#ifdef MALLOC_FILL
	if (opt_dss && opt_junk)
		memset(node->addr, 0x5a, node->size);
#endif
#endif
	chunk_dealloc(node->addr, node->size);

	base_node_dealloc(node);
}

#ifdef MOZ_MEMORY_BSD
static inline unsigned
malloc_ncpus(void)
{
	unsigned ret;
	int mib[2];
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(ret);
	if (sysctl(mib, 2, &ret, &len, (void *) 0, 0) == -1) {
		
		return (1);
	}

	return (ret);
}
#elif (defined(MOZ_MEMORY_LINUX))
#include <fcntl.h>

static inline unsigned
malloc_ncpus(void)
{
	unsigned ret;
	int fd, nread, column;
	char buf[1];
	static const char matchstr[] = "processor\t:";

	




	fd = open("/proc/cpuinfo", O_RDONLY);
	if (fd == -1)
		return (1); 
	



	column = 0;
	ret = 0;
	while (true) {
		nread = read(fd, &buf, sizeof(buf));
		if (nread <= 0)
			break; 

		if (buf[0] == '\n')
			column = 0;
		else if (column != -1) {
			if (buf[0] == matchstr[column]) {
				column++;
				if (column == sizeof(matchstr) - 1) {
					column = -1;
					ret++;
				}
			} else
				column = -1;
		}
	}
	if (ret == 0)
		ret = 1; 
	close(fd);

	return (ret);
}
#elif (defined(MOZ_MEMORY_DARWIN))
#include <mach/mach_init.h>
#include <mach/mach_host.h>

static inline unsigned
malloc_ncpus(void)
{
	kern_return_t error;
	natural_t n;
	processor_info_array_t pinfo;
	mach_msg_type_number_t pinfocnt;

	error = host_processor_info(mach_host_self(), PROCESSOR_BASIC_INFO,
				    &n, &pinfo, &pinfocnt);
	if (error != KERN_SUCCESS)
		return (1); 
	else
		return (n);
}
#elif (defined(MOZ_MEMORY_SOLARIS))
#include <kstat.h>

static inline unsigned
malloc_ncpus(void)
{
	unsigned ret;
	kstat_ctl_t *ctl;
	kstat_t *kstat;
	kstat_named_t *named;
	unsigned i;

	if ((ctl = kstat_open()) == NULL)
		return (1); 

	if ((kstat = kstat_lookup(ctl, "unix", -1, "system_misc")) == NULL)
		return (1); 

	if (kstat_read(ctl, kstat, NULL) == -1)
		return (1); 

	named = KSTAT_NAMED_PTR(kstat);

	for (i = 0; i < kstat->ks_ndata; i++) {
		if (strcmp(named[i].name, "ncpus") == 0) {
			
			switch(named[i].data_type) {
			case KSTAT_DATA_INT32:
				ret = named[i].value.i32;
				break;
			case KSTAT_DATA_UINT32:
				ret = named[i].value.ui32;
				break;
			case KSTAT_DATA_INT64:
				ret = named[i].value.i64;
				break;
			case KSTAT_DATA_UINT64:
				ret = named[i].value.ui64;
				break;
			default:
				return (1); 
			}
		}
	}

	kstat_close(ctl); 

	return (ret);
}
#else
static inline unsigned
malloc_ncpus(void)
{

	



	return (1);
}
#endif

static void
malloc_print_stats(void)
{

	if (opt_print_stats) {
		char s[UMAX2S_BUFSIZE];
		_malloc_message("___ Begin malloc statistics ___\n", "", "",
		    "");
		_malloc_message("Assertions ",
#ifdef NDEBUG
		    "disabled",
#else
		    "enabled",
#endif
		    "\n", "");
		_malloc_message("Boolean MALLOC_OPTIONS: ",
		    opt_abort ? "A" : "a", "", "");
#ifdef MALLOC_DSS
		_malloc_message(opt_dss ? "D" : "d", "", "", "");
#endif
#ifdef MALLOC_FILL
		_malloc_message(opt_junk ? "J" : "j", "", "", "");
#endif
#ifdef MALLOC_DSS
		_malloc_message(opt_mmap ? "M" : "m", "", "", "");
#endif
		_malloc_message("P", "", "", "");
#ifdef MALLOC_UTRACE
		_malloc_message(opt_utrace ? "U" : "u", "", "", "");
#endif
#ifdef MALLOC_SYSV
		_malloc_message(opt_sysv ? "V" : "v", "", "", "");
#endif
#ifdef MALLOC_XMALLOC
		_malloc_message(opt_xmalloc ? "X" : "x", "", "", "");
#endif
#ifdef MALLOC_FILL
		_malloc_message(opt_zero ? "Z" : "z", "", "", "");
#endif
		_malloc_message("\n", "", "", "");

		_malloc_message("CPUs: ", umax2s(ncpus, s), "\n", "");
		_malloc_message("Max arenas: ", umax2s(narenas, s), "\n", "");
#ifdef MALLOC_LAZY_FREE
		if (opt_lazy_free_2pow >= 0) {
			_malloc_message("Lazy free slots: ",
			    umax2s(1U << opt_lazy_free_2pow, s), "\n", "");
		} else
			_malloc_message("Lazy free slots: 0\n", "", "", "");
#endif
#ifdef MALLOC_BALANCE
		_malloc_message("Arena balance threshold: ",
		    umax2s(opt_balance_threshold, s), "\n", "");
#endif
		_malloc_message("Pointer size: ", umax2s(sizeof(void *), s),
		    "\n", "");
		_malloc_message("Quantum size: ", umax2s(quantum, s), "\n", "");
		_malloc_message("Max small size: ", umax2s(small_max, s), "\n",
		    "");
		_malloc_message("Max dirty pages per arena: ",
		    umax2s(opt_dirty_max, s), "\n", "");

		_malloc_message("Chunk size: ", umax2s(chunksize, s), "", "");
		_malloc_message(" (2^", umax2s(opt_chunk_2pow, s), ")\n", "");

#ifdef MALLOC_STATS
		{
			size_t allocated, mapped;
#ifdef MALLOC_BALANCE
			uint64_t nbalance = 0;
#endif
			unsigned i;
			arena_t *arena;

			

			
			for (i = 0, allocated = 0; i < narenas; i++) {
				if (arenas[i] != NULL) {
					malloc_spin_lock(&arenas[i]->lock);
					allocated +=
					    arenas[i]->stats.allocated_small;
					allocated +=
					    arenas[i]->stats.allocated_large;
#ifdef MALLOC_BALANCE
					nbalance += arenas[i]->stats.nbalance;
#endif
					malloc_spin_unlock(&arenas[i]->lock);
				}
			}

			
			malloc_mutex_lock(&huge_mtx);
			allocated += huge_allocated;
			mapped = stats_chunks.curchunks * chunksize;
			malloc_mutex_unlock(&huge_mtx);

			malloc_mutex_lock(&base_mtx);
			mapped += base_mapped;
			malloc_mutex_unlock(&base_mtx);

#ifdef MOZ_MEMORY_WINDOWS
			malloc_printf("Allocated: %lu, mapped: %lu\n",
			    allocated, mapped);
#else
			malloc_printf("Allocated: %zu, mapped: %zu\n",
			    allocated, mapped);
#endif

#ifdef MALLOC_BALANCE
			malloc_printf("Arena balance reassignments: %llu\n",
			    nbalance);
#endif

			
			{
				chunk_stats_t chunks_stats;

				malloc_mutex_lock(&huge_mtx);
				chunks_stats = stats_chunks;
				malloc_mutex_unlock(&huge_mtx);

				malloc_printf("chunks: nchunks   "
				    "highchunks    curchunks\n");
				malloc_printf("  %13llu%13lu%13lu\n",
				    chunks_stats.nchunks,
				    chunks_stats.highchunks,
				    chunks_stats.curchunks);
			}

			
			malloc_printf(
			    "huge: nmalloc      ndalloc    allocated\n");
#ifdef MOZ_MEMORY_WINDOWS
			malloc_printf(" %12llu %12llu %12lu\n",
			    huge_nmalloc, huge_ndalloc, huge_allocated);
#else
			malloc_printf(" %12llu %12llu %12zu\n",
			    huge_nmalloc, huge_ndalloc, huge_allocated);
#endif
			
			for (i = 0; i < narenas; i++) {
				arena = arenas[i];
				if (arena != NULL) {
					malloc_printf(
					    "\narenas[%u]:\n", i);
					malloc_spin_lock(&arena->lock);
					stats_print(arena);
					malloc_spin_unlock(&arena->lock);
				}
			}
		}
#endif 
		_malloc_message("--- End malloc statistics ---\n", "", "", "");
	}
}






#if (defined(MOZ_MEMORY_WINDOWS) || defined(MOZ_MEMORY_DARWIN))
#define	malloc_init() false
#else
static inline bool
malloc_init(void)
{

	if (malloc_initialized == false)
		return (malloc_init_hard());

	return (false);
}
#endif

#ifndef MOZ_MEMORY_WINDOWS
static
#endif
bool
malloc_init_hard(void)
{
	unsigned i;
	char buf[PATH_MAX + 1];
	const char *opts;
	long result;
#ifndef MOZ_MEMORY_WINDOWS
	int linklen;
#endif

#ifndef MOZ_MEMORY_WINDOWS
	malloc_mutex_lock(&init_lock);
#endif

	if (malloc_initialized) {
		



#ifndef MOZ_MEMORY_WINDOWS
		malloc_mutex_unlock(&init_lock);
#endif
		return (false);
	}

#ifdef MOZ_MEMORY_WINDOWS
	
	tlsIndex = TlsAlloc();
#endif

	
#ifdef MOZ_MEMORY_WINDOWS
	{
		SYSTEM_INFO info;

		GetSystemInfo(&info);
		result = info.dwPageSize;

		pagesize = (unsigned) result;

		ncpus = info.dwNumberOfProcessors;
	}
#else
	ncpus = malloc_ncpus();

	result = sysconf(_SC_PAGESIZE);
	assert(result != -1);

	pagesize = (unsigned) result;
#endif

	



	assert(((result - 1) & result) == 0);
	pagesize_mask = result - 1;
	pagesize_2pow = ffs((int)result) - 1;

#ifdef MALLOC_LAZY_FREE
		if (ncpus == 1)
			opt_lazy_free_2pow = -1;
#endif

	for (i = 0; i < 3; i++) {
		unsigned j;

		
		switch (i) {
		case 0:
#ifndef MOZ_MEMORY_WINDOWS
			if ((linklen = readlink("/etc/malloc.conf", buf,
						sizeof(buf) - 1)) != -1) {
				



				buf[linklen] = '\0';
				opts = buf;
			} else
#endif
			{
				
				buf[0] = '\0';
				opts = buf;
			}
			break;
		case 1:
			if (issetugid() == 0 && (opts =
			    getenv("MALLOC_OPTIONS")) != NULL) {
				




			} else {
				
				buf[0] = '\0';
				opts = buf;
			}
			break;
		case 2:
			if (_malloc_options != NULL) {
				



				opts = _malloc_options;
			} else {
				
				buf[0] = '\0';
				opts = buf;
			}
			break;
		default:
			
			buf[0] = '\0';
			opts = buf;
			assert(false);
		}

		for (j = 0; opts[j] != '\0'; j++) {
			unsigned k, nreps;
			bool nseen;

			
			for (nreps = 0, nseen = false;; j++, nseen = true) {
				switch (opts[j]) {
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
					case '8': case '9':
						nreps *= 10;
						nreps += opts[j] - '0';
						break;
					default:
						goto MALLOC_OUT;
				}
			}
MALLOC_OUT:
			if (nseen == false)
				nreps = 1;

			for (k = 0; k < nreps; k++) {
				switch (opts[j]) {
				case 'a':
					opt_abort = false;
					break;
				case 'A':
					opt_abort = true;
					break;
				case 'b':
#ifdef MALLOC_BALANCE
					opt_balance_threshold >>= 1;
#endif
					break;
				case 'B':
#ifdef MALLOC_BALANCE
					if (opt_balance_threshold == 0)
						opt_balance_threshold = 1;
					else if ((opt_balance_threshold << 1)
					    > opt_balance_threshold)
						opt_balance_threshold <<= 1;
#endif
					break;
				case 'd':
#ifdef MALLOC_DSS
					opt_dss = false;
#endif
					break;
				case 'D':
#ifdef MALLOC_DSS
					opt_dss = true;
#endif
					break;
				case 'f':
					opt_dirty_max >>= 1;
					break;
				case 'F':
					if (opt_dirty_max == 0)
						opt_dirty_max = 1;
					else if ((opt_dirty_max << 1) != 0)
						opt_dirty_max <<= 1;
					break;
#ifdef MALLOC_FILL
				case 'j':
					opt_junk = false;
					break;
				case 'J':
					opt_junk = true;
					break;
#endif
				case 'k':
					




					if (opt_chunk_2pow > pagesize_2pow + 1)
						opt_chunk_2pow--;
					break;
				case 'K':
					if (opt_chunk_2pow + 1 <
					    (sizeof(size_t) << 3))
						opt_chunk_2pow++;
					break;
				case 'l':
#ifdef MALLOC_LAZY_FREE
					if (opt_lazy_free_2pow >= 0)
						opt_lazy_free_2pow--;
#endif
					break;
				case 'L':
#ifdef MALLOC_LAZY_FREE
					if (ncpus > 1)
						opt_lazy_free_2pow++;
#endif
					break;
				case 'm':
#ifdef MALLOC_DSS
					opt_mmap = false;
#endif
					break;
				case 'M':
#ifdef MALLOC_DSS
					opt_mmap = true;
#endif
					break;
				case 'n':
					opt_narenas_lshift--;
					break;
				case 'N':
					opt_narenas_lshift++;
					break;
				case 'p':
					opt_print_stats = false;
					break;
				case 'P':
					opt_print_stats = true;
					break;
				case 'q':
					if (opt_quantum_2pow > QUANTUM_2POW_MIN)
						opt_quantum_2pow--;
					break;
				case 'Q':
					if (opt_quantum_2pow < pagesize_2pow -
					    1)
						opt_quantum_2pow++;
					break;
				case 's':
					if (opt_small_max_2pow >
					    QUANTUM_2POW_MIN)
						opt_small_max_2pow--;
					break;
				case 'S':
					if (opt_small_max_2pow < pagesize_2pow
					    - 1)
						opt_small_max_2pow++;
					break;
#ifdef MALLOC_UTRACE
				case 'u':
					opt_utrace = false;
					break;
				case 'U':
					opt_utrace = true;
					break;
#endif
#ifdef MALLOC_SYSV
				case 'v':
					opt_sysv = false;
					break;
				case 'V':
					opt_sysv = true;
					break;
#endif
#ifdef MALLOC_XMALLOC
				case 'x':
					opt_xmalloc = false;
					break;
				case 'X':
					opt_xmalloc = true;
					break;
#endif
#ifdef MALLOC_FILL
				case 'z':
					opt_zero = false;
					break;
				case 'Z':
					opt_zero = true;
					break;
#endif
				default: {
					char cbuf[2];

					cbuf[0] = opts[j];
					cbuf[1] = '\0';
					_malloc_message(_getprogname(),
					    ": (malloc) Unsupported character "
					    "in malloc options: '", cbuf,
					    "'\n");
				}
				}
			}
		}
	}

#ifdef MALLOC_DSS
	
	if (opt_dss == false && opt_mmap == false)
		opt_mmap = true;
#endif

	
	if (opt_print_stats) {
#ifndef MOZ_MEMORY_WINDOWS
		
		atexit(malloc_print_stats);
#endif
	}

	
	if (opt_small_max_2pow < opt_quantum_2pow)
		opt_small_max_2pow = opt_quantum_2pow;
	small_max = (1U << opt_small_max_2pow);

	
	bin_maxclass = (pagesize >> 1);
	assert(opt_quantum_2pow >= TINY_MIN_2POW);
	ntbins = opt_quantum_2pow - TINY_MIN_2POW;
	assert(ntbins <= opt_quantum_2pow);
	nqbins = (small_max >> opt_quantum_2pow);
	nsbins = pagesize_2pow - opt_small_max_2pow - 1;

	
	quantum = (1U << opt_quantum_2pow);
	quantum_mask = quantum - 1;
	if (ntbins > 0)
		small_min = (quantum >> 1) + 1;
	else
		small_min = 1;
	assert(small_min <= quantum);

	
	chunksize = (1LU << opt_chunk_2pow);
	chunksize_mask = chunksize - 1;
	chunk_npages = (chunksize >> pagesize_2pow);
	{
		size_t header_size;

		






		header_size = sizeof(arena_chunk_t) +
		    (sizeof(arena_chunk_map_t) * (chunk_npages - 1)) +
		    (sizeof(extent_node_t) * chunk_npages);
		arena_chunk_header_npages = (header_size >> pagesize_2pow) +
		    ((header_size & pagesize_mask) != 0);
	}
	arena_maxclass = chunksize - (arena_chunk_header_npages <<
	    pagesize_2pow);
#ifdef MALLOC_LAZY_FREE
	



	while ((sizeof(void *) << opt_lazy_free_2pow) > chunksize)
		opt_lazy_free_2pow--;
#endif

	UTRACE(0, 0, 0);

#ifdef MALLOC_STATS
	memset(&stats_chunks, 0, sizeof(chunk_stats_t));
#endif

	
	assert(quantum >= sizeof(void *));
	assert(quantum <= pagesize);
	assert(chunksize >= pagesize);
	assert(quantum * 4 <= chunksize);

	
	malloc_mutex_init(&huge_mtx);
	RB_INIT(&huge);
#ifdef MALLOC_DSS
	malloc_mutex_init(&dss_mtx);
	dss_base = sbrk(0);
	dss_prev = dss_base;
	dss_max = dss_base;
	RB_INIT(&dss_chunks_szad);
	RB_INIT(&dss_chunks_ad);
#endif
#ifdef MALLOC_STATS
	huge_nmalloc = 0;
	huge_ndalloc = 0;
	huge_allocated = 0;
#endif

	
#ifdef MALLOC_STATS
	base_mapped = 0;
#endif
#ifdef MALLOC_DSS
	




	if (opt_dss)
		base_pages_alloc(0);
#endif
	base_nodes = NULL;
	malloc_mutex_init(&base_mtx);

	if (ncpus > 1) {
		



		opt_narenas_lshift += 2;
	}

	
	narenas = ncpus;
	if (opt_narenas_lshift > 0) {
		if ((narenas << opt_narenas_lshift) > narenas)
			narenas <<= opt_narenas_lshift;
		



		if (narenas * sizeof(arena_t *) > chunksize)
			narenas = chunksize / sizeof(arena_t *);
	} else if (opt_narenas_lshift < 0) {
		if ((narenas >> -opt_narenas_lshift) < narenas)
			narenas >>= -opt_narenas_lshift;
		
		if (narenas == 0)
			narenas = 1;
	}
#ifdef MALLOC_BALANCE
	assert(narenas != 0);
	for (narenas_2pow = 0;
	     (narenas >> (narenas_2pow + 1)) != 0;
	     narenas_2pow++);
#endif

#ifdef NO_TLS
	if (narenas > 1) {
		static const unsigned primes[] = {1, 3, 5, 7, 11, 13, 17, 19,
		    23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83,
		    89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149,
		    151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211,
		    223, 227, 229, 233, 239, 241, 251, 257, 263};
		unsigned nprimes, parenas;

		




		assert((narenas & 1) == 0); 
		nprimes = (sizeof(primes) >> SIZEOF_INT_2POW);
		parenas = primes[nprimes - 1]; 
		for (i = 1; i < nprimes; i++) {
			if (primes[i] > narenas) {
				parenas = primes[i];
				break;
			}
		}
		narenas = parenas;
	}
#endif

#ifndef NO_TLS
#  ifndef MALLOC_BALANCE
	next_arena = 0;
#  endif
#endif

	
	arenas = (arena_t **)base_alloc(sizeof(arena_t *) * narenas);
	if (arenas == NULL) {
#ifndef MOZ_MEMORY_WINDOWS
		malloc_mutex_unlock(&init_lock);
#endif
		return (true);
	}
	



	memset(arenas, 0, sizeof(arena_t *) * narenas);

	



	arenas_extend(0);
	if (arenas[0] == NULL) {
#ifndef MOZ_MEMORY_WINDOWS
		malloc_mutex_unlock(&init_lock);
#endif
		return (true);
	}
#ifndef NO_TLS
	




#ifdef MOZ_MEMORY_WINDOWS
	TlsSetValue(tlsIndex, arenas[0]);
#else
	arenas_map = arenas[0];
#endif
#endif

	



#ifdef MALLOC_LAZY_FREE
	SPRN(lazy_free, 42);
#endif
#ifdef MALLOC_BALANCE
	SPRN(balance, 42);
#endif

	malloc_spin_init(&arenas_lock);

	malloc_initialized = true;
#ifndef MOZ_MEMORY_WINDOWS
	malloc_mutex_unlock(&init_lock);
#endif
	return (false);
}


#ifdef MOZ_MEMORY_WINDOWS
void
malloc_shutdown()
{

	malloc_print_stats();
}
#endif









VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline void *
moz_malloc(size_t size)
#else
void *
malloc(size_t size)
#endif
{
	void *ret;

	if (malloc_init()) {
		ret = NULL;
		goto RETURN;
	}

	if (size == 0) {
#ifdef MALLOC_SYSV
		if (opt_sysv == false)
#endif
			size = 1;
#ifdef MALLOC_SYSV
		else {
			ret = NULL;
			goto RETURN;
		}
#endif
	}

	ret = imalloc(size);

RETURN:
	if (ret == NULL) {
#ifdef MALLOC_XMALLOC
		if (opt_xmalloc) {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in malloc(): out of memory\n", "",
			    "");
			abort();
		}
#endif
		errno = ENOMEM;
	}

	UTRACE(0, size, ret);
	return (ret);
}

VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline int
moz_posix_memalign(void **memptr, size_t alignment, size_t size)
#else
int
posix_memalign(void **memptr, size_t alignment, size_t size)
#endif
{
	int ret;
	void *result;

	if (malloc_init())
		result = NULL;
	else {
		
		if (((alignment - 1) & alignment) != 0
		    || alignment < sizeof(void *)) {
#ifdef MALLOC_XMALLOC
			if (opt_xmalloc) {
				_malloc_message(_getprogname(),
				    ": (malloc) Error in posix_memalign(): "
				    "invalid alignment\n", "", "");
				abort();
			}
#endif
			result = NULL;
			ret = EINVAL;
			goto RETURN;
		}

		result = ipalloc(alignment, size);
	}

	if (result == NULL) {
#ifdef MALLOC_XMALLOC
		if (opt_xmalloc) {
			_malloc_message(_getprogname(),
			": (malloc) Error in posix_memalign(): out of memory\n",
			"", "");
			abort();
		}
#endif
		ret = ENOMEM;
		goto RETURN;
	}

	*memptr = result;
	ret = 0;

RETURN:
	UTRACE(0, size, result);
	return (ret);
}

VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline void *
moz_memalign(size_t alignment, size_t size)
#else
void *
memalign(size_t alignment, size_t size)
#endif
{
	void *ret;

#ifdef MOZ_MEMORY_DARWIN
	if (moz_posix_memalign(&ret, alignment, size) != 0)
#else
	if (posix_memalign(&ret, alignment, size) != 0)
#endif
		return (NULL);

	return ret;
}

VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline void *
moz_valloc(size_t size)
#else
void *
valloc(size_t size)
#endif
{
#ifdef MOZ_MEMORY_DARWIN
	return (moz_memalign(pagesize, size));
#else
	return (memalign(pagesize, size));
#endif
}

VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline void *
moz_calloc(size_t num, size_t size)
#else
void *
calloc(size_t num, size_t size)
#endif
{
	void *ret;
	size_t num_size;

	if (malloc_init()) {
		num_size = 0;
		ret = NULL;
		goto RETURN;
	}

	num_size = num * size;
	if (num_size == 0) {
#ifdef MALLOC_SYSV
		if ((opt_sysv == false) && ((num == 0) || (size == 0)))
#endif
			num_size = 1;
#ifdef MALLOC_SYSV
		else {
			ret = NULL;
			goto RETURN;
		}
#endif
	




	} else if (((num | size) & (SIZE_T_MAX << (sizeof(size_t) << 2)))
	    && (num_size / size != num)) {
		
		ret = NULL;
		goto RETURN;
	}

	ret = icalloc(num_size);

RETURN:
	if (ret == NULL) {
#ifdef MALLOC_XMALLOC
		if (opt_xmalloc) {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in calloc(): out of memory\n", "",
			    "");
			abort();
		}
#endif
		errno = ENOMEM;
	}

	UTRACE(0, num_size, ret);
	return (ret);
}

VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline void *
moz_realloc(void *ptr, size_t size)
#else
void *
realloc(void *ptr, size_t size)
#endif
{
	void *ret;

	if (size == 0) {
#ifdef MALLOC_SYSV
		if (opt_sysv == false)
#endif
			size = 1;
#ifdef MALLOC_SYSV
		else {
			if (ptr != NULL)
				idalloc(ptr);
			ret = NULL;
			goto RETURN;
		}
#endif
	}

	if (ptr != NULL) {
		assert(malloc_initialized);

		ret = iralloc(ptr, size);

		if (ret == NULL) {
#ifdef MALLOC_XMALLOC
			if (opt_xmalloc) {
				_malloc_message(_getprogname(),
				    ": (malloc) Error in realloc(): out of "
				    "memory\n", "", "");
				abort();
			}
#endif
			errno = ENOMEM;
		}
	} else {
		if (malloc_init())
			ret = NULL;
		else
			ret = imalloc(size);

		if (ret == NULL) {
#ifdef MALLOC_XMALLOC
			if (opt_xmalloc) {
				_malloc_message(_getprogname(),
				    ": (malloc) Error in realloc(): out of "
				    "memory\n", "", "");
				abort();
			}
#endif
			errno = ENOMEM;
		}
	}

#ifdef MALLOC_SYSV
RETURN:
#endif
	UTRACE(ptr, size, ret);
	return (ret);
}

VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline void
moz_free(void *ptr)
#else
void
free(void *ptr)
#endif
{

	UTRACE(ptr, 0, 0);
	if (ptr != NULL) {
		assert(malloc_initialized);

		idalloc(ptr);
	}
}









VISIBLE
#ifdef MOZ_MEMORY_DARWIN
inline size_t
moz_malloc_usable_size(const void *ptr)
#else
size_t
malloc_usable_size(const void *ptr)
#endif
{

	assert(ptr != NULL);

	return (isalloc(ptr));
}

#ifdef MOZ_MEMORY_WINDOWS
void*
_recalloc(void *ptr, size_t count, size_t size)
{
	size_t oldsize = (ptr != NULL) ? isalloc(ptr) : 0;
	size_t newsize = count * size;

	







	ptr = realloc(ptr, newsize);
	if (ptr != NULL && oldsize < newsize) {
		memset((void *)((uintptr_t)ptr + oldsize), 0, newsize -
		    oldsize);
	}

	return ptr;
}





void*
_expand(void *ptr, size_t newsize)
{
	if (isalloc(ptr) >= newsize)
		return ptr;

	return NULL;
}

size_t
_msize(const void *ptr)
{

	return malloc_usable_size(ptr);
}
#endif













void
_malloc_prefork(void)
{
	unsigned i;

	

	malloc_spin_lock(&arenas_lock);
	for (i = 0; i < narenas; i++) {
		if (arenas[i] != NULL)
			malloc_spin_lock(&arenas[i]->lock);
	}
	malloc_spin_unlock(&arenas_lock);

	malloc_mutex_lock(&base_mtx);

	malloc_mutex_lock(&huge_mtx);

#ifdef MALLOC_DSS
	malloc_mutex_lock(&dss_mtx);
#endif
}

void
_malloc_postfork(void)
{
	unsigned i;

	

#ifdef MALLOC_DSS
	malloc_mutex_unlock(&dss_mtx);
#endif

	malloc_mutex_unlock(&huge_mtx);

	malloc_mutex_unlock(&base_mtx);

	malloc_spin_lock(&arenas_lock);
	for (i = 0; i < narenas; i++) {
		if (arenas[i] != NULL)
			malloc_spin_unlock(&arenas[i]->lock);
	}
	malloc_spin_unlock(&arenas_lock);
}







#ifdef MOZ_MEMORY_DARWIN
static malloc_zone_t zone;
static struct malloc_introspection_t zone_introspect;

static size_t
zone_size(malloc_zone_t *zone, void *ptr)
{
	size_t ret = 0;
	arena_chunk_t *chunk;

	









	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk != ptr) {
		arena_t *arena;
		unsigned i;
		arena_t *arenas_snapshot[narenas];

		





		malloc_spin_lock(&arenas_lock);
		memcpy(&arenas_snapshot, arenas, sizeof(arena_t *) * narenas);
		malloc_spin_unlock(&arenas_lock);

		
		for (i = 0; i < narenas; i++) {
			arena = arenas_snapshot[i];

			if (arena != NULL) {
				bool own;

				
				malloc_spin_lock(&arena->lock);
				if (RB_FIND(arena_chunk_tree_s, &arena->chunks,
				    chunk) == chunk)
					own = true;
				else
					own = false;
				malloc_spin_unlock(&arena->lock);

				if (own) {
					ret = arena_salloc(ptr);
					goto RETURN;
				}
			}
		}
	} else {
		extent_node_t *node;
		extent_node_t key;

		
		key.addr = (void *)chunk;
		malloc_mutex_lock(&huge_mtx);
		node = RB_FIND(extent_tree_ad_s, &huge, &key);
		if (node != NULL)
			ret = node->size;
		else
			ret = 0;
		malloc_mutex_unlock(&huge_mtx);
	}

RETURN:
	return (ret);
}

static void *
zone_malloc(malloc_zone_t *zone, size_t size)
{

	return (moz_malloc(size));
}

static void *
zone_calloc(malloc_zone_t *zone, size_t num, size_t size)
{

	return (moz_calloc(num, size));
}

static void *
zone_valloc(malloc_zone_t *zone, size_t size)
{
	void *ret = NULL; 

	moz_posix_memalign(&ret, pagesize, size);

	return (ret);
}

static void
zone_free(malloc_zone_t *zone, void *ptr)
{

	moz_free(ptr);
}

static void *
zone_realloc(malloc_zone_t *zone, void *ptr, size_t size)
{

	return (moz_realloc(ptr, size));
}

static void *
zone_destroy(malloc_zone_t *zone)
{

	
	assert(false);
	return (NULL);
}

static size_t
zone_good_size(malloc_zone_t *zone, size_t size)
{
	size_t ret;
	void *p;

	




	p = moz_malloc(size);
	if (p != NULL) {
		ret = isalloc(p);
		moz_free(p);
	} else
		ret = size;

	return (ret);
}

static void
zone_force_lock(malloc_zone_t *zone)
{

	_malloc_prefork();
}

static void
zone_force_unlock(malloc_zone_t *zone)
{

	_malloc_postfork();
}

static malloc_zone_t *
create_zone(void)
{

	assert(malloc_initialized);

	zone.size = (void *)zone_size;
	zone.malloc = (void *)zone_malloc;
	zone.calloc = (void *)zone_calloc;
	zone.valloc = (void *)zone_valloc;
	zone.free = (void *)zone_free;
	zone.realloc = (void *)zone_realloc;
	zone.destroy = (void *)zone_destroy;
	zone.zone_name = "jemalloc_zone";
	zone.batch_malloc = NULL;
	zone.batch_free = NULL;
	zone.introspect = &zone_introspect;

	zone_introspect.enumerator = NULL;
	zone_introspect.good_size = (void *)zone_good_size;
	zone_introspect.check = NULL;
	zone_introspect.print = NULL;
	zone_introspect.log = NULL;
	zone_introspect.force_lock = (void *)zone_force_lock;
	zone_introspect.force_unlock = (void *)zone_force_unlock;
	zone_introspect.statistics = NULL;

	return (&zone);
}

__attribute__((constructor))
void
jemalloc_darwin_init(void)
{
	extern unsigned malloc_num_zones;
	extern malloc_zone_t **malloc_zones;

	if (malloc_init_hard())
		abort();

	




	
	malloc_zone_register(create_zone());
	assert(malloc_zones[malloc_num_zones - 1] == &zone);

	



	assert(malloc_num_zones > 1);
	memmove(&malloc_zones[1], &malloc_zones[0],
		sizeof(malloc_zone_t *) * (malloc_num_zones - 1));
	malloc_zones[0] = &zone;
}
#endif
