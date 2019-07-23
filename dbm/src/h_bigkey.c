

































#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)hash_bigkey.c	8.3 (Berkeley) 5/31/94";
#endif 



















#if !defined(_WIN32) && !defined(_WINDOWS) && !defined(macintosh)
#include <sys/param.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <assert.h>
#endif

#include "mcom_db.h"
#include "hash.h"
#include "page.h"


static int collect_key __P((HTAB *, BUFHEAD *, int, DBT *, int));
static int collect_data __P((HTAB *, BUFHEAD *, int, int));










extern int
__big_insert(HTAB *hashp, BUFHEAD *bufp, const DBT *key, const DBT *val)
{
	register uint16 *p;
	uint key_size, n, val_size;
	uint16 space, move_bytes, off;
	char *cp, *key_data, *val_data;

	cp = bufp->page;		
	p = (uint16 *)cp;

	key_data = (char *)key->data;
	key_size = key->size;
	val_data = (char *)val->data;
	val_size = val->size;

	
	for (space = FREESPACE(p) - BIGOVERHEAD; key_size;
	    space = FREESPACE(p) - BIGOVERHEAD) {
		move_bytes = PR_MIN(space, key_size);
		off = OFFSET(p) - move_bytes;
		memmove(cp + off, key_data, move_bytes);
		key_size -= move_bytes;
		key_data += move_bytes;
		n = p[0];
		p[++n] = off;
		p[0] = ++n;
		FREESPACE(p) = off - PAGE_META(n);
		OFFSET(p) = off;
		p[n] = PARTIAL_KEY;
		bufp = __add_ovflpage(hashp, bufp);
		if (!bufp)
			return (-1);
		n = p[0];
		if (!key_size) {
			if (FREESPACE(p)) {
				move_bytes = PR_MIN(FREESPACE(p), val_size);
				off = OFFSET(p) - move_bytes;
				p[n] = off;
				memmove(cp + off, val_data, move_bytes);
				val_data += move_bytes;
				val_size -= move_bytes;
				p[n - 2] = FULL_KEY_DATA;
				FREESPACE(p) = FREESPACE(p) - move_bytes;
				OFFSET(p) = off;
			} else
				p[n - 2] = FULL_KEY;
		}
		p = (uint16 *)bufp->page;
		cp = bufp->page;
		bufp->flags |= BUF_MOD;
	}

	
	for (space = FREESPACE(p) - BIGOVERHEAD; val_size;
	    space = FREESPACE(p) - BIGOVERHEAD) {
		move_bytes = PR_MIN(space, val_size);
		



		if (space == val_size && val_size == val->size)
			move_bytes--;
		off = OFFSET(p) - move_bytes;
		memmove(cp + off, val_data, move_bytes);
		val_size -= move_bytes;
		val_data += move_bytes;
		n = p[0];
		p[++n] = off;
		p[0] = ++n;
		FREESPACE(p) = off - PAGE_META(n);
		OFFSET(p) = off;
		if (val_size) {
			p[n] = FULL_KEY;
			bufp = __add_ovflpage(hashp, bufp);
			if (!bufp)
				return (-1);
			cp = bufp->page;
			p = (uint16 *)cp;
		} else
			p[n] = FULL_KEY_DATA;
		bufp->flags |= BUF_MOD;
	}
	return (0);
}












