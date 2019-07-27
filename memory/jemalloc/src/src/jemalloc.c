#define	JEMALLOC_C_
#include "jemalloc/internal/jemalloc_internal.h"





const char	*je_malloc_conf JEMALLOC_ATTR(weak);
bool	opt_abort =
#ifdef JEMALLOC_DEBUG
    true
#else
    false
#endif
    ;
const char	*opt_junk =
#if (defined(JEMALLOC_DEBUG) && defined(JEMALLOC_FILL))
    "true"
#else
    "false"
#endif
    ;
bool	opt_junk_alloc =
#if (defined(JEMALLOC_DEBUG) && defined(JEMALLOC_FILL))
    true
#else
    false
#endif
    ;
bool	opt_junk_free =
#if (defined(JEMALLOC_DEBUG) && defined(JEMALLOC_FILL))
    true
#else
    false
#endif
    ;

size_t	opt_quarantine = ZU(0);
bool	opt_redzone = false;
bool	opt_utrace = false;
bool	opt_xmalloc = false;
bool	opt_zero = false;
size_t	opt_narenas = 0;


bool	in_valgrind;

unsigned	ncpus;


static malloc_mutex_t	arenas_lock;








static arena_t		**arenas;
static unsigned		narenas_total;
static arena_t		*a0; 
static unsigned		narenas_auto; 


static bool		malloc_initialized = false;

JEMALLOC_ALIGNED(CACHELINE)
const size_t	index2size_tab[NSIZES] = {
#define	SC(index, lg_grp, lg_delta, ndelta, bin, lg_delta_lookup) \
	((ZU(1)<<lg_grp) + (ZU(ndelta)<<lg_delta)),
	SIZE_CLASSES
#undef SC
};

JEMALLOC_ALIGNED(CACHELINE)
const uint8_t	size2index_tab[] = {
#if LG_TINY_MIN == 0
#warning "Dangerous LG_TINY_MIN"
#define	S2B_0(i)	i,
#elif LG_TINY_MIN == 1
#warning "Dangerous LG_TINY_MIN"
#define	S2B_1(i)	i,
#elif LG_TINY_MIN == 2
#warning "Dangerous LG_TINY_MIN"
#define	S2B_2(i)	i,
#elif LG_TINY_MIN == 3
#define	S2B_3(i)	i,
#elif LG_TINY_MIN == 4
#define	S2B_4(i)	i,
#elif LG_TINY_MIN == 5
#define	S2B_5(i)	i,
#elif LG_TINY_MIN == 6
#define	S2B_6(i)	i,
#elif LG_TINY_MIN == 7
#define	S2B_7(i)	i,
#elif LG_TINY_MIN == 8
#define	S2B_8(i)	i,
#elif LG_TINY_MIN == 9
#define	S2B_9(i)	i,
#elif LG_TINY_MIN == 10
#define	S2B_10(i)	i,
#elif LG_TINY_MIN == 11
#define	S2B_11(i)	i,
#else
#error "Unsupported LG_TINY_MIN"
#endif
#if LG_TINY_MIN < 1
#define	S2B_1(i)	S2B_0(i) S2B_0(i)
#endif
#if LG_TINY_MIN < 2
#define	S2B_2(i)	S2B_1(i) S2B_1(i)
#endif
#if LG_TINY_MIN < 3
#define	S2B_3(i)	S2B_2(i) S2B_2(i)
#endif
#if LG_TINY_MIN < 4
#define	S2B_4(i)	S2B_3(i) S2B_3(i)
#endif
#if LG_TINY_MIN < 5
#define	S2B_5(i)	S2B_4(i) S2B_4(i)
#endif
#if LG_TINY_MIN < 6
#define	S2B_6(i)	S2B_5(i) S2B_5(i)
#endif
#if LG_TINY_MIN < 7
#define	S2B_7(i)	S2B_6(i) S2B_6(i)
#endif
#if LG_TINY_MIN < 8
#define	S2B_8(i)	S2B_7(i) S2B_7(i)
#endif
#if LG_TINY_MIN < 9
#define	S2B_9(i)	S2B_8(i) S2B_8(i)
#endif
#if LG_TINY_MIN < 10
#define	S2B_10(i)	S2B_9(i) S2B_9(i)
#endif
#if LG_TINY_MIN < 11
#define	S2B_11(i)	S2B_10(i) S2B_10(i)
#endif
#define	S2B_no(i)
#define	SC(index, lg_grp, lg_delta, ndelta, bin, lg_delta_lookup) \
	S2B_##lg_delta_lookup(index)
	SIZE_CLASSES
#undef S2B_3
#undef S2B_4
#undef S2B_5
#undef S2B_6
#undef S2B_7
#undef S2B_8
#undef S2B_9
#undef S2B_10
#undef S2B_11
#undef S2B_no
#undef SC
};

#ifdef JEMALLOC_THREADED_INIT

#  define NO_INITIALIZER	((unsigned long)0)
#  define INITIALIZER		pthread_self()
#  define IS_INITIALIZER	(malloc_initializer == pthread_self())
static pthread_t		malloc_initializer = NO_INITIALIZER;
#else
#  define NO_INITIALIZER	false
#  define INITIALIZER		true
#  define IS_INITIALIZER	malloc_initializer
static bool			malloc_initializer = NO_INITIALIZER;
#endif


#ifdef _WIN32
static malloc_mutex_t	init_lock;

JEMALLOC_ATTR(constructor)
static void WINAPI
_init_init_lock(void)
{

	malloc_mutex_init(&init_lock);
}

#ifdef _MSC_VER
#  pragma section(".CRT$XCU", read)
JEMALLOC_SECTION(".CRT$XCU") JEMALLOC_ATTR(used)
static const void (WINAPI *init_init_lock)(void) = _init_init_lock;
#endif

#else
static malloc_mutex_t	init_lock = MALLOC_MUTEX_INITIALIZER;
#endif

typedef struct {
	void	*p;	
	size_t	s;	
	void	*r;	
} malloc_utrace_t;

#ifdef JEMALLOC_UTRACE
#  define UTRACE(a, b, c) do {						\
	if (unlikely(opt_utrace)) {					\
		int utrace_serrno = errno;				\
		malloc_utrace_t ut;					\
		ut.p = (a);						\
		ut.s = (b);						\
		ut.r = (c);						\
		utrace(&ut, sizeof(ut));				\
		errno = utrace_serrno;					\
	}								\
} while (0)
#else
#  define UTRACE(a, b, c)
#endif







static bool	malloc_init_hard(void);






JEMALLOC_ALWAYS_INLINE_C void
malloc_thread_init(void)
{

	








	if (config_fill && unlikely(opt_quarantine))
		quarantine_alloc_hook();
}

JEMALLOC_ALWAYS_INLINE_C bool
malloc_init(void)
{

	if (unlikely(!malloc_initialized) && malloc_init_hard())
		return (true);
	malloc_thread_init();

	return (false);
}








arena_t *
a0get(void)
{

	assert(a0 != NULL);
	return (a0);
}

static void *
a0alloc(size_t size, bool zero)
{
	void *ret;

	if (unlikely(malloc_init()))
		return (NULL);

	if (size == 0)
		size = 1;

	if (likely(size <= arena_maxclass))
		ret = arena_malloc(NULL, a0get(), size, zero, false);
	else
		ret = huge_malloc(NULL, a0get(), size, zero, false);

	return (ret);
}

void *
a0malloc(size_t size)
{

	return (a0alloc(size, false));
}

void *
a0calloc(size_t num, size_t size)
{

	return (a0alloc(num * size, true));
}

void
a0free(void *ptr)
{
	arena_chunk_t *chunk;

	if (ptr == NULL)
		return;

	chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
	if (likely(chunk != ptr))
		arena_dalloc(NULL, chunk, ptr, false);
	else
		huge_dalloc(NULL, ptr, false);
}


