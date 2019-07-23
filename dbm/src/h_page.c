

































#if defined(unix)
#define MY_LSEEK lseek
#else
#define MY_LSEEK new_lseek
extern long new_lseek(int fd, long pos, int start);
#endif

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)hash_page.c	8.7 (Berkeley) 8/16/94";
#endif 
















#ifndef macintosh
#include <sys/types.h>
#endif

#if defined(macintosh)
#include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>
#if defined(_WIN32) || defined(_WINDOWS) 
#include <io.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32) && !defined(_WINDOWS) && !defined(macintosh)
#include <unistd.h>
#endif

#include <assert.h>

#include "mcom_db.h"
#include "hash.h"
#include "page.h"


extern int mkstempflags(char *path, int extraFlags);

static uint32	*fetch_bitmap __P((HTAB *, uint32));
static uint32	 first_free __P((uint32));
static int	 open_temp __P((HTAB *));
static uint16	 overflow_page __P((HTAB *));
static void	 squeeze_key __P((uint16 *, const DBT *, const DBT *));
static int	 ugly_split
		    __P((HTAB *, uint32, BUFHEAD *, BUFHEAD *, int, int));

#define	PAGE_INIT(P) { \
	((uint16 *)(P))[0] = 0; \
	((uint16 *)(P))[1] = hashp->BSIZE - 3 * sizeof(uint16); \
	((uint16 *)(P))[2] = hashp->BSIZE; \
}





long new_lseek(int fd, long offset, int origin)
{
 	long cur_pos=0;
	long end_pos=0;
	long seek_pos=0;

	if(origin == SEEK_CUR)
      {	
      	if(offset < 1)							  
	    	return(lseek(fd, offset, SEEK_CUR));

		cur_pos = lseek(fd, 0, SEEK_CUR);

		if(cur_pos < 0)
			return(cur_pos);
	  }
										 
	end_pos = lseek(fd, 0, SEEK_END);
	if(end_pos < 0)
		return(end_pos);

	if(origin == SEEK_SET)
		seek_pos = offset;
	else if(origin == SEEK_CUR)
		seek_pos = cur_pos + offset;
	else if(origin == SEEK_END)
		seek_pos = end_pos + offset;
 	else
	  {
	  	assert(0);
		return(-1);
	  }

 	



 	if(seek_pos <= end_pos)
 		return(lseek(fd, seek_pos, SEEK_SET));
 		
 	  








 	  { 
 	 	char buffer[1024];
	   	long len = seek_pos-end_pos;
	   	memset(&buffer, 0, 1024);
	   	while(len > 0)
	      {
	        write(fd, (char*)&buffer, (size_t)(1024 > len ? len : 1024));
		    len -= 1024;
		  }
		return(lseek(fd, seek_pos, SEEK_SET));
	  }		

}






static void
putpair(char *p, const DBT *key, DBT * val)
{
	register uint16 *bp, n, off;

	bp = (uint16 *)p;

	
	n = bp[0];

	off = OFFSET(bp) - key->size;
	memmove(p + off, key->data, key->size);
	bp[++n] = off;

	
	off -= val->size;
	memmove(p + off, val->data, val->size);
	bp[++n] = off;

	
	bp[0] = n;
	bp[n + 1] = off - ((n + 3) * sizeof(uint16));
	bp[n + 2] = off;
}






