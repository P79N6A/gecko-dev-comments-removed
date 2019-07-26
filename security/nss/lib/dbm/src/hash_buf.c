

































#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)hash_buf.c	8.5 (Berkeley) 7/15/94";
#endif 
















#if !defined(_WIN32) && !defined(_WINDOWS) && !defined(macintosh)
#include <sys/param.h>
#endif

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <assert.h>
#endif

#include "mcom_db.h"
#include "hash.h"
#include "page.h"


static BUFHEAD *newbuf __P((HTAB *, uint32, BUFHEAD *));


#define BUF_REMOVE(B) { \
	(B)->prev->next = (B)->next; \
	(B)->next->prev = (B)->prev; \
}


#define BUF_INSERT(B, P) { \
	(B)->next = (P)->next; \
	(B)->prev = (P); \
	(P)->next = (B); \
	(B)->next->prev = (B); \
}

#define	MRU	hashp->bufhead.next
#define	LRU	hashp->bufhead.prev

#define MRU_INSERT(B)	BUF_INSERT((B), &hashp->bufhead)
#define LRU_INSERT(B)	BUF_INSERT((B), LRU)










extern BUFHEAD *
__get_buf(HTAB *hashp, uint32 addr, BUFHEAD *prev_bp, int newpage)

{
	register BUFHEAD *bp;
	register uint32 is_disk_mask;
	register int is_disk, segment_ndx = 0;
	SEGMENT segp = 0;

	is_disk = 0;
	is_disk_mask = 0;
	if (prev_bp) {
		bp = prev_bp->ovfl;
		if (!bp || (bp->addr != addr))
			bp = NULL;
		if (!newpage)
			is_disk = BUF_DISK;
	} else {
		
		segment_ndx = addr & (hashp->SGSIZE - 1);

		
		segp = hashp->dir[addr >> hashp->SSHIFT];
#ifdef DEBUG
		assert(segp != NULL);
#endif  

		bp = PTROF(segp[segment_ndx]);

		is_disk_mask = ISDISK(segp[segment_ndx]);
		is_disk = is_disk_mask || !hashp->new_file;
	}

	if (!bp) {
		bp = newbuf(hashp, addr, prev_bp);
		if (!bp)
			return(NULL);
		if(__get_page(hashp, bp->page, addr, !prev_bp, is_disk, 0))
		  {
			
			if(prev_bp)
			  {
				






				prev_bp->ovfl = NULL;
			  }
			BUF_REMOVE(bp);
			free(bp->page);
			free(bp);
			return (NULL);
		  }

		if (!prev_bp)			
		  {
#if 0
			

			
			segp[segment_ndx] =
			    (BUFHEAD *)((ptrdiff_t)bp | is_disk_mask);
#else   
			

			bp->is_disk = is_disk_mask;
			segp[segment_ndx] = bp;
#endif
		  }
	} else {  
		BUF_REMOVE(bp);
		MRU_INSERT(bp);                 
	}
	return (bp);
}







