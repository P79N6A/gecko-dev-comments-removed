
































#ifndef	_COMPAT_H_
#define	_COMPAT_H_

#include <sys/types.h>





#if 0
typedef unsigned char	u_char;		
typedef unsigned int	u_int;
typedef unsigned long	u_long;
typedef unsigned short	u_short;
#endif


#if 0
typedef unsigned int	size_t;		
#endif


#if 0
typedef	int		ssize_t;	
#endif





#if 0					
typedef unsigned int	sigset_t;
#endif





#if defined (__sun) && !defined(__SVR4) 
#define	VSPRINTF_CHARSTAR
#endif






#ifdef	NO_POSIX_SIGNALS
#define	sigemptyset(set)	(*(set) = 0)
#define	sigfillset(set)		(*(set) = ~(sigset_t)0, 0)
#define	sigaddset(set,signo)	(*(set) |= sigmask(signo), 0)
#define	sigdelset(set,signo)	(*(set) &= ~sigmask(signo), 0)
#define	sigismember(set,signo)	((*(set) & sigmask(signo)) != 0)

#define	SIG_BLOCK	1
#define	SIG_UNBLOCK	2
#define	SIG_SETMASK	3

static int __sigtemp;		


#define	sigprocmask(how, set, oset)					\
	((__sigtemp =							\
	(((how) == SIG_BLOCK) ?						\
		sigblock(0) | *(set) :					\
	(((how) == SIG_UNBLOCK) ?					\
		sigblock(0) & ~(*(set)) :				\
	((how) == SIG_SETMASK ?						\
		*(set) : sigblock(0))))),				\
	((oset) ? (*(oset ? oset : set) = sigsetmask(__sigtemp)) :	\
		sigsetmask(__sigtemp)), 0)
#endif





#ifndef BYTE_ORDER
#define	LITTLE_ENDIAN	1234		/* LSB first: i386, vax */
#define	BIG_ENDIAN	4321		/* MSB first: 68000, ibm, net */
#define	BYTE_ORDER	BIG_ENDIAN	/* Set for your system. */
#endif

#if defined(SYSV) || defined(SYSTEM5) || defined(__sun)
#define	index(a, b)		strchr(a, b)
#define	rindex(a, b)		strrchr(a, b)
#define	bzero(a, b)		memset(a, 0, b)
#define	bcmp(a, b, n)		memcmp(a, b, n)
#define	bcopy(a, b, n)		memmove(b, a, n)
#endif

#if defined(BSD) || defined(BSD4_3)
#define	strchr(a, b)		index(a, b)
#define	strrchr(a, b)		rindex(a, b)
#define	memcmp(a, b, n)		bcmp(a, b, n)
#define	memmove(a, b, n)	bcopy(b, a, n)
#endif







#ifndef USHRT_MAX
#define	USHRT_MAX		0xFFFF
#define	ULONG_MAX		0xFFFFFFFF
#endif

#ifndef O_ACCMODE			
#define	O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
#endif

#ifndef	_POSIX2_RE_DUP_MAX		
#define	_POSIX2_RE_DUP_MAX	255
#endif





#ifndef O_EXLOCK			
#define	O_EXLOCK	0
#endif

#ifndef O_SHLOCK			
#define	O_SHLOCK	0
#endif

#ifndef EFTYPE
#define	EFTYPE		EINVAL		/* POSIX 1003.1 format errno. */
#endif

#ifndef	WCOREDUMP			
#define	WCOREDUMP(a)	0
#endif

#ifndef	STDERR_FILENO
#define	STDIN_FILENO	0		/* ANSI C #defines */
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2
#endif

#ifndef SEEK_END
#define	SEEK_SET	0		/* POSIX 1003.1 seek values */
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

#ifndef _POSIX_VDISABLE			
#define	_POSIX_VDISABLE	0		/* Some systems used 0. */
#endif

#ifndef	TCSASOFT			
#define	TCSASOFT	0
#endif

#ifndef _POSIX2_RE_DUP_MAX		
#define	_POSIX2_RE_DUP_MAX	255
#endif

#ifndef NULL				
#define	NULL		0
#endif

#ifndef	MAX				
#define	MAX(_a,_b)	((_a)<(_b)?(_b):(_a))
#endif
#ifndef	MIN				
#define	MIN(_a,_b)	((_a)<(_b)?(_a):(_b))
#endif


#ifndef DEFFILEMODE			
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#endif

#ifndef __sun
#ifndef S_ISDIR				
#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */
#define	S_ISFIFO(m)	((m & 0170000) == 0010000)	/* fifo */
#endif
#ifndef S_ISLNK				
#define	S_ISLNK(m)	((m & 0170000) == 0120000)	/* symbolic link */
#define	S_ISSOCK(m)	((m & 0170000) == 0140000)	/* socket */
#endif
#endif 


#ifndef _BSD_VA_LIST_			
#define	_BSD_VA_LIST_	char *
#endif

#endif 
