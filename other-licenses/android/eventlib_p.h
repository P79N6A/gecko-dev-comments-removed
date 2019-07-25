


































#ifndef _EVENTLIB_P_H
#define _EVENTLIB_P_H

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#define EVENTLIB_DEBUG 1

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "list.h"
#include "memcluster.h"


#define	EV_MASK_ALL	(EV_READ | EV_WRITE | EV_EXCEPT)
#define EV_ERR(e)		return (errno = (e), -1)
#define OK(x)		if ((x) < 0) EV_ERR(errno); else (void)NULL


#if HAVE_MEM_GET_SET
#define	NEW(p)		if (((p) = memget(sizeof *(p))) != NULL) \
				FILL(p); \
			else \
				(void)NULL;
#define OKNEW(p)	if (!((p) = memget(sizeof *(p)))) { \
				errno = ENOMEM; \
				return (-1); \
			} else \
				FILL(p)
#define FREE(p)		memput((p), sizeof *(p))

#if EVENTLIB_DEBUG
#define FILL(p)		memset((p), 0xF5, sizeof *(p))
#else
#define FILL(p)
#endif

#else

#define	NEW(p) 	p = malloc(sizeof *(p));
#define OKNEW(p) if (!((p) = malloc(sizeof *(p)))) { errno = ENOMEM; return (-1); }
#define FREE(p)	free(p)
#define FILL(p)

#endif


typedef struct evConn {
	evConnFunc	func;
	void *		uap;
	int		fd;
	int		flags;
#define EV_CONN_LISTEN		0x0001		/* Connection is a listener. */
#define EV_CONN_SELECTED	0x0002		/* evSelectFD(conn->file). */
#define EV_CONN_BLOCK		0x0004		/* Listener fd was blocking. */
	evFileID	file;
	struct evConn *	prev;
	struct evConn *	next;
} evConn;

typedef struct evAccept {
	int		fd;
	union {
		struct sockaddr		sa;
		struct sockaddr_in	in;
#ifndef NO_SOCKADDR_UN
		struct sockaddr_un	un;
#endif
	}		la;
	socklen_t	lalen;
	union {
		struct sockaddr		sa;
		struct sockaddr_in	in;
#ifndef NO_SOCKADDR_UN
		struct sockaddr_un	un;
#endif
	}		ra;
	socklen_t	ralen;
	int		ioErrno;
	evConn *	conn;
	LINK(struct evAccept) link;
} evAccept;

typedef struct evFile {
	evFileFunc	func;
	void *		uap;
	int		fd;
	int		eventmask;
	int		preemptive;
	struct evFile *	prev;
	struct evFile *	next;
	struct evFile *	fdprev;
	struct evFile *	fdnext;
} evFile;

typedef struct evStream {
	evStreamFunc	func;
	void *		uap;
	evFileID	file;
	evTimerID	timer;
	int		flags;
#define EV_STR_TIMEROK	0x0001	/* IFF timer valid. */
	int		fd;
	struct iovec *	iovOrig;
	int		iovOrigCount;
	struct iovec *	iovCur;
	int		iovCurCount;
	int		ioTotal;
	int		ioDone;
	int		ioErrno;
	struct evStream	*prevDone, *nextDone;
	struct evStream	*prev, *next;
} evStream;

typedef struct evTimer {
	evTimerFunc	func;
	void *		uap;
	struct timespec	due, inter;
	int		index;
	int		mode;
#define EV_TMR_RATE	1
} evTimer;

typedef struct evWait {
	evWaitFunc	func;
	void *		uap;
	const void *	tag;
	struct evWait *	next;
} evWait;

typedef struct evWaitList {
	evWait *		first;
	evWait *		last;
	struct evWaitList *	prev;
	struct evWaitList *	next;
} evWaitList;

typedef struct evEvent_p {
	enum {  Accept, File, Stream, Timer, Wait, Free, Null  } type;
	union {
		struct {  evAccept *this;  }			accept;
		struct {  evFile *this; int eventmask;  }	file;
		struct {  evStream *this;  }			stream;
		struct {  evTimer *this;  }			timer;
		struct {  evWait *this;  }			wait;
		struct {  struct evEvent_p *next;  }		free;
		struct {  const void *placeholder;  }		null;
	} u;
} evEvent_p;

typedef struct {
	
	const evEvent_p	*cur;
	
	int		debug;
	FILE		*output;
	
	evConn		*conns;
	LIST(evAccept)	accepts;
	
	evFile		*files, *fdNext;
	fd_set		rdLast, rdNext;
	fd_set		wrLast, wrNext;
	fd_set		exLast, exNext;
	fd_set		nonblockBefore;
	int		fdMax, fdCount, highestFD;
	evFile		*fdTable[FD_SETSIZE];
#ifdef EVENTLIB_TIME_CHECKS
	struct timespec	lastSelectTime;
	int		lastFdCount;
#endif
	
	evStream	*streams;
	evStream	*strDone, *strLast;
	
	struct timespec	lastEventTime;
	heap_context	timers;
	
	evWaitList	*waitLists;
	evWaitList	waitDone;
} evContext_p;


#define evPrintf __evPrintf
void evPrintf(const evContext_p *ctx, int level, const char *fmt, ...);


#define evCreateTimers __evCreateTimers
heap_context evCreateTimers(const evContext_p *);
#define evDestroyTimers __evDestroyTimers
void evDestroyTimers(const evContext_p *);


#define evFreeWait __evFreeWait
evWait *evFreeWait(evContext_p *ctx, evWait *old);


int		__evOptMonoTime;

#endif 