extern int
__delpair(HTAB *hashp, BUFHEAD *bufp, int ndx)
{
	register uint16 *bp, newoff;
	register int n;
	uint16 pairlen;

	bp = (uint16 *)bufp->page;
	n = bp[0];

	if (bp[ndx + 1] < REAL_KEY)
		return (__big_delete(hashp, bufp));
	if (ndx != 1)
		newoff = bp[ndx - 1];
	else
		newoff = hashp->BSIZE;
	pairlen = newoff - bp[ndx + 1];

	if (ndx != (n - 1)) {
		
		register int i;
		register char *src = bufp->page + (int)OFFSET(bp);
		uint32 dst_offset = (uint32)OFFSET(bp) + (uint32)pairlen;
		register char *dst = bufp->page + dst_offset;
		uint32 length = bp[ndx + 1] - OFFSET(bp);

		













		if(dst_offset > (uint32)hashp->BSIZE)
			return(DATABASE_CORRUPTED_ERROR);

		if(length > (uint32)(hashp->BSIZE - dst_offset))
			return(DATABASE_CORRUPTED_ERROR);

		memmove(dst, src, length);

		
		for (i = ndx + 2; i <= n; i += 2) {
			if (bp[i + 1] == OVFLPAGE) {
				bp[i - 2] = bp[i];
				bp[i - 1] = bp[i + 1];
			} else {
				bp[i - 2] = bp[i] + pairlen;
				bp[i - 1] = bp[i + 1] + pairlen;
			}
		}
	}
	
	bp[n] = OFFSET(bp) + pairlen;
	bp[n - 1] = bp[n + 1] + pairlen + 2 * sizeof(uint16);
	bp[0] = n - 2;
	hashp->NKEYS--;

	bufp->flags |= BUF_MOD;
	return (0);
}





extern int
__split_page(HTAB *hashp, uint32 obucket, uint32 nbucket)
{
	register BUFHEAD *new_bufp, *old_bufp;
	register uint16 *ino;
	register uint16 *tmp_uint16_array;
	register char *np;
	DBT key, val;
    uint16 n, ndx;
	int retval;
	uint16 copyto, diff, moved;
	size_t off;
	char *op;

	copyto = (uint16)hashp->BSIZE;
	off = (uint16)hashp->BSIZE;
	old_bufp = __get_buf(hashp, obucket, NULL, 0);
	if (old_bufp == NULL)
		return (-1);
	new_bufp = __get_buf(hashp, nbucket, NULL, 0);
	if (new_bufp == NULL)
		return (-1);

	old_bufp->flags |= (BUF_MOD | BUF_PIN);
	new_bufp->flags |= (BUF_MOD | BUF_PIN);

	ino = (uint16 *)(op = old_bufp->page);
	np = new_bufp->page;

	moved = 0;

	for (n = 1, ndx = 1; n < ino[0]; n += 2) {
		if (ino[n + 1] < REAL_KEY) {
			retval = ugly_split(hashp, obucket, old_bufp, new_bufp,
			    (int)copyto, (int)moved);
			old_bufp->flags &= ~BUF_PIN;
			new_bufp->flags &= ~BUF_PIN;
			return (retval);

		}
		key.data = (uint8 *)op + ino[n];

		



		if(ino[n] > off)
			return(DATABASE_CORRUPTED_ERROR);

		key.size = off - ino[n];

#ifdef DEBUG
		
		assert(((int)key.size) > -1);
#endif

		if (__call_hash(hashp, (char *)key.data, key.size) == obucket) {
			
			diff = copyto - off;
			if (diff) {
				copyto = ino[n + 1] + diff;
				memmove(op + copyto, op + ino[n + 1],
				    off - ino[n + 1]);
				ino[ndx] = copyto + ino[n] - ino[n + 1];
				ino[ndx + 1] = copyto;
			} else
				copyto = ino[n + 1];
			ndx += 2;
		} else {
			
			val.data = (uint8 *)op + ino[n + 1];
			val.size = ino[n] - ino[n + 1];

			


			tmp_uint16_array = (uint16*)np;
			if(!PAIRFITS(tmp_uint16_array, &key, &val))
				return(DATABASE_CORRUPTED_ERROR);

			putpair(np, &key, &val);
			moved += 2;
		}

		off = ino[n + 1];
	}

	
	ino[0] -= moved;
	FREESPACE(ino) = copyto - sizeof(uint16) * (ino[0] + 3);
	OFFSET(ino) = copyto;

#ifdef DEBUG3
	(void)fprintf(stderr, "split %d/%d\n",
	    ((uint16 *)np)[0] / 2,
	    ((uint16 *)op)[0] / 2);
#endif
	
	old_bufp->flags &= ~BUF_PIN;
	new_bufp->flags &= ~BUF_PIN;
	return (0);
}





















