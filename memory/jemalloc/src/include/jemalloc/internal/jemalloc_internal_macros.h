








#if defined(JEMALLOC_DEBUG) || defined(JEMALLOC_CODE_COVERAGE)
   
#  define JEMALLOC_ALWAYS_INLINE
#  define JEMALLOC_ALWAYS_INLINE_C static
#  define JEMALLOC_INLINE
#  define JEMALLOC_INLINE_C static
#  define inline
#else
#  define JEMALLOC_ENABLE_INLINE
#  ifdef JEMALLOC_HAVE_ATTR
#    define JEMALLOC_ALWAYS_INLINE \
	 static inline JEMALLOC_ATTR(unused) JEMALLOC_ATTR(always_inline)
#    define JEMALLOC_ALWAYS_INLINE_C \
	 static inline JEMALLOC_ATTR(always_inline)
#  else
#    define JEMALLOC_ALWAYS_INLINE static inline
#    define JEMALLOC_ALWAYS_INLINE_C static inline
#  endif
#  define JEMALLOC_INLINE static inline
#  define JEMALLOC_INLINE_C static inline
#  ifdef _MSC_VER
#    define inline _inline
#  endif
#endif

#ifdef JEMALLOC_CC_SILENCE
#  define UNUSED JEMALLOC_ATTR(unused)
#else
#  define UNUSED
#endif

#define	ZU(z)	((size_t)z)
#define	QU(q)	((uint64_t)q)
#define	QI(q)	((int64_t)q)

#ifndef __DECONST
#  define	__DECONST(type, var)	((type)(uintptr_t)(const void *)(var))
#endif

#ifndef JEMALLOC_HAS_RESTRICT
#  define restrict
#endif
