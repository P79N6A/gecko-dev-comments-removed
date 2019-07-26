





































#include <stdio.h>
#include "mcom_db.h"
typedef enum {
	HASH_GET, HASH_PUT, HASH_PUTNEW, HASH_DELETE, HASH_FIRST, HASH_NEXT
} ACTION;


typedef struct _bufhead BUFHEAD;

struct _bufhead {
	BUFHEAD		*prev;		
	BUFHEAD		*next;		
	BUFHEAD		*ovfl;		
	uint32	 	addr;		
	char		*page;		
	char     	is_disk;
	char	 	flags;
#define	BUF_MOD		0x0001
#define BUF_DISK	0x0002
#define	BUF_BUCKET	0x0004
#define	BUF_PIN		0x0008
};

#define IS_BUCKET(X)	((X) & BUF_BUCKET)

typedef BUFHEAD **SEGMENT;

typedef int DBFILE_PTR;
#define NO_FILE -1
#ifdef macintosh
#define DBFILE_OPEN(path, flag,mode) open((path), flag)
#define EXISTS(path)
#else
#define DBFILE_OPEN(path, flag,mode) open((path), (flag), (mode))
#endif

typedef struct hashhdr {		
	int32		magic;		
	int32		version;	
	uint32		lorder;		
	int32		bsize;		
	int32		bshift;		
	int32		dsize;		
	int32		ssize;		
	int32		sshift;		
	int32		ovfl_point;	

	int32		last_freed;	
	int32		max_bucket;	
	int32		high_mask;	
	int32		low_mask;	

	int32		ffactor;	
	int32		nkeys;		
	int32		hdrpages;	
	uint32		h_charkey;	
#define NCACHED	32			/* number of bit maps and spare 
					 * points */
	int32		spares[NCACHED];
	uint16		bitmaps[NCACHED];	

} HASHHDR;

typedef struct htab	 {		
	HASHHDR 	hdr;		
	int		nsegs;		
	int		exsegs;		

	uint32			
	    (*hash)(const void *, size_t);
	int		flags;		
	DBFILE_PTR	fp;		
	char            *filename;
	char		*tmp_buf;	
	char		*tmp_key;	
	BUFHEAD 	*cpage;		
	int		cbucket;	
	int		cndx;		
	int		dbmerrno;		

	int		new_file;	

	int		save_file;	


	uint32		*mapp[NCACHED];	
	int		nmaps;		
	int		nbufs;		

	BUFHEAD 	bufhead;	
	SEGMENT 	*dir;		
	off_t		file_size;	
	char		is_temp;	
	char		updateEOF;	
} HTAB;




#define DATABASE_CORRUPTED_ERROR -999   /* big ugly abort, delete database */
#define	OLD_MAX_BSIZE		65536		/* 2^16 */
#define MAX_BSIZE       	32l*1024l         /* 2^15 */
#define MIN_BUFFERS		6
#define MINHDRSIZE		512
#define DEF_BUFSIZE		65536l		/* 64 K */
#define DEF_BUCKET_SIZE		4096
#define DEF_BUCKET_SHIFT	12		/* log2(BUCKET) */
#define DEF_SEGSIZE		256
#define DEF_SEGSIZE_SHIFT	8		/* log2(SEGSIZE)	 */
#define DEF_DIRSIZE		256
#define DEF_FFACTOR		65536l
#define MIN_FFACTOR		4
#define SPLTMAX			8
#define CHARKEY			"%$sniglet^&"
#define NUMKEY			1038583l
#define BYTE_SHIFT		3
#define INT_TO_BYTE		2
#define INT_BYTE_SHIFT		5
#define ALL_SET			((uint32)0xFFFFFFFF)
#define ALL_CLEAR		0

#define PTROF(X)	((ptrdiff_t)(X) == BUF_DISK ? 0 : (X))
#define ISDISK(X)	((X) ? ((ptrdiff_t)(X) == BUF_DISK ? BUF_DISK \
				: (X)->is_disk) : 0)

#define BITS_PER_MAP	32


