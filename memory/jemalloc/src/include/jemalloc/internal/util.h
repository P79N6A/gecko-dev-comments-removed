
#ifdef JEMALLOC_H_TYPES


#define	BUFERROR_BUF		64





#define	MALLOC_PRINTF_BUFSIZE	4096





#define	JEMALLOC_ARG_CONCAT(...) __VA_ARGS__






#ifdef JEMALLOC_CC_SILENCE
#  define JEMALLOC_CC_SILENCE_INIT(v) = v
#else
#  define JEMALLOC_CC_SILENCE_INIT(v)
#endif





#ifndef assert
#define	assert(e) do {							\
	if (config_debug && !(e)) {					\
		malloc_printf(						\
		    "<jemalloc>: %s:%d: Failed assertion: \"%s\"\n",	\
		    __FILE__, __LINE__, #e);				\
		abort();						\
	}								\
} while (0)
#endif

#ifndef not_reached
#define	not_reached() do {						\
	if (config_debug) {						\
		malloc_printf(						\
		    "<jemalloc>: %s:%d: Unreachable code reached\n",	\
		    __FILE__, __LINE__);				\
		abort();						\
	}								\
} while (0)
#endif

#ifndef not_implemented
#define	not_implemented() do {						\
	if (config_debug) {						\
		malloc_printf("<jemalloc>: %s:%d: Not implemented\n",	\
		    __FILE__, __LINE__);				\
		abort();						\
	}								\
} while (0)
#endif

#ifndef assert_not_implemented
#define	assert_not_implemented(e) do {					\
	if (config_debug && !(e))					\
		not_implemented();					\
} while (0)
#endif


#define	cassert(c) do {							\
	if ((c) == false)						\
		not_reached();						\
} while (0)

#endif 

#ifdef JEMALLOC_H_STRUCTS

#endif 

#ifdef JEMALLOC_H_EXTERNS

int	buferror(int err, char *buf, size_t buflen);
uintmax_t	malloc_strtoumax(const char *restrict nptr,
    char **restrict endptr, int base);
void	malloc_write(const char *s);





int	malloc_vsnprintf(char *str, size_t size, const char *format,
    va_list ap);
int	malloc_snprintf(char *str, size_t size, const char *format, ...)
    JEMALLOC_ATTR(format(printf, 3, 4));
void	malloc_vcprintf(void (*write_cb)(void *, const char *), void *cbopaque,
    const char *format, va_list ap);
void malloc_cprintf(void (*write)(void *, const char *), void *cbopaque,
    const char *format, ...) JEMALLOC_ATTR(format(printf, 3, 4));
void	malloc_printf(const char *format, ...)
    JEMALLOC_ATTR(format(printf, 1, 2));

#endif 

#ifdef JEMALLOC_H_INLINES

#ifndef JEMALLOC_ENABLE_INLINE
size_t	pow2_ceil(size_t x);
void	set_errno(int errnum);
int	get_errno(void);
#endif

#if (defined(JEMALLOC_ENABLE_INLINE) || defined(JEMALLOC_UTIL_C_))

JEMALLOC_INLINE size_t
pow2_ceil(size_t x)
{

	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
#if (LG_SIZEOF_PTR == 3)
	x |= x >> 32;
#endif
	x++;
	return (x);
}


JEMALLOC_INLINE void
set_errno(int errnum)
{

#ifdef _WIN32
	SetLastError(errnum);
#else
	errno = errnum;
#endif
}


JEMALLOC_INLINE int
get_errno(void)
{

#ifdef _WIN32
	return (GetLastError());
#else
	return (errno);
#endif
}
#endif

#endif 

