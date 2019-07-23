






































































































#ifndef MOZ_MEMORY_DEBUG
#  define	MALLOC_PRODUCTION
#endif






#define	MOZ_MEMORY_NARENAS_DEFAULT_ONE





#define MALLOC_STATS

#ifndef MALLOC_PRODUCTION
   



#  define MALLOC_DEBUG


#  define MALLOC_FILL


#  ifndef MOZ_MEMORY_WINDOWS
#    define MALLOC_UTRACE
#  endif

   
#  define MALLOC_XMALLOC


#  define MALLOC_SYSV
#endif






#define MALLOC_VALIDATE


#ifdef MOZ_VALGRIND
#  define MALLOC_VALGRIND
#endif
#ifdef MALLOC_VALGRIND
#  include <valgrind/valgrind.h>
#else
#  define VALGRIND_MALLOCLIKE_BLOCK(addr, sizeB, rzB, is_zeroed)
#  define VALGRIND_FREELIKE_BLOCK(addr, rzB)
#endif








#if (!defined(MOZ_MEMORY_WINDOWS) && !defined(MOZ_MEMORY_DARWIN))
   







#define MALLOC_PAGEFILE
#endif

#ifdef MALLOC_PAGEFILE

#  define MALLOC_PAGEFILE_WRITE_SIZE 512
#endif

#ifdef MOZ_MEMORY_LINUX
#define	_GNU_SOURCE
#define	issetugid() 0
#if 0 
#  define MALLOC_DECOMMIT
#endif
#endif

#ifndef MOZ_MEMORY_WINCE
#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#endif
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef MOZ_MEMORY_WINDOWS
#ifndef MOZ_MEMORY_WINCE
#include <cruntime.h>
#include <internal.h>
#include <io.h>
#else
#include <cmnintrin.h>
#include <crtdefs.h>
#define SIZE_MAX UINT_MAX
#endif
#include <windows.h>

#pragma warning( disable: 4267 4996 4146 )

#define	bool BOOL
#define	false FALSE
#define	true TRUE
#define	inline __inline
#define	SIZE_T_MAX SIZE_MAX
#define	STDERR_FILENO 2
#define	PATH_MAX MAX_PATH
#define	vsnprintf _vsnprintf

#ifndef NO_TLS
static unsigned long tlsIndex = 0xffffffff;
#endif 

#define	__thread
#ifdef MOZ_MEMORY_WINCE
#define	_pthread_self() GetCurrentThreadId()
#else
#define	_pthread_self() __threadid()
#endif
#define	issetugid() 0

#ifndef MOZ_MEMORY_WINCE

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

#else 

#define ENOMEM          12
#define EINVAL          22

static __forceinline int
ffs(int x)
{

	return 32 - _CountLeadingZeros((-x) & x);
}
#endif

typedef unsigned char uint8_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long uintmax_t;
typedef long ssize_t;

#define	MALLOC_DECOMMIT
#endif

#ifndef MOZ_MEMORY_WINDOWS
#ifndef MOZ_MEMORY_SOLARIS
#include <sys/cdefs.h>
#endif
#ifndef __DECONST
#  define __DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif
#ifndef MOZ_MEMORY
__FBSDID("$FreeBSD: head/lib/libc/stdlib/malloc.c 180599 2008-07-18 19:35:44Z jasone $");
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
#ifndef MAP_NOSYNC
#  define MAP_NOSYNC	0
#endif
#include <sys/param.h>
#ifndef MOZ_MEMORY
#include <sys/stddef.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#ifndef MOZ_MEMORY_SOLARIS
#include <sys/sysctl.h>
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
#include <stdio.h>
#include <stdbool.h>
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

#include "jemalloc.h"

#ifdef MOZ_MEMORY_DARWIN
static const bool __isthreaded = true;
#endif

#if defined(MOZ_MEMORY_SOLARIS) && defined(MAP_ALIGN) && !defined(JEMALLOC_NEVER_USES_MAP_ALIGN)
#define JEMALLOC_USES_MAP_ALIGN
#endif

#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
#define JEMALLOC_USES_MAP_ALIGN
#endif

#define __DECONST(type, var) ((type)(uintptr_t)(const void *)(var))

#ifdef MOZ_MEMORY_WINDOWS
   
#  define RB_NO_C99_VARARRAYS
#endif
#include "rb.h"

#ifdef MALLOC_DEBUG
   
#ifdef inline
#undef inline
#endif

#  define inline
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
#ifdef __mips__
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
#endif





#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
#define	CHUNK_2POW_DEFAULT	21
#else
#define	CHUNK_2POW_DEFAULT	20
#endif

#define	DIRTY_MAX_DEFAULT	(1U << 10)






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

	
	size_t		committed;
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
	
	rb_node(extent_node_t) link_szad;

	
	rb_node(extent_node_t) link_ad;

	
	void	*addr;

	
	size_t	size;
};
typedef rb_tree(extent_node_t) extent_tree_t;






#ifdef MALLOC_VALIDATE
   



#  if (SIZEOF_PTR == 4)
#    define MALLOC_RTREE_NODESIZE (1U << 14)
#  else
#    define MALLOC_RTREE_NODESIZE CACHELINE
#  endif

typedef struct malloc_rtree_s malloc_rtree_t;
struct malloc_rtree_s {
	malloc_spinlock_t	lock;
	void			**root;
	unsigned		height;
	unsigned		level2bits[1]; 
};
#endif






typedef struct arena_s arena_t;
typedef struct arena_bin_s arena_bin_t;


typedef struct arena_chunk_map_s arena_chunk_map_t;
struct arena_chunk_map_s {
	






	rb_node(arena_chunk_map_t)	link;

	








































	size_t				bits;
#ifdef MALLOC_DECOMMIT
#define	CHUNK_MAP_DECOMMITTED	((size_t)0x20U)
#endif
#define	CHUNK_MAP_KEY		((size_t)0x10U)
#define	CHUNK_MAP_DIRTY		((size_t)0x08U)
#define	CHUNK_MAP_ZEROED	((size_t)0x04U)
#define	CHUNK_MAP_LARGE		((size_t)0x02U)
#define	CHUNK_MAP_ALLOCATED	((size_t)0x01U)
};
typedef rb_tree(arena_chunk_map_t) arena_avail_tree_t;
typedef rb_tree(arena_chunk_map_t) arena_run_tree_t;


typedef struct arena_chunk_s arena_chunk_t;
struct arena_chunk_s {
	
	arena_t		*arena;

	
	rb_node(arena_chunk_t) link_dirty;

	
	size_t		ndirty;

	
	arena_chunk_map_t map[1]; 
};
typedef rb_tree(arena_chunk_t) arena_chunk_tree_t;

typedef struct arena_run_s arena_run_t;
struct arena_run_s {
#ifdef MALLOC_DEBUG
	uint32_t	magic;
#  define ARENA_RUN_MAGIC 0x384adf93
#endif

	
	arena_bin_t	*bin;

	
	unsigned	regs_minelm;

	
	unsigned	nfree;

	
	unsigned	regs_mask[1]; 
};

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

	
	arena_chunk_tree_t	chunks_dirty;

	









	arena_chunk_t		*spare;

	





	size_t			ndirty;

	



	arena_avail_tree_t	runs_avail;

#ifdef MALLOC_BALANCE
	



	uint32_t		contention;
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






#ifdef MALLOC_VALIDATE
static malloc_rtree_t *chunk_rtree;
#endif


static malloc_mutex_t	huge_mtx;


static extent_tree_t	huge;

#ifdef MALLOC_STATS

static uint64_t		huge_nmalloc;
static uint64_t		huge_ndalloc;
static size_t		huge_allocated;
#endif

#ifdef MALLOC_PAGEFILE
static char		pagefile_templ[PATH_MAX];
#endif











static void		*base_pages;
static void		*base_next_addr;
#ifdef MALLOC_DECOMMIT
static void		*base_next_decommitted;
#endif
static void		*base_past_addr; 
static extent_node_t	*base_nodes;
static malloc_mutex_t	base_mtx;
#ifdef MALLOC_STATS
static size_t		base_mapped;
#  ifdef MALLOC_DECOMMIT
static size_t		base_committed;
#  endif
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





const char	*_malloc_options;

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
static size_t	opt_dirty_max = DIRTY_MAX_DEFAULT;
#ifdef MALLOC_BALANCE
static uint64_t	opt_balance_threshold = BALANCE_THRESHOLD_DEFAULT;
#endif
static bool	opt_print_stats = false;
static size_t	opt_quantum_2pow = QUANTUM_2POW_MIN;
static size_t	opt_small_max_2pow = SMALL_MAX_2POW_DEFAULT;
static size_t	opt_chunk_2pow = CHUNK_2POW_DEFAULT;
#ifdef MALLOC_PAGEFILE
static bool	opt_pagefile = false;
#endif
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






static char	*umax2s(uintmax_t x, char *s);
static bool	malloc_mutex_init(malloc_mutex_t *mutex);
static bool	malloc_spin_init(malloc_spinlock_t *lock);
static void	wrtmessage(const char *p1, const char *p2, const char *p3,
		const char *p4);
#ifdef MALLOC_STATS
#ifdef MOZ_MEMORY_DARWIN