static BUFHEAD *
newbuf(HTAB *hashp, uint32 addr, BUFHEAD *prev_bp)
{
	register BUFHEAD *bp;		
	register BUFHEAD *xbp;		
	register BUFHEAD *next_xbp;
	SEGMENT segp;
	int segment_ndx;
	uint16 oaddr, *shortp;

	oaddr = 0;
	bp = LRU;
	



	if (hashp->nbufs || (bp->flags & BUF_PIN)) {
		
		if ((bp = (BUFHEAD *)malloc(sizeof(BUFHEAD))) == NULL)
			return (NULL);

		


		memset(bp, 0xff, sizeof(BUFHEAD));

		if ((bp->page = (char *)malloc((size_t)hashp->BSIZE)) == NULL) {
			free(bp);
			return (NULL);
		}

		


		memset(bp->page, 0xff, (size_t)hashp->BSIZE);

		if (hashp->nbufs)
			hashp->nbufs--;
	} else {
		
		BUF_REMOVE(bp);
		



		if ((bp->addr != 0) || (bp->flags & BUF_BUCKET)) {
			



			shortp = (uint16 *)bp->page;
			if (shortp[0])
			  {
				if(shortp[0] > (hashp->BSIZE / sizeof(uint16)))
				  {
					return(NULL);
				  }
				oaddr = shortp[shortp[0] - 1];
			  }
			if ((bp->flags & BUF_MOD) && __put_page(hashp, bp->page,
			    bp->addr, (int)IS_BUCKET(bp->flags), 0))
				return (NULL);
			







			if (IS_BUCKET(bp->flags)) {
				segment_ndx = bp->addr & (hashp->SGSIZE - 1);
				segp = hashp->dir[bp->addr >> hashp->SSHIFT];
#ifdef DEBUG
				assert(segp != NULL);
#endif

				if (hashp->new_file &&
				    ((bp->flags & BUF_MOD) ||
				    ISDISK(segp[segment_ndx])))
					segp[segment_ndx] = (BUFHEAD *)BUF_DISK;
				else
					segp[segment_ndx] = NULL;
			}
			




			for (xbp = bp; xbp->ovfl;) {
				next_xbp = xbp->ovfl;
				xbp->ovfl = 0;
				xbp = next_xbp;

				

				if (xbp->flags & BUF_PIN) {
					continue;
				}

				
				if (IS_BUCKET(xbp->flags) ||
				    (oaddr != xbp->addr))
					break;

				shortp = (uint16 *)xbp->page;
				if (shortp[0])
				  {
					


					if(shortp[0] > hashp->BSIZE/sizeof(uint16))
						return NULL;
					
					oaddr = shortp[shortp[0] - 1];
				  }
				if ((xbp->flags & BUF_MOD) && __put_page(hashp,
				    xbp->page, xbp->addr, 0, 0))
					return (NULL);
				xbp->addr = 0;
				xbp->flags = 0;
				BUF_REMOVE(xbp);
				LRU_INSERT(xbp);
			}
		}
	}

	
	bp->addr = addr;
#ifdef DEBUG1
	(void)fprintf(stderr, "NEWBUF1: %d->ovfl was %d is now %d\n",
	    bp->addr, (bp->ovfl ? bp->ovfl->addr : 0), 0);
#endif
	bp->ovfl = NULL;
	if (prev_bp) {
		



#ifdef DEBUG1
		(void)fprintf(stderr, "NEWBUF2: %d->ovfl was %d is now %d\n",
		    prev_bp->addr, (prev_bp->ovfl ? bp->ovfl->addr : 0),
		    (bp ? bp->addr : 0));
#endif
		prev_bp->ovfl = bp;
		bp->flags = 0;
	} else
		bp->flags = BUF_BUCKET;
	MRU_INSERT(bp);
	return (bp);
}

extern void __buf_init(HTAB *hashp, int32 nbytes)
{
	BUFHEAD *bfp;
	int npages;

	bfp = &(hashp->bufhead);
	npages = (nbytes + hashp->BSIZE - 1) >> hashp->BSHIFT;
	npages = PR_MAX(npages, MIN_BUFFERS);

	hashp->nbufs = npages;
	bfp->next = bfp;
	bfp->prev = bfp;
	







}

extern int
__buf_free(HTAB *hashp, int do_free, int to_disk)
{
	BUFHEAD *bp;
	int status = -1;

	
	if (!LRU)
		return (0);
	for (bp = LRU; bp != &hashp->bufhead;) {
		
		if (bp->addr || IS_BUCKET(bp->flags)) {
			if (to_disk && (bp->flags & BUF_MOD) &&
			    (status = __put_page(hashp, bp->page,
			    bp->addr, IS_BUCKET(bp->flags), 0))) {
			  
				if (do_free) {
					if (bp->page)
						free(bp->page);
					BUF_REMOVE(bp);
					free(bp);
				}
				
				return (status);
			}
		}
		
		if (do_free) {
			if (bp->page)
				free(bp->page);
			BUF_REMOVE(bp);
			free(bp);
			bp = LRU;
		} else
			bp = bp->prev;
	}
	return (0);
}

extern void
__reclaim_buf(HTAB *hashp, BUFHEAD *bp)
{
	bp->ovfl = 0;
	bp->addr = 0;
	bp->flags = 0;
	BUF_REMOVE(bp);
	LRU_INSERT(bp);
}
