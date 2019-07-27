
































#ifndef _USER_MALLOC_H_
#define	_USER_MALLOC_H_


#include <stdlib.h>
#include <sys/types.h>
#if !defined (__Userspace_os_Windows)
#include <strings.h>
#include <stdint.h>
#else
#if defined(_MSC_VER) && _MSC_VER >= 1600
#include <stdint.h>
#elif defined(SCTP_STDINT_INCLUDE)
#include SCTP_STDINT_INCLUDE
#else
#define uint32_t unsigned __int32
#define uint64_t unsigned __int64
#endif
#include <winsock2.h>
#endif

#define	MINALLOCSIZE	UMA_SMALLEST_UNIT




#define	M_NOWAIT	0x0001		/* do not block */
#define	M_WAITOK	0x0002		/* ok to block */
#define	M_ZERO		0x0100		/* bzero the allocation */
#define	M_NOVM		0x0200		/* don't ask VM for pages */
#define	M_USE_RESERVE	0x0400		/* can alloc out of reserve memory */

#define	M_MAGIC		877983977	/* time when first defined :-) */

















struct malloc_type_stats {
	uint64_t	mts_memalloced;	
	uint64_t	mts_memfreed;	
	uint64_t	mts_numallocs;	
	uint64_t	mts_numfrees;	
	uint64_t	mts_size;	
	uint64_t	_mts_reserved1;	
	uint64_t	_mts_reserved2;	
	uint64_t	_mts_reserved3;	
};

#ifndef MAXCPU 
#define MAXCPU 4 /* arbitrary? */
#endif

struct malloc_type_internal {
	struct malloc_type_stats	mti_stats[MAXCPU];
};







struct malloc_type {
	struct malloc_type *ks_next;	
	u_long		 _ks_memuse;	
	u_long		 _ks_size;	
	u_long		 _ks_inuse;	
	uint64_t	 _ks_calls;	
	u_long		 _ks_maxused;	
	u_long		 ks_magic;	
	const char	*ks_shortdesc;	

	





	void		*ks_handle;	
	const char	*_lo_name;
	const char	*_lo_type;
	u_int		 _lo_flags;
	void		*_lo_list_next;
	struct witness	*_lo_witness;
	uintptr_t	 _mtx_lock;
	u_int		 _mtx_recurse;
};








#define	MALLOC_TYPE_STREAM_VERSION	0x00000001
struct malloc_type_stream_header {
	uint32_t	mtsh_version;	
	uint32_t	mtsh_maxcpus;	
	uint32_t	mtsh_count;	
	uint32_t	_mtsh_pad;	
};

#define	MALLOC_MAX_NAME	32
struct malloc_type_header {
	char				mth_name[MALLOC_MAX_NAME];
};















#define	MALLOC_DEFINE(type, shortdesc, longdesc)			\
	struct malloc_type type[1] = {					\
		{ NULL, 0, 0, 0, 0, 0, M_MAGIC, shortdesc, NULL, NULL,	\
		    NULL, 0, NULL, NULL, 0, 0 }				\
	}














#define	MALLOC_DECLARE(type) \
	extern struct malloc_type type[1]

#define	FREE(addr, type) free((addr))



#define	MALLOC(space, cast, size, type, flags)                          \
    ((space) = (cast)malloc((u_long)(size)));                           \
    do {								\
        if(flags & M_ZERO) {                                            \
	  memset(space,0,size);                                         \
	}								\
    } while (0);




#if 0
#ifdef _KERNEL
#define	MALLOC_DEFINE(type, shortdesc, longdesc)			\
	struct malloc_type type[1] = {					\
		{ NULL, 0, 0, 0, 0, 0, M_MAGIC, shortdesc, NULL, NULL,	\
		    NULL, 0, NULL, NULL, 0, 0 }				\
	};								\
	SYSINIT(type##_init, SI_SUB_KMEM, SI_ORDER_SECOND, malloc_init,	\
	    type);							\
	SYSUNINIT(type##_uninit, SI_SUB_KMEM, SI_ORDER_ANY,		\
	    malloc_uninit, type)


#define	MALLOC_DECLARE(type) \
	extern struct malloc_type type[1]

MALLOC_DECLARE(M_CACHE);
MALLOC_DECLARE(M_DEVBUF);
MALLOC_DECLARE(M_TEMP);

MALLOC_DECLARE(M_IP6OPT); 
MALLOC_DECLARE(M_IP6NDP); 




#define	MALLOC(space, cast, size, type, flags) \
	((space) = (cast)malloc((u_long)(size), (type), (flags)))
#define	FREE(addr, type) free((addr), (type))






MALLOC_DECLARE(M_IOV);

extern struct mtx malloc_mtx;


void	contigfree(void *addr, unsigned long size, struct malloc_type *type);
void	*contigmalloc(unsigned long size, struct malloc_type *type, int flags,
	    vm_paddr_t low, vm_paddr_t high, unsigned long alignment,
	    unsigned long boundary);
void	free(void *addr, struct malloc_type *type);
void	*malloc(unsigned long size, struct malloc_type *type, int flags);
void	malloc_init(void *);
int	malloc_last_fail(void);
void	malloc_type_allocated(struct malloc_type *type, unsigned long size);
void	malloc_type_freed(struct malloc_type *type, unsigned long size);
void	malloc_uninit(void *);
void	*realloc(void *addr, unsigned long size, struct malloc_type *type,
	    int flags);
void	*reallocf(void *addr, unsigned long size, struct malloc_type *type,
	    int flags);


#endif 
#endif

#endif 