#define malloc_printf moz_malloc_printf
#endif
static void	malloc_printf(const char *format, ...);
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
static void	*pages_map(void *addr, size_t size, int pfd);
static void	pages_unmap(void *addr, size_t size);
static void	*chunk_alloc_mmap(size_t size, bool pagefile);
#ifdef MALLOC_PAGEFILE
static int	pagefile_init(size_t size);
static void	pagefile_close(int pfd);
#endif
static void	*chunk_alloc(size_t size, bool zero, bool pagefile);
static void	chunk_dealloc_mmap(void *chunk, size_t size);
static void	chunk_dealloc(void *chunk, size_t size);
#ifndef NO_TLS
static arena_t	*choose_arena_hard(void);
#endif
static void	arena_run_split(arena_t *arena, arena_run_t *run, size_t size,
    bool large, bool zero);
static void arena_chunk_init(arena_t *arena, arena_chunk_t *chunk);
static void	arena_chunk_dealloc(arena_t *arena, arena_chunk_t *chunk);
static arena_run_t *arena_run_alloc(arena_t *arena, arena_bin_t *bin,
    size_t size, bool large, bool zero);
static void	arena_purge(arena_t *arena);
static void	arena_run_dalloc(arena_t *arena, arena_run_t *run, bool dirty);
static void	arena_run_trim_head(arena_t *arena, arena_chunk_t *chunk,
    arena_run_t *run, size_t oldsize, size_t newsize);
static void	arena_run_trim_tail(arena_t *arena, arena_chunk_t *chunk,
    arena_run_t *run, size_t oldsize, size_t newsize, bool dirty);
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

void		_malloc_prefork(void);
void		_malloc_postfork(void);











#define	UMAX2S_BUFSIZE	21
static char *
umax2s(uintmax_t x, char *s)
{
	unsigned i;

	i = UMAX2S_BUFSIZE - 1;
	s[i] = '\0';
	do {
		i--;
		s[i] = "0123456789"[x % 10];
		x /= 10;
	} while (x > 0);

	return (&s[i]);
}

static void
wrtmessage(const char *p1, const char *p2, const char *p3, const char *p4)
{
#ifdef MOZ_MEMORY_WINCE
       wchar_t buf[1024];
#define WRT_PRINT(s) \
       MultiByteToWideChar(CP_ACP, 0, s, -1, buf, 1024); \
       OutputDebugStringW(buf)

       WRT_PRINT(p1);
       WRT_PRINT(p2);
       WRT_PRINT(p3);
       WRT_PRINT(p4);
#else
#if defined(MOZ_MEMORY) && !defined(MOZ_MEMORY_WINDOWS)
#define	_write	write
#endif
	_write(STDERR_FILENO, p1, (unsigned int) strlen(p1));
	_write(STDERR_FILENO, p2, (unsigned int) strlen(p2));
	_write(STDERR_FILENO, p3, (unsigned int) strlen(p3));
	_write(STDERR_FILENO, p4, (unsigned int) strlen(p4));
#endif

}

#define _malloc_message malloc_message

void	(*_malloc_message)(const char *p1, const char *p2, const char *p3,
	    const char *p4) = wrtmessage;

#ifdef MALLOC_DEBUG
#  define assert(e) do {						\
	if (!(e)) {							\
		char line_buf[UMAX2S_BUFSIZE];				\
		_malloc_message(__FILE__, ":", umax2s(__LINE__,		\
		    line_buf), ": Failed assertion: ");			\
		_malloc_message("\"", #e, "\"\n", "");			\
		abort();						\
	}								\
} while (0)
#else
#define assert(e)
#endif








static bool
malloc_mutex_init(malloc_mutex_t *mutex)
{
#if defined(MOZ_MEMORY_WINCE)
	InitializeCriticalSection(mutex);
#elif defined(MOZ_MEMORY_WINDOWS)
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
#if defined(MOZ_MEMORY_WINCE)
	InitializeCriticalSection(lock);
#elif defined(MOZ_MEMORY_WINDOWS)
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

#ifdef MALLOC_BALANCE


















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

#ifdef MALLOC_STATS



static void
malloc_printf(const char *format, ...)
{
#ifndef WINCE
	char buf[4096];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	_malloc_message(buf, "", "", "");
#endif
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

static bool
base_pages_alloc_mmap(size_t minsize)
{
	bool ret;
	size_t csize;
#ifdef MALLOC_DECOMMIT
	size_t pminsize;
#endif
	int pfd;

	assert(minsize != 0);
	csize = CHUNK_CEILING(minsize);
#ifdef MALLOC_PAGEFILE
	if (opt_pagefile) {
		pfd = pagefile_init(csize);
		if (pfd == -1)
			return (true);
	} else
#endif
		pfd = -1;
	base_pages = pages_map(NULL, csize, pfd);
	if (base_pages == NULL) {
		ret = true;
		goto RETURN;
	}
	base_next_addr = base_pages;
	base_past_addr = (void *)((uintptr_t)base_pages + csize);
#ifdef MALLOC_DECOMMIT
	



	pminsize = PAGE_CEILING(minsize);
	base_next_decommitted = (void *)((uintptr_t)base_pages + pminsize);
	if (pminsize < csize)
		pages_decommit(base_next_decommitted, csize - pminsize);
#endif
#ifdef MALLOC_STATS
	base_mapped += csize;
#  ifdef MALLOC_DECOMMIT
	base_committed += pminsize;
#  endif
#endif

	ret = false;
RETURN:
#ifdef MALLOC_PAGEFILE
	if (pfd != -1)
		pagefile_close(pfd);
#endif
	return (false);
}

static bool
base_pages_alloc(size_t minsize)
{

	if (base_pages_alloc_mmap(minsize) == false)
		return (false);

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
		if (base_pages_alloc(csize)) {
			malloc_mutex_unlock(&base_mtx);
			return (NULL);
		}
	}
	
	ret = base_next_addr;
	base_next_addr = (void *)((uintptr_t)base_next_addr + csize);
#ifdef MALLOC_DECOMMIT
	
	if ((uintptr_t)base_next_addr > (uintptr_t)base_next_decommitted) {
		void *pbase_next_addr =
		    (void *)(PAGE_CEILING((uintptr_t)base_next_addr));

		pages_commit(base_next_decommitted, (uintptr_t)pbase_next_addr -
		    (uintptr_t)base_next_decommitted);
		base_next_decommitted = pbase_next_addr;
#  ifdef MALLOC_STATS
		base_committed += (uintptr_t)pbase_next_addr -
		    (uintptr_t)base_next_decommitted;
#  endif
	}
#endif
	malloc_mutex_unlock(&base_mtx);
	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, false);

	return (ret);
}

static void *
base_calloc(size_t number, size_t size)
{
	void *ret;

	ret = base_alloc(number * size);
#ifdef MALLOC_VALGRIND
	if (ret != NULL) {
		VALGRIND_FREELIKE_BLOCK(ret, 0);
		VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, true);
	}
#endif
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
		VALGRIND_FREELIKE_BLOCK(ret, 0);
		VALGRIND_MALLOCLIKE_BLOCK(ret, sizeof(extent_node_t), 0, false);
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
	VALGRIND_FREELIKE_BLOCK(node, 0);
	VALGRIND_MALLOCLIKE_BLOCK(node, sizeof(extent_node_t *), 0, false);
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


rb_wrap(static, extent_tree_szad_, extent_tree_t, extent_node_t,
    link_szad, extent_szad_comp)

static inline int
extent_ad_comp(extent_node_t *a, extent_node_t *b)
{
	uintptr_t a_addr = (uintptr_t)a->addr;
	uintptr_t b_addr = (uintptr_t)b->addr;

	return ((a_addr > b_addr) - (a_addr < b_addr));
}


rb_wrap(static, extent_tree_ad_, extent_tree_t, extent_node_t, link_ad,
    extent_ad_comp)









#ifdef MOZ_MEMORY_WINDOWS
#ifdef MOZ_MEMORY_WINCE
#define ALIGN_ADDR2OFFSET(al, ad) \
	((uintptr_t)ad & (al - 1))
static void *
pages_map_align(size_t size, int pfd, size_t alignment)
{
	
	void *ret; 
	int offset;
	if (size % alignment)
		size += (alignment - (size % alignment));
	assert(size >= alignment);
	ret = pages_map(NULL, size, pfd);
	offset = ALIGN_ADDR2OFFSET(alignment, ret);
	if (offset) {  
		
		void *tmp;
		pages_unmap(ret, size);
		tmp = VirtualAlloc(NULL, size + alignment - offset, 
					 MEM_RESERVE, PAGE_NOACCESS);
		if (offset == ALIGN_ADDR2OFFSET(alignment, tmp))
			ret = VirtualAlloc((void*)((intptr_t)tmp + alignment 
						   - offset), size, MEM_COMMIT,
					   PAGE_READWRITE);
		else 
			VirtualFree(tmp, 0, MEM_RELEASE);
		offset = ALIGN_ADDR2OFFSET(alignment, ret);
		
	
		if (offset) {  
			
			ret = VirtualAlloc(NULL, size + alignment, MEM_RESERVE, 
					   PAGE_NOACCESS);
			offset = ALIGN_ADDR2OFFSET(alignment, ret);
			ret = VirtualAlloc((void*)((intptr_t)ret + 
						   alignment - offset),
					   size, MEM_COMMIT, PAGE_READWRITE);
		}
	}
	return (ret);
}
#endif

static void *
pages_map(void *addr, size_t size, int pfd)
{
	void *ret = NULL;
#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
	void *va_ret;
	assert(addr == NULL);
	va_ret = VirtualAlloc(addr, size, MEM_RESERVE, PAGE_NOACCESS);
	if (va_ret)
		ret = VirtualAlloc(va_ret, size, MEM_COMMIT, PAGE_READWRITE);
	assert(va_ret == ret);
#else
	ret = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE,
	    PAGE_READWRITE);