static arena_t *
arena_init_locked(unsigned ind)
{
	arena_t *arena;

	
	assert(ind <= narenas_total);
	if (ind == narenas_total) {
		unsigned narenas_new = narenas_total + 1;
		arena_t **arenas_new =
		    (arena_t **)a0malloc(CACHELINE_CEILING(narenas_new *
		    sizeof(arena_t *)));
		if (arenas_new == NULL)
			return (NULL);
		memcpy(arenas_new, arenas, narenas_total * sizeof(arena_t *));
		arenas_new[ind] = NULL;
		



		if (narenas_total != narenas_auto)
			a0free(arenas);
		arenas = arenas_new;
		narenas_total = narenas_new;
	}

	



	arena = arenas[ind];
	if (arena != NULL) {
		assert(ind < narenas_auto);
		return (arena);
	}

	
	arena = arenas[ind] = arena_new(ind);
	return (arena);
}

arena_t *
arena_init(unsigned ind)
{
	arena_t *arena;

	malloc_mutex_lock(&arenas_lock);
	arena = arena_init_locked(ind);
	malloc_mutex_unlock(&arenas_lock);
	return (arena);
}

unsigned
narenas_total_get(void)
{
	unsigned narenas;

	malloc_mutex_lock(&arenas_lock);
	narenas = narenas_total;
	malloc_mutex_unlock(&arenas_lock);

	return (narenas);
}

static void
arena_bind_locked(tsd_t *tsd, unsigned ind)
{
	arena_t *arena;

	arena = arenas[ind];
	arena->nthreads++;

	if (tsd_nominal(tsd))
		tsd_arena_set(tsd, arena);
}

static void
arena_bind(tsd_t *tsd, unsigned ind)
{

	malloc_mutex_lock(&arenas_lock);
	arena_bind_locked(tsd, ind);
	malloc_mutex_unlock(&arenas_lock);
}

void
arena_migrate(tsd_t *tsd, unsigned oldind, unsigned newind)
{
	arena_t *oldarena, *newarena;

	malloc_mutex_lock(&arenas_lock);
	oldarena = arenas[oldind];
	newarena = arenas[newind];
	oldarena->nthreads--;
	newarena->nthreads++;
	malloc_mutex_unlock(&arenas_lock);
	tsd_arena_set(tsd, newarena);
}

unsigned
arena_nbound(unsigned ind)
{
	unsigned nthreads;

	malloc_mutex_lock(&arenas_lock);
	nthreads = arenas[ind]->nthreads;
	malloc_mutex_unlock(&arenas_lock);
	return (nthreads);
}

static void
arena_unbind(tsd_t *tsd, unsigned ind)
{
	arena_t *arena;

	malloc_mutex_lock(&arenas_lock);
	arena = arenas[ind];
	arena->nthreads--;
	malloc_mutex_unlock(&arenas_lock);
	tsd_arena_set(tsd, NULL);
}

arena_t *
arena_get_hard(tsd_t *tsd, unsigned ind, bool init_if_missing)
{
	arena_t *arena;
	arena_t **arenas_cache = tsd_arenas_cache_get(tsd);
	unsigned narenas_cache = tsd_narenas_cache_get(tsd);
	unsigned narenas_actual = narenas_total_get();

	
	if (arenas_cache != NULL && narenas_cache < narenas_actual) {
		a0free(arenas_cache);
		arenas_cache = NULL;
		narenas_cache = 0;
		tsd_arenas_cache_set(tsd, arenas_cache);
		tsd_narenas_cache_set(tsd, narenas_cache);
	}

	
	if (arenas_cache == NULL) {
		bool *arenas_cache_bypassp = tsd_arenas_cache_bypassp_get(tsd);
		assert(ind < narenas_actual || !init_if_missing);
		narenas_cache = (ind < narenas_actual) ? narenas_actual : ind+1;

		if (!*arenas_cache_bypassp) {
			*arenas_cache_bypassp = true;
			arenas_cache = (arena_t **)a0malloc(sizeof(arena_t *) *
			    narenas_cache);
			*arenas_cache_bypassp = false;
		} else
			arenas_cache = NULL;
		if (arenas_cache == NULL) {
			





			if (ind >= narenas_actual)
				return (NULL);
			malloc_mutex_lock(&arenas_lock);
			arena = arenas[ind];
			malloc_mutex_unlock(&arenas_lock);
			return (arena);
		}
		tsd_arenas_cache_set(tsd, arenas_cache);
		tsd_narenas_cache_set(tsd, narenas_cache);
	}

	






	malloc_mutex_lock(&arenas_lock);
	memcpy(arenas_cache, arenas, sizeof(arena_t *) * narenas_actual);
	malloc_mutex_unlock(&arenas_lock);
	if (narenas_cache > narenas_actual) {
		memset(&arenas_cache[narenas_actual], 0, sizeof(arena_t *) *
		    (narenas_cache - narenas_actual));
	}

	
	arena = arenas_cache[ind];
	if (init_if_missing && arena == NULL)
		arena = arenas_cache[ind] = arena_init(ind);
	return (arena);
}


arena_t *
arena_choose_hard(tsd_t *tsd)
{
	arena_t *ret;

	if (narenas_auto > 1) {
		unsigned i, choose, first_null;

		choose = 0;
		first_null = narenas_auto;
		malloc_mutex_lock(&arenas_lock);
		assert(a0get() != NULL);
		for (i = 1; i < narenas_auto; i++) {
			if (arenas[i] != NULL) {
				



				if (arenas[i]->nthreads <
				    arenas[choose]->nthreads)
					choose = i;
			} else if (first_null == narenas_auto) {
				








				first_null = i;
			}
		}

		if (arenas[choose]->nthreads == 0
		    || first_null == narenas_auto) {
			



			ret = arenas[choose];
		} else {
			
			choose = first_null;
			ret = arena_init_locked(choose);
			if (ret == NULL) {
				malloc_mutex_unlock(&arenas_lock);
				return (NULL);
			}
		}
		arena_bind_locked(tsd, choose);
		malloc_mutex_unlock(&arenas_lock);
	} else {
		ret = a0get();
		arena_bind(tsd, 0);
	}

	return (ret);
}

void
thread_allocated_cleanup(tsd_t *tsd)
{

	
}

void
thread_deallocated_cleanup(tsd_t *tsd)
{

	
}

void
arena_cleanup(tsd_t *tsd)
{
	arena_t *arena;

	arena = tsd_arena_get(tsd);
	if (arena != NULL)
		arena_unbind(tsd, arena->ind);
}

void
arenas_cache_cleanup(tsd_t *tsd)
{
	arena_t **arenas_cache;

	arenas_cache = tsd_arenas_cache_get(tsd);
	if (arenas != NULL)
		a0free(arenas_cache);
}

void
narenas_cache_cleanup(tsd_t *tsd)
{

	
}

void
arenas_cache_bypass_cleanup(tsd_t *tsd)
{

	
}

static void
stats_print_atexit(void)
{

	if (config_tcache && config_stats) {
		unsigned narenas, i;

		






		for (i = 0, narenas = narenas_total_get(); i < narenas; i++) {
			arena_t *arena = arenas[i];
			if (arena != NULL) {
				tcache_t *tcache;

				





				malloc_mutex_lock(&arena->lock);
				ql_foreach(tcache, &arena->tcache_ql, link) {
					tcache_stats_merge(tcache, arena);
				}
				malloc_mutex_unlock(&arena->lock);
			}
		}
	}
	je_malloc_stats_print(NULL, NULL, NULL);
}









#ifndef JEMALLOC_HAVE_SECURE_GETENV
#  ifdef JEMALLOC_HAVE_ISSETUGID
static char *
secure_getenv(const char *name)
{

	if (issetugid() == 0)
		return (getenv(name));
	else
		return (NULL);
}
#  else
static char *
secure_getenv(const char *name)
{

	return (getenv(name));
}
#  endif
#endif

static unsigned
malloc_ncpus(void)
{
	long result;

#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	result = si.dwNumberOfProcessors;
#else
	result = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	return ((result == -1) ? 1 : (unsigned)result);
}

