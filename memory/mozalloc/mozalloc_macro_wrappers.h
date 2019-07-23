







































#ifndef mozilla_mozalloc_macro_wrappers_h
#define mozilla_mozalloc_macro_wrappers_h









#define malloc(_) moz_malloc(_)

#define calloc(_, __) moz_calloc(_, __)

#define realloc(_, __) moz_realloc(_, __)

#define strdup(_) moz_strdup(_)

#if defined(HAVE_STRNDUP)
#define strndup(_, __) moz_strndup(_, __)
#endif

#if defined(HAVE_POSIX_MEMALIGN)
#define posix_memalign(_, __, ___) moz_posix_memalign(_, __, ___)
#endif

#if defined(HAVE_MEMALIGN)
#define memalign(_, __) moz_memalign(_, __)
#endif

#if defined(HAVE_VALLOC)
#define valloc(_) moz_valloc(_)
#endif


#endif 