#endif
	return (ret);
}

static void
pages_unmap(void *addr, size_t size)
{
	if (VirtualFree(addr, 0, MEM_RELEASE) == 0) {
#if defined(MOZ_MEMORY_WINCE) && !defined(MOZ_MEMORY_WINCE6)
		if (GetLastError() == ERROR_INVALID_PARAMETER) {
			MEMORY_BASIC_INFORMATION info;
			VirtualQuery(addr, &info, sizeof(info));
			if (VirtualFree(info.AllocationBase, 0, MEM_RELEASE))
				return;
		}
#endif
		_malloc_message(_getprogname(),
		    ": (malloc) Error in VirtualFree()\n", "", "");
		if (opt_abort)
			abort();
	}
}
#elif (defined(MOZ_MEMORY_DARWIN))
static void *
pages_map(void *addr, size_t size, int pfd)
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
#ifdef JEMALLOC_USES_MAP_ALIGN
static void *
pages_map_align(size_t size, int pfd, size_t alignment)
{
	void *ret;

	



#ifdef MALLOC_PAGEFILE
	if (pfd != -1) {
		ret = mmap((void *)alignment, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_NOSYNC | MAP_ALIGN, pfd, 0);
	} else
#endif
	       {
		ret = mmap((void *)alignment, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_NOSYNC | MAP_ALIGN | MAP_ANON, -1, 0);
	}
	assert(ret != NULL);

	if (ret == MAP_FAILED)
		ret = NULL;
	return (ret);
}
#endif

static void *
pages_map(void *addr, size_t size, int pfd)
{
	void *ret;

	



#ifdef MALLOC_PAGEFILE
	if (pfd != -1) {
		ret = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_NOSYNC, pfd, 0);
	} else
#endif
	       {
		ret = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
		    MAP_ANON, -1, 0);
	}
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

#ifdef MALLOC_VALIDATE
static inline malloc_rtree_t *
malloc_rtree_new(unsigned bits)
{
	malloc_rtree_t *ret;
	unsigned bits_per_level, height, i;

	bits_per_level = ffs(pow2_ceil((MALLOC_RTREE_NODESIZE /
	    sizeof(void *)))) - 1;
	height = bits / bits_per_level;
	if (height * bits_per_level != bits)
		height++;
	assert(height * bits_per_level >= bits);

	ret = (malloc_rtree_t*)base_calloc(1, sizeof(malloc_rtree_t) + (sizeof(unsigned) *
	    (height - 1)));
	if (ret == NULL)
		return (NULL);

	malloc_spin_init(&ret->lock);
	ret->height = height;
	if (bits_per_level * height > bits)
		ret->level2bits[0] = bits % bits_per_level;
	else
		ret->level2bits[0] = bits_per_level;
	for (i = 1; i < height; i++)
		ret->level2bits[i] = bits_per_level;

	ret->root = (void**)base_calloc(1, sizeof(void *) << ret->level2bits[0]);
	if (ret->root == NULL) {
		



		return (NULL);
	}

	return (ret);
}


static inline void *
malloc_rtree_get(malloc_rtree_t *rtree, uintptr_t key)
{
	void *ret;
	uintptr_t subkey;
	unsigned i, lshift, height, bits;
	void **node, **child;

	malloc_spin_lock(&rtree->lock);
	for (i = lshift = 0, height = rtree->height, node = rtree->root;
	    i < height - 1;
	    i++, lshift += bits, node = child) {
		bits = rtree->level2bits[i];
		subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
		child = (void**)node[subkey];
		if (child == NULL) {
			malloc_spin_unlock(&rtree->lock);
			return (NULL);
		}
	}

	
	bits = rtree->level2bits[i];
	subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
	ret = node[subkey];
	malloc_spin_unlock(&rtree->lock);

	return (ret);
}

static inline bool
malloc_rtree_set(malloc_rtree_t *rtree, uintptr_t key, void *val)
{
	uintptr_t subkey;
	unsigned i, lshift, height, bits;
	void **node, **child;

	malloc_spin_lock(&rtree->lock);
	for (i = lshift = 0, height = rtree->height, node = rtree->root;
	    i < height - 1;
	    i++, lshift += bits, node = child) {
		bits = rtree->level2bits[i];
		subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
		child = (void**)node[subkey];
		if (child == NULL) {
			child = (void**)base_calloc(1, sizeof(void *) <<
			    rtree->level2bits[i+1]);
			if (child == NULL) {
				malloc_spin_unlock(&rtree->lock);
				return (true);
			}
			node[subkey] = child;
		}
	}

	
	bits = rtree->level2bits[i];
	subkey = (key << lshift) >> ((SIZEOF_PTR << 3) - bits);
	node[subkey] = val;
	malloc_spin_unlock(&rtree->lock);

	return (false);
}
#endif

static void *
chunk_alloc_mmap(size_t size, bool pagefile)
{
	void *ret;
#ifndef JEMALLOC_USES_MAP_ALIGN
	size_t offset;
#endif
	int pfd;

#ifdef MALLOC_PAGEFILE
	if (opt_pagefile && pagefile) {
		pfd = pagefile_init(size);
		if (pfd == -1)
			return (NULL);
	} else
#endif
		pfd = -1;

	









#ifdef JEMALLOC_USES_MAP_ALIGN
	ret = pages_map_align(size, pfd, chunksize);
#else
	ret = pages_map(NULL, size, pfd);
	if (ret == NULL)
		goto RETURN;

	offset = CHUNK_ADDR2OFFSET(ret);
	if (offset != 0) {
		
		pages_unmap(ret, size);
		ret = pages_map((void *)((uintptr_t)ret + size - offset), size,
		    pfd);
		while (ret == NULL) {
			



			ret = pages_map(NULL, size + chunksize, -1);
			if (ret == NULL)
				goto RETURN;
			



			offset = CHUNK_ADDR2OFFSET(ret);
			pages_unmap(ret, size + chunksize);
			if (offset == 0)
				ret = pages_map(ret, size, pfd);
			else {
				ret = pages_map((void *)((uintptr_t)ret +
				    chunksize - offset), size, pfd);
			}
			



		}
	}
RETURN:
#endif
#ifdef MALLOC_PAGEFILE
	if (pfd != -1)
		pagefile_close(pfd);
#endif
#ifdef MALLOC_STATS
	if (ret != NULL)
		stats_chunks.nchunks += (size / chunksize);
#endif
	return (ret);
}

#ifdef MALLOC_PAGEFILE
static int
pagefile_init(size_t size)
{
	int ret;
	size_t i;
	char pagefile_path[PATH_MAX];
	char zbuf[MALLOC_PAGEFILE_WRITE_SIZE];

	



	strcpy(pagefile_path, pagefile_templ);
	ret = mkstemp(pagefile_path);
	if (ret == -1)
		return (ret);
	if (unlink(pagefile_path)) {
		char buf[STRERROR_BUF];

		strerror_r(errno, buf, sizeof(buf));
		_malloc_message(_getprogname(), ": (malloc) Error in unlink(\"",
		    pagefile_path, "\"):");
		_malloc_message(buf, "\n", "", "");
		if (opt_abort)
			abort();
	}

	





	memset(zbuf, 0, sizeof(zbuf));
	for (i = 0; i < size; i += sizeof(zbuf)) {
		if (write(ret, zbuf, sizeof(zbuf)) != sizeof(zbuf)) {
			if (errno != ENOSPC) {
				char buf[STRERROR_BUF];

				strerror_r(errno, buf, sizeof(buf));
				_malloc_message(_getprogname(),
				    ": (malloc) Error in write(): ", buf, "\n");
				if (opt_abort)
					abort();
			}
			pagefile_close(ret);
			return (-1);
		}
	}

	return (ret);
}

static void
pagefile_close(int pfd)
{

	if (close(pfd)) {
		char buf[STRERROR_BUF];

		strerror_r(errno, buf, sizeof(buf));
		_malloc_message(_getprogname(),
		    ": (malloc) Error in close(): ", buf, "\n");
		if (opt_abort)
			abort();
	}
}
#endif

static void *
chunk_alloc(size_t size, bool zero, bool pagefile)
{
	void *ret;

	assert(size != 0);
	assert((size & chunksize_mask) == 0);

	ret = chunk_alloc_mmap(size, pagefile);
	if (ret != NULL) {
		goto RETURN;
	}

	
	ret = NULL;
RETURN:
#ifdef MALLOC_STATS
	if (ret != NULL)
		stats_chunks.curchunks += (size / chunksize);
	if (stats_chunks.curchunks > stats_chunks.highchunks)
		stats_chunks.highchunks = stats_chunks.curchunks;
#endif

#ifdef MALLOC_VALIDATE
	if (ret != NULL) {
		if (malloc_rtree_set(chunk_rtree, (uintptr_t)ret, ret)) {
			chunk_dealloc(ret, size);
			return (NULL);
		}
	}
#endif

	assert(CHUNK_ADDR2BASE(ret) == ret);
	return (ret);
}

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
#ifdef MALLOC_VALIDATE
	malloc_rtree_set(chunk_rtree, (uintptr_t)chunk, NULL);
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
	ret = (arena_t*)TlsGetValue(tlsIndex);
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


rb_wrap(static, arena_chunk_tree_dirty_, arena_chunk_tree_t,
    arena_chunk_t, link_dirty, arena_chunk_comp)

