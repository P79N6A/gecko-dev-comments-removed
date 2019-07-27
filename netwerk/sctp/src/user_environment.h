





























#ifndef _USER_ENVIRONMENT_H_
#define _USER_ENVIRONMENT_H_

#include <sys/types.h>

#ifdef __Userspace_os_FreeBSD
#ifndef _SYS_MUTEX_H_
#include <sys/mutex.h>
#endif
#endif
#if defined (__Userspace_os_Windows)
#include "netinet/sctp_os_userspace.h"
#endif




extern int maxsockets;





extern int hz;



extern int ipport_firstauto, ipport_lastauto;




extern int nmbclusters;

#if !defined (__Userspace_os_Windows)
#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

extern int read_random(void *buf, int count);



		
		
		
		
		
		
		


extern u_short ip_id;

#if defined(__Userspace_os_Linux)
#define IPV6_VERSION            0x60
#endif
#if defined(INVARIANTS)
#define panic(args...)            \
	do {                      \
		SCTP_PRINTF(args);\
		exit(1);          \
} while (0)
#endif

#if defined(INVARIANTS)
#define KASSERT(cond, args)          \
	do {                         \
		if (!(cond)) {       \
			printf args ;\
			exit(1);     \
		}                    \
	} while (0)
#else
#define KASSERT(cond, args)
#endif


extern int ip_defttl;
#endif
