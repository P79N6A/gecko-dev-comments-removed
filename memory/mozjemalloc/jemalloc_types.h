






























#ifndef _JEMALLOC_TYPES_H_
#define _JEMALLOC_TYPES_H_


#ifdef _MSC_VER
#include <crtdefs.h>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char jemalloc_bool;






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

#ifdef __cplusplus
} 
#endif

#endif