#define MAX_UGLY_SPLIT_LOOPS 10000

static int
ugly_split(HTAB *hashp, uint32 obucket, BUFHEAD *old_bufp,
 BUFHEAD *new_bufp, int copyto, int moved)
	
	
{
	register BUFHEAD *bufp;	
	register uint16 *ino;	
	register uint16 *np;	
	register uint16 *op;	
    uint32 loop_detection=0;

	BUFHEAD *last_bfp;	
	DBT key, val;
	SPLIT_RETURN ret;
	uint16 n, off, ov_addr, scopyto;
	char *cino;		
	int status;

	bufp = old_bufp;
	ino = (uint16 *)old_bufp->page;
	np = (uint16 *)new_bufp->page;
	op = (uint16 *)old_bufp->page;
	last_bfp = NULL;
	scopyto = (uint16)copyto;	

	n = ino[0] - 1;
	while (n < ino[0]) {


        




        loop_detection++;

        if(loop_detection > MAX_UGLY_SPLIT_LOOPS)
            return DATABASE_CORRUPTED_ERROR;

		if (ino[2] < REAL_KEY && ino[2] != OVFLPAGE) {
			if ((status = __big_split(hashp, old_bufp,
			    new_bufp, bufp, bufp->addr, obucket, &ret)))
				return (status);
			old_bufp = ret.oldp;
			if (!old_bufp)
				return (-1);
			op = (uint16 *)old_bufp->page;
			new_bufp = ret.newp;
			if (!new_bufp)
				return (-1);
			np = (uint16 *)new_bufp->page;
			bufp = ret.nextp;
			if (!bufp)
				return (0);
			cino = (char *)bufp->page;
			ino = (uint16 *)cino;
			last_bfp = ret.nextp;
		} else if (ino[n + 1] == OVFLPAGE) {
			ov_addr = ino[n];
			



			ino[0] -= (moved + 2);
			FREESPACE(ino) =
			    scopyto - sizeof(uint16) * (ino[0] + 3);
			OFFSET(ino) = scopyto;

			bufp = __get_buf(hashp, ov_addr, bufp, 0);
			if (!bufp)
				return (-1);

			ino = (uint16 *)bufp->page;
			n = 1;
			scopyto = hashp->BSIZE;
			moved = 0;

			if (last_bfp)
				__free_ovflpage(hashp, last_bfp);
			last_bfp = bufp;
		}
		
		off = hashp->BSIZE;
		for (n = 1; (n < ino[0]) && (ino[n + 1] >= REAL_KEY); n += 2) {
			cino = (char *)ino;
			key.data = (uint8 *)cino + ino[n];
			key.size = off - ino[n];
			val.data = (uint8 *)cino + ino[n + 1];
			val.size = ino[n] - ino[n + 1];
			off = ino[n + 1];

			if (__call_hash(hashp, (char*)key.data, key.size) == obucket) {
				
				if (PAIRFITS(op, (&key), (&val)))
					putpair((char *)op, &key, &val);
				else {
					old_bufp =
					    __add_ovflpage(hashp, old_bufp);
					if (!old_bufp)
						return (-1);
					op = (uint16 *)old_bufp->page;
					putpair((char *)op, &key, &val);
				}
				old_bufp->flags |= BUF_MOD;
			} else {
				
				if (PAIRFITS(np, (&key), (&val)))
					putpair((char *)np, &key, &val);
				else {
					new_bufp =
					    __add_ovflpage(hashp, new_bufp);
					if (!new_bufp)
						return (-1);
					np = (uint16 *)new_bufp->page;
					putpair((char *)np, &key, &val);
				}
				new_bufp->flags |= BUF_MOD;
			}
		}
	}
	if (last_bfp)
		__free_ovflpage(hashp, last_bfp);
	return (0);
}