static inline int
arena_run_comp(arena_chunk_map_t *a, arena_chunk_map_t *b)
{
	uintptr_t a_mapelm = (uintptr_t)a;
	uintptr_t b_mapelm = (uintptr_t)b;

	assert(a != NULL);
	assert(b != NULL);

	return ((a_mapelm > b_mapelm) - (a_mapelm < b_mapelm));
}


rb_wrap(static, arena_run_tree_, arena_run_tree_t, arena_chunk_map_t, link,
    arena_run_comp)

static inline int
arena_avail_comp(arena_chunk_map_t *a, arena_chunk_map_t *b)
{
	int ret;
	size_t a_size = a->bits & ~pagesize_mask;
	size_t b_size = b->bits & ~pagesize_mask;

	ret = (a_size > b_size) - (a_size < b_size);
	if (ret == 0) {
		uintptr_t a_mapelm, b_mapelm;

		if ((a->bits & CHUNK_MAP_KEY) == 0)
			a_mapelm = (uintptr_t)a;
		else {
			



			a_mapelm = 0;
		}
		b_mapelm = (uintptr_t)b;

		ret = (a_mapelm > b_mapelm) - (a_mapelm < b_mapelm);
	}

	return (ret);
}


rb_wrap(static, arena_avail_tree_, arena_avail_tree_t, arena_chunk_map_t, link,
    arena_avail_comp)

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
arena_run_split(arena_t *arena, arena_run_t *run, size_t size, bool large,
    bool zero)
{
	arena_chunk_t *chunk;
	size_t old_ndirty, run_ind, total_pages, need_pages, rem_pages, i;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(run);
	old_ndirty = chunk->ndirty;
	run_ind = (unsigned)(((uintptr_t)run - (uintptr_t)chunk)
	    >> pagesize_2pow);
	total_pages = (chunk->map[run_ind].bits & ~pagesize_mask) >>
	    pagesize_2pow;
	need_pages = (size >> pagesize_2pow);
	assert(need_pages > 0);
	assert(need_pages <= total_pages);
	rem_pages = total_pages - need_pages;

	arena_avail_tree_remove(&arena->runs_avail, &chunk->map[run_ind]);

	
	if (rem_pages > 0) {
		chunk->map[run_ind+need_pages].bits = (rem_pages <<
		    pagesize_2pow) | (chunk->map[run_ind+need_pages].bits &
		    pagesize_mask);
		chunk->map[run_ind+total_pages-1].bits = (rem_pages <<
		    pagesize_2pow) | (chunk->map[run_ind+total_pages-1].bits &
		    pagesize_mask);
		arena_avail_tree_insert(&arena->runs_avail,
		    &chunk->map[run_ind+need_pages]);
	}

	for (i = 0; i < need_pages; i++) {
#ifdef MALLOC_DECOMMIT
		





		if (chunk->map[run_ind + i].bits & CHUNK_MAP_DECOMMITTED) {
			size_t j;

			




			for (j = 0; i + j < need_pages && (chunk->map[run_ind +
			    i + j].bits & CHUNK_MAP_DECOMMITTED); j++) {
				chunk->map[run_ind + i + j].bits ^=
				    CHUNK_MAP_DECOMMITTED;
			}

			pages_commit((void *)((uintptr_t)chunk + ((run_ind + i)
			    << pagesize_2pow)), (j << pagesize_2pow));
#  ifdef MALLOC_STATS
			arena->stats.ncommit++;
			arena->stats.committed += j;
#  endif
		} else 
#endif

		
		if (zero) {
			if ((chunk->map[run_ind + i].bits & CHUNK_MAP_ZEROED)
			    == 0) {
				VALGRIND_MALLOCLIKE_BLOCK((void *)((uintptr_t)
				    chunk + ((run_ind + i) << pagesize_2pow)),
				    pagesize, 0, false);
				memset((void *)((uintptr_t)chunk + ((run_ind
				    + i) << pagesize_2pow)), 0, pagesize);
				VALGRIND_FREELIKE_BLOCK((void *)((uintptr_t)
				    chunk + ((run_ind + i) << pagesize_2pow)),
				    0);
				
			}
		}

		
		if (chunk->map[run_ind + i].bits & CHUNK_MAP_DIRTY) {
			chunk->ndirty--;
			arena->ndirty--;
			
		}

		
		if (large) {
			chunk->map[run_ind + i].bits = CHUNK_MAP_LARGE
			    | CHUNK_MAP_ALLOCATED;
		} else {
			chunk->map[run_ind + i].bits = (size_t)run
			    | CHUNK_MAP_ALLOCATED;
		}
	}

	





	if (large)
		chunk->map[run_ind].bits |= size;

	if (chunk->ndirty == 0 && old_ndirty > 0)
		arena_chunk_tree_dirty_remove(&arena->chunks_dirty, chunk);
}

static void
arena_chunk_init(arena_t *arena, arena_chunk_t *chunk)
{
	arena_run_t *run;
	size_t i;

	VALGRIND_MALLOCLIKE_BLOCK(chunk, (arena_chunk_header_npages <<
	    pagesize_2pow), 0, false);
#ifdef MALLOC_STATS
	arena->stats.mapped += chunksize;
#endif

	chunk->arena = arena;

	


	chunk->ndirty = 0;

	
	run = (arena_run_t *)((uintptr_t)chunk + (arena_chunk_header_npages <<
	    pagesize_2pow));
	for (i = 0; i < arena_chunk_header_npages; i++)
		chunk->map[i].bits = 0;
	chunk->map[i].bits = arena_maxclass
#ifdef MALLOC_DECOMMIT
	    | CHUNK_MAP_DECOMMITTED
#endif
	    | CHUNK_MAP_ZEROED;
	for (i++; i < chunk_npages-1; i++) {
		chunk->map[i].bits =
#ifdef MALLOC_DECOMMIT
		    CHUNK_MAP_DECOMMITTED |
#endif
		    CHUNK_MAP_ZEROED;
	}
	chunk->map[chunk_npages-1].bits = arena_maxclass
#ifdef MALLOC_DECOMMIT
	    | CHUNK_MAP_DECOMMITTED
#endif
	    | CHUNK_MAP_ZEROED;

#ifdef MALLOC_DECOMMIT
	



	pages_decommit(run, arena_maxclass);
#  ifdef MALLOC_STATS
	arena->stats.ndecommit++;
	arena->stats.decommitted += (chunk_npages - arena_chunk_header_npages);
	arena->stats.committed += arena_chunk_header_npages;
#  endif
#endif

	
	arena_avail_tree_insert(&arena->runs_avail,
	    &chunk->map[arena_chunk_header_npages]);
}

static void
arena_chunk_dealloc(arena_t *arena, arena_chunk_t *chunk)
{

	if (arena->spare != NULL) {
		if (arena->spare->ndirty > 0) {
			arena_chunk_tree_dirty_remove(
			    &chunk->arena->chunks_dirty, arena->spare);
			arena->ndirty -= arena->spare->ndirty;
#if (defined(MALLOC_STATS) && defined(MALLOC_DECOMMIT))
			arena->stats.committed -= arena->spare->ndirty;
#endif
		}
		VALGRIND_FREELIKE_BLOCK(arena->spare, 0);
		chunk_dealloc((void *)arena->spare, chunksize);
#ifdef MALLOC_STATS
		arena->stats.mapped -= chunksize;
#  ifdef MALLOC_DECOMMIT
		arena->stats.committed -= arena_chunk_header_npages;
#  endif
#endif
	}

	




	arena_avail_tree_remove(&arena->runs_avail,
	    &chunk->map[arena_chunk_header_npages]);

	arena->spare = chunk;
}

static arena_run_t *
arena_run_alloc(arena_t *arena, arena_bin_t *bin, size_t size, bool large,
    bool zero)
{
	arena_chunk_t *chunk;
	arena_run_t *run;
	arena_chunk_map_t *mapelm, key;

	assert(size <= arena_maxclass);
	assert((size & pagesize_mask) == 0);

	chunk = NULL;
	while (true) {
		
		key.bits = size | CHUNK_MAP_KEY;
		mapelm = arena_avail_tree_nsearch(&arena->runs_avail, &key);
		if (mapelm != NULL) {
			arena_chunk_t *run_chunk =
			    (arena_chunk_t*)CHUNK_ADDR2BASE(mapelm);
			size_t pageind = ((uintptr_t)mapelm -
			    (uintptr_t)run_chunk->map) /
			    sizeof(arena_chunk_map_t);

			if (chunk != NULL)
				chunk_dealloc(chunk, chunksize);
			run = (arena_run_t *)((uintptr_t)run_chunk + (pageind
			    << pagesize_2pow));
			arena_run_split(arena, run, size, large, zero);
			return (run);
		}

		if (arena->spare != NULL) {
			
			chunk = arena->spare;
			arena->spare = NULL;
			run = (arena_run_t *)((uintptr_t)chunk +
			    (arena_chunk_header_npages << pagesize_2pow));
			
			arena_avail_tree_insert(&arena->runs_avail,
			    &chunk->map[arena_chunk_header_npages]);
			arena_run_split(arena, run, size, large, zero);
			return (run);
		}

		



		if (chunk == NULL) {
			chunk = (arena_chunk_t *)chunk_alloc(chunksize, true,
			    true);
			if (chunk == NULL)
				return (NULL);
		}

		arena_chunk_init(arena, chunk);
		run = (arena_run_t *)((uintptr_t)chunk +
		    (arena_chunk_header_npages << pagesize_2pow));
		
		arena_run_split(arena, run, size, large, zero);
		return (run);
	}
}

