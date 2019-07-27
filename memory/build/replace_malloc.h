



#ifndef replace_malloc_h
#define replace_malloc_h






















































#ifdef replace_malloc_bridge_h
#error Do not include replace_malloc_bridge.h before replace_malloc.h. \
  In fact, you only need the latter.
#endif

#define REPLACE_MALLOC_IMPL

#include "replace_malloc_bridge.h"


#define MOZ_NO_MOZALLOC 1

#include "mozilla/Types.h"

MOZ_BEGIN_EXTERN_C




#ifndef MOZ_NO_REPLACE_FUNC_DECL

#  ifndef MOZ_REPLACE_WEAK
#    define MOZ_REPLACE_WEAK
#  endif

#  define MALLOC_DECL(name, return_type, ...) \
    MOZ_EXPORT return_type replace_ ## name(__VA_ARGS__) MOZ_REPLACE_WEAK;

#  define MALLOC_FUNCS MALLOC_FUNCS_ALL
#  include "malloc_decls.h"

#endif 








#ifdef MOZ_REPLACE_ONLY_MEMALIGN
#include <errno.h>

int replace_posix_memalign(void **ptr, size_t alignment, size_t size)
{
  if (size == 0) {
    *ptr = NULL;
    return 0;
  }
  
  if (((alignment - 1) & alignment) != 0 || (alignment % sizeof(void *)))
    return EINVAL;
  *ptr = replace_memalign(alignment, size);
  return *ptr ? 0 : ENOMEM;
}

void *replace_aligned_alloc(size_t alignment, size_t size)
{
  
  if (size % alignment)
    return NULL;
  return replace_memalign(alignment, size);
}

void *replace_valloc(size_t size)
{
  return replace_memalign(PAGE_SIZE, size);
}
#endif

MOZ_END_EXTERN_C

#endif 
