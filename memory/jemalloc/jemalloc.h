#ifndef MOZ_MEMORY_WINDOWS
#  include <stdbool.h>
#else
#  include <windows.h>
#  ifndef bool
#    define bool BOOL
#  endif
#endif

extern const char	*_malloc_options;






typedef struct {
	


	bool	opt_abort;	
	bool	opt_junk;	
	bool	opt_utrace;	
	bool	opt_sysv;	
	bool	opt_xmalloc;	
	bool	opt_zero;	
	size_t	narenas;	
	size_t	balance_threshold; 
	size_t	quantum;	
	size_t	small_max;	
	size_t	large_max;	
	size_t	chunksize;	
	size_t	dirty_max;	
	size_t	reserve_min;	
	size_t	reserve_max;	

	


	size_t	mapped;		
	size_t	committed;	
	size_t	allocated;	
	size_t	dirty;		
	size_t	reserve_cur;	
} jemalloc_stats_t;

#ifndef MOZ_MEMORY_DARWIN
void	*malloc(size_t size);
void	*valloc(size_t size);
void	*calloc(size_t num, size_t size);
void	*realloc(void *ptr, size_t size);
void	free(void *ptr);
#endif

int	posix_memalign(void **memptr, size_t alignment, size_t size);
void	*memalign(size_t alignment, size_t size);
size_t	malloc_usable_size(const void *ptr);
void	jemalloc_stats(jemalloc_stats_t *stats);


void	*xmalloc(size_t size);
void	*xcalloc(size_t num, size_t size);
void	*xrealloc(void *ptr, size_t size);
void	*xmemalign(size_t alignment, size_t size);
















































typedef enum {
	RESERVE_CND_LOW,
	RESERVE_CND_CRIT,
	RESERVE_CND_FAIL
} reserve_cnd_t;









typedef void reserve_cb_t(void *ctx, reserve_cnd_t cnd, size_t size);











bool	reserve_cb_register(reserve_cb_t *cb, void *ctx);












bool	reserve_cb_unregister(reserve_cb_t *cb, void *ctx);






size_t	reserve_cur_get(void);







size_t	reserve_min_get(void);









bool	reserve_min_set(size_t min);