static void
arena_purge(arena_t *arena)
{
	arena_chunk_t *chunk;
	size_t i, npages;
#ifdef MALLOC_DEBUG
	size_t ndirty = 0;
	rb_foreach_begin(arena_chunk_t, link_dirty, &arena->chunks_dirty,
	    chunk) {
		ndirty += chunk->ndirty;
	} rb_foreach_end(arena_chunk_t, link_dirty, &arena->chunks_dirty, chunk)
	assert(ndirty == arena->ndirty);
#endif
	assert(arena->ndirty > opt_dirty_max);

#ifdef MALLOC_STATS
	arena->stats.npurge++;
#endif

	





	while (arena->ndirty > (opt_dirty_max >> 1)) {
		chunk = arena_chunk_tree_dirty_last(&arena->chunks_dirty);
		assert(chunk != NULL);

		for (i = chunk_npages - 1; chunk->ndirty > 0; i--) {
			assert(i >= arena_chunk_header_npages);

			if (chunk->map[i].bits & CHUNK_MAP_DIRTY) {
#ifdef MALLOC_DECOMMIT
				assert((chunk->map[i].bits &
				    CHUNK_MAP_DECOMMITTED) == 0);
#endif
				chunk->map[i].bits ^=
#ifdef MALLOC_DECOMMIT
				    CHUNK_MAP_DECOMMITTED |
#endif
				    CHUNK_MAP_DIRTY;
				
				for (npages = 1; i > arena_chunk_header_npages
				    && (chunk->map[i - 1].bits &
				    CHUNK_MAP_DIRTY); npages++) {
					i--;
#ifdef MALLOC_DECOMMIT
					assert((chunk->map[i].bits &
					    CHUNK_MAP_DECOMMITTED) == 0);
#endif
					chunk->map[i].bits ^=
#ifdef MALLOC_DECOMMIT
					    CHUNK_MAP_DECOMMITTED |
#endif
					    CHUNK_MAP_DIRTY;
				}
				chunk->ndirty -= npages;
				arena->ndirty -= npages;

#ifdef MALLOC_DECOMMIT
				pages_decommit((void *)((uintptr_t)
				    chunk + (i << pagesize_2pow)),
				    (npages << pagesize_2pow));
#  ifdef MALLOC_STATS
				arena->stats.ndecommit++;
				arena->stats.decommitted += npages;
				arena->stats.committed -= npages;
#  endif
#else
				madvise((void *)((uintptr_t)chunk + (i <<
				    pagesize_2pow)), (npages << pagesize_2pow),
				    MADV_FREE);
#endif
#ifdef MALLOC_STATS
				arena->stats.nmadvise++;
				arena->stats.purged += npages;
#endif
				if (arena->ndirty <= (opt_dirty_max >> 1))
					break;
			}
		}

		if (chunk->ndirty == 0) {
			arena_chunk_tree_dirty_remove(&arena->chunks_dirty,
			    chunk);
		}
	}
}

static void
arena_run_dalloc(arena_t *arena, arena_run_t *run, bool dirty)
{
	arena_chunk_t *chunk;
	size_t size, run_ind, run_pages;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(run);
	run_ind = (size_t)(((uintptr_t)run - (uintptr_t)chunk)
	    >> pagesize_2pow);
	assert(run_ind >= arena_chunk_header_npages);
	assert(run_ind < chunk_npages);
	if ((chunk->map[run_ind].bits & CHUNK_MAP_LARGE) != 0)
		size = chunk->map[run_ind].bits & ~pagesize_mask;
	else
		size = run->bin->run_size;
	run_pages = (size >> pagesize_2pow);

	
	if (dirty) {
		size_t i;

		for (i = 0; i < run_pages; i++) {
			assert((chunk->map[run_ind + i].bits & CHUNK_MAP_DIRTY)
			    == 0);
			chunk->map[run_ind + i].bits = CHUNK_MAP_DIRTY;
		}

		if (chunk->ndirty == 0) {
			arena_chunk_tree_dirty_insert(&arena->chunks_dirty,
			    chunk);
		}
		chunk->ndirty += run_pages;
		arena->ndirty += run_pages;
	} else {
		size_t i;

		for (i = 0; i < run_pages; i++) {
			chunk->map[run_ind + i].bits &= ~(CHUNK_MAP_LARGE |
			    CHUNK_MAP_ALLOCATED);
		}
	}
	chunk->map[run_ind].bits = size | (chunk->map[run_ind].bits &
	    pagesize_mask);
	chunk->map[run_ind+run_pages-1].bits = size |
	    (chunk->map[run_ind+run_pages-1].bits & pagesize_mask);

	
	if (run_ind + run_pages < chunk_npages &&
	    (chunk->map[run_ind+run_pages].bits & CHUNK_MAP_ALLOCATED) == 0) {
		size_t nrun_size = chunk->map[run_ind+run_pages].bits &
		    ~pagesize_mask;

		



		arena_avail_tree_remove(&arena->runs_avail,
		    &chunk->map[run_ind+run_pages]);

		size += nrun_size;
		run_pages = size >> pagesize_2pow;

		assert((chunk->map[run_ind+run_pages-1].bits & ~pagesize_mask)
		    == nrun_size);
		chunk->map[run_ind].bits = size | (chunk->map[run_ind].bits &
		    pagesize_mask);
		chunk->map[run_ind+run_pages-1].bits = size |
		    (chunk->map[run_ind+run_pages-1].bits & pagesize_mask);
	}

	
	if (run_ind > arena_chunk_header_npages && (chunk->map[run_ind-1].bits &
	    CHUNK_MAP_ALLOCATED) == 0) {
		size_t prun_size = chunk->map[run_ind-1].bits & ~pagesize_mask;

		run_ind -= prun_size >> pagesize_2pow;

		



		arena_avail_tree_remove(&arena->runs_avail,
		    &chunk->map[run_ind]);

		size += prun_size;
		run_pages = size >> pagesize_2pow;

		assert((chunk->map[run_ind].bits & ~pagesize_mask) ==
		    prun_size);
		chunk->map[run_ind].bits = size | (chunk->map[run_ind].bits &
		    pagesize_mask);
		chunk->map[run_ind+run_pages-1].bits = size |
		    (chunk->map[run_ind+run_pages-1].bits & pagesize_mask);
	}

	
	arena_avail_tree_insert(&arena->runs_avail, &chunk->map[run_ind]);

	
	if ((chunk->map[arena_chunk_header_npages].bits & (~pagesize_mask |
	    CHUNK_MAP_ALLOCATED)) == arena_maxclass)
		arena_chunk_dealloc(arena, chunk);

	
	if (arena->ndirty > opt_dirty_max)
		arena_purge(arena);
}

static void
arena_run_trim_head(arena_t *arena, arena_chunk_t *chunk, arena_run_t *run,
    size_t oldsize, size_t newsize)
{
	size_t pageind = ((uintptr_t)run - (uintptr_t)chunk) >> pagesize_2pow;
	size_t head_npages = (oldsize - newsize) >> pagesize_2pow;

	assert(oldsize > newsize);

	



	chunk->map[pageind].bits = (oldsize - newsize) | CHUNK_MAP_LARGE |
	    CHUNK_MAP_ALLOCATED;
	chunk->map[pageind+head_npages].bits = newsize | CHUNK_MAP_LARGE |
	    CHUNK_MAP_ALLOCATED;

	arena_run_dalloc(arena, run, false);
}

static void
arena_run_trim_tail(arena_t *arena, arena_chunk_t *chunk, arena_run_t *run,
    size_t oldsize, size_t newsize, bool dirty)
{
	size_t pageind = ((uintptr_t)run - (uintptr_t)chunk) >> pagesize_2pow;
	size_t npages = newsize >> pagesize_2pow;

	assert(oldsize > newsize);

	



	chunk->map[pageind].bits = newsize | CHUNK_MAP_LARGE |
	    CHUNK_MAP_ALLOCATED;
	chunk->map[pageind+npages].bits = (oldsize - newsize) | CHUNK_MAP_LARGE
	    | CHUNK_MAP_ALLOCATED;

	arena_run_dalloc(arena, (arena_run_t *)((uintptr_t)run + newsize),
	    dirty);
}

static arena_run_t *
arena_bin_nonfull_run_get(arena_t *arena, arena_bin_t *bin)
{
	arena_chunk_map_t *mapelm;
	arena_run_t *run;
	unsigned i, remainder;

	
	mapelm = arena_run_tree_first(&bin->runs);
	if (mapelm != NULL) {
		
		arena_run_tree_remove(&bin->runs, mapelm);
		run = (arena_run_t *)(mapelm->bits & ~pagesize_mask);
#ifdef MALLOC_STATS
		bin->stats.reruns++;
#endif
		return (run);
	}
	

	
	run = arena_run_alloc(arena, bin, bin->run_size, false, false);
	if (run == NULL)
		return (NULL);
	



	if (run == bin->runcur)
		return (run);

	VALGRIND_MALLOCLIKE_BLOCK(run, sizeof(arena_run_t) + (sizeof(unsigned) *
	    (bin->regs_mask_nelms - 1)), 0, false);

	
	run->bin = bin;

	for (i = 0; i < bin->regs_mask_nelms - 1; i++)
		run->regs_mask[i] = UINT_MAX;
	remainder = bin->nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1);
	if (remainder == 0)
		run->regs_mask[i] = UINT_MAX;
	else {
		
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

	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, zero);
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
	ret = (void *)arena_run_alloc(arena, NULL, size, true, zero);
	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}
