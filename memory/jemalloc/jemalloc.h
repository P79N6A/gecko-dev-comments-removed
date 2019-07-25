






























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


#ifdef MOZ_MEMORY_ANDROID
int	posix_memalign(void **memptr, size_t alignment, size_t size);


void    _malloc_prefork(void);
void    _malloc_postfork(void);
#endif

#if defined(MOZ_MEMORY_DARWIN) || defined(MOZ_MEMORY_WINDOWS)
void	*je_malloc(size_t size);
void	*je_valloc(size_t size);
void	*je_calloc(size_t num, size_t size);
void	*je_realloc(void *ptr, size_t size);
void	je_free(void *ptr);
void *je_memalign(size_t alignment, size_t size);
int	je_posix_memalign(void **memptr, size_t alignment, size_t size);
char    *je_strndup(const char *src, size_t len);
char    *je_strdup(const char *src);
size_t	je_malloc_usable_size(const void *ptr);
#endif


#if !defined(MOZ_MEMORY_LINUX)
void	*memalign(size_t alignment, size_t size);
size_t	malloc_usable_size(const void *ptr);
#endif 

void	jemalloc_stats(jemalloc_stats_t *stats);


size_t	je_malloc_usable_size_in_advance(size_t size);


























void    jemalloc_purge_freed_pages();

#ifdef __cplusplus
} 
#endif

#endif