extern int
__addel(HTAB *hashp, BUFHEAD *bufp, const DBT *key, const DBT * val)
{
	register uint16 *bp, *sop;
	int do_expand;

	bp = (uint16 *)bufp->page;
	do_expand = 0;
	while (bp[0] && (bp[2] < REAL_KEY || bp[bp[0]] < REAL_KEY))
		
		if (bp[2] == FULL_KEY_DATA && bp[0] == 2)
			

			break;
		else if (bp[2] < REAL_KEY && bp[bp[0]] != OVFLPAGE) {
			bufp = __get_buf(hashp, bp[bp[0] - 1], bufp, 0);
			if (!bufp)
			  {
#ifdef DEBUG
				assert(0);
#endif
				return (-1);
			  }
			bp = (uint16 *)bufp->page;
		} else
			
			if (FREESPACE(bp) > PAIRSIZE(key, val)) {
			  {
				squeeze_key(bp, key, val);

				




				hashp->NKEYS++;
				return (0);
			  }
			} else {
				bufp = __get_buf(hashp, bp[bp[0] - 1], bufp, 0);
				if (!bufp)
			      {
#ifdef DEBUG
				    assert(0);
#endif
					return (-1);
				  }
				bp = (uint16 *)bufp->page;
			}

	if (PAIRFITS(bp, key, val))
		putpair(bufp->page, key, (DBT *)val);
	else {
		do_expand = 1;
		bufp = __add_ovflpage(hashp, bufp);
		if (!bufp)
	      {
#ifdef DEBUG
		    assert(0);
#endif
			return (-1);
		  }
		sop = (uint16 *)bufp->page;

		if (PAIRFITS(sop, key, val))
			putpair((char *)sop, key, (DBT *)val);
		else
			if (__big_insert(hashp, bufp, key, val))
	          {
#ifdef DEBUG
		        assert(0);
#endif
			    return (-1);
		      }
	}
	bufp->flags |= BUF_MOD;
	



	hashp->NKEYS++;
	if (do_expand ||
	    (hashp->NKEYS / (hashp->MAX_BUCKET + 1) > hashp->FFACTOR))
		return (__expand_table(hashp));
	return (0);
}







extern BUFHEAD *
__add_ovflpage(HTAB *hashp, BUFHEAD *bufp)
{
	register uint16 *sp;
	uint16 ndx, ovfl_num;
#ifdef DEBUG1
	int tmp1, tmp2;
#endif
	sp = (uint16 *)bufp->page;

	
	if (hashp->FFACTOR == DEF_FFACTOR) {
		hashp->FFACTOR = sp[0] >> 1;
		if (hashp->FFACTOR < MIN_FFACTOR)
			hashp->FFACTOR = MIN_FFACTOR;
	}
	bufp->flags |= BUF_MOD;
	ovfl_num = overflow_page(hashp);
#ifdef DEBUG1
	tmp1 = bufp->addr;
	tmp2 = bufp->ovfl ? bufp->ovfl->addr : 0;
#endif
	if (!ovfl_num || !(bufp->ovfl = __get_buf(hashp, ovfl_num, bufp, 1)))
		return (NULL);
	bufp->ovfl->flags |= BUF_MOD;
#ifdef DEBUG1
	(void)fprintf(stderr, "ADDOVFLPAGE: %d->ovfl was %d is now %d\n",
	    tmp1, tmp2, bufp->ovfl->addr);
#endif
	ndx = sp[0];
	




	sp[ndx + 4] = OFFSET(sp);
	sp[ndx + 3] = FREESPACE(sp) - OVFLSIZE;
	sp[ndx + 1] = ovfl_num;
	sp[ndx + 2] = OVFLPAGE;
	sp[0] = ndx + 2;
#ifdef HASH_STATISTICS
	hash_overflows++;
#endif
	return (bufp->ovfl);
}