extern int
__big_delete(HTAB *hashp, BUFHEAD *bufp)
{
	register BUFHEAD *last_bfp, *rbufp;
	uint16 *bp, pageno;
	int key_done, n;

	rbufp = bufp;
	last_bfp = NULL;
	bp = (uint16 *)bufp->page;
	pageno = 0;
	key_done = 0;

	while (!key_done || (bp[2] != FULL_KEY_DATA)) {
		if (bp[2] == FULL_KEY || bp[2] == FULL_KEY_DATA)
			key_done = 1;

		




		if (bp[2] == FULL_KEY_DATA && FREESPACE(bp))
			break;
		pageno = bp[bp[0] - 1];
		rbufp->flags |= BUF_MOD;
		rbufp = __get_buf(hashp, pageno, rbufp, 0);
		if (last_bfp)
			__free_ovflpage(hashp, last_bfp);
		last_bfp = rbufp;
		if (!rbufp)
			return (-1);		
		bp = (uint16 *)rbufp->page;
	}

	






	
	n = bp[0];
	pageno = bp[n - 1];

	
	bp = (uint16 *)bufp->page;
	if (n > 2) {
		
		bp[1] = pageno;
		bp[2] = OVFLPAGE;
		bufp->ovfl = rbufp->ovfl;
	} else
		
		bufp->ovfl = NULL;
	n -= 2;
	bp[0] = n;
	FREESPACE(bp) = hashp->BSIZE - PAGE_META(n);
	OFFSET(bp) = hashp->BSIZE - 1;

	bufp->flags |= BUF_MOD;
	if (rbufp)
		__free_ovflpage(hashp, rbufp);
	if (last_bfp != rbufp)
		__free_ovflpage(hashp, last_bfp);

	hashp->NKEYS--;
	return (0);
}







extern int
__find_bigpair(HTAB *hashp, BUFHEAD *bufp, int ndx, char *key, int size)
{
	register uint16 *bp;
	register char *p;
	int ksize;
	uint16 bytes;
	char *kkey;

	bp = (uint16 *)bufp->page;
	p = bufp->page;
	ksize = size;
	kkey = key;

	for (bytes = hashp->BSIZE - bp[ndx];
	    bytes <= size && bp[ndx + 1] == PARTIAL_KEY;
	    bytes = hashp->BSIZE - bp[ndx]) {
		if (memcmp(p + bp[ndx], kkey, bytes))
			return (-2);
		kkey += bytes;
		ksize -= bytes;
		bufp = __get_buf(hashp, bp[ndx + 2], bufp, 0);
		if (!bufp)
			return (-3);
		p = bufp->page;
		bp = (uint16 *)p;
		ndx = 1;
	}

	if (bytes != ksize || memcmp(p + bp[ndx], kkey, bytes)) {
#ifdef HASH_STATISTICS
		++hash_collisions;
#endif
		return (-2);
	} else
		return (ndx);
}










extern uint16
__find_last_page(HTAB *hashp, BUFHEAD **bpp)
{
	BUFHEAD *bufp;
	uint16 *bp, pageno;
	uint n;

	bufp = *bpp;
	bp = (uint16 *)bufp->page;
	for (;;) {
		n = bp[0];

		




		if (bp[2] == FULL_KEY_DATA &&
		    ((n == 2) || (bp[n] == OVFLPAGE) || (FREESPACE(bp))))
			break;

		

		if(n > hashp->BSIZE/sizeof(uint16))
			return(0);

		pageno = bp[n - 1];
		bufp = __get_buf(hashp, pageno, bufp, 0);
		if (!bufp)
			return (0);	
		bp = (uint16 *)bufp->page;
	}

	*bpp = bufp;
	if (bp[0] > 2)
		return (bp[3]);
	else
		return (0);
}





