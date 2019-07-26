

































#ifndef _DB_H_
#define	_DB_H_


#ifdef WINCE
#define off_t long
#endif

#ifndef macintosh
#include <sys/types.h>
#endif
#include "prtypes.h"

#include <limits.h>

#ifdef __DBINTERFACE_PRIVATE

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#else
#include "cdefs.h"
#endif

#ifdef HAVE_SYS_BYTEORDER_H
#include <sys/byteorder.h>
#endif

#if defined(__linux) || defined(__BEOS__)
#include <endian.h>
#ifndef BYTE_ORDER
#define BYTE_ORDER    __BYTE_ORDER
#define BIG_ENDIAN    __BIG_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif
#endif 

#ifdef __sgi
#define BYTE_ORDER BIG_ENDIAN
#define BIG_ENDIAN      4321
#define LITTLE_ENDIAN   1234            /* LSB first: i386, vax, all NT risc */
#endif

#ifdef __sun
#define BIG_ENDIAN      4321
#define LITTLE_ENDIAN   1234            /* LSB first: i386, vax, all NT risc */

#ifndef __SVR4

#include <compat.h>
#endif







#ifndef BYTE_ORDER

#if defined(_BIG_ENDIAN)
#define BYTE_ORDER BIG_ENDIAN
#elif defined(_LITTLE_ENDIAN)
#define BYTE_ORDER LITTLE_ENDIAN
#elif !defined(__SVR4)

#define BYTE_ORDER BIG_ENDIAN
#elif !defined(vax) && !defined(ntohl) && !defined(lint) && !defined(i386)


#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#endif 
#endif 

#if defined(__hpux) || defined(__hppa)
#define BYTE_ORDER BIG_ENDIAN
#define BIG_ENDIAN      4321
#define LITTLE_ENDIAN   1234            /* LSB first: i386, vax, all NT risc */
#endif

#if defined(AIXV3) || defined(AIX)

#include <sys/machine.h>
#endif


#ifdef __osf__
#include <machine/endian.h>
#endif

#ifdef __alpha
#ifndef WIN32
#else

#define BYTE_ORDER LITTLE_ENDIAN
#define BIG_ENDIAN      4321
#define LITTLE_ENDIAN   1234 
#endif
#endif

#ifdef NCR
#include <sys/endian.h>
#endif

#ifdef __QNX__
#ifdef __QNXNTO__
#include <sys/param.h>
#else
#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321
#define BYTE_ORDER	LITTLE_ENDIAN
#endif
#endif

#ifdef SNI

#define BYTE_ORDER BIG_ENDIAN
#define BIG_ENDIAN      4321
#define LITTLE_ENDIAN   1234
#endif

#ifdef _WINDOWS
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif

#define BYTE_ORDER LITTLE_ENDIAN
#define LITTLE_ENDIAN   1234            /* LSB first: i386, vax, all NT risc */
#define BIG_ENDIAN      4321
#endif

#ifdef macintosh
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER BIG_ENDIAN
#endif

#endif  

#ifdef SCO
#define MAXPATHLEN 	1024              
#endif

#include <fcntl.h>

#if defined(_WINDOWS) || defined(XP_OS2)
#include <stdio.h>
#include <io.h>

#ifndef XP_OS2 
#define MAXPATHLEN 	1024               
#endif

#define	EFTYPE		EINVAL		/* POSIX 1003.1 format errno. */

#ifndef	STDERR_FILENO
#define	STDIN_FILENO	0		/* ANSI C #defines */
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2
#endif

#ifndef O_ACCMODE			
#define	O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
#endif
#endif

#ifdef macintosh
#include <stdio.h>
#include "xp_mcom.h"
#define O_ACCMODE       3       /* Mask for file access modes */
#define EFTYPE 2000
PR_BEGIN_EXTERN_C
int mkstemp(const char *path);
PR_END_EXTERN_C
#endif	

#if !defined(_WINDOWS) && !defined(macintosh)
#include <sys/stat.h>
#include <errno.h>
#endif


#ifndef EFTYPE
#define EFTYPE      EINVAL      /* POSIX 1003.1 format errno. */
#endif

#define	RET_ERROR	-1		/* Return values. */
#define	RET_SUCCESS	 0
#define	RET_SPECIAL	 1

#define	MAX_PAGE_NUMBER	0xffffffff	/* >= # of pages in a file */

#ifndef __sgi
typedef uint32	pgno_t;
#endif

#define	MAX_PAGE_OFFSET	65535		/* >= # of bytes in a page */
typedef uint16	indx_t;
#define	MAX_REC_NUMBER	0xffffffff	/* >= # of records in a tree */
typedef uint32	recno_t;


typedef struct {
	void	*data;			
	size_t	 size;			
} DBT;


#define	R_CURSOR	1		/* del, put, seq */
#define	__R_UNUSED	2		/* UNUSED */
#define	R_FIRST		3		/* seq */
#define	R_IAFTER	4		/* put (RECNO) */
#define	R_IBEFORE	5		/* put (RECNO) */
#define	R_LAST		6		/* seq (BTREE, RECNO) */
#define	R_NEXT		7		/* seq */
#define	R_NOOVERWRITE	8		/* put */
#define	R_PREV		9		/* seq (BTREE, RECNO) */
#define	R_SETCURSOR	10		/* put (RECNO) */
#define	R_RECNOSYNC	11		/* sync (RECNO) */

