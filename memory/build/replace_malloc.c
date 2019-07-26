



#ifndef MOZ_MEMORY
#  error Should not compile this file when MOZ_MEMORY is not set
#endif

#ifndef MOZ_REPLACE_MALLOC
#  error Should not compile this file when replace-malloc is disabled
#endif

#ifdef MOZ_NATIVE_JEMALLOC
#  error Should not compile this file when we want to use native jemalloc
#endif

#include "mozmemory_wrap.h"


#define MALLOC_DECL(name, return_type, ...) \
  return_type je_ ## name(__VA_ARGS__);
#include "malloc_decls.h"

#include "mozilla/Likely.h"










#ifdef XP_DARWIN
#  define MOZ_REPLACE_WEAK __attribute__((weak_import))
#elif defined(XP_WIN) || defined(MOZ_WIDGET_ANDROID)
#  define MOZ_NO_REPLACE_FUNC_DECL
#elif defined(__GNUC__)
#  define MOZ_REPLACE_WEAK __attribute__((weak))
#endif

#include "replace_malloc.h"

#define MALLOC_DECL(name, return_type, ...) \
    je_ ## name,

static const malloc_table_t malloc_table = {
#include "malloc_decls.h"
};

#ifdef MOZ_NO_REPLACE_FUNC_DECL
#  define MALLOC_DECL(name, return_type, ...) \
    typedef return_type (replace_ ## name ## _impl_t)(__VA_ARGS__); \
    replace_ ## name ## _impl_t *replace_ ## name = NULL;
#  define MALLOC_FUNCS MALLOC_FUNCS_ALL
#  include "malloc_decls.h"

#  ifdef XP_WIN
#    include <windows.h>
static void
replace_malloc_init_funcs()
{
  char replace_malloc_lib[1024];
  if (GetEnvironmentVariableA("MOZ_REPLACE_MALLOC_LIB", (LPSTR)&replace_malloc_lib,
                              sizeof(replace_malloc_lib)) > 0) {
    HMODULE handle = LoadLibraryA(replace_malloc_lib);
    if (handle) {
#define MALLOC_DECL(name, ...) \
  replace_ ## name = (replace_ ## name ## _impl_t *) GetProcAddress(handle, "replace_" # name);

#  define MALLOC_FUNCS MALLOC_FUNCS_ALL
#include "malloc_decls.h"
    }
  }
}
#  elif defined(MOZ_WIDGET_ANDROID)
#    include <dlfcn.h>
static void
replace_malloc_init_funcs()
{
  char *replace_malloc_lib = getenv("MOZ_REPLACE_MALLOC_LIB");
  if (replace_malloc_lib && *replace_malloc_lib) {
    void *handle = dlopen(replace_malloc_lib, RTLD_LAZY);
    if (handle) {
#define MALLOC_DECL(name, ...) \
  replace_ ## name = (replace_ ## name ## _impl_t *) dlsym(handle, "replace_" # name);

#  define MALLOC_FUNCS MALLOC_FUNCS_ALL
#include "malloc_decls.h"
    }
  }
}
#  else
#    error No implementation for replace_malloc_init_funcs()
#  endif

#endif 












#ifdef XP_DARWIN
#undef MOZ_MEMORY_API
#define MOZ_MEMORY_API static
#endif





#define MALLOC_DECL(name, return_type, ...) \
  MOZ_MEMORY_API return_type name ## _impl(__VA_ARGS__);
#define MALLOC_FUNCS MALLOC_FUNCS_MALLOC
#include "malloc_decls.h"

#define MALLOC_DECL(name, return_type, ...) \
  MOZ_JEMALLOC_API return_type name ## _impl(__VA_ARGS__);
#define MALLOC_FUNCS MALLOC_FUNCS_JEMALLOC
#include "malloc_decls.h"

static int replace_malloc_initialized = 0;
static void
init()
{
#ifdef MOZ_NO_REPLACE_FUNC_DECL
  replace_malloc_init_funcs();
#endif
  
  
  replace_malloc_initialized = 1;
  if (replace_init)
    replace_init(&malloc_table);
}

void*
malloc_impl(size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_malloc))
    return je_malloc(size);
  return replace_malloc(size);
}