extern int
__get_page(HTAB *hashp,
	char * p,
	uint32 bucket, 
	int is_bucket, 
	int is_disk, 
	int is_bitmap)
{
	register int fd, page;
	size_t size;
	int rsize;
	uint16 *bp;

	fd = hashp->fp;
	size = hashp->BSIZE;

	if ((fd == -1) || !is_disk) {
		PAGE_INIT(p);
		return (0);
	}
	if (is_bucket)
		page = BUCKET_TO_PAGE(bucket);
	else
		page = OADDR_TO_PAGE(bucket);
	if ((MY_LSEEK(fd, (off_t)page << hashp->BSHIFT, SEEK_SET) == -1) ||
	    ((rsize = read(fd, p, size)) == -1))
		return (-1);

	bp = (uint16 *)p;
	if (!rsize)
		bp[0] = 0;	
	else
		if ((unsigned)rsize != size) {
			errno = EFTYPE;
			return (-1);
		}

	if (!is_bitmap && !bp[0]) {
		PAGE_INIT(p);
	} else {

#ifdef DEBUG
		if(BYTE_ORDER == LITTLE_ENDIAN)
		  {
			int is_little_endian;
			is_little_endian = BYTE_ORDER;
		  }
		else if(BYTE_ORDER == BIG_ENDIAN)
		  {
			int is_big_endian;
			is_big_endian = BYTE_ORDER;
		  }
		else
		  {
			assert(0);
		  }
#endif

		if (hashp->LORDER != BYTE_ORDER) {
			register int i, max;

			if (is_bitmap) {
				max = hashp->BSIZE >> 2; 
				for (i = 0; i < max; i++)
					M_32_SWAP(((int *)p)[i]);
			} else {
				M_16_SWAP(bp[0]);
				max = bp[0] + 2;

	    		



				if((unsigned)max > (size / sizeof(uint16)))
					return(DATABASE_CORRUPTED_ERROR);

				

				for (i = 1; i <= max; i++)
					M_16_SWAP(bp[i]);
			}
		}

		


		if(!is_bitmap && bp[0] != 0)
		  {
			uint16 num_keys = bp[0];
			uint16 offset;
			uint16 i;

			




			if(bp[0] > (size / sizeof(uint16)))
				return(DATABASE_CORRUPTED_ERROR);
			
			
			if(FREESPACE(bp) > size)
				return(DATABASE_CORRUPTED_ERROR);
		
			




			offset = size;
			for(i=1 ; i <= num_keys; i+=2)
  			  {
				
				if(bp[i+1] >= REAL_KEY)
	  			  {
						
					if(bp[i] > offset || bp[i+1] > bp[i])			
						return(DATABASE_CORRUPTED_ERROR);
			
					offset = bp[i+1];
	  			  }
				else
	  			  {
					


					break;
	  			  }
  			  }
		}
	}
	return (0);
}








extern int
__put_page(HTAB *hashp, char *p, uint32 bucket, int is_bucket, int is_bitmap)
{
	register int fd, page;
	size_t size;
	int wsize;
	off_t offset;

	size = hashp->BSIZE;
	if ((hashp->fp == -1) && open_temp(hashp))
		return (-1);
	fd = hashp->fp;

	if (hashp->LORDER != BYTE_ORDER) {
		register int i;
		register int max;

		if (is_bitmap) {
			max = hashp->BSIZE >> 2;	
			for (i = 0; i < max; i++)
				M_32_SWAP(((int *)p)[i]);
		} else {
			max = ((uint16 *)p)[0] + 2;

            



            if((unsigned)max > (size / sizeof(uint16)))
                return(DATABASE_CORRUPTED_ERROR);

			for (i = 0; i <= max; i++)
				M_16_SWAP(((uint16 *)p)[i]);

		}
	}

	if (is_bucket)
		page = BUCKET_TO_PAGE(bucket);
	else
		page = OADDR_TO_PAGE(bucket);
	offset = (off_t)page << hashp->BSHIFT;
	if ((MY_LSEEK(fd, offset, SEEK_SET) == -1) ||
	    ((wsize = write(fd, p, size)) == -1))
		
		return (-1);
	if ((unsigned)wsize != size) {
		errno = EFTYPE;
		return (-1);
	}
#if defined(_WIN32) || defined(_WINDOWS) 
	if (offset + size > hashp->file_size) {
		hashp->updateEOF = 1;
	}
#endif
	


	if (hashp->LORDER != BYTE_ORDER) {
		register int i;
		register int max;

		if (is_bitmap) {
			max = hashp->BSIZE >> 2;	
			for (i = 0; i < max; i++)
				M_32_SWAP(((int *)p)[i]);
		} else {
    		uint16 *bp = (uint16 *)p;

			M_16_SWAP(bp[0]);
			max = bp[0] + 2;

			



			

			for (i = 1; i <= max; i++)
				M_16_SWAP(bp[i]);
		}
	}

	return (0);
}