extern int
__big_return(
	HTAB *hashp,
	BUFHEAD *bufp,
	int ndx,
	DBT *val,
	int set_current)
{
	BUFHEAD *save_p;
	uint16 *bp, len, off, save_addr;
	char *tp;
	int save_flags;

	bp = (uint16 *)bufp->page;
	while (bp[ndx + 1] == PARTIAL_KEY) {
		bufp = __get_buf(hashp, bp[bp[0] - 1], bufp, 0);
		if (!bufp)
			return (-1);
		bp = (uint16 *)bufp->page;
		ndx = 1;
	}

	if (bp[ndx + 1] == FULL_KEY) {
		bufp = __get_buf(hashp, bp[bp[0] - 1], bufp, 0);
		if (!bufp)
			return (-1);
		bp = (uint16 *)bufp->page;
		save_p = bufp;
		save_addr = save_p->addr;
		off = bp[1];
		len = 0;
	} else
		if (!FREESPACE(bp)) {
			






			off = bp[bp[0]];
			len = bp[1] - off;
			save_p = bufp;
			save_addr = bufp->addr;
			bufp = __get_buf(hashp, bp[bp[0] - 1], bufp, 0);
			if (!bufp)
				return (-1);
			bp = (uint16 *)bufp->page;
		} else {
			
			tp = (char *)bp;
			off = bp[bp[0]];
			val->data = (uint8 *)tp + off;
			val->size = bp[1] - off;
			if (set_current) {
				if (bp[0] == 2) {	

					hashp->cpage = NULL;
					hashp->cbucket++;
					hashp->cndx = 1;
				} else {
					hashp->cpage = __get_buf(hashp,
					    bp[bp[0] - 1], bufp, 0);
					if (!hashp->cpage)
						return (-1);
					hashp->cndx = 1;
					if (!((uint16 *)
					    hashp->cpage->page)[0]) {
						hashp->cbucket++;
						hashp->cpage = NULL;
					}
				}
			}
			return (0);
		}

	

 	save_flags = save_p->flags;
	save_p->flags |= BUF_PIN;
	val->size = collect_data(hashp, bufp, (int)len, set_current);
	save_p->flags = save_flags;
	if (val->size == (size_t)-1)
		return (-1);
	if (save_p->addr != save_addr) {
		
		errno = EINVAL;			
		return (-1);
	}
	memmove(hashp->tmp_buf, (save_p->page) + off, len);
	val->data = (uint8 *)hashp->tmp_buf;
	return (0);
}









static int
collect_data(
	HTAB *hashp,
	BUFHEAD *bufp,
	int len, int set)
{
	register uint16 *bp;
	BUFHEAD *save_bufp;
	int save_flags;
	int mylen, totlen;

	




	save_bufp = bufp;
	save_flags = save_bufp->flags;
	save_bufp->flags |= BUF_PIN;

	
	for (totlen = len; bufp ; bufp = __get_buf(hashp, bp[bp[0]-1], bufp, 0)) {
		bp = (uint16 *)bufp->page;
		mylen = hashp->BSIZE - bp[1];

		


		if (mylen < 0) {
			save_bufp->flags = save_flags;
			return (-1);
 		}
		totlen += mylen;
		if (bp[2] == FULL_KEY_DATA) {		
			break;
		}
	}

 	if (!bufp) {
		save_bufp->flags = save_flags;
		return (-1);
	}

	
	if (hashp->tmp_buf)
		free(hashp->tmp_buf);
	if ((hashp->tmp_buf = (char *)malloc((size_t)totlen)) == NULL) {
		save_bufp->flags = save_flags;
		return (-1);
 	}

	
	for (bufp = save_bufp; bufp ;
				bufp = __get_buf(hashp, bp[bp[0]-1], bufp, 0)) {
		bp = (uint16 *)bufp->page;
		mylen = hashp->BSIZE - bp[1];
		memmove(&hashp->tmp_buf[len], (bufp->page) + bp[1], (size_t)mylen);
		len += mylen;
		if (bp[2] == FULL_KEY_DATA) {
			break;
		}
	}

	
	save_bufp->flags = save_flags;

	
	if (set) {
		hashp->cndx = 1;
		if (bp[0] == 2) {	
			hashp->cpage = NULL;
			hashp->cbucket++;
		} else {
			hashp->cpage = __get_buf(hashp, bp[bp[0] - 1], bufp, 0);
			if (!hashp->cpage)
				return (-1);
			else if (!((uint16 *)hashp->cpage->page)[0]) {
				hashp->cbucket++;
				hashp->cpage = NULL;
			}
		}
	}
	return (totlen);
}




extern int
__big_keydata(
	HTAB *hashp, 
	BUFHEAD *bufp, 
	DBT *key, DBT *val,
	int set)
{
	key->size = collect_key(hashp, bufp, 0, val, set);
	if (key->size == (size_t)-1)
		return (-1);
	key->data = (uint8 *)hashp->tmp_key;
	return (0);
}