int
posix_memalign_impl(void **memptr, size_t alignment, size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_posix_memalign))
    return je_posix_memalign(memptr, alignment, size);
  return replace_posix_memalign(memptr, alignment, size);
}

void*
aligned_alloc_impl(size_t alignment, size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_aligned_alloc))
    return je_aligned_alloc(alignment, size);
  return replace_aligned_alloc(alignment, size);
}

void*
calloc_impl(size_t num, size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_calloc))
    return je_calloc(num, size);
  return replace_calloc(num, size);
}

void*
realloc_impl(void *ptr, size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_realloc))
    return je_realloc(ptr, size);
  return replace_realloc(ptr, size);
}

void
free_impl(void *ptr)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_free))
    je_free(ptr);
  else
    replace_free(ptr);
}

void*
memalign_impl(size_t alignment, size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_memalign))
    return je_memalign(alignment, size);
  return replace_memalign(alignment, size);
}

void*
valloc_impl(size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_valloc))
    return je_valloc(size);
  return replace_valloc(size);
}

size_t
malloc_usable_size_impl(usable_ptr_t ptr)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_malloc_usable_size))
    return je_malloc_usable_size(ptr);
  return replace_malloc_usable_size(ptr);
}

size_t
malloc_good_size_impl(size_t size)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_malloc_good_size))
    return je_malloc_good_size(size);
  return replace_malloc_good_size(size);
}

void
jemalloc_stats_impl(jemalloc_stats_t *stats)
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_jemalloc_stats))
    je_jemalloc_stats(stats);
  else
    replace_jemalloc_stats(stats);
}

void
jemalloc_purge_freed_pages_impl()
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_jemalloc_purge_freed_pages))
    je_jemalloc_purge_freed_pages();
  else
    replace_jemalloc_purge_freed_pages();
}

void
jemalloc_free_dirty_pages_impl()
{
  if (MOZ_UNLIKELY(!replace_malloc_initialized))
    init();
  if (MOZ_LIKELY(!replace_jemalloc_free_dirty_pages))
    je_jemalloc_free_dirty_pages();
  else
    replace_jemalloc_free_dirty_pages();
}


#if defined(__GLIBC__) && !defined(__UCLIBC__)











typedef void (* __free_hook_type)(void *ptr);
typedef void *(* __malloc_hook_type)(size_t size);
typedef void *(* __realloc_hook_type)(void *ptr, size_t size);
typedef void *(* __memalign_hook_type)(size_t alignment, size_t size);

MOZ_MEMORY_API __free_hook_type __free_hook = free_impl;
MOZ_MEMORY_API __malloc_hook_type __malloc_hook = malloc_impl;
MOZ_MEMORY_API __realloc_hook_type __realloc_hook = realloc_impl;
MOZ_MEMORY_API __memalign_hook_type __memalign_hook = memalign_impl;

#endif









#ifdef XP_DARWIN
#include <stdlib.h>
#include <malloc/malloc.h>
#include "mozilla/Assertions.h"

static size_t
zone_size(malloc_zone_t *zone, void *ptr)
{
  return malloc_usable_size_impl(ptr);
}

static void *
zone_malloc(malloc_zone_t *zone, size_t size)
{
  return malloc_impl(size);
}

static void *
zone_calloc(malloc_zone_t *zone, size_t num, size_t size)
{
  return calloc_impl(num, size);
}

static void *
zone_realloc(malloc_zone_t *zone, void *ptr, size_t size)
{
  if (malloc_usable_size_impl(ptr))
    return realloc_impl(ptr, size);
  return realloc(ptr, size);
}