#ifdef MALLOC_STATS
	arena->stats.nmalloc_large++;
	arena->stats.allocated_large += size;
#endif
	malloc_spin_unlock(&arena->lock);

	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, zero);
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

	assert((size & pagesize_mask) == 0);
	assert((alignment & pagesize_mask) == 0);

#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	ret = (void *)arena_run_alloc(arena, NULL, alloc_size, true, false);
	if (ret == NULL) {
		malloc_spin_unlock(&arena->lock);
		return (NULL);
	}

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ret);

	offset = (uintptr_t)ret & (alignment - 1);
	assert((offset & pagesize_mask) == 0);
	assert(offset < alloc_size);
	if (offset == 0)
		arena_run_trim_tail(arena, chunk, (arena_run_t*)ret, alloc_size, size, false);
	else {
		size_t leadsize, trailsize;

		leadsize = alignment - offset;
		if (leadsize > 0) {
			arena_run_trim_head(arena, chunk, (arena_run_t*)ret, alloc_size,
			    alloc_size - leadsize);
			ret = (void *)((uintptr_t)ret + leadsize);
		}

		trailsize = alloc_size - leadsize - size;
		if (trailsize != 0) {
			
			assert(trailsize < alloc_size);
			arena_run_trim_tail(arena, chunk, (arena_run_t*)ret, size + trailsize,
			    size, false);
		}
	}

#ifdef MALLOC_STATS
	arena->stats.nmalloc_large++;
	arena->stats.allocated_large += size;
#endif
	malloc_spin_unlock(&arena->lock);

	VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, false);
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
	size_t pageind, mapbits;

	assert(ptr != NULL);
	assert(CHUNK_ADDR2BASE(ptr) != ptr);

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	pageind = (((uintptr_t)ptr - (uintptr_t)chunk) >> pagesize_2pow);
	mapbits = chunk->map[pageind].bits;
	assert((mapbits & CHUNK_MAP_ALLOCATED) != 0);
	if ((mapbits & CHUNK_MAP_LARGE) == 0) {
		arena_run_t *run = (arena_run_t *)(mapbits & ~pagesize_mask);
		assert(run->magic == ARENA_RUN_MAGIC);
		ret = run->bin->reg_size;
	} else {
		ret = mapbits & ~pagesize_mask;
		assert(ret != 0);
	}

	return (ret);
}

#if (defined(MALLOC_VALIDATE) || defined(MOZ_MEMORY_DARWIN))








static inline size_t
isalloc_validate(const void *ptr)
{
	arena_chunk_t *chunk;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (chunk == NULL)
		return (0);

	if (malloc_rtree_get(chunk_rtree, (uintptr_t)chunk) == NULL)
		return (0);

	if (chunk != ptr) {
		assert(chunk->arena->magic == ARENA_MAGIC);
		return (arena_salloc(ptr));
	} else {
		size_t ret;
		extent_node_t *node;
		extent_node_t key;

		
		key.addr = (void *)chunk;
		malloc_mutex_lock(&huge_mtx);
		node = extent_tree_ad_search(&huge, &key);
		if (node != NULL)
			ret = node->size;
		else
			ret = 0;
		malloc_mutex_unlock(&huge_mtx);
		return (ret);
	}
}
#endif

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
		node = extent_tree_ad_search(&huge, &key);
		assert(node != NULL);

		ret = node->size;

		malloc_mutex_unlock(&huge_mtx);
	}

	return (ret);
}

static inline void
arena_dalloc_small(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    arena_chunk_map_t *mapelm)
{
	arena_run_t *run;
	arena_bin_t *bin;
	size_t size;

	run = (arena_run_t *)(mapelm->bits & ~pagesize_mask);
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
			size_t run_pageind = (((uintptr_t)run -
			    (uintptr_t)chunk)) >> pagesize_2pow;
			arena_chunk_map_t *run_mapelm =
			    &chunk->map[run_pageind];
			




			assert(arena_run_tree_search(&bin->runs, run_mapelm) ==
			    run_mapelm);
			arena_run_tree_remove(&bin->runs, run_mapelm);
		}
#ifdef MALLOC_DEBUG
		run->magic = 0;
#endif
		VALGRIND_FREELIKE_BLOCK(run, 0);
		arena_run_dalloc(arena, run, true);
#ifdef MALLOC_STATS
		bin->stats.curruns--;
#endif
	} else if (run->nfree == 1 && run != bin->runcur) {
		



		if (bin->runcur == NULL)
			bin->runcur = run;
		else if ((uintptr_t)run < (uintptr_t)bin->runcur) {
			
			if (bin->runcur->nfree > 0) {
				arena_chunk_t *runcur_chunk =
				    (arena_chunk_t*)CHUNK_ADDR2BASE(bin->runcur);
				size_t runcur_pageind =
				    (((uintptr_t)bin->runcur -
				    (uintptr_t)runcur_chunk)) >> pagesize_2pow;
				arena_chunk_map_t *runcur_mapelm =
				    &runcur_chunk->map[runcur_pageind];

				
				assert(arena_run_tree_search(&bin->runs,
				    runcur_mapelm) == NULL);
				arena_run_tree_insert(&bin->runs,
				    runcur_mapelm);
			}
			bin->runcur = run;
		} else {
			size_t run_pageind = (((uintptr_t)run -
			    (uintptr_t)chunk)) >> pagesize_2pow;
			arena_chunk_map_t *run_mapelm =
			    &chunk->map[run_pageind];

			assert(arena_run_tree_search(&bin->runs, run_mapelm) ==
			    NULL);
			arena_run_tree_insert(&bin->runs, run_mapelm);
		}
	}
#ifdef MALLOC_STATS
	arena->stats.allocated_small -= size;
	arena->stats.ndalloc_small++;
#endif
}

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
		size_t pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >>
		    pagesize_2pow;
		size_t size = chunk->map[pageind].bits & ~pagesize_mask;

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
	assert((mapelm->bits & CHUNK_MAP_ALLOCATED) != 0);
	if ((mapelm->bits & CHUNK_MAP_LARGE) == 0) {
		
		malloc_spin_lock(&arena->lock);
		arena_dalloc_small(arena, chunk, ptr, mapelm);
		malloc_spin_unlock(&arena->lock);
	} else
		arena_dalloc_large(arena, chunk, ptr);
	VALGRIND_FREELIKE_BLOCK(ptr, 0);
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

	assert(size < oldsize);

	



#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	arena_run_trim_tail(arena, chunk, (arena_run_t *)ptr, oldsize, size,
	    true);
#ifdef MALLOC_STATS
	arena->stats.allocated_large -= oldsize - size;
#endif
	malloc_spin_unlock(&arena->lock);
}

