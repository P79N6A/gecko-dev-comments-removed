































#include <stdlib.h>
#if !defined (__Userspace_os_Windows)
#include <stdint.h>
#if !defined(__Userspace_os_FreeBSD)
#include <sys/sysctl.h>
#endif
#include <netinet/sctp_os_userspace.h>
#endif
#include <user_environment.h>
#include <sys/types.h>

#if !defined(MIN)
#define MIN(arg1,arg2) ((arg1) < (arg2) ? (arg1) : (arg2))
#endif
#include <string.h>

#define uHZ 1000


int maxsockets = 25600;
int hz = uHZ;
int ip_defttl = 64;
int ipport_firstauto = 49152, ipport_lastauto = 65535;
int nmbclusters = 65536;


u_short ip_id = 0; 




userland_mutex_t atomic_mtx;


static int read_random_phony(void *, int);

static int (*read_func)(void *, int) = read_random_phony;


int
read_random(void *buf, int count)
{
	return ((*read_func)(buf, count));
}





static int
read_random_phony(void *buf, int count)
{
	uint32_t randval;
	int size, i;

	

	
	for (i = 0; i < count; i+= (int)sizeof(uint32_t)) {
		randval = random();
		size = MIN(count - i, sizeof(uint32_t));
		memcpy(&((char *)buf)[i], &randval, (size_t)size);
	}

	return (count);
}