static void
zone_free(malloc_zone_t *zone, void *ptr)
{
  if (malloc_usable_size_impl(ptr)) {
    free_impl(ptr);
    return;
  }
  free(ptr);
}

static void
zone_free_definite_size(malloc_zone_t *zone, void *ptr, size_t size)
{
  size_t current_size = malloc_usable_size_impl(ptr);
  if (current_size) {
    MOZ_ASSERT(current_size == size);
    free_impl(ptr);
    return;
  }
  free(ptr);
}

static void *
zone_memalign(malloc_zone_t *zone, size_t alignment, size_t size)
{
  void *ptr;
  if (posix_memalign_impl(&ptr, alignment, size) == 0)
    return ptr;
  return NULL;
}

static void *
zone_valloc(malloc_zone_t *zone, size_t size)
{
  return valloc_impl(size);
}

static void *
zone_destroy(malloc_zone_t *zone)
{
  
  MOZ_CRASH();
}

static size_t
zone_good_size(malloc_zone_t *zone, size_t size)
{
  return malloc_good_size_impl(size);
}

#ifdef MOZ_JEMALLOC

#include "jemalloc/internal/jemalloc_internal.h"

static void
zone_force_lock(malloc_zone_t *zone)
{
  

  if (isthreaded)
    jemalloc_prefork();
}

static void
zone_force_unlock(malloc_zone_t *zone)
{
  

  if (isthreaded)
    jemalloc_postfork_parent();
}

#else

#define JEMALLOC_ZONE_VERSION 6



static void
zone_force_lock(malloc_zone_t *zone)
{
}

static void
zone_force_unlock(malloc_zone_t *zone)
{
}

#endif

static malloc_zone_t zone;
static struct malloc_introspection_t zone_introspect;

__attribute__((constructor)) void
register_zone(void)
{
  zone.size = (void *)zone_size;
  zone.malloc = (void *)zone_malloc;
  zone.calloc = (void *)zone_calloc;
  zone.valloc = (void *)zone_valloc;
  zone.free = (void *)zone_free;
  zone.realloc = (void *)zone_realloc;
  zone.destroy = (void *)zone_destroy;
  zone.zone_name = "replace_malloc_zone";
  zone.batch_malloc = NULL;
  zone.batch_free = NULL;
  zone.introspect = &zone_introspect;
  zone.version = JEMALLOC_ZONE_VERSION;
  zone.memalign = zone_memalign;
  zone.free_definite_size = zone_free_definite_size;
#if (JEMALLOC_ZONE_VERSION >= 8)
  zone.pressure_relief = NULL;
#endif
  zone_introspect.enumerator = NULL;
  zone_introspect.good_size = (void *)zone_good_size;
  zone_introspect.check = NULL;
  zone_introspect.print = NULL;
  zone_introspect.log = NULL;
  zone_introspect.force_lock = (void *)zone_force_lock;
  zone_introspect.force_unlock = (void *)zone_force_unlock;
  zone_introspect.statistics = NULL;
  zone_introspect.zone_locked = NULL;
#if (JEMALLOC_ZONE_VERSION >= 7)
  zone_introspect.enable_discharge_checking = NULL;
  zone_introspect.disable_discharge_checking = NULL;
  zone_introspect.discharge = NULL;
#ifdef __BLOCKS__
  zone_introspect.enumerate_discharged_pointers = NULL;
#else
  zone_introspect.enumerate_unavailable_without_blocks = NULL;
#endif
#endif

  








  malloc_zone_t *purgeable_zone = malloc_default_purgeable_zone();

  
  malloc_zone_register(&zone);

  do {
    malloc_zone_t *default_zone = malloc_default_zone();
    







    malloc_zone_unregister(default_zone);
    malloc_zone_register(default_zone);
    










    malloc_zone_unregister(purgeable_zone);
    malloc_zone_register(purgeable_zone);
  } while (malloc_default_zone() != &zone);
}
#endif
