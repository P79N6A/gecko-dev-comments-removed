













































#ifndef mozilla_mozalloc_macro_wrappers_h
#  error "mozalloc macro wrappers haven't been defined"
#endif





#undef mozilla_mozalloc_macro_wrappers_h

#undef free
#undef malloc
#undef calloc
#undef realloc
#undef strdup

#if defined(HAVE_STRNDUP)
#  undef strndup
#endif

#if defined(HAVE_POSIX_MEMALIGN) || defined(HAVE_JEMALLOC_POSIX_MEMALIGN)
#  undef posix_memalign
#endif

#if defined(HAVE_MEMALIGN) || defined(HAVE_JEMALLOC_MEMALIGN)
#  undef memalign
#endif

#if defined(HAVE_VALLOC)
#  undef valloc
#endif
