






























#ifndef _JEMALLOC_H_
#define _JEMALLOC_H_


#ifdef _MSC_VER
#include <crtdefs.h>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char jemalloc_bool;

extern const char *_malloc_options;






typedef struct {
	


	jemalloc_bool	opt_abort;	
	jemalloc_bool	opt_junk;	
	jemalloc_bool	opt_utrace;	
	jemalloc_bool	opt_sysv;	
	jemalloc_bool	opt_xmalloc;	
	jemalloc_bool	opt_zero;	
	size_t	narenas;	
	size_t	balance_threshold; 
	size_t	quantum;	
	size_t	small_max;	
	size_t	large_max;	
	size_t	chunksize;	
	size_t	dirty_max;	

	


	size_t	mapped;		
	size_t	committed;	
	size_t	allocated;	
	size_t	dirty;		
} jemalloc_stats_t;


#if (!defined(MOZ_MEMORY_DARWIN) && !defined(MOZ_MEMORY_LINUX))
void	*malloc(size_t size);
void	*valloc(size_t size);
void	*calloc(size_t num, size_t size);
void	*realloc(void *ptr, size_t size);
void	free(void *ptr);
int	posix_memalign(void **memptr, size_t alignment, size_t size);
#endif 

#if defined(MOZ_MEMORY_ANDROID) || defined(WRAP_MALLOC)
void	*je_malloc(size_t size);
void	*je_valloc(size_t size);
void	*je_calloc(size_t num, size_t size);
void	*je_realloc(void *ptr, size_t size);
void	je_free(void *ptr);
int	je_posix_memalign(void **memptr, size_t alignment, size_t size);
char    *je_strndup(const char *src, size_t len);
char    *je_strdup(const char *src);
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
