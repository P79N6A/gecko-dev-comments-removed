
































#include <sys/queue.h>








#define	HASHSIZE	128
#define	HASHKEY(pgno)	((pgno - 1) % HASHSIZE)


typedef struct _bkt {
	CIRCLEQ_ENTRY(_bkt) hq;		
	CIRCLEQ_ENTRY(_bkt) q;		
	void    *page;			
	pgno_t   pgno;			

#define	MPOOL_DIRTY	0x01		/* page needs to be written */
#define	MPOOL_PINNED	0x02		/* page is pinned into memory */
	uint8 flags;			
} BKT;

typedef struct MPOOL {
	CIRCLEQ_HEAD(_lqh, _bkt) lqh;	
					
	CIRCLEQ_HEAD(_hqh, _bkt) hqh[HASHSIZE];
	pgno_t	curcache;		
	pgno_t	maxcache;		
	pgno_t	npages;			
	uint32	pagesize;		
	int	fd;			
					
	void    (*pgin) (void *, pgno_t, void *);
					
	void    (*pgout) (void *, pgno_t, void *);
	void	*pgcookie;		
#ifdef STATISTICS
	uint32	cachehit;
	uint32	cachemiss;
	uint32	pagealloc;
	uint32	pageflush;
	uint32	pageget;
	uint32	pagenew;
	uint32	pageput;
	uint32	pageread;
	uint32	pagewrite;
#endif
} MPOOL;

__BEGIN_DECLS
MPOOL	*mpool_open (void *, int, pgno_t, pgno_t);
void	 mpool_filter (MPOOL *, void (*)(void *, pgno_t, void *),
	    void (*)(void *, pgno_t, void *), void *);
void	*mpool_new (MPOOL *, pgno_t *);
void	*mpool_get (MPOOL *, pgno_t, uint);
int	 mpool_put (MPOOL *, void *, uint);
int	 mpool_sync (MPOOL *);
int	 mpool_close (MPOOL *);
#ifdef STATISTICS
void	 mpool_stat (MPOOL *);
#endif
__END_DECLS
