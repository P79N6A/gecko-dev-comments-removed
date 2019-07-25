






























#ifndef _JEMALLOC_H_
#define _JEMALLOC_H_

#include "jemalloc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char *_malloc_options;


#if (!defined(MOZ_MEMORY_DARWIN) && !defined(MOZ_MEMORY_LINUX))
void	*malloc(size_t size);
void	*valloc(size_t size);
void	*calloc(size_t num, size_t size);
void	*realloc(void *ptr, size_t size);
void	free(void *ptr);
int	posix_memalign(void **memptr, size_t alignment, size_t size);
#endif 

#if defined(MOZ_MEMORY_ANDROID) || defined(WRAP_MALLOC) || defined(WIN32_NEW_STYLE_JEMALLOC)
void	*je_malloc(size_t size);
void	*je_valloc(size_t size);
void	*je_calloc(size_t num, size_t size);
void	*je_realloc(void *ptr, size_t size);
void	je_free(void *ptr);
void *je_memalign(size_t alignment, size_t size);
int	je_posix_memalign(void **memptr, size_t alignment, size_t size);
char    *je_strndup(const char *src, size_t len);
char    *je_strdup(const char *src);
#if defined(WIN32_NEW_STYLE_JEMALLOC)
size_t  je_malloc_usable_size(void *ptr)
#endif
#endif


#if !defined(MOZ_MEMORY_LINUX)
void	*memalign(size_t alignment, size_t size);
size_t	malloc_usable_size(const void *ptr);
#endif 

void	jemalloc_stats(jemalloc_stats_t *stats);

#ifdef __cplusplus
} 
#endif

#endif