#define BYTE_MASK	((1 << INT_BYTE_SHIFT) -1)




extern int
__ibitmap(HTAB *hashp, int pnum, int nbits, int ndx)
{
	uint32 *ip;
	size_t clearbytes, clearints;

	if ((ip = (uint32 *)malloc((size_t)hashp->BSIZE)) == NULL)
		return (1);
	hashp->nmaps++;
	clearints = ((nbits - 1) >> INT_BYTE_SHIFT) + 1;
	clearbytes = clearints << INT_TO_BYTE;
	(void)memset((char *)ip, 0, clearbytes);
	(void)memset(((char *)ip) + clearbytes, 0xFF,
	    hashp->BSIZE - clearbytes);
	ip[clearints - 1] = ALL_SET << (nbits & BYTE_MASK);
	SETBIT(ip, 0);
	hashp->BITMAPS[ndx] = (uint16)pnum;
	hashp->mapp[ndx] = ip;
	return (0);
}

static uint32
first_free(uint32 map)
{
	register uint32 i, mask;

	mask = 0x1;
	for (i = 0; i < BITS_PER_MAP; i++) {
		if (!(mask & map))
			return (i);
		mask = mask << 1;
	}
	return (i);
}

static uint16
overflow_page(HTAB *hashp)
{
	register uint32 *freep=NULL;
	register int max_free, offset, splitnum;
	uint16 addr;
	uint32 i;
	int bit, first_page, free_bit, free_page, in_use_bits, j;
#ifdef DEBUG2
	int tmp1, tmp2;
#endif
	splitnum = hashp->OVFL_POINT;
	max_free = hashp->SPARES[splitnum];

	free_page = (max_free - 1) >> (hashp->BSHIFT + BYTE_SHIFT);
	free_bit = (max_free - 1) & ((hashp->BSIZE << BYTE_SHIFT) - 1);

	
	first_page = hashp->LAST_FREED >>(hashp->BSHIFT + BYTE_SHIFT);
	for ( i = first_page; i <= (unsigned)free_page; i++ ) {
		if (!(freep = (uint32 *)hashp->mapp[i]) &&
		    !(freep = fetch_bitmap(hashp, i)))
			return (0);
		if (i == (unsigned)free_page)
			in_use_bits = free_bit;
		else
			in_use_bits = (hashp->BSIZE << BYTE_SHIFT) - 1;
		
		if (i == (unsigned)first_page) {
			bit = hashp->LAST_FREED &
			    ((hashp->BSIZE << BYTE_SHIFT) - 1);
			j = bit / BITS_PER_MAP;
			bit = bit & ~(BITS_PER_MAP - 1);
		} else {
			bit = 0;
			j = 0;
		}
		for (; bit <= in_use_bits; j++, bit += BITS_PER_MAP)
			if (freep[j] != ALL_SET)
				goto found;
	}

	
	hashp->LAST_FREED = hashp->SPARES[splitnum];
	hashp->SPARES[splitnum]++;
	offset = hashp->SPARES[splitnum] -
	    (splitnum ? hashp->SPARES[splitnum - 1] : 0);

#define	OVMSG	"HASH: Out of overflow pages.  Increase page size\n"
	if (offset > SPLITMASK) {
		if (++splitnum >= NCACHED) {
#ifndef macintosh
			(void)write(STDERR_FILENO, OVMSG, sizeof(OVMSG) - 1);
#endif
			return (0);
		}
		hashp->OVFL_POINT = splitnum;
		hashp->SPARES[splitnum] = hashp->SPARES[splitnum-1];
		hashp->SPARES[splitnum-1]--;
		offset = 1;
	}

	
	if (free_bit == (hashp->BSIZE << BYTE_SHIFT) - 1) {
		free_page++;
		if (free_page >= NCACHED) {
#ifndef macintosh
			(void)write(STDERR_FILENO, OVMSG, sizeof(OVMSG) - 1);
#endif
			return (0);
		}
		










		if (__ibitmap(hashp,
		    (int)OADDR_OF(splitnum, offset), 1, free_page))
			return (0);
		hashp->SPARES[splitnum]++;
#ifdef DEBUG2
		free_bit = 2;
#endif
		offset++;
		if (offset > SPLITMASK) {
			if (++splitnum >= NCACHED) {
#ifndef macintosh
				(void)write(STDERR_FILENO, OVMSG,
				    sizeof(OVMSG) - 1);
#endif
				return (0);
			}
			hashp->OVFL_POINT = splitnum;
			hashp->SPARES[splitnum] = hashp->SPARES[splitnum-1];
			hashp->SPARES[splitnum-1]--;
			offset = 0;
		}
	} else {
		



		free_bit++;
		SETBIT(freep, free_bit);
	}

	
	addr = OADDR_OF(splitnum, offset);
#ifdef DEBUG2
	(void)fprintf(stderr, "OVERFLOW_PAGE: ADDR: %d BIT: %d PAGE %d\n",
	    addr, free_bit, free_page);
#endif
	return (addr);

found:
	bit = bit + first_free(freep[j]);
	SETBIT(freep, bit);
#ifdef DEBUG2
	tmp1 = bit;
	tmp2 = i;
#endif
	




	bit = 1 + bit + (i * (hashp->BSIZE << BYTE_SHIFT));
	if (bit >= hashp->LAST_FREED)
		hashp->LAST_FREED = bit - 1;

	
	for (i = 0; (i < (unsigned)splitnum) && (bit > hashp->SPARES[i]); i++) {}
	offset = (i ? bit - hashp->SPARES[i - 1] : bit);
	if (offset >= SPLITMASK)
		return (0);	
	addr = OADDR_OF(i, offset);
#ifdef DEBUG2
	(void)fprintf(stderr, "OVERFLOW_PAGE: ADDR: %d BIT: %d PAGE %d\n",
	    addr, tmp1, tmp2);
#endif

	
	return (addr);
}