static bool
arena_ralloc_large_grow(arena_t *arena, arena_chunk_t *chunk, void *ptr,
    size_t size, size_t oldsize)
{
	size_t pageind = ((uintptr_t)ptr - (uintptr_t)chunk) >> pagesize_2pow;
	size_t npages = oldsize >> pagesize_2pow;

	assert(oldsize == (chunk->map[pageind].bits & ~pagesize_mask));

	
	assert(size > oldsize);
#ifdef MALLOC_BALANCE
	arena_lock_balance(arena);
#else
	malloc_spin_lock(&arena->lock);
#endif
	if (pageind + npages < chunk_npages && (chunk->map[pageind+npages].bits
	    & CHUNK_MAP_ALLOCATED) == 0 && (chunk->map[pageind+npages].bits &
	    ~pagesize_mask) >= size - oldsize) {
		




		arena_run_split(arena, (arena_run_t *)((uintptr_t)chunk +
		    ((pageind+npages) << pagesize_2pow)), size - oldsize, true,
		    false);

		chunk->map[pageind].bits = size | CHUNK_MAP_LARGE |
		    CHUNK_MAP_ALLOCATED;
		chunk->map[pageind+npages].bits = CHUNK_MAP_LARGE |
		    CHUNK_MAP_ALLOCATED;

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

#ifndef MALLOC_VALGRIND
	if (size <= arena_maxclass)
		return (arena_ralloc(ptr, size, oldsize));
	else
		return (huge_ralloc(ptr, size, oldsize));
#else
	



	{
		void *ret = imalloc(size);
		if (ret != NULL) {
			if (oldsize < size)
			    memcpy(ret, ptr, oldsize);
			else
			    memcpy(ret, ptr, size);
			idalloc(ptr);
		}
		return (ret);
	}
#endif
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

	
	arena_chunk_tree_dirty_new(&arena->chunks_dirty);
	arena->spare = NULL;

	arena->ndirty = 0;

	arena_avail_tree_new(&arena->runs_avail);

#ifdef MALLOC_BALANCE
	arena->contention = 0;
#endif

	
	prev_run_size = pagesize;

	
	for (i = 0; i < ntbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		arena_run_tree_new(&bin->runs);

		bin->reg_size = (1U << (TINY_MIN_2POW + i));

		prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);

#ifdef MALLOC_STATS
		memset(&bin->stats, 0, sizeof(malloc_bin_stats_t));
#endif
	}

	
	for (; i < ntbins + nqbins; i++) {
		bin = &arena->bins[i];
		bin->runcur = NULL;
		arena_run_tree_new(&bin->runs);

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
		arena_run_tree_new(&bin->runs);

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
#ifdef MALLOC_DECOMMIT
	size_t psize;
#endif
	extent_node_t *node;

	

	csize = CHUNK_CEILING(size);
	if (csize == 0) {
		
		return (NULL);
	}

	
	node = base_node_alloc();
	if (node == NULL)
		return (NULL);

	ret = chunk_alloc(csize, zero, true);
	if (ret == NULL) {
		base_node_dealloc(node);
		return (NULL);
	}

	
	node->addr = ret;
#ifdef MALLOC_DECOMMIT
	psize = PAGE_CEILING(size);
	node->size = psize;
#else
	node->size = csize;
#endif

	malloc_mutex_lock(&huge_mtx);
	extent_tree_ad_insert(&huge, node);
#ifdef MALLOC_STATS
	huge_nmalloc++;
#  ifdef MALLOC_DECOMMIT
	huge_allocated += psize;
#  else
	huge_allocated += csize;
#  endif
#endif
	malloc_mutex_unlock(&huge_mtx);

#ifdef MALLOC_DECOMMIT
	if (csize - psize > 0)
		pages_decommit((void *)((uintptr_t)ret + psize), csize - psize);
#endif

#ifdef MALLOC_DECOMMIT
	VALGRIND_MALLOCLIKE_BLOCK(ret, psize, 0, zero);
#else
	VALGRIND_MALLOCLIKE_BLOCK(ret, csize, 0, zero);
#endif

#ifdef MALLOC_FILL
	if (zero == false) {
		if (opt_junk)
#  ifdef MALLOC_DECOMMIT
			memset(ret, 0xa5, psize);
#  else
			memset(ret, 0xa5, csize);
#  endif
		else if (opt_zero)
#  ifdef MALLOC_DECOMMIT
			memset(ret, 0, psize);
#  else
			memset(ret, 0, csize);
#  endif
	}
#endif

	return (ret);
}


static void *
huge_palloc(size_t alignment, size_t size)
{
	void *ret;
	size_t alloc_size, chunk_size, offset;
#ifdef MALLOC_DECOMMIT
	size_t psize;
#endif
	extent_node_t *node;
	int pfd;

	







	assert(alignment >= chunksize);

	chunk_size = CHUNK_CEILING(size);

	if (size >= alignment)
		alloc_size = chunk_size + alignment - chunksize;
	else
		alloc_size = (alignment << 1) - chunksize;

	
	node = base_node_alloc();
	if (node == NULL)
		return (NULL);

	







#ifdef MALLOC_PAGEFILE
	if (opt_pagefile) {
		pfd = pagefile_init(size);
		if (pfd == -1)
			return (NULL);
	} else
#endif
		pfd = -1;
#ifdef JEMALLOC_USES_MAP_ALIGN
		ret = pages_map_align(chunk_size, pfd, alignment);
#else
	do {
		void *over;

		over = chunk_alloc(alloc_size, false, false);
		if (over == NULL) {
			base_node_dealloc(node);
			ret = NULL;
			goto RETURN;
		}

		offset = (uintptr_t)over & (alignment - 1);
		assert((offset & chunksize_mask) == 0);
		assert(offset < alloc_size);
		ret = (void *)((uintptr_t)over + offset);
		chunk_dealloc(over, alloc_size);
		ret = pages_map(ret, chunk_size, pfd);
		



	} while (ret == NULL);
#endif
	
	node->addr = ret;
#ifdef MALLOC_DECOMMIT
	psize = PAGE_CEILING(size);
	node->size = psize;
#else
	node->size = chunk_size;
#endif

	malloc_mutex_lock(&huge_mtx);
	extent_tree_ad_insert(&huge, node);
#ifdef MALLOC_STATS
	huge_nmalloc++;
#  ifdef MALLOC_DECOMMIT
	huge_allocated += psize;
#  else
	huge_allocated += chunk_size;
#  endif
#endif
	malloc_mutex_unlock(&huge_mtx);

#ifdef MALLOC_DECOMMIT
	if (chunk_size - psize > 0) {
		pages_decommit((void *)((uintptr_t)ret + psize),
		    chunk_size - psize);
	}
#endif

#ifdef MALLOC_DECOMMIT
	VALGRIND_MALLOCLIKE_BLOCK(ret, psize, 0, false);
#else
	VALGRIND_MALLOCLIKE_BLOCK(ret, chunk_size, 0, false);
#endif

#ifdef MALLOC_FILL
	if (opt_junk)
#  ifdef MALLOC_DECOMMIT
		memset(ret, 0xa5, psize);
#  else
		memset(ret, 0xa5, chunk_size);
#  endif
	else if (opt_zero)
#  ifdef MALLOC_DECOMMIT
		memset(ret, 0, psize);
#  else
		memset(ret, 0, chunk_size);
#  endif
#endif

RETURN:
#ifdef MALLOC_PAGEFILE
	if (pfd != -1)
		pagefile_close(pfd);
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
#ifdef MALLOC_DECOMMIT
		size_t psize = PAGE_CEILING(size);
#endif
#ifdef MALLOC_FILL
		if (opt_junk && size < oldsize) {
			memset((void *)((uintptr_t)ptr + size), 0x5a, oldsize
			    - size);
		}
#endif
#ifdef MALLOC_DECOMMIT
		if (psize < oldsize) {
			extent_node_t *node, key;

			pages_decommit((void *)((uintptr_t)ptr + psize),
			    oldsize - psize);

			
			malloc_mutex_lock(&huge_mtx);
			key.addr = __DECONST(void *, ptr);
			node = extent_tree_ad_search(&huge, &key);
			assert(node != NULL);
			assert(node->size == oldsize);
#  ifdef MALLOC_STATS
			huge_allocated -= oldsize - psize;
#  endif
			node->size = psize;
			malloc_mutex_unlock(&huge_mtx);
		} else if (psize > oldsize) {
			extent_node_t *node, key;

			pages_commit((void *)((uintptr_t)ptr + oldsize),
			    psize - oldsize);

			
			malloc_mutex_lock(&huge_mtx);
			key.addr = __DECONST(void *, ptr);
			node = extent_tree_ad_search(&huge, &key);
			assert(node != NULL);
			assert(node->size == oldsize);
#  ifdef MALLOC_STATS
			huge_allocated += psize - oldsize;
#  endif
			node->size = psize;
			malloc_mutex_unlock(&huge_mtx);
		}
#endif
#ifdef MALLOC_FILL
		if (opt_zero && size > oldsize) {
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
	node = extent_tree_ad_search(&huge, &key);
	assert(node != NULL);
	assert(node->addr == ptr);
	extent_tree_ad_remove(&huge, node);

#ifdef MALLOC_STATS
	huge_ndalloc++;
	huge_allocated -= node->size;
#endif

	malloc_mutex_unlock(&huge_mtx);

	
#ifdef MALLOC_FILL
	if (opt_junk)
		memset(node->addr, 0x5a, node->size);
#endif
#ifdef MALLOC_DECOMMIT
	chunk_dealloc(node->addr, CHUNK_CEILING(node->size));
#else
	chunk_dealloc(node->addr, node->size);
#endif
	VALGRIND_FREELIKE_BLOCK(node->addr, 0);

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
	char buf[1024];
	static const char matchstr[] = "processor\t:";
	int i;

	




	fd = open("/proc/cpuinfo", O_RDONLY);
	if (fd == -1)
		return (1); 
	



	column = 0;
	ret = 0;
	while (true) {
		nread = read(fd, &buf, sizeof(buf));
		if (nread <= 0)
			break; 
		for (i = 0;i < nread;i++) {
			char c = buf[i];
			if (c == '\n')
				column = 0;
			else if (column != -1) {
				if (c == matchstr[column]) {
					column++;
					if (column == sizeof(matchstr) - 1) {
						column = -1;
						ret++;
					}
				} else
					column = -1;
			}
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

static inline unsigned
malloc_ncpus(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
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
#ifdef MALLOC_FILL
		_malloc_message(opt_junk ? "J" : "j", "", "", "");
#endif
#ifdef MALLOC_PAGEFILE
		_malloc_message(opt_pagefile ? "o" : "O", "", "", "");
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






#if (defined(MOZ_MEMORY_WINDOWS) || defined(MOZ_MEMORY_DARWIN)) && !defined(MOZ_MEMORY_WINCE)
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

#if !defined(MOZ_MEMORY_WINDOWS) || defined(MOZ_MEMORY_WINCE) 
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

#ifdef MALLOC_PAGEFILE
	









	{
		char *s;
		size_t slen;
		static const char suffix[] = "/jemalloc.XXXXXX";

		if ((s = getenv("MALLOC_TMPDIR")) == NULL && (s =
		    getenv("TMPDIR")) == NULL)
			s = P_tmpdir;
		slen = strlen(s);
		if (slen + sizeof(suffix) > sizeof(pagefile_templ)) {
			_malloc_message(_getprogname(),
			    ": (malloc) Page file path too long\n",
			    "", "");
			abort();
		}
		memcpy(pagefile_templ, s, slen);
		memcpy(&pagefile_templ[slen], suffix, sizeof(suffix));
	}
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
				case 'n':
					opt_narenas_lshift--;
					break;
				case 'N':
					opt_narenas_lshift++;
					break;
#ifdef MALLOC_PAGEFILE
				case 'o':
					
					opt_pagefile = true;
					break;
				case 'O':
					
					opt_pagefile = false;
					break;
#endif
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

	
	if (opt_print_stats) {
#ifndef MOZ_MEMORY_WINDOWS
		
		atexit(malloc_print_stats);
#endif
	}

#if (!defined(MOZ_MEMORY_WINDOWS) && !defined(MOZ_MEMORY_DARWIN))
	
	pthread_atfork(_malloc_prefork, _malloc_postfork, _malloc_postfork);
#endif

	
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
		    (sizeof(arena_chunk_map_t) * (chunk_npages - 1));
		arena_chunk_header_npages = (header_size >> pagesize_2pow) +
		    ((header_size & pagesize_mask) != 0);
	}
	arena_maxclass = chunksize - (arena_chunk_header_npages <<
	    pagesize_2pow);

#ifdef JEMALLOC_USES_MAP_ALIGN
	



	assert((chunksize % pagesize) == 0);
	assert((1 << (ffs(chunksize / pagesize) - 1)) == (chunksize/pagesize));
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
	extent_tree_ad_new(&huge);
#ifdef MALLOC_STATS
	huge_nmalloc = 0;
	huge_ndalloc = 0;
	huge_allocated = 0;
#endif

	
#ifdef MALLOC_STATS
	base_mapped = 0;
#  ifdef MALLOC_DECOMMIT
	base_committed = 0;
#  endif
#endif
	base_nodes = NULL;
	malloc_mutex_init(&base_mtx);

#ifdef MOZ_MEMORY_NARENAS_DEFAULT_ONE
	narenas = 1;
#else
	if (ncpus > 1) {
		



		opt_narenas_lshift += 2;
	}

	
	narenas = ncpus;
#endif
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

	



#ifdef MALLOC_BALANCE
	SPRN(balance, 42);
#endif

	malloc_spin_init(&arenas_lock);

#ifdef MALLOC_VALIDATE
	chunk_rtree = malloc_rtree_new((SIZEOF_PTR << 3) - opt_chunk_2pow);
	if (chunk_rtree == NULL)
		return (true);
#endif

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













#ifdef MOZ_MEMORY_DARWIN
#  define ZONE_INLINE	inline
#else
#  define ZONE_INLINE
#endif



#if defined(MOZ_MEMORY_DARWIN)
#define	malloc(a)	moz_malloc(a)
#define	valloc(a)	moz_valloc(a)
#define	calloc(a, b)	moz_calloc(a, b)
#define	realloc(a, b)	moz_realloc(a, b)
#define	free(a)		moz_free(a)
#endif

ZONE_INLINE
void *
malloc(size_t size)
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

#ifdef MOZ_MEMORY_SOLARIS
#  ifdef __SUNPRO_C
void *
memalign(size_t alignment, size_t size);
#pragma no_inline(memalign)
#  elif (defined(__GNU_C__))
__attribute__((noinline))
#  endif
#else
inline
#endif
void *
memalign(size_t alignment, size_t size)
{
	void *ret;

	assert(((alignment - 1) & alignment) == 0);

	if (malloc_init()) {
		ret = NULL;
		goto RETURN;
	}

	alignment = alignment < sizeof(void*) ? sizeof(void*) : alignment;
	ret = ipalloc(alignment, size);

RETURN:
#ifdef MALLOC_XMALLOC
	if (opt_xmalloc && ret == NULL) {
		_malloc_message(_getprogname(),
		": (malloc) Error in memalign(): out of memory\n", "", "");
		abort();
	}
#endif
	UTRACE(0, size, ret);
	return (ret);
}

ZONE_INLINE
int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
	void *result;

	
	if (((alignment - 1) & alignment) != 0 || alignment < sizeof(void *)) {
#ifdef MALLOC_XMALLOC
		if (opt_xmalloc) {
			_malloc_message(_getprogname(),
			    ": (malloc) Error in posix_memalign(): "
			    "invalid alignment\n", "", "");
			abort();
		}
#endif
		return (EINVAL);
	}

#ifdef MOZ_MEMORY_DARWIN
	result = moz_memalign(alignment, size);
#else
	result = memalign(alignment, size);
#endif
	if (result == NULL)
		return (ENOMEM);

	*memptr = result;
	return (0);
}

ZONE_INLINE
void *
valloc(size_t size)
{
#ifdef MOZ_MEMORY_DARWIN
	return (moz_memalign(pagesize, size));
#else
	return (memalign(pagesize, size));
#endif
}

ZONE_INLINE
void *
calloc(size_t num, size_t size)
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

ZONE_INLINE
void *
realloc(void *ptr, size_t size)
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

ZONE_INLINE
void
free(void *ptr)
{

	UTRACE(ptr, 0, 0);
	if (ptr != NULL) {
		assert(malloc_initialized);

		idalloc(ptr);
	}
}









size_t
malloc_usable_size(const void *ptr)
{

#ifdef MALLOC_VALIDATE
	return (isalloc_validate(ptr));
#else
	assert(ptr != NULL);

	return (isalloc(ptr));
#endif
}

void
jemalloc_stats(jemalloc_stats_t *stats)
{
	size_t i;

	assert(stats != NULL);

	


	stats->opt_abort = opt_abort;
	stats->opt_junk =
#ifdef MALLOC_FILL
	    opt_junk ? true :
#endif
	    false;
	stats->opt_utrace =
#ifdef MALLOC_UTRACE
	    opt_utrace ? true :
#endif
	    false;
	stats->opt_sysv =
#ifdef MALLOC_SYSV
	    opt_sysv ? true :
#endif
	    false;
	stats->opt_xmalloc =
#ifdef MALLOC_XMALLOC
	    opt_xmalloc ? true :
#endif
	    false;
	stats->opt_zero =
#ifdef MALLOC_FILL
	    opt_zero ? true :
#endif
	    false;
	stats->narenas = narenas;
	stats->balance_threshold =
#ifdef MALLOC_BALANCE
	    opt_balance_threshold
#else
	    SIZE_T_MAX
#endif
	    ;
	stats->quantum = quantum;
	stats->small_max = small_max;
	stats->large_max = arena_maxclass;
	stats->chunksize = chunksize;
	stats->dirty_max = opt_dirty_max;

	


	stats->mapped = 0;
	stats->committed = 0;
	stats->allocated = 0;
	stats->dirty = 0;

	
	malloc_mutex_lock(&huge_mtx);
	stats->mapped += stats_chunks.curchunks * chunksize;
#ifdef MALLOC_DECOMMIT
	stats->committed += huge_allocated;
#endif
	stats->allocated += huge_allocated;
	malloc_mutex_unlock(&huge_mtx);

	
	malloc_mutex_lock(&base_mtx);
	stats->mapped += base_mapped;
#ifdef MALLOC_DECOMMIT
	assert(base_committed <= base_mapped);
	stats->committed += base_committed;
#endif
	malloc_mutex_unlock(&base_mtx);

	
	for (i = 0; i < narenas; i++) {
		arena_t *arena = arenas[i];
		if (arena != NULL) {
			arena_chunk_t *chunk;

			malloc_spin_lock(&arena->lock);
			stats->allocated += arena->stats.allocated_small;
			stats->allocated += arena->stats.allocated_large;
#ifdef MALLOC_DECOMMIT
			stats->committed += (arena->stats.committed <<
			    pagesize_2pow);
#endif
			stats->dirty += (arena->ndirty << pagesize_2pow);
			malloc_spin_unlock(&arena->lock);
		}
	}

#ifndef MALLOC_DECOMMIT
	stats->committed = stats->mapped;
#endif
	assert(stats->mapped >= stats->committed);
	assert(stats->committed >= stats->allocated);
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
}

void
_malloc_postfork(void)
{
	unsigned i;

	

	malloc_mutex_unlock(&huge_mtx);

	malloc_mutex_unlock(&base_mtx);

	malloc_spin_lock(&arenas_lock);
	for (i = 0; i < narenas; i++) {
		if (arenas[i] != NULL)
			malloc_spin_unlock(&arenas[i]->lock);
	}
	malloc_spin_unlock(&arenas_lock);
}






#ifdef HAVE_LIBDL
#  include <dlfcn.h>
#endif

#ifdef MOZ_MEMORY_DARWIN
static malloc_zone_t zone;
static struct malloc_introspection_t zone_introspect;

static size_t
zone_size(malloc_zone_t *zone, void *ptr)
{

	








	return (isalloc_validate(ptr));
}

static void *
zone_malloc(malloc_zone_t *zone, size_t size)
{

	return (malloc(size));
}

static void *
zone_calloc(malloc_zone_t *zone, size_t num, size_t size)
{

	return (calloc(num, size));
}

static void *
zone_valloc(malloc_zone_t *zone, size_t size)
{
	void *ret = NULL; 

	posix_memalign(&ret, pagesize, size);

	return (ret);
}

static void
zone_free(malloc_zone_t *zone, void *ptr)
{

	free(ptr);
}

static void *
zone_realloc(malloc_zone_t *zone, void *ptr, size_t size)
{

	return (realloc(ptr, size));
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

	




	p = malloc(size);
	if (p != NULL) {
		ret = isalloc(p);
		free(p);
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

#elif defined(__GLIBC__) && !defined(__UCLIBC__)









void (*__free_hook)(void *ptr) = free;
void *(*__malloc_hook)(size_t size) = malloc;
void *(*__realloc_hook)(void *ptr, size_t size) = realloc;
void *(*__memalign_hook)(size_t alignment, size_t size) = memalign;

#elif defined(RTLD_DEEPBIND)





#  error "Interposing malloc is unsafe on this system without libc malloc hooks."
#endif