static int
collect_key(
	HTAB *hashp,
	BUFHEAD *bufp,
	int len,
	DBT *val,
	int set)
{
	BUFHEAD *xbp;
	char *p;
	int mylen, totlen;
	uint16 *bp, save_addr;

	p = bufp->page;
	bp = (uint16 *)p;
	mylen = hashp->BSIZE - bp[1];

	save_addr = bufp->addr;
	totlen = len + mylen;
	if (bp[2] == FULL_KEY || bp[2] == FULL_KEY_DATA) {    
		if (hashp->tmp_key != NULL)
			free(hashp->tmp_key);
		if ((hashp->tmp_key = (char *)malloc((size_t)totlen)) == NULL)
			return (-1);
		if (__big_return(hashp, bufp, 1, val, set))
			return (-1);
	} else {
		xbp = __get_buf(hashp, bp[bp[0] - 1], bufp, 0);
		if (!xbp || ((totlen =
		    collect_key(hashp, xbp, totlen, val, set)) < 1))
			return (-1);
	}
	if (bufp->addr != save_addr) {
		errno = EINVAL;		
		return (-1);
	}
	memmove(&hashp->tmp_key[len], (bufp->page) + bp[1], (size_t)mylen);
	return (totlen);
}






extern int
__big_split(
	HTAB *hashp,
	BUFHEAD *op,	
	BUFHEAD *np,	
			
	BUFHEAD *big_keyp,
	uint32 addr,	
	uint32   obucket,
	SPLIT_RETURN *ret)
{
	register BUFHEAD *tmpp;
	register uint16 *tp;
	BUFHEAD *bp;
	DBT key, val;
	uint32 change;
	uint16 free_space, n, off;

	bp = big_keyp;

	
	if (__big_keydata(hashp, big_keyp, &key, &val, 0))
		return (-1);
	change = (__call_hash(hashp,(char*) key.data, key.size) != obucket);

	if ((ret->next_addr = __find_last_page(hashp, &big_keyp))) {
		if (!(ret->nextp =
		    __get_buf(hashp, ret->next_addr, big_keyp, 0)))
			return (-1);;
	} else
		ret->nextp = NULL;

	
#ifdef DEBUG
	assert(np->ovfl == NULL);
#endif
	if (change)
		tmpp = np;
	else
		tmpp = op;

	tmpp->flags |= BUF_MOD;
#ifdef DEBUG1
	(void)fprintf(stderr,
	    "BIG_SPLIT: %d->ovfl was %d is now %d\n", tmpp->addr,
	    (tmpp->ovfl ? tmpp->ovfl->addr : 0), (bp ? bp->addr : 0));
#endif
	tmpp->ovfl = bp;	
	tp = (uint16 *)tmpp->page;


#if 0  
	assert(FREESPACE(tp) >= OVFLSIZE);
#endif
	if(FREESPACE(tp) < OVFLSIZE)
		return(DATABASE_CORRUPTED_ERROR);

	n = tp[0];
	off = OFFSET(tp);
	free_space = FREESPACE(tp);
	tp[++n] = (uint16)addr;
	tp[++n] = OVFLPAGE;
	tp[0] = n;
	OFFSET(tp) = off;
	FREESPACE(tp) = free_space - OVFLSIZE;

	






	ret->newp = np;
	ret->oldp = op;

	tp = (uint16 *)big_keyp->page;
	big_keyp->flags |= BUF_MOD;
	if (tp[0] > 2) {
		






		n = tp[4];
		free_space = FREESPACE(tp);
		off = OFFSET(tp);
		tp[0] -= 2;
		FREESPACE(tp) = free_space + OVFLSIZE;
		OFFSET(tp) = off;
		tmpp = __add_ovflpage(hashp, big_keyp);
		if (!tmpp)
			return (-1);
		tp[4] = n;
	} else
		tmpp = big_keyp;

	if (change)
		ret->newp = tmpp;
	else
		ret->oldp = tmpp;
	return (0);
}