#define CLRBIT(A, N)	((A)[(N)/BITS_PER_MAP] &= ~(1<<((N)%BITS_PER_MAP)))
#define SETBIT(A, N)	((A)[(N)/BITS_PER_MAP] |= (1<<((N)%BITS_PER_MAP)))
#define ISSET(A, N)	((A)[(N)/BITS_PER_MAP] & (1<<((N)%BITS_PER_MAP)))










#define SPLITSHIFT	11
#define SPLITMASK	0x7FF
#define SPLITNUM(N)	(((uint32)(N)) >> SPLITSHIFT)
#define OPAGENUM(N)	((N) & SPLITMASK)
#define	OADDR_OF(S,O)	((uint32)((uint32)(S) << SPLITSHIFT) + (O))

#define BUCKET_TO_PAGE(B) \
	(B) + hashp->HDRPAGES + ((B) ? hashp->SPARES[__log2((uint32)((B)+1))-1] : 0)
#define OADDR_TO_PAGE(B) 	\
	BUCKET_TO_PAGE ( (1 << SPLITNUM((B))) -1 ) + OPAGENUM((B));



























































































#define OVFLPAGE	0
#define PARTIAL_KEY	1
#define FULL_KEY	2
#define FULL_KEY_DATA	3
#define	REAL_KEY	4


#undef BSIZE
#define BSIZE		hdr.bsize
#undef BSHIFT
#define BSHIFT		hdr.bshift
#define DSIZE		hdr.dsize
#define SGSIZE		hdr.ssize
#define SSHIFT		hdr.sshift
#define LORDER		hdr.lorder
#define OVFL_POINT	hdr.ovfl_point
#define	LAST_FREED	hdr.last_freed
#define MAX_BUCKET	hdr.max_bucket
#define FFACTOR		hdr.ffactor
#define HIGH_MASK	hdr.high_mask
#define LOW_MASK	hdr.low_mask
#define NKEYS		hdr.nkeys
#define HDRPAGES	hdr.hdrpages
#define SPARES		hdr.spares
#define BITMAPS		hdr.bitmaps
#define VERSION		hdr.version
#define MAGIC		hdr.magic
#define NEXT_FREE	hdr.next_free
#define H_CHARKEY	hdr.h_charkey

extern uint32 (*__default_hash) (const void *, size_t);
void __buf_init(HTAB *hashp, int32 nbytes);
int __big_delete(HTAB *hashp, BUFHEAD *bufp);
BUFHEAD * __get_buf(HTAB *hashp, uint32 addr, BUFHEAD *prev_bp, int newpage);
uint32 __call_hash(HTAB *hashp, char *k, size_t len);
#include "page.h"
extern int __big_split(HTAB *hashp, BUFHEAD *op,BUFHEAD *np,
BUFHEAD *big_keyp,uint32 addr,uint32   obucket, SPLIT_RETURN *ret);
void __free_ovflpage(HTAB *hashp, BUFHEAD *obufp);
BUFHEAD * __add_ovflpage(HTAB *hashp, BUFHEAD *bufp);
int __big_insert(HTAB *hashp, BUFHEAD *bufp, const DBT *key, const DBT *val);
int __expand_table(HTAB *hashp);
uint32 __log2(uint32 num);
void __reclaim_buf(HTAB *hashp, BUFHEAD *bp);
int __get_page(HTAB *hashp, char * p, uint32 bucket, int is_bucket, int is_disk, int is_bitmap);
int __put_page(HTAB *hashp, char *p, uint32 bucket, int is_bucket, int is_bitmap);
int __ibitmap(HTAB *hashp, int pnum, int nbits, int ndx);
int __buf_free(HTAB *hashp, int do_free, int to_disk);
int __find_bigpair(HTAB *hashp, BUFHEAD *bufp, int ndx, char *key, int size);
uint16 __find_last_page(HTAB *hashp, BUFHEAD **bpp);
int __addel(HTAB *hashp, BUFHEAD *bufp, const DBT *key, const DBT * val);
int __big_return(HTAB *hashp, BUFHEAD *bufp, int ndx, DBT *val, int set_current);
int __delpair(HTAB *hashp, BUFHEAD *bufp, int ndx);
int __big_keydata(HTAB *hashp, BUFHEAD *bufp, DBT *key, DBT *val, int set);
int __split_page(HTAB *hashp, uint32 obucket, uint32 nbucket);