typedef enum { DB_BTREE, DB_HASH, DB_RECNO } DBTYPE;

typedef enum { LockOutDatabase, UnlockDatabase } DBLockFlagEnum;














#if UINT_MAX > 65535
#define	DB_LOCK		0x20000000	/* Do locking. */
#define	DB_SHMEM	0x40000000	/* Use shared memory. */
#define	DB_TXN		0x80000000	/* Do transactions. */
#else
#define	DB_LOCK		    0x2000	/* Do locking. */
#define	DB_SHMEM	    0x4000	/* Use shared memory. */
#define	DB_TXN		    0x8000	/* Do transactions. */
#endif


typedef struct __db {
	DBTYPE type;			
	int (*close)	(struct __db *);
	int (*del)	(const struct __db *, const DBT *, uint);
	int (*get)	(const struct __db *, const DBT *, DBT *, uint);
	int (*put)	(const struct __db *, DBT *, const DBT *, uint);
	int (*seq)	(const struct __db *, DBT *, DBT *, uint);
	int (*sync)	(const struct __db *, uint);
	void *internal;			
	int (*fd)	(const struct __db *);
} DB;

#define	BTREEMAGIC	0x053162
#define	BTREEVERSION	3


typedef struct {
#define	R_DUP		0x01	/* duplicate keys */
	uint32	flags;
	uint	cachesize;	
	int	maxkeypage;	
	int	minkeypage;	
	uint	psize;		
	int	(*compare)	
	    (const DBT *, const DBT *);
	size_t	(*prefix)	
	    (const DBT *, const DBT *);
	int	lorder;		
} BTREEINFO;

#define	HASHMAGIC	0x061561
#define	HASHVERSION	2


typedef struct {
	uint	bsize;		
	uint	ffactor;	
	uint	nelem;		
	uint	cachesize;	
	uint32		
		(*hash) (const void *, size_t);
	int	lorder;		
} HASHINFO;


typedef struct {
#define	R_FIXEDLEN	0x01	/* fixed-length records */
#define	R_NOKEY		0x02	/* key not required */
#define	R_SNAPSHOT	0x04	/* snapshot the input */
	uint32	flags;
	uint	cachesize;	
	uint	psize;		
	int	lorder;		
	size_t	reclen;		
	uint8	bval;		
	char	*bfname;	 
} RECNOINFO;

#ifdef __DBINTERFACE_PRIVATE






#define	M_32_SWAP(a) {							\
	uint32 _tmp = a;						\
	((char *)&a)[0] = ((char *)&_tmp)[3];				\
	((char *)&a)[1] = ((char *)&_tmp)[2];				\
	((char *)&a)[2] = ((char *)&_tmp)[1];				\
	((char *)&a)[3] = ((char *)&_tmp)[0];				\
}
#define	P_32_SWAP(a) {							\
	uint32 _tmp = *(uint32 *)a;				\
	((char *)a)[0] = ((char *)&_tmp)[3];				\
	((char *)a)[1] = ((char *)&_tmp)[2];				\
	((char *)a)[2] = ((char *)&_tmp)[1];				\
	((char *)a)[3] = ((char *)&_tmp)[0];				\
}
#define	P_32_COPY(a, b) {						\
	((char *)&(b))[0] = ((char *)&(a))[3];				\
	((char *)&(b))[1] = ((char *)&(a))[2];				\
	((char *)&(b))[2] = ((char *)&(a))[1];				\
	((char *)&(b))[3] = ((char *)&(a))[0];				\
}







#define	M_16_SWAP(a) {							\
	uint16 _tmp = a;						\
	((char *)&a)[0] = ((char *)&_tmp)[1];				\
	((char *)&a)[1] = ((char *)&_tmp)[0];				\
}
#define	P_16_SWAP(a) {							\
	uint16 _tmp = *(uint16 *)a;				\
	((char *)a)[0] = ((char *)&_tmp)[1];				\
	((char *)a)[1] = ((char *)&_tmp)[0];				\
}
#define	P_16_COPY(a, b) {						\
	((char *)&(b))[0] = ((char *)&(a))[1];				\
	((char *)&(b))[1] = ((char *)&(a))[0];				\
}
#endif

PR_BEGIN_EXTERN_C

extern DB *
dbopen (const char *, int, int, DBTYPE, const void *);




void dbSetOrClearDBLock(DBLockFlagEnum type);

#ifdef __DBINTERFACE_PRIVATE
DB	*__bt_open (const char *, int, int, const BTREEINFO *, int);
DB	*__hash_open (const char *, int, int, const HASHINFO *, int);
DB	*__rec_open (const char *, int, int, const RECNOINFO *, int);
void	 __dbpanic (DB *dbp);
#endif

PR_END_EXTERN_C

#endif 