extern void
__free_ovflpage(HTAB *hashp, BUFHEAD *obufp)
{
	uint16 addr;
	uint32 *freep;
	uint32 bit_address, free_page, free_bit;
	uint16 ndx;

	if(!obufp || !obufp->addr)
	    return;

	addr = obufp->addr;
#ifdef DEBUG1
	(void)fprintf(stderr, "Freeing %d\n", addr);
#endif
	ndx = (((uint16)addr) >> SPLITSHIFT);
	bit_address =
	    (ndx ? hashp->SPARES[ndx - 1] : 0) + (addr & SPLITMASK) - 1;
	if (bit_address < (uint32)hashp->LAST_FREED)
		hashp->LAST_FREED = bit_address;
	free_page = (bit_address >> (hashp->BSHIFT + BYTE_SHIFT));
	free_bit = bit_address & ((hashp->BSIZE << BYTE_SHIFT) - 1);

	if (!(freep = hashp->mapp[free_page])) 
		freep = fetch_bitmap(hashp, free_page);

#ifdef DEBUG
	




	if (!freep)
	  {
		assert(0);
		return;
	  }
#endif
	CLRBIT(freep, free_bit);
#ifdef DEBUG2
	(void)fprintf(stderr, "FREE_OVFLPAGE: ADDR: %d BIT: %d PAGE %d\n",
	    obufp->addr, free_bit, free_page);
#endif
	__reclaim_buf(hashp, obufp);
}