static bool
malloc_conf_next(char const **opts_p, char const **k_p, size_t *klen_p,
    char const **v_p, size_t *vlen_p)
{
	bool accept;
	const char *opts = *opts_p;

	*k_p = opts;

	for (accept = false; !accept;) {
		switch (*opts) {
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
		case 's': case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
		case '_':
			opts++;
			break;
		case ':':
			opts++;
			*klen_p = (uintptr_t)opts - 1 - (uintptr_t)*k_p;
			*v_p = opts;
			accept = true;
			break;
		case '\0':
			if (opts != *opts_p) {
				malloc_write("<jemalloc>: Conf string ends "
				    "with key\n");
			}
			return (true);
		default:
			malloc_write("<jemalloc>: Malformed conf string\n");
			return (true);
		}
	}

	for (accept = false; !accept;) {
		switch (*opts) {
		case ',':
			opts++;
			






			if (*opts == '\0') {
				malloc_write("<jemalloc>: Conf string ends "
				    "with comma\n");
			}
			*vlen_p = (uintptr_t)opts - 1 - (uintptr_t)*v_p;
			accept = true;
			break;
		case '\0':
			*vlen_p = (uintptr_t)opts - (uintptr_t)*v_p;
			accept = true;
			break;
		default:
			opts++;
			break;
		}
	}

	*opts_p = opts;
	return (false);
}

static void
malloc_conf_error(const char *msg, const char *k, size_t klen, const char *v,
    size_t vlen)
{

	malloc_printf("<jemalloc>: %s: %.*s:%.*s\n", msg, (int)klen, k,
	    (int)vlen, v);
}

static void
malloc_conf_init(void)
{
	unsigned i;
	char buf[PATH_MAX + 1];
	const char *opts, *k, *v;
	size_t klen, vlen;

	



	if (config_valgrind) {
		in_valgrind = (RUNNING_ON_VALGRIND != 0) ? true : false;
		if (config_fill && unlikely(in_valgrind)) {
			opt_junk = "false";
			opt_junk_alloc = false;
			opt_junk_free = false;
			assert(!opt_zero);
			opt_quarantine = JEMALLOC_VALGRIND_QUARANTINE_DEFAULT;
			opt_redzone = true;
		}
		if (config_tcache && unlikely(in_valgrind))
			opt_tcache = false;
	}

	for (i = 0; i < 3; i++) {
		
		switch (i) {
		case 0:
			if (je_malloc_conf != NULL) {
				



				opts = je_malloc_conf;
			} else {
				
				buf[0] = '\0';
				opts = buf;
			}
			break;
		case 1: {
			int linklen = 0;
#ifndef _WIN32
			int saved_errno = errno;
			const char *linkname =
#  ifdef JEMALLOC_PREFIX
			    "/etc/"JEMALLOC_PREFIX"malloc.conf"
#  else
			    "/etc/malloc.conf"
#  endif
			    ;

			



			linklen = readlink(linkname, buf, sizeof(buf) - 1);
			if (linklen == -1) {
				
				linklen = 0;
				
				set_errno(saved_errno);
			}
#endif
			buf[linklen] = '\0';
			opts = buf;
			break;
		} case 2: {
			const char *envname =
#ifdef JEMALLOC_PREFIX
			    JEMALLOC_CPREFIX"MALLOC_CONF"
#else
			    "MALLOC_CONF"
#endif
			    ;

			if ((opts = secure_getenv(envname)) != NULL) {
				




			} else {
				
				buf[0] = '\0';
				opts = buf;
			}
			break;
		} default:
			not_reached();
			buf[0] = '\0';
			opts = buf;
		}

		while (*opts != '\0' && !malloc_conf_next(&opts, &k, &klen, &v,
		    &vlen)) {
#define	CONF_MATCH(n)							\
	(sizeof(n)-1 == klen && strncmp(n, k, klen) == 0)
#define	CONF_MATCH_VALUE(n)						\
	(sizeof(n)-1 == vlen && strncmp(n, v, vlen) == 0)
#define	CONF_HANDLE_BOOL(o, n, cont)					\
			if (CONF_MATCH(n)) {				\
				if (CONF_MATCH_VALUE("true"))		\
					o = true;			\
				else if (CONF_MATCH_VALUE("false"))	\
					o = false;			\
				else {					\
					malloc_conf_error(		\
					    "Invalid conf value",	\
					    k, klen, v, vlen);		\
				}					\
				if (cont)				\
					continue;			\
			}
#define	CONF_HANDLE_SIZE_T(o, n, min, max, clip)			\
			if (CONF_MATCH(n)) {				\
				uintmax_t um;				\
				char *end;				\
									\
				set_errno(0);				\
				um = malloc_strtoumax(v, &end, 0);	\
				if (get_errno() != 0 || (uintptr_t)end -\
				    (uintptr_t)v != vlen) {		\
					malloc_conf_error(		\
					    "Invalid conf value",	\
					    k, klen, v, vlen);		\
				} else if (clip) {			\
					if ((min) != 0 && um < (min))	\
						o = (min);		\
					else if (um > (max))		\
						o = (max);		\
					else				\
						o = um;			\
				} else {				\
					if (((min) != 0 && um < (min))	\
					    || um > (max)) {		\
						malloc_conf_error(	\
						    "Out-of-range "	\
						    "conf value",	\
						    k, klen, v, vlen);	\
					} else				\
						o = um;			\
				}					\
				continue;				\
			}
#define	CONF_HANDLE_SSIZE_T(o, n, min, max)				\
			if (CONF_MATCH(n)) {				\
				long l;					\
				char *end;				\
									\
				set_errno(0);				\
				l = strtol(v, &end, 0);			\
				if (get_errno() != 0 || (uintptr_t)end -\
				    (uintptr_t)v != vlen) {		\
					malloc_conf_error(		\
					    "Invalid conf value",	\
					    k, klen, v, vlen);		\
				} else if (l < (ssize_t)(min) || l >	\
				    (ssize_t)(max)) {			\
					malloc_conf_error(		\
					    "Out-of-range conf value",	\
					    k, klen, v, vlen);		\
				} else					\
					o = l;				\
				continue;				\
			}
#define	CONF_HANDLE_CHAR_P(o, n, d)					\
			if (CONF_MATCH(n)) {				\
				size_t cpylen = (vlen <=		\
				    sizeof(o)-1) ? vlen :		\
				    sizeof(o)-1;			\
				strncpy(o, v, cpylen);			\
				o[cpylen] = '\0';			\
				continue;				\
			}

			CONF_HANDLE_BOOL(opt_abort, "abort", true)
			







			CONF_HANDLE_SIZE_T(opt_lg_chunk, "lg_chunk", LG_PAGE +
			    LG_SIZE_CLASS_GROUP + (config_fill ? 2 : 1),
			    (sizeof(size_t) << 3) - 1, true)
			if (strncmp("dss", k, klen) == 0) {
				int i;
				bool match = false;
				for (i = 0; i < dss_prec_limit; i++) {
					if (strncmp(dss_prec_names[i], v, vlen)
					    == 0) {
						if (chunk_dss_prec_set(i)) {
							malloc_conf_error(
							    "Error setting dss",
							    k, klen, v, vlen);
						} else {
							opt_dss =
							    dss_prec_names[i];
							match = true;
							break;
						}
					}
				}
				if (!match) {
					malloc_conf_error("Invalid conf value",
					    k, klen, v, vlen);
				}
				continue;
			}
			CONF_HANDLE_SIZE_T(opt_narenas, "narenas", 1,
			    SIZE_T_MAX, false)
			CONF_HANDLE_SSIZE_T(opt_lg_dirty_mult, "lg_dirty_mult",
			    -1, (sizeof(size_t) << 3) - 1)
			CONF_HANDLE_BOOL(opt_stats_print, "stats_print", true)
			if (config_fill) {
				if (CONF_MATCH("junk")) {
					if (CONF_MATCH_VALUE("true")) {
						opt_junk = "true";
						opt_junk_alloc = opt_junk_free =
						    true;
					} else if (CONF_MATCH_VALUE("false")) {
						opt_junk = "false";
						opt_junk_alloc = opt_junk_free =
						    false;
					} else if (CONF_MATCH_VALUE("alloc")) {
						opt_junk = "alloc";
						opt_junk_alloc = true;
						opt_junk_free = false;
					} else if (CONF_MATCH_VALUE("free")) {
						opt_junk = "free";
						opt_junk_alloc = false;
						opt_junk_free = true;
					} else {
						malloc_conf_error(
						    "Invalid conf value", k,
						    klen, v, vlen);
					}
					continue;
				}
				CONF_HANDLE_SIZE_T(opt_quarantine, "quarantine",
				    0, SIZE_T_MAX, false)
				CONF_HANDLE_BOOL(opt_redzone, "redzone", true)
				CONF_HANDLE_BOOL(opt_zero, "zero", true)
			}
			if (config_utrace) {
				CONF_HANDLE_BOOL(opt_utrace, "utrace", true)
			}
			if (config_xmalloc) {
				CONF_HANDLE_BOOL(opt_xmalloc, "xmalloc", true)
			}
			if (config_tcache) {
				CONF_HANDLE_BOOL(opt_tcache, "tcache",
				    !config_valgrind || !in_valgrind)
				if (CONF_MATCH("tcache")) {
					assert(config_valgrind && in_valgrind);
					if (opt_tcache) {
						opt_tcache = false;
						malloc_conf_error(
						"tcache cannot be enabled "
						"while running inside Valgrind",
						k, klen, v, vlen);
					}
					continue;
				}
				CONF_HANDLE_SSIZE_T(opt_lg_tcache_max,
				    "lg_tcache_max", -1,
				    (sizeof(size_t) << 3) - 1)
			}
			if (config_prof) {
				CONF_HANDLE_BOOL(opt_prof, "prof", true)
				CONF_HANDLE_CHAR_P(opt_prof_prefix,
				    "prof_prefix", "jeprof")
				CONF_HANDLE_BOOL(opt_prof_active, "prof_active",
				    true)
				CONF_HANDLE_BOOL(opt_prof_thread_active_init,
				    "prof_thread_active_init", true)
				CONF_HANDLE_SIZE_T(opt_lg_prof_sample,
				    "lg_prof_sample", 0,
				    (sizeof(uint64_t) << 3) - 1, true)
				CONF_HANDLE_BOOL(opt_prof_accum, "prof_accum",
				    true)
				CONF_HANDLE_SSIZE_T(opt_lg_prof_interval,
				    "lg_prof_interval", -1,
				    (sizeof(uint64_t) << 3) - 1)
				CONF_HANDLE_BOOL(opt_prof_gdump, "prof_gdump",
				    true)
				CONF_HANDLE_BOOL(opt_prof_final, "prof_final",
				    true)
				CONF_HANDLE_BOOL(opt_prof_leak, "prof_leak",
				    true)
			}
			malloc_conf_error("Invalid conf pair", k, klen, v,
			    vlen);
#undef CONF_MATCH
#undef CONF_HANDLE_BOOL
#undef CONF_HANDLE_SIZE_T
#undef CONF_HANDLE_SSIZE_T
#undef CONF_HANDLE_CHAR_P
		}
	}
}

