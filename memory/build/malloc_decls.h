











#ifndef malloc_decls_h
#  define malloc_decls_h

#  include "jemalloc_types.h"

typedef MALLOC_USABLE_SIZE_CONST_PTR void * usable_ptr_t;

#  define MALLOC_FUNCS_MALLOC 1
#  define MALLOC_FUNCS_JEMALLOC 2
#  define MALLOC_FUNCS_INIT 4
#  define MALLOC_FUNCS_BRIDGE 8
#  define MALLOC_FUNCS_ALL (MALLOC_FUNCS_INIT | MALLOC_FUNCS_BRIDGE | \
                            MALLOC_FUNCS_MALLOC | MALLOC_FUNCS_JEMALLOC)

#endif 

#ifndef MALLOC_FUNCS
#  define MALLOC_FUNCS (MALLOC_FUNCS_MALLOC | MALLOC_FUNCS_JEMALLOC)
#endif

#ifdef MALLOC_DECL
#  if MALLOC_FUNCS & MALLOC_FUNCS_INIT
MALLOC_DECL(init, void, const malloc_table_t *)
#  endif
#  if MALLOC_FUNCS & MALLOC_FUNCS_BRIDGE
MALLOC_DECL(get_bridge, struct ReplaceMallocBridge*, void)
#  endif
#  if MALLOC_FUNCS & MALLOC_FUNCS_MALLOC
MALLOC_DECL(malloc, void *, size_t)
MALLOC_DECL(posix_memalign, int, void **, size_t, size_t)
MALLOC_DECL(aligned_alloc, void *, size_t, size_t)
MALLOC_DECL(calloc, void *, size_t, size_t)
MALLOC_DECL(realloc, void *, void *, size_t)
MALLOC_DECL(free, void, void *)
MALLOC_DECL(memalign, void *, size_t, size_t)
MALLOC_DECL(valloc, void *, size_t)
MALLOC_DECL(malloc_usable_size, size_t, usable_ptr_t)
MALLOC_DECL(malloc_good_size, size_t, size_t)
#  endif
#  if MALLOC_FUNCS & MALLOC_FUNCS_JEMALLOC
MALLOC_DECL(jemalloc_stats, void, jemalloc_stats_t *)
MALLOC_DECL(jemalloc_purge_freed_pages, void, void)
MALLOC_DECL(jemalloc_free_dirty_pages, void, void)
#  endif
#endif 

#undef MALLOC_DECL
#undef MALLOC_FUNCS