static int
open_temp(HTAB *hashp)
{
#ifdef XP_OS2
 	hashp->fp = mkstemp(NULL);
#else
#if !defined(_WIN32) && !defined(_WINDOWS) && !defined(macintosh)
	sigset_t set, oset;
#endif
#if !defined(macintosh)
	char * tmpdir;
	size_t len;
	char last;
#endif
	static const char namestr[] = "/_hashXXXXXX";
	char filename[1024];

#if !defined(_WIN32) && !defined(_WINDOWS) && !defined(macintosh)
	
	(void)sigfillset(&set);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);
#endif

	filename[0] = 0;
#if defined(macintosh)
	strcat(filename, namestr + 1);
#else
	tmpdir = getenv("TMP");
	if (!tmpdir)
		tmpdir = getenv("TMPDIR");
	if (!tmpdir)
		tmpdir = getenv("TEMP");
	if (!tmpdir)
		tmpdir = ".";
	len = strlen(tmpdir);
	if (len && len < (sizeof filename - sizeof namestr)) {
		strcpy(filename, tmpdir);
	}
	len = strlen(filename);
	last = tmpdir[len - 1];
	strcat(filename, (last == '/' || last == '\\') ? namestr + 1 : namestr);
#endif

#if defined(_WIN32) || defined(_WINDOWS)
	if ((hashp->fp = mkstempflags(filename, _O_BINARY|_O_TEMPORARY)) != -1) {
		if (hashp->filename) {
			free(hashp->filename);
		}
		hashp->filename = strdup(filename);
		hashp->is_temp = 1;
	}
#else
	if ((hashp->fp = mkstemp(filename)) != -1) {
		(void)unlink(filename);
#if !defined(macintosh)
		(void)fcntl(hashp->fp, F_SETFD, 1);
#endif									  
	}
#endif

#if !defined(_WIN32) && !defined(_WINDOWS) && !defined(macintosh)
	(void)sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
#endif
#endif  
	return (hashp->fp != -1 ? 0 : -1);
}





static void
squeeze_key(uint16 *sp, const DBT * key, const DBT * val)
{
	register char *p;
	uint16 free_space, n, off, pageno;

	p = (char *)sp;
	n = sp[0];
	free_space = FREESPACE(sp);
	off = OFFSET(sp);

	pageno = sp[n - 1];
	off -= key->size;
	sp[n - 1] = off;
	memmove(p + off, key->data, key->size);
	off -= val->size;
	sp[n] = off;
	memmove(p + off, val->data, val->size);
	sp[0] = n + 2;
	sp[n + 1] = pageno;
	sp[n + 2] = OVFLPAGE;
	FREESPACE(sp) = free_space - PAIRSIZE(key, val);
	OFFSET(sp) = off;
}

static uint32 *
fetch_bitmap(HTAB *hashp, uint32 ndx)
{
	if (ndx >= (unsigned)hashp->nmaps)
		return (NULL);
	if ((hashp->mapp[ndx] = (uint32 *)malloc((size_t)hashp->BSIZE)) == NULL)
		return (NULL);
	if (__get_page(hashp,
	    (char *)hashp->mapp[ndx], hashp->BITMAPS[ndx], 0, 1, 1)) {
		free(hashp->mapp[ndx]);
		hashp->mapp[ndx] = NULL; 
		return (NULL);
	}                 
	return (hashp->mapp[ndx]);
}

#ifdef DEBUG4
int
print_chain(int addr)
{
	BUFHEAD *bufp;
	short *bp, oaddr;

	(void)fprintf(stderr, "%d ", addr);
	bufp = __get_buf(hashp, addr, NULL, 0);
	bp = (short *)bufp->page;
	while (bp[0] && ((bp[bp[0]] == OVFLPAGE) ||
		((bp[0] > 2) && bp[2] < REAL_KEY))) {
		oaddr = bp[bp[0] - 1];
		(void)fprintf(stderr, "%d ", (int)oaddr);
		bufp = __get_buf(hashp, (int)oaddr, bufp, 0);
		bp = (short *)bufp->page;
	}
	(void)fprintf(stderr, "\n");
}
#endif