static bool
malloc_init_hard(void)
{
	arena_t *init_arenas[1];

	malloc_mutex_lock(&init_lock);
	if (malloc_initialized || IS_INITIALIZER) {
		




		malloc_mutex_unlock(&init_lock);
		return (false);
	}
#ifdef JEMALLOC_THREADED_INIT
	if (malloc_initializer != NO_INITIALIZER && !IS_INITIALIZER) {
		
		do {
			malloc_mutex_unlock(&init_lock);
			CPU_SPINWAIT;
			malloc_mutex_lock(&init_lock);
		} while (!malloc_initialized);
		malloc_mutex_unlock(&init_lock);
		return (false);
	}
#endif
	malloc_initializer = INITIALIZER;

	if (malloc_tsd_boot0()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (config_prof)
		prof_boot0();

	malloc_conf_init();

	if (opt_stats_print) {
		
		if (atexit(stats_print_atexit) != 0) {
			malloc_write("<jemalloc>: Error in atexit()\n");
			if (opt_abort)
				abort();
		}
	}

	if (base_boot()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (chunk_boot()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (ctl_boot()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (config_prof)
		prof_boot1();

	arena_boot();

	if (config_tcache && tcache_boot()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (huge_boot()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (malloc_mutex_init(&arenas_lock)) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	



	narenas_total = narenas_auto = 1;
	arenas = init_arenas;
	memset(arenas, 0, sizeof(arena_t *) * narenas_auto);

	



	a0 = arena_init(0);
	if (a0 == NULL) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (config_prof && prof_boot2()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	malloc_mutex_unlock(&init_lock);
	
	

	ncpus = malloc_ncpus();

#if (!defined(JEMALLOC_MUTEX_INIT_CB) && !defined(JEMALLOC_ZONE) \
    && !defined(_WIN32) && !defined(__native_client__))
	
	if (pthread_atfork(jemalloc_prefork, jemalloc_postfork_parent,
	    jemalloc_postfork_child) != 0) {
		malloc_write("<jemalloc>: Error in pthread_atfork()\n");
		if (opt_abort)
			abort();
	}
#endif

	
	
	malloc_mutex_lock(&init_lock);

	if (mutex_boot()) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}

	if (opt_narenas == 0) {
		



		if (ncpus > 1)
			opt_narenas = ncpus << 2;
		else
			opt_narenas = 1;
	}
	narenas_auto = opt_narenas;
	




	if (narenas_auto > chunksize / sizeof(arena_t *)) {
		narenas_auto = chunksize / sizeof(arena_t *);
		malloc_printf("<jemalloc>: Reducing narenas to limit (%d)\n",
		    narenas_auto);
	}
	narenas_total = narenas_auto;

	
	arenas = (arena_t **)base_alloc(sizeof(arena_t *) * narenas_total);
	if (arenas == NULL) {
		malloc_mutex_unlock(&init_lock);
		return (true);
	}
	



	memset(arenas, 0, sizeof(arena_t *) * narenas_total);
	
	arenas[0] = init_arenas[0];

	malloc_initialized = true;
	malloc_mutex_unlock(&init_lock);
	malloc_tsd_boot1();

	return (false);
}









static void *
imalloc_prof_sample(tsd_t *tsd, size_t usize, prof_tctx_t *tctx)
{
	void *p;

	if (tctx == NULL)
		return (NULL);
	if (usize <= SMALL_MAXCLASS) {
		p = imalloc(tsd, LARGE_MINCLASS);
		if (p == NULL)
			return (NULL);
		arena_prof_promoted(p, usize);
	} else
		p = imalloc(tsd, usize);

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
imalloc_prof(tsd_t *tsd, size_t usize)
{
	void *p;
	prof_tctx_t *tctx;

	tctx = prof_alloc_prep(tsd, usize, true);
	if (unlikely((uintptr_t)tctx != (uintptr_t)1U))
		p = imalloc_prof_sample(tsd, usize, tctx);
	else
		p = imalloc(tsd, usize);
	if (unlikely(p == NULL)) {
		prof_alloc_rollback(tsd, tctx, true);
		return (NULL);
	}
	prof_malloc(p, usize, tctx);

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
imalloc_body(size_t size, tsd_t **tsd, size_t *usize)
{

	if (unlikely(malloc_init()))
		return (NULL);
	*tsd = tsd_fetch();

	if (config_prof && opt_prof) {
		*usize = s2u(size);
		return (imalloc_prof(*tsd, *usize));
	}

	if (config_stats || (config_valgrind && unlikely(in_valgrind)))
		*usize = s2u(size);
	return (imalloc(*tsd, size));
}

void *
je_malloc(size_t size)
{
	void *ret;
	tsd_t *tsd;
	size_t usize JEMALLOC_CC_SILENCE_INIT(0);

	if (size == 0)
		size = 1;

	ret = imalloc_body(size, &tsd, &usize);
	if (unlikely(ret == NULL)) {
		if (config_xmalloc && unlikely(opt_xmalloc)) {
			malloc_write("<jemalloc>: Error in malloc(): "
			    "out of memory\n");
			abort();
		}
		set_errno(ENOMEM);
	}
	if (config_stats && likely(ret != NULL)) {
		assert(usize == isalloc(ret, config_prof));
		*tsd_thread_allocatedp_get(tsd) += usize;
	}
	UTRACE(0, size, ret);
	JEMALLOC_VALGRIND_MALLOC(ret != NULL, ret, usize, false);
	return (ret);
}

static void *
imemalign_prof_sample(tsd_t *tsd, size_t alignment, size_t usize,
    prof_tctx_t *tctx)
{
	void *p;

	if (tctx == NULL)
		return (NULL);
	if (usize <= SMALL_MAXCLASS) {
		assert(sa2u(LARGE_MINCLASS, alignment) == LARGE_MINCLASS);
		p = imalloc(tsd, LARGE_MINCLASS);
		if (p == NULL)
			return (NULL);
		arena_prof_promoted(p, usize);
	} else
		p = ipalloc(tsd, usize, alignment, false);

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
imemalign_prof(tsd_t *tsd, size_t alignment, size_t usize)
{
	void *p;
	prof_tctx_t *tctx;

	tctx = prof_alloc_prep(tsd, usize, true);
	if (unlikely((uintptr_t)tctx != (uintptr_t)1U))
		p = imemalign_prof_sample(tsd, alignment, usize, tctx);
	else
		p = ipalloc(tsd, usize, alignment, false);
	if (unlikely(p == NULL)) {
		prof_alloc_rollback(tsd, tctx, true);
		return (NULL);
	}
	prof_malloc(p, usize, tctx);

	return (p);
}

JEMALLOC_ATTR(nonnull(1))
static int
imemalign(void **memptr, size_t alignment, size_t size, size_t min_alignment)
{
	int ret;
	tsd_t *tsd;
	size_t usize;
	void *result;

	assert(min_alignment != 0);

	if (unlikely(malloc_init())) {
		result = NULL;
		goto label_oom;
	} else {
		tsd = tsd_fetch();
		if (size == 0)
			size = 1;

		
		if (unlikely(((alignment - 1) & alignment) != 0
		    || (alignment < min_alignment))) {
			if (config_xmalloc && unlikely(opt_xmalloc)) {
				malloc_write("<jemalloc>: Error allocating "
				    "aligned memory: invalid alignment\n");
				abort();
			}
			result = NULL;
			ret = EINVAL;
			goto label_return;
		}

		usize = sa2u(size, alignment);
		if (unlikely(usize == 0)) {
			result = NULL;
			goto label_oom;
		}

		if (config_prof && opt_prof)
			result = imemalign_prof(tsd, alignment, usize);
		else
			result = ipalloc(tsd, usize, alignment, false);
		if (unlikely(result == NULL))
			goto label_oom;
	}

	*memptr = result;
	ret = 0;
label_return:
	if (config_stats && likely(result != NULL)) {
		assert(usize == isalloc(result, config_prof));
		*tsd_thread_allocatedp_get(tsd) += usize;
	}
	UTRACE(0, size, result);
	return (ret);
label_oom:
	assert(result == NULL);
	if (config_xmalloc && unlikely(opt_xmalloc)) {
		malloc_write("<jemalloc>: Error allocating aligned memory: "
		    "out of memory\n");
		abort();
	}
	ret = ENOMEM;
	goto label_return;
}

int
je_posix_memalign(void **memptr, size_t alignment, size_t size)
{
	int ret = imemalign(memptr, alignment, size, sizeof(void *));
	JEMALLOC_VALGRIND_MALLOC(ret == 0, *memptr, isalloc(*memptr,
	    config_prof), false);
	return (ret);
}

void *
je_aligned_alloc(size_t alignment, size_t size)
{
	void *ret;
	int err;

	if (unlikely((err = imemalign(&ret, alignment, size, 1)) != 0)) {
		ret = NULL;
		set_errno(err);
	}
	JEMALLOC_VALGRIND_MALLOC(err == 0, ret, isalloc(ret, config_prof),
	    false);
	return (ret);
}

static void *
icalloc_prof_sample(tsd_t *tsd, size_t usize, prof_tctx_t *tctx)
{
	void *p;

	if (tctx == NULL)
		return (NULL);
	if (usize <= SMALL_MAXCLASS) {
		p = icalloc(tsd, LARGE_MINCLASS);
		if (p == NULL)
			return (NULL);
		arena_prof_promoted(p, usize);
	} else
		p = icalloc(tsd, usize);

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
icalloc_prof(tsd_t *tsd, size_t usize)
{
	void *p;
	prof_tctx_t *tctx;

	tctx = prof_alloc_prep(tsd, usize, true);
	if (unlikely((uintptr_t)tctx != (uintptr_t)1U))
		p = icalloc_prof_sample(tsd, usize, tctx);
	else
		p = icalloc(tsd, usize);
	if (unlikely(p == NULL)) {
		prof_alloc_rollback(tsd, tctx, true);
		return (NULL);
	}
	prof_malloc(p, usize, tctx);

	return (p);
}

void *
je_calloc(size_t num, size_t size)
{
	void *ret;
	tsd_t *tsd;
	size_t num_size;
	size_t usize JEMALLOC_CC_SILENCE_INIT(0);

	if (unlikely(malloc_init())) {
		num_size = 0;
		ret = NULL;
		goto label_return;
	}
	tsd = tsd_fetch();

	num_size = num * size;
	if (unlikely(num_size == 0)) {
		if (num == 0 || size == 0)
			num_size = 1;
		else {
			ret = NULL;
			goto label_return;
		}
	




	} else if (unlikely(((num | size) & (SIZE_T_MAX << (sizeof(size_t) <<
	    2))) && (num_size / size != num))) {
		
		ret = NULL;
		goto label_return;
	}

	if (config_prof && opt_prof) {
		usize = s2u(num_size);
		ret = icalloc_prof(tsd, usize);
	} else {
		if (config_stats || (config_valgrind && unlikely(in_valgrind)))
			usize = s2u(num_size);
		ret = icalloc(tsd, num_size);
	}

label_return:
	if (unlikely(ret == NULL)) {
		if (config_xmalloc && unlikely(opt_xmalloc)) {
			malloc_write("<jemalloc>: Error in calloc(): out of "
			    "memory\n");
			abort();
		}
		set_errno(ENOMEM);
	}
	if (config_stats && likely(ret != NULL)) {
		assert(usize == isalloc(ret, config_prof));
		*tsd_thread_allocatedp_get(tsd) += usize;
	}
	UTRACE(0, num_size, ret);
	JEMALLOC_VALGRIND_MALLOC(ret != NULL, ret, usize, true);
	return (ret);
}

static void *
irealloc_prof_sample(tsd_t *tsd, void *oldptr, size_t old_usize, size_t usize,
    prof_tctx_t *tctx)
{
	void *p;

	if (tctx == NULL)
		return (NULL);
	if (usize <= SMALL_MAXCLASS) {
		p = iralloc(tsd, oldptr, old_usize, LARGE_MINCLASS, 0, false);
		if (p == NULL)
			return (NULL);
		arena_prof_promoted(p, usize);
	} else
		p = iralloc(tsd, oldptr, old_usize, usize, 0, false);

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
irealloc_prof(tsd_t *tsd, void *oldptr, size_t old_usize, size_t usize)
{
	void *p;
	prof_tctx_t *old_tctx, *tctx;

	old_tctx = prof_tctx_get(oldptr);
	tctx = prof_alloc_prep(tsd, usize, true);
	if (unlikely((uintptr_t)tctx != (uintptr_t)1U))
		p = irealloc_prof_sample(tsd, oldptr, old_usize, usize, tctx);
	else
		p = iralloc(tsd, oldptr, old_usize, usize, 0, false);
	if (p == NULL)
		return (NULL);
	prof_realloc(tsd, p, usize, tctx, true, old_usize, old_tctx);

	return (p);
}

JEMALLOC_INLINE_C void
ifree(tsd_t *tsd, void *ptr, bool try_tcache)
{
	size_t usize;
	UNUSED size_t rzsize JEMALLOC_CC_SILENCE_INIT(0);

	assert(ptr != NULL);
	assert(malloc_initialized || IS_INITIALIZER);

	if (config_prof && opt_prof) {
		usize = isalloc(ptr, config_prof);
		prof_free(tsd, ptr, usize);
	} else if (config_stats || config_valgrind)
		usize = isalloc(ptr, config_prof);
	if (config_stats)
		*tsd_thread_deallocatedp_get(tsd) += usize;
	if (config_valgrind && unlikely(in_valgrind))
		rzsize = p2rz(ptr);
	iqalloc(tsd, ptr, try_tcache);
	JEMALLOC_VALGRIND_FREE(ptr, rzsize);
}

JEMALLOC_INLINE_C void
isfree(tsd_t *tsd, void *ptr, size_t usize, bool try_tcache)
{
	UNUSED size_t rzsize JEMALLOC_CC_SILENCE_INIT(0);

	assert(ptr != NULL);
	assert(malloc_initialized || IS_INITIALIZER);

	if (config_prof && opt_prof)
		prof_free(tsd, ptr, usize);
	if (config_stats)
		*tsd_thread_deallocatedp_get(tsd) += usize;
	if (config_valgrind && unlikely(in_valgrind))
		rzsize = p2rz(ptr);
	isqalloc(tsd, ptr, usize, try_tcache);
	JEMALLOC_VALGRIND_FREE(ptr, rzsize);
}

void *
je_realloc(void *ptr, size_t size)
{
	void *ret;
	tsd_t *tsd JEMALLOC_CC_SILENCE_INIT(NULL);
	size_t usize JEMALLOC_CC_SILENCE_INIT(0);
	size_t old_usize = 0;
	UNUSED size_t old_rzsize JEMALLOC_CC_SILENCE_INIT(0);

	if (unlikely(size == 0)) {
		if (ptr != NULL) {
			
			UTRACE(ptr, 0, 0);
			tsd = tsd_fetch();
			ifree(tsd, ptr, true);
			return (NULL);
		}
		size = 1;
	}

	if (likely(ptr != NULL)) {
		assert(malloc_initialized || IS_INITIALIZER);
		malloc_thread_init();
		tsd = tsd_fetch();

		old_usize = isalloc(ptr, config_prof);
		if (config_valgrind && unlikely(in_valgrind))
			old_rzsize = config_prof ? p2rz(ptr) : u2rz(old_usize);

		if (config_prof && opt_prof) {
			usize = s2u(size);
			ret = irealloc_prof(tsd, ptr, old_usize, usize);
		} else {
			if (config_stats || (config_valgrind &&
			    unlikely(in_valgrind)))
				usize = s2u(size);
			ret = iralloc(tsd, ptr, old_usize, size, 0, false);
		}
	} else {
		
		ret = imalloc_body(size, &tsd, &usize);
	}

	if (unlikely(ret == NULL)) {
		if (config_xmalloc && unlikely(opt_xmalloc)) {
			malloc_write("<jemalloc>: Error in realloc(): "
			    "out of memory\n");
			abort();
		}
		set_errno(ENOMEM);
	}
	if (config_stats && likely(ret != NULL)) {
		assert(usize == isalloc(ret, config_prof));
		*tsd_thread_allocatedp_get(tsd) += usize;
		*tsd_thread_deallocatedp_get(tsd) += old_usize;
	}
	UTRACE(ptr, size, ret);
	JEMALLOC_VALGRIND_REALLOC(true, ret, usize, true, ptr, old_usize,
	    old_rzsize, true, false);
	return (ret);
}

void
je_free(void *ptr)
{

	UTRACE(ptr, 0, 0);
	if (likely(ptr != NULL))
		ifree(tsd_fetch(), ptr, true);
}









#ifdef JEMALLOC_OVERRIDE_MEMALIGN
void *
je_memalign(size_t alignment, size_t size)
{
	void *ret JEMALLOC_CC_SILENCE_INIT(NULL);
	imemalign(&ret, alignment, size, 1);
	JEMALLOC_VALGRIND_MALLOC(ret != NULL, ret, size, false);
	return (ret);
}
#endif

#ifdef JEMALLOC_OVERRIDE_VALLOC
void *
je_valloc(size_t size)
{
	void *ret JEMALLOC_CC_SILENCE_INIT(NULL);
	imemalign(&ret, PAGE, size, 1);
	JEMALLOC_VALGRIND_MALLOC(ret != NULL, ret, size, false);
	return (ret);
}
#endif





#define	malloc_is_malloc 1
#define	is_malloc_(a) malloc_is_ ## a
#define	is_malloc(a) is_malloc_(a)

#if ((is_malloc(je_malloc) == 1) && defined(JEMALLOC_GLIBC_MALLOC_HOOK))









JEMALLOC_EXPORT void (*__free_hook)(void *ptr) = je_free;
JEMALLOC_EXPORT void *(*__malloc_hook)(size_t size) = je_malloc;
JEMALLOC_EXPORT void *(*__realloc_hook)(void *ptr, size_t size) = je_realloc;
# ifdef JEMALLOC_GLIBC_MEMALIGN_HOOK
JEMALLOC_EXPORT void *(*__memalign_hook)(size_t alignment, size_t size) =
    je_memalign;
# endif
#endif









JEMALLOC_ALWAYS_INLINE_C bool
imallocx_flags_decode_hard(tsd_t *tsd, size_t size, int flags, size_t *usize,
    size_t *alignment, bool *zero, bool *try_tcache, arena_t **arena)
{

	if ((flags & MALLOCX_LG_ALIGN_MASK) == 0) {
		*alignment = 0;
		*usize = s2u(size);
	} else {
		*alignment = MALLOCX_ALIGN_GET_SPECIFIED(flags);
		*usize = sa2u(size, *alignment);
	}
	*zero = MALLOCX_ZERO_GET(flags);
	if ((flags & MALLOCX_ARENA_MASK) != 0) {
		unsigned arena_ind = MALLOCX_ARENA_GET(flags);
		*try_tcache = false;
		*arena = arena_get(tsd, arena_ind, true, true);
		if (unlikely(*arena == NULL))
			return (true);
	} else {
		*try_tcache = true;
		*arena = NULL;
	}
	return (false);
}

JEMALLOC_ALWAYS_INLINE_C bool
imallocx_flags_decode(tsd_t *tsd, size_t size, int flags, size_t *usize,
    size_t *alignment, bool *zero, bool *try_tcache, arena_t **arena)
{

	if (likely(flags == 0)) {
		*usize = s2u(size);
		assert(usize != 0);
		*alignment = 0;
		*zero = false;
		*try_tcache = true;
		*arena = NULL;
		return (false);
	} else {
		return (imallocx_flags_decode_hard(tsd, size, flags, usize,
		    alignment, zero, try_tcache, arena));
	}
}

JEMALLOC_ALWAYS_INLINE_C void *
imallocx_flags(tsd_t *tsd, size_t usize, size_t alignment, bool zero,
    bool try_tcache, arena_t *arena)
{

	if (alignment != 0) {
		return (ipalloct(tsd, usize, alignment, zero, try_tcache,
		    arena));
	}
	if (zero)
		return (icalloct(tsd, usize, try_tcache, arena));
	return (imalloct(tsd, usize, try_tcache, arena));
}

JEMALLOC_ALWAYS_INLINE_C void *
imallocx_maybe_flags(tsd_t *tsd, size_t size, int flags, size_t usize,
    size_t alignment, bool zero, bool try_tcache, arena_t *arena)
{

	if (likely(flags == 0))
		return (imalloc(tsd, size));
	return (imallocx_flags(tsd, usize, alignment, zero, try_tcache, arena));
}

static void *
imallocx_prof_sample(tsd_t *tsd, size_t size, int flags, size_t usize,
    size_t alignment, bool zero, bool try_tcache, arena_t *arena)
{
	void *p;

	if (usize <= SMALL_MAXCLASS) {
		assert(((alignment == 0) ? s2u(LARGE_MINCLASS) :
		    sa2u(LARGE_MINCLASS, alignment)) == LARGE_MINCLASS);
		p = imalloct(tsd, LARGE_MINCLASS, try_tcache, arena);
		if (p == NULL)
			return (NULL);
		arena_prof_promoted(p, usize);
	} else {
		p = imallocx_maybe_flags(tsd, size, flags, usize, alignment,
		    zero, try_tcache, arena);
	}

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
imallocx_prof(tsd_t *tsd, size_t size, int flags, size_t *usize)
{
	void *p;
	size_t alignment;
	bool zero;
	bool try_tcache;
	arena_t *arena;
	prof_tctx_t *tctx;

	if (unlikely(imallocx_flags_decode(tsd, size, flags, usize, &alignment,
	    &zero, &try_tcache, &arena)))
		return (NULL);
	tctx = prof_alloc_prep(tsd, *usize, true);
	if (likely((uintptr_t)tctx == (uintptr_t)1U)) {
		p = imallocx_maybe_flags(tsd, size, flags, *usize, alignment,
		    zero, try_tcache, arena);
	} else if ((uintptr_t)tctx > (uintptr_t)1U) {
		p = imallocx_prof_sample(tsd, size, flags, *usize, alignment,
		    zero, try_tcache, arena);
	} else
		p = NULL;
	if (unlikely(p == NULL)) {
		prof_alloc_rollback(tsd, tctx, true);
		return (NULL);
	}
	prof_malloc(p, *usize, tctx);

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
imallocx_no_prof(tsd_t *tsd, size_t size, int flags, size_t *usize)
{
	size_t alignment;
	bool zero;
	bool try_tcache;
	arena_t *arena;

	if (likely(flags == 0)) {
		if (config_stats || (config_valgrind && unlikely(in_valgrind)))
			*usize = s2u(size);
		return (imalloc(tsd, size));
	}

	if (unlikely(imallocx_flags_decode_hard(tsd, size, flags, usize,
	    &alignment, &zero, &try_tcache, &arena)))
		return (NULL);
	return (imallocx_flags(tsd, *usize, alignment, zero, try_tcache,
	    arena));
}

void *
je_mallocx(size_t size, int flags)
{
	tsd_t *tsd;
	void *p;
	size_t usize;

	assert(size != 0);

	if (unlikely(malloc_init()))
		goto label_oom;
	tsd = tsd_fetch();

	if (config_prof && opt_prof)
		p = imallocx_prof(tsd, size, flags, &usize);
	else
		p = imallocx_no_prof(tsd, size, flags, &usize);
	if (unlikely(p == NULL))
		goto label_oom;

	if (config_stats) {
		assert(usize == isalloc(p, config_prof));
		*tsd_thread_allocatedp_get(tsd) += usize;
	}
	UTRACE(0, size, p);
	JEMALLOC_VALGRIND_MALLOC(true, p, usize, MALLOCX_ZERO_GET(flags));
	return (p);
label_oom:
	if (config_xmalloc && unlikely(opt_xmalloc)) {
		malloc_write("<jemalloc>: Error in mallocx(): out of memory\n");
		abort();
	}
	UTRACE(0, size, 0);
	return (NULL);
}

static void *
irallocx_prof_sample(tsd_t *tsd, void *oldptr, size_t old_usize, size_t size,
    size_t alignment, size_t usize, bool zero, bool try_tcache_alloc,
    bool try_tcache_dalloc, arena_t *arena, prof_tctx_t *tctx)
{
	void *p;

	if (tctx == NULL)
		return (NULL);
	if (usize <= SMALL_MAXCLASS) {
		p = iralloct(tsd, oldptr, old_usize, LARGE_MINCLASS, alignment,
		    zero, try_tcache_alloc, try_tcache_dalloc, arena);
		if (p == NULL)
			return (NULL);
		arena_prof_promoted(p, usize);
	} else {
		p = iralloct(tsd, oldptr, old_usize, size, alignment, zero,
		    try_tcache_alloc, try_tcache_dalloc, arena);
	}

	return (p);
}

JEMALLOC_ALWAYS_INLINE_C void *
irallocx_prof(tsd_t *tsd, void *oldptr, size_t old_usize, size_t size,
    size_t alignment, size_t *usize, bool zero, bool try_tcache_alloc,
    bool try_tcache_dalloc, arena_t *arena)
{
	void *p;
	prof_tctx_t *old_tctx, *tctx;

	old_tctx = prof_tctx_get(oldptr);
	tctx = prof_alloc_prep(tsd, *usize, false);
	if (unlikely((uintptr_t)tctx != (uintptr_t)1U)) {
		p = irallocx_prof_sample(tsd, oldptr, old_usize, size,
		    alignment, *usize, zero, try_tcache_alloc,
		    try_tcache_dalloc, arena, tctx);
	} else {
		p = iralloct(tsd, oldptr, old_usize, size, alignment, zero,
		    try_tcache_alloc, try_tcache_dalloc, arena);
	}
	if (unlikely(p == NULL)) {
		prof_alloc_rollback(tsd, tctx, false);
		return (NULL);
	}

	if (p == oldptr && alignment != 0) {
		







		*usize = isalloc(p, config_prof);
	}
	prof_realloc(tsd, p, *usize, tctx, false, old_usize, old_tctx);

	return (p);
}

void *
je_rallocx(void *ptr, size_t size, int flags)
{
	void *p;
	tsd_t *tsd;
	size_t usize;
	size_t old_usize;
	UNUSED size_t old_rzsize JEMALLOC_CC_SILENCE_INIT(0);
	size_t alignment = MALLOCX_ALIGN_GET(flags);
	bool zero = flags & MALLOCX_ZERO;
	bool try_tcache_alloc, try_tcache_dalloc;
	arena_t *arena;

	assert(ptr != NULL);
	assert(size != 0);
	assert(malloc_initialized || IS_INITIALIZER);
	malloc_thread_init();
	tsd = tsd_fetch();

	if (unlikely((flags & MALLOCX_ARENA_MASK) != 0)) {
		unsigned arena_ind = MALLOCX_ARENA_GET(flags);
		arena_chunk_t *chunk;
		try_tcache_alloc = false;
		chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		arena = arena_get(tsd, arena_ind, true, true);
		if (unlikely(arena == NULL))
			goto label_oom;
		try_tcache_dalloc = (chunk == ptr || chunk->arena != arena);
	} else {
		try_tcache_alloc = true;
		try_tcache_dalloc = true;
		arena = NULL;
	}

	old_usize = isalloc(ptr, config_prof);
	if (config_valgrind && unlikely(in_valgrind))
		old_rzsize = u2rz(old_usize);

	if (config_prof && opt_prof) {
		usize = (alignment == 0) ? s2u(size) : sa2u(size, alignment);
		assert(usize != 0);
		p = irallocx_prof(tsd, ptr, old_usize, size, alignment, &usize,
		    zero, try_tcache_alloc, try_tcache_dalloc, arena);
		if (unlikely(p == NULL))
			goto label_oom;
	} else {
		p = iralloct(tsd, ptr, old_usize, size, alignment, zero,
		     try_tcache_alloc, try_tcache_dalloc, arena);
		if (unlikely(p == NULL))
			goto label_oom;
		if (config_stats || (config_valgrind && unlikely(in_valgrind)))
			usize = isalloc(p, config_prof);
	}

	if (config_stats) {
		*tsd_thread_allocatedp_get(tsd) += usize;
		*tsd_thread_deallocatedp_get(tsd) += old_usize;
	}
	UTRACE(ptr, size, p);
	JEMALLOC_VALGRIND_REALLOC(true, p, usize, false, ptr, old_usize,
	    old_rzsize, false, zero);
	return (p);
label_oom:
	if (config_xmalloc && unlikely(opt_xmalloc)) {
		malloc_write("<jemalloc>: Error in rallocx(): out of memory\n");
		abort();
	}
	UTRACE(ptr, size, 0);
	return (NULL);
}

JEMALLOC_ALWAYS_INLINE_C size_t
ixallocx_helper(void *ptr, size_t old_usize, size_t size, size_t extra,
    size_t alignment, bool zero)
{
	size_t usize;

	if (ixalloc(ptr, old_usize, size, extra, alignment, zero))
		return (old_usize);
	usize = isalloc(ptr, config_prof);

	return (usize);
}

static size_t
ixallocx_prof_sample(void *ptr, size_t old_usize, size_t size, size_t extra,
    size_t alignment, size_t max_usize, bool zero, prof_tctx_t *tctx)
{
	size_t usize;

	if (tctx == NULL)
		return (old_usize);
	
	if (((alignment == 0) ? s2u(size) : sa2u(size, alignment)) <=
	    SMALL_MAXCLASS) {
		if (ixalloc(ptr, old_usize, SMALL_MAXCLASS+1,
		    (SMALL_MAXCLASS+1 >= size+extra) ? 0 : size+extra -
		    (SMALL_MAXCLASS+1), alignment, zero))
			return (old_usize);
		usize = isalloc(ptr, config_prof);
		if (max_usize < LARGE_MINCLASS)
			arena_prof_promoted(ptr, usize);
	} else {
		usize = ixallocx_helper(ptr, old_usize, size, extra, alignment,
		    zero);
	}

	return (usize);
}

JEMALLOC_ALWAYS_INLINE_C size_t
ixallocx_prof(tsd_t *tsd, void *ptr, size_t old_usize, size_t size,
    size_t extra, size_t alignment, bool zero)
{
	size_t max_usize, usize;
	prof_tctx_t *old_tctx, *tctx;

	old_tctx = prof_tctx_get(ptr);
	





	max_usize = (alignment == 0) ? s2u(size+extra) : sa2u(size+extra,
	    alignment);
	tctx = prof_alloc_prep(tsd, max_usize, false);
	if (unlikely((uintptr_t)tctx != (uintptr_t)1U)) {
		usize = ixallocx_prof_sample(ptr, old_usize, size, extra,
		    alignment, zero, max_usize, tctx);
	} else {
		usize = ixallocx_helper(ptr, old_usize, size, extra, alignment,
		    zero);
	}
	if (unlikely(usize == old_usize)) {
		prof_alloc_rollback(tsd, tctx, false);
		return (usize);
	}
	prof_realloc(tsd, ptr, usize, tctx, false, old_usize, old_tctx);

	return (usize);
}

size_t
je_xallocx(void *ptr, size_t size, size_t extra, int flags)
{
	tsd_t *tsd;
	size_t usize, old_usize;
	UNUSED size_t old_rzsize JEMALLOC_CC_SILENCE_INIT(0);
	size_t alignment = MALLOCX_ALIGN_GET(flags);
	bool zero = flags & MALLOCX_ZERO;

	assert(ptr != NULL);
	assert(size != 0);
	assert(SIZE_T_MAX - size >= extra);
	assert(malloc_initialized || IS_INITIALIZER);
	malloc_thread_init();
	tsd = tsd_fetch();

	old_usize = isalloc(ptr, config_prof);
	if (config_valgrind && unlikely(in_valgrind))
		old_rzsize = u2rz(old_usize);

	if (config_prof && opt_prof) {
		usize = ixallocx_prof(tsd, ptr, old_usize, size, extra,
		    alignment, zero);
	} else {
		usize = ixallocx_helper(ptr, old_usize, size, extra, alignment,
		    zero);
	}
	if (unlikely(usize == old_usize))
		goto label_not_resized;

	if (config_stats) {
		*tsd_thread_allocatedp_get(tsd) += usize;
		*tsd_thread_deallocatedp_get(tsd) += old_usize;
	}
	JEMALLOC_VALGRIND_REALLOC(false, ptr, usize, false, ptr, old_usize,
	    old_rzsize, false, zero);
label_not_resized:
	UTRACE(ptr, size, ptr);
	return (usize);
}

size_t
je_sallocx(const void *ptr, int flags)
{
	size_t usize;

	assert(malloc_initialized || IS_INITIALIZER);
	malloc_thread_init();

	if (config_ivsalloc)
		usize = ivsalloc(ptr, config_prof);
	else {
		assert(ptr != NULL);
		usize = isalloc(ptr, config_prof);
	}

	return (usize);
}

void
je_dallocx(void *ptr, int flags)
{
	tsd_t *tsd;
	bool try_tcache;

	assert(ptr != NULL);
	assert(malloc_initialized || IS_INITIALIZER);

	tsd = tsd_fetch();
	if (unlikely((flags & MALLOCX_ARENA_MASK) != 0)) {
		unsigned arena_ind = MALLOCX_ARENA_GET(flags);
		arena_chunk_t *chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		arena_t *arena = arena_get(tsd, arena_ind, true, true);
		




		assert(arena != NULL);
		try_tcache = (chunk == ptr || chunk->arena != arena);
	} else
		try_tcache = true;

	UTRACE(ptr, 0, 0);
	ifree(tsd_fetch(), ptr, try_tcache);
}

JEMALLOC_ALWAYS_INLINE_C size_t
inallocx(size_t size, int flags)
{
	size_t usize;

	if (likely((flags & MALLOCX_LG_ALIGN_MASK) == 0))
		usize = s2u(size);
	else
		usize = sa2u(size, MALLOCX_ALIGN_GET_SPECIFIED(flags));
	assert(usize != 0);
	return (usize);
}

void
je_sdallocx(void *ptr, size_t size, int flags)
{
	tsd_t *tsd;
	bool try_tcache;
	size_t usize;

	assert(ptr != NULL);
	assert(malloc_initialized || IS_INITIALIZER);
	usize = inallocx(size, flags);
	assert(usize == isalloc(ptr, config_prof));

	tsd = tsd_fetch();
	if (unlikely((flags & MALLOCX_ARENA_MASK) != 0)) {
		unsigned arena_ind = MALLOCX_ARENA_GET(flags);
		arena_chunk_t *chunk = (arena_chunk_t *)CHUNK_ADDR2BASE(ptr);
		arena_t *arena = arena_get(tsd, arena_ind, true, true);
		




		try_tcache = (chunk == ptr || chunk->arena != arena);
	} else
		try_tcache = true;

	UTRACE(ptr, 0, 0);
	isfree(tsd, ptr, usize, try_tcache);
}

size_t
je_nallocx(size_t size, int flags)
{

	assert(size != 0);

	if (unlikely(malloc_init()))
		return (0);

	return (inallocx(size, flags));
}

int
je_mallctl(const char *name, void *oldp, size_t *oldlenp, void *newp,
    size_t newlen)
{

	if (unlikely(malloc_init()))
		return (EAGAIN);

	return (ctl_byname(name, oldp, oldlenp, newp, newlen));
}

int
je_mallctlnametomib(const char *name, size_t *mibp, size_t *miblenp)
{

	if (unlikely(malloc_init()))
		return (EAGAIN);

	return (ctl_nametomib(name, mibp, miblenp));
}

int
je_mallctlbymib(const size_t *mib, size_t miblen, void *oldp, size_t *oldlenp,
  void *newp, size_t newlen)
{

	if (unlikely(malloc_init()))
		return (EAGAIN);

	return (ctl_bymib(mib, miblen, oldp, oldlenp, newp, newlen));
}

void
je_malloc_stats_print(void (*write_cb)(void *, const char *), void *cbopaque,
    const char *opts)
{

	stats_print(write_cb, cbopaque, opts);
}

size_t
je_malloc_usable_size(JEMALLOC_USABLE_SIZE_CONST void *ptr)
{
	size_t ret;

	assert(malloc_initialized || IS_INITIALIZER);
	malloc_thread_init();

	if (config_ivsalloc)
		ret = ivsalloc(ptr, config_prof);
	else
		ret = (ptr != NULL) ? isalloc(ptr, config_prof) : 0;

	return (ret);
}























JEMALLOC_ATTR(constructor)
static void
jemalloc_constructor(void)
{

	malloc_init();
}

#ifndef JEMALLOC_MUTEX_INIT_CB
void
jemalloc_prefork(void)
#else
JEMALLOC_EXPORT void
_malloc_prefork(void)
#endif
{
	unsigned i;

#ifdef JEMALLOC_MUTEX_INIT_CB
	if (!malloc_initialized)
		return;
#endif
	assert(malloc_initialized);

	
	ctl_prefork();
	prof_prefork();
	malloc_mutex_prefork(&arenas_lock);
	for (i = 0; i < narenas_total; i++) {
		if (arenas[i] != NULL)
			arena_prefork(arenas[i]);
	}
	chunk_prefork();
	base_prefork();
	huge_prefork();
}

#ifndef JEMALLOC_MUTEX_INIT_CB
void
jemalloc_postfork_parent(void)
#else
JEMALLOC_EXPORT void
_malloc_postfork(void)
#endif
{
	unsigned i;

#ifdef JEMALLOC_MUTEX_INIT_CB
	if (!malloc_initialized)
		return;
#endif
	assert(malloc_initialized);

	
	huge_postfork_parent();
	base_postfork_parent();
	chunk_postfork_parent();
	for (i = 0; i < narenas_total; i++) {
		if (arenas[i] != NULL)
			arena_postfork_parent(arenas[i]);
	}
	malloc_mutex_postfork_parent(&arenas_lock);
	prof_postfork_parent();
	ctl_postfork_parent();
}

void
jemalloc_postfork_child(void)
{
	unsigned i;

	assert(malloc_initialized);

	
	huge_postfork_child();
	base_postfork_child();
	chunk_postfork_child();
	for (i = 0; i < narenas_total; i++) {
		if (arenas[i] != NULL)
			arena_postfork_child(arenas[i]);
	}
	malloc_mutex_postfork_child(&arenas_lock);
	prof_postfork_child();
	ctl_postfork_child();
}


