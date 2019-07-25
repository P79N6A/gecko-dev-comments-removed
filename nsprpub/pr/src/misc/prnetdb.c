




































#include "primpl.h"

#include <string.h>






#if defined(XP_UNIX)
#if defined(_PR_NEED_H_ERRNO)
extern int h_errno;
#endif
#define _MD_GETHOST_ERRNO() h_errno
#else
#define _MD_GETHOST_ERRNO() _MD_ERRNO()
#endif














#if defined(_PR_NO_PREEMPT) || defined(_PR_HAVE_GETHOST_R) \
    || defined(_PR_HAVE_THREADSAFE_GETHOST)
#define _PR_NO_DNS_LOCK
#endif

#if defined(_PR_NO_DNS_LOCK)
#define LOCK_DNS()
#define UNLOCK_DNS()
#else
PRLock *_pr_dnsLock = NULL;
#define LOCK_DNS() PR_Lock(_pr_dnsLock)
#define UNLOCK_DNS() PR_Unlock(_pr_dnsLock)
#endif  







#if defined(XP_BEOS) && defined(BONE_VERSION)
#include <arpa/inet.h>  
#include <sys/socket.h>
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_GETPROTO_R_POINTER
#endif

#if defined(SOLARIS) || (defined(BSDI) && defined(_REENTRANT)) \
	|| (defined(LINUX) && defined(_REENTRANT) \
        && !(defined(__GLIBC__) && __GLIBC__ >= 2) \
        && !defined(ANDROID))
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_GETPROTO_R_POINTER
#endif

#if defined(OSF1) \
        || defined(AIX4_3_PLUS) || (defined(AIX) && defined(_THREAD_SAFE)) \
	|| (defined(HPUX10_10) && defined(_REENTRANT)) \
        || (defined(HPUX10_20) && defined(_REENTRANT)) \
        || defined(OPENBSD)
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_GETPROTO_R_INT
#endif

#if __FreeBSD_version >= 602000
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_5_ARG_GETPROTO_R
#endif


#if (defined(__GLIBC__) && __GLIBC__ >= 2 && !defined(XP_BEOS))
#define _PR_HAVE_GETPROTO_R
#define _PR_HAVE_5_ARG_GETPROTO_R
#endif

#if !defined(_PR_HAVE_GETPROTO_R)
PRLock* _getproto_lock = NULL;
#endif

#if defined(_PR_INET6_PROBE)
extern PRBool _pr_ipv6_is_present(void);
#endif

#define _PR_IN6_IS_ADDR_UNSPECIFIED(a)				\
				(((a)->pr_s6_addr32[0] == 0) &&	\
				((a)->pr_s6_addr32[1] == 0) &&		\
				((a)->pr_s6_addr32[2] == 0) &&		\
				((a)->pr_s6_addr32[3] == 0))
 
#define _PR_IN6_IS_ADDR_LOOPBACK(a)					\
               (((a)->pr_s6_addr32[0] == 0)	&&	\
               ((a)->pr_s6_addr32[1] == 0)		&&	\
               ((a)->pr_s6_addr32[2] == 0)		&&	\
               ((a)->pr_s6_addr[12] == 0)		&&	\
               ((a)->pr_s6_addr[13] == 0)		&&	\
               ((a)->pr_s6_addr[14] == 0)		&&	\
               ((a)->pr_s6_addr[15] == 0x1U))
 
const PRIPv6Addr _pr_in6addr_any =	{{{ 0, 0, 0, 0,
										0, 0, 0, 0,
										0, 0, 0, 0,
										0, 0, 0, 0 }}};

const PRIPv6Addr _pr_in6addr_loopback = {{{ 0, 0, 0, 0,
											0, 0, 0, 0,
											0, 0, 0, 0,
											0, 0, 0, 0x1U }}};






#define _PR_IN6_IS_ADDR_V4MAPPED(a)			\
		(((a)->pr_s6_addr32[0] == 0) 	&&	\
		((a)->pr_s6_addr32[1] == 0)	&&	\
		((a)->pr_s6_addr[8] == 0)		&&	\
		((a)->pr_s6_addr[9] == 0)		&&	\
		((a)->pr_s6_addr[10] == 0xff)	&&	\
		((a)->pr_s6_addr[11] == 0xff))

#define _PR_IN6_IS_ADDR_V4COMPAT(a)			\
		(((a)->pr_s6_addr32[0] == 0) &&	\
		((a)->pr_s6_addr32[1] == 0) &&		\
		((a)->pr_s6_addr32[2] == 0))

#define _PR_IN6_V4MAPPED_TO_IPADDR(a) ((a)->pr_s6_addr32[3])

#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)













static PRLock *_pr_query_ifs_lock = NULL;
static PRBool _pr_have_inet_if = PR_FALSE;
static PRBool _pr_have_inet6_if = PR_FALSE;

#undef DEBUG_QUERY_IFS

#if defined(AIX) \
    || (defined(DARWIN) && (!defined(HAVE_GETIFADDRS) \
        || (defined(XP_MACOSX) && (!defined(MAC_OS_X_VERSION_10_2) || \
        MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_2))))












#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#ifdef DEBUG_QUERY_IFS
static void
_pr_PrintIfreq(struct ifreq *ifr)
{
    PRNetAddr addr;
    struct sockaddr *sa;
    const char* family;
    char addrstr[64];

    sa = &ifr->ifr_addr;
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        family = "inet";
        memcpy(&addr.inet.ip, &sin->sin_addr, sizeof(sin->sin_addr));
    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
        family = "inet6";
        memcpy(&addr.ipv6.ip, &sin6->sin6_addr, sizeof(sin6->sin6_addr));
    } else {
        return;  
    }
    addr.raw.family = sa->sa_family;
    PR_NetAddrToString(&addr, addrstr, sizeof(addrstr));
    printf("%s: %s %s\n", ifr->ifr_name, family, addrstr);
}
#endif

static void
_pr_QueryNetIfs(void)
{
    int sock;
    int rv;
    struct ifconf ifc;
    struct ifreq *ifr;
    struct ifreq *lifr;
    PRUint32 len, lastlen;
    char *buf;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return;
    }

    
    lastlen = 0;
    len = 100 * sizeof(struct ifreq);  
    for (;;) {
        buf = (char *)PR_Malloc(len);
        if (NULL == buf) {
            close(sock);
            return;
        }
        ifc.ifc_buf = buf;
        ifc.ifc_len = len;
        rv = ioctl(sock, SIOCGIFCONF, &ifc);
        if (rv < 0) {
            if (errno != EINVAL || lastlen != 0) {
                close(sock);
                PR_Free(buf);
                return;
            }
        } else {
            if (ifc.ifc_len == lastlen)
                break;  
            lastlen = ifc.ifc_len;
        }
        len += 10 * sizeof(struct ifreq);  
        PR_Free(buf);
    }
    close(sock);

    ifr = ifc.ifc_req;
    lifr = (struct ifreq *)&ifc.ifc_buf[ifc.ifc_len];

    while (ifr < lifr) {
        struct sockaddr *sa;
        int sa_len;

#ifdef DEBUG_QUERY_IFS
        _pr_PrintIfreq(ifr);
#endif
        sa = &ifr->ifr_addr;
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *) sa;
            if (sin->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                _pr_have_inet_if = PR_TRUE;
            } 
        } else if (sa->sa_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;
            if (!IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr)
                    && !IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
                _pr_have_inet6_if = PR_TRUE;
            } 
        }

#ifdef _PR_HAVE_SOCKADDR_LEN
        sa_len = PR_MAX(sa->sa_len, sizeof(struct sockaddr));
#else
        switch (sa->sa_family) {
#ifdef AF_LINK
        case AF_LINK:
            sa_len = sizeof(struct sockaddr_dl);
            break;
#endif
        case AF_INET6:
            sa_len = sizeof(struct sockaddr_in6);
            break;
        default:
            sa_len = sizeof(struct sockaddr);
            break;
        }
#endif
        ifr = (struct ifreq *)(((char *)sa) + sa_len);
    }
    PR_Free(buf);
}

#elif (defined(DARWIN) && defined(HAVE_GETIFADDRS)) || defined(FREEBSD) \
    || defined(NETBSD) || defined(OPENBSD)





#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#ifdef DEBUG_QUERY_IFS
static void
_pr_PrintIfaddrs(struct ifaddrs *ifa)
{
    struct sockaddr *sa;
    const char* family;
    void *addrp;
    char addrstr[64];

    sa = ifa->ifa_addr;
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        family = "inet";
        addrp = &sin->sin_addr;
    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
        family = "inet6";
        addrp = &sin6->sin6_addr;
    } else {
        return;  
    }
    inet_ntop(sa->sa_family, addrp, addrstr, sizeof(addrstr));
    printf("%s: %s %s\n", ifa->ifa_name, family, addrstr);
}
#endif

static void
_pr_QueryNetIfs(void)
{
    struct ifaddrs *ifp;
    struct ifaddrs *ifa;

    if (getifaddrs(&ifp) == -1) {
        return;
    }
    for (ifa = ifp; ifa; ifa = ifa->ifa_next) {
        struct sockaddr *sa;

#ifdef DEBUG_QUERY_IFS
        _pr_PrintIfaddrs(ifa);
#endif
        sa = ifa->ifa_addr;
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *) sa;
            if (sin->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                _pr_have_inet_if = 1;
            } 
        } else if (sa->sa_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;
            if (!IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr)
                    && !IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
                _pr_have_inet6_if = 1;
            } 
        }
    } 
    freeifaddrs(ifp);
}

#else  





static void
_pr_QueryNetIfs(void)
{
    _pr_have_inet_if = PR_TRUE;
    _pr_have_inet6_if = PR_TRUE;
}

#endif

#endif  

void _PR_InitNet(void)
{
#if defined(XP_UNIX)
#ifdef HAVE_NETCONFIG
	



	 (void)setnetconfig();
#endif
#endif
#if !defined(_PR_NO_DNS_LOCK)
	_pr_dnsLock = PR_NewLock();
#endif
#if !defined(_PR_HAVE_GETPROTO_R)
	_getproto_lock = PR_NewLock();
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)
	_pr_query_ifs_lock = PR_NewLock();
#endif
}

void _PR_CleanupNet(void)
{
#if !defined(_PR_NO_DNS_LOCK)
    if (_pr_dnsLock) {
        PR_DestroyLock(_pr_dnsLock);
        _pr_dnsLock = NULL;
    }
#endif
#if !defined(_PR_HAVE_GETPROTO_R)
    if (_getproto_lock) {
        PR_DestroyLock(_getproto_lock);
        _getproto_lock = NULL;
    }
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)
    if (_pr_query_ifs_lock) {
        PR_DestroyLock(_pr_query_ifs_lock);
        _pr_query_ifs_lock = NULL;
    }
#endif
}





static char *Alloc(PRIntn amount, char **bufp, PRIntn *buflenp, PRIntn align)
{
	char *buf = *bufp;
	PRIntn buflen = *buflenp;

	if (align && ((long)buf & (align - 1))) {
		PRIntn skip = align - ((ptrdiff_t)buf & (align - 1));
		if (buflen < skip) {
			return 0;
		}
		buf += skip;
		buflen -= skip;
	}
	if (buflen < amount) {
		return 0;
	}
	*bufp = buf + amount;
	*buflenp = buflen - amount;
	return buf;
}

typedef enum _PRIPAddrConversion {
    _PRIPAddrNoConversion,
    _PRIPAddrIPv4Mapped,
    _PRIPAddrIPv4Compat
} _PRIPAddrConversion;




static void MakeIPv4MappedAddr(const char *v4, char *v6)
{
    memset(v6, 0, 10);
    memset(v6 + 10, 0xff, 2);
    memcpy(v6 + 12, v4, 4);
}




static void MakeIPv4CompatAddr(const char *v4, char *v6)
{
    memset(v6, 0, 12);
    memcpy(v6 + 12, v4, 4);
}





static PRStatus CopyHostent(
    struct hostent *from,
    char **buf,
    PRIntn *bufsize,
    _PRIPAddrConversion conversion,
    PRHostEnt *to)
{
	PRIntn len, na;
	char **ap;

	if (conversion != _PRIPAddrNoConversion
			&& from->h_addrtype == AF_INET) {
		PR_ASSERT(from->h_length == 4);
		to->h_addrtype = PR_AF_INET6;
		to->h_length = 16;
	} else {
#if defined(_PR_INET6) || defined(_PR_INET6_PROBE)
		if (AF_INET6 == from->h_addrtype)
			to->h_addrtype = PR_AF_INET6;
		else
#endif
			to->h_addrtype = from->h_addrtype;
		to->h_length = from->h_length;
	}

	
	if (!from->h_name) return PR_FAILURE;
	len = strlen(from->h_name) + 1;
	to->h_name = Alloc(len, buf, bufsize, 0);
	if (!to->h_name) return PR_FAILURE;
	memcpy(to->h_name, from->h_name, len);

	
	if (!from->h_aliases) {
		na = 1;
	} else {
		for (na = 1, ap = from->h_aliases; *ap != 0; na++, ap++){;} 
	}
	to->h_aliases = (char**)Alloc(
	    na * sizeof(char*), buf, bufsize, sizeof(char**));
	if (!to->h_aliases) return PR_FAILURE;

	
	if (!from->h_aliases) {
		to->h_aliases[0] = 0;
	} else {
		for (na = 0, ap = from->h_aliases; *ap != 0; na++, ap++) {
			len = strlen(*ap) + 1;
			to->h_aliases[na] = Alloc(len, buf, bufsize, 0);
			if (!to->h_aliases[na]) return PR_FAILURE;
			memcpy(to->h_aliases[na], *ap, len);
		}
		to->h_aliases[na] = 0;
	}

	
	for (na = 1, ap = from->h_addr_list; *ap != 0; na++, ap++){;} 
	to->h_addr_list = (char**)Alloc(
	    na * sizeof(char*), buf, bufsize, sizeof(char**));
	if (!to->h_addr_list) return PR_FAILURE;

	
	for (na = 0, ap = from->h_addr_list; *ap != 0; na++, ap++) {
		to->h_addr_list[na] = Alloc(to->h_length, buf, bufsize, 0);
		if (!to->h_addr_list[na]) return PR_FAILURE;
		if (conversion != _PRIPAddrNoConversion
				&& from->h_addrtype == AF_INET) {
			if (conversion == _PRIPAddrIPv4Mapped) {
				MakeIPv4MappedAddr(*ap, to->h_addr_list[na]);
			} else {
				PR_ASSERT(conversion == _PRIPAddrIPv4Compat);
				MakeIPv4CompatAddr(*ap, to->h_addr_list[na]);
			}
		} else {
			memcpy(to->h_addr_list[na], *ap, to->h_length);
		}
	}
	to->h_addr_list[na] = 0;
	return PR_SUCCESS;
}

#ifdef SYMBIAN

static void AssignAliases(struct protoent *Protoent, char** aliases)
{
    if (NULL == Protoent->p_aliases) {
        if (0 == strcmp(Protoent->p_name, "ip"))
            aliases[0] = "IP";
        else if (0 == strcmp(Protoent->p_name, "tcp"))
            aliases[0] = "TCP";
        else if (0 == strcmp(Protoent->p_name, "udp"))
            aliases[0] = "UDP";
        else
            aliases[0] = "UNKNOWN";
        aliases[1] = NULL;
        Protoent->p_aliases = aliases;
    }
}
#endif

#if !defined(_PR_HAVE_GETPROTO_R)




static PRStatus CopyProtoent(
    struct protoent *from, char *buf, PRIntn bufsize, PRProtoEnt *to)
{
	PRIntn len, na;
	char **ap;

	
	to->p_num = from->p_proto;

	
	if (!from->p_name) return PR_FAILURE;
	len = strlen(from->p_name) + 1;
	to->p_name = Alloc(len, &buf, &bufsize, 0);
	if (!to->p_name) return PR_FAILURE;
	memcpy(to->p_name, from->p_name, len);

	
	for (na = 1, ap = from->p_aliases; *ap != 0; na++, ap++){;} 
	to->p_aliases = (char**)Alloc(
	    na * sizeof(char*), &buf, &bufsize, sizeof(char**));
	if (!to->p_aliases) return PR_FAILURE;

	
	for (na = 0, ap = from->p_aliases; *ap != 0; na++, ap++) {
		len = strlen(*ap) + 1;
		to->p_aliases[na] = Alloc(len, &buf, &bufsize, 0);
		if (!to->p_aliases[na]) return PR_FAILURE;
		memcpy(to->p_aliases[na], *ap, len);
	}
	to->p_aliases[na] = 0;

	return PR_SUCCESS;
}
#endif 









#if defined(_PR_HAVE_GETHOST_R_INT)

#define GETHOSTBYNAME(name) \
    (gethostbyname_r(name, &tmphe, tmpbuf, bufsize, &h, &h_err), h)
#define GETHOSTBYNAME2(name, af) \
    (gethostbyname2_r(name, af, &tmphe, tmpbuf, bufsize, &h, &h_err), h)
#define GETHOSTBYADDR(addr, addrlen, af) \
    (gethostbyaddr_r(addr, addrlen, af, \
    &tmphe, tmpbuf, bufsize, &h, &h_err), h)

#elif defined(_PR_HAVE_GETHOST_R_POINTER)

#define GETHOSTBYNAME(name) \
    gethostbyname_r(name, &tmphe, tmpbuf, bufsize, &h_err)
#define GETHOSTBYNAME2(name, af) \
    gethostbyname2_r(name, af, &tmphe, tmpbuf, bufsize, &h_err)
#define GETHOSTBYADDR(addr, addrlen, af) \
    gethostbyaddr_r(addr, addrlen, af, &tmphe, tmpbuf, bufsize, &h_err)

#else

#define GETHOSTBYNAME(name) gethostbyname(name)
#define GETHOSTBYNAME2(name, af) gethostbyname2(name, af)
#define GETHOSTBYADDR(addr, addrlen, af) gethostbyaddr(addr, addrlen, af)

#endif  

PR_IMPLEMENT(PRStatus) PR_GetHostByName(
    const char *name, char *buf, PRIntn bufsize, PRHostEnt *hp)
{
	struct hostent *h;
	PRStatus rv = PR_FAILURE;
#if defined(_PR_HAVE_GETHOST_R)
    char localbuf[PR_NETDB_BUF_SIZE];
    char *tmpbuf;
    struct hostent tmphe;
    int h_err;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

#if defined(_PR_HAVE_GETHOST_R)
    tmpbuf = localbuf;
    if (bufsize > sizeof(localbuf))
    {
        tmpbuf = (char *)PR_Malloc(bufsize);
        if (NULL == tmpbuf)
        {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            return rv;
        }
    }
#endif

	LOCK_DNS();

	h = GETHOSTBYNAME(name);
    
	if (NULL == h)
	{
	    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
	}
	else
	{
		_PRIPAddrConversion conversion = _PRIPAddrNoConversion;
		rv = CopyHostent(h, &buf, &bufsize, conversion, hp);
		if (PR_SUCCESS != rv)
		    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
	}
	UNLOCK_DNS();
#if defined(_PR_HAVE_GETHOST_R)
    if (tmpbuf != localbuf)
        PR_Free(tmpbuf);
#endif
	return rv;
}

#if !defined(_PR_INET6) && \
        defined(_PR_INET6_PROBE) && defined(_PR_HAVE_GETIPNODEBYNAME)
typedef struct hostent  * (*_pr_getipnodebyname_t)(const char *, int,
										int, int *);
typedef struct hostent  * (*_pr_getipnodebyaddr_t)(const void *, size_t,
													int, int *);
typedef void (*_pr_freehostent_t)(struct hostent *);
static void * _pr_getipnodebyname_fp;
static void * _pr_getipnodebyaddr_fp;
static void * _pr_freehostent_fp;





PRStatus
_pr_find_getipnodebyname(void)
{
    PRLibrary *lib;	
    PRStatus rv;
#define GETIPNODEBYNAME "getipnodebyname"
#define GETIPNODEBYADDR "getipnodebyaddr"
#define FREEHOSTENT     "freehostent"

    _pr_getipnodebyname_fp = PR_FindSymbolAndLibrary(GETIPNODEBYNAME, &lib);
    if (NULL != _pr_getipnodebyname_fp) {
        _pr_freehostent_fp = PR_FindSymbol(lib, FREEHOSTENT);
        if (NULL != _pr_freehostent_fp) {
            _pr_getipnodebyaddr_fp = PR_FindSymbol(lib, GETIPNODEBYADDR);
            if (NULL != _pr_getipnodebyaddr_fp)
                rv = PR_SUCCESS;
            else
                rv = PR_FAILURE;
        } else
            rv = PR_FAILURE;
        (void)PR_UnloadLibrary(lib);
    } else
        rv = PR_FAILURE;
    return rv;
}
#endif

#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)



static PRStatus AppendV4AddrsToHostent(
    struct hostent *from,
    char **buf,
    PRIntn *bufsize,
    PRHostEnt *to)
{
    PRIntn na, na_old;
    char **ap;
    char **new_addr_list;
			
    
    for (na_old = 0, ap = to->h_addr_list; *ap != 0; na_old++, ap++)
        {;} 
    for (na = na_old + 1, ap = from->h_addr_list; *ap != 0; na++, ap++)
        {;} 
    new_addr_list = (char**)Alloc(
        na * sizeof(char*), buf, bufsize, sizeof(char**));
    if (!new_addr_list) return PR_FAILURE;

    
    for (na = 0, ap = to->h_addr_list; *ap != 0; na++, ap++) {
        new_addr_list[na] = to->h_addr_list[na];
    }
    to->h_addr_list = new_addr_list;

    
    for (ap = from->h_addr_list; *ap != 0; na++, ap++) {
        to->h_addr_list[na] = Alloc(to->h_length, buf, bufsize, 0);
        if (!to->h_addr_list[na]) return PR_FAILURE;
        MakeIPv4MappedAddr(*ap, to->h_addr_list[na]);
    }
    to->h_addr_list[na] = 0;
    return PR_SUCCESS;
}
#endif

PR_IMPLEMENT(PRStatus) PR_GetIPNodeByName(
    const char *name, PRUint16 af, PRIntn flags,
    char *buf, PRIntn bufsize, PRHostEnt *hp)
{
	struct hostent *h = 0;
	PRStatus rv = PR_FAILURE;
#if defined(_PR_HAVE_GETHOST_R)
    char localbuf[PR_NETDB_BUF_SIZE];
    char *tmpbuf;
    struct hostent tmphe;
    int h_err;
#endif
#if defined(_PR_HAVE_GETIPNODEBYNAME)
	PRUint16 md_af = af;
	int error_num;
	int tmp_flags = 0;
#endif
#if defined(_PR_HAVE_GETHOSTBYNAME2)
    PRBool did_af_inet = PR_FALSE;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (af != PR_AF_INET && af != PR_AF_INET6) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)
    PR_Lock(_pr_query_ifs_lock);
    




    if (!_pr_have_inet_if && !_pr_have_inet6_if) {
	_pr_QueryNetIfs();
#ifdef DEBUG_QUERY_IFS
	if (_pr_have_inet_if)
		printf("Have IPv4 source address\n");
	if (_pr_have_inet6_if)
		printf("Have IPv6 source address\n");
#endif
    }
    PR_Unlock(_pr_query_ifs_lock);
#endif

#if defined(_PR_HAVE_GETIPNODEBYNAME)
	if (flags & PR_AI_V4MAPPED)
		tmp_flags |= AI_V4MAPPED;
	if (flags & PR_AI_ADDRCONFIG)
		tmp_flags |= AI_ADDRCONFIG;
	if (flags & PR_AI_ALL)
		tmp_flags |= AI_ALL;
    if (af == PR_AF_INET6)
    	md_af = AF_INET6;
	else
    	md_af = af;
#endif

#if defined(_PR_HAVE_GETHOST_R)
    tmpbuf = localbuf;
    if (bufsize > sizeof(localbuf))
    {
        tmpbuf = (char *)PR_Malloc(bufsize);
        if (NULL == tmpbuf)
        {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            return rv;
        }
    }
#endif

    
#ifdef _PR_INET6
#ifdef _PR_HAVE_GETHOSTBYNAME2
    LOCK_DNS();
    if (af == PR_AF_INET6)
    {
        if ((flags & PR_AI_ADDRCONFIG) == 0 || _pr_have_inet6_if)
        {
#ifdef _PR_INET6_PROBE
          if (_pr_ipv6_is_present())
#endif
            h = GETHOSTBYNAME2(name, AF_INET6); 
        }
        if ((NULL == h) && (flags & PR_AI_V4MAPPED)
        && ((flags & PR_AI_ADDRCONFIG) == 0 || _pr_have_inet_if))
        {
            did_af_inet = PR_TRUE;
            h = GETHOSTBYNAME2(name, AF_INET);
        }
    }
    else
    {
        if ((flags & PR_AI_ADDRCONFIG) == 0 || _pr_have_inet_if)
        {
            did_af_inet = PR_TRUE;
            h = GETHOSTBYNAME2(name, af);
        }
    }
#elif defined(_PR_HAVE_GETIPNODEBYNAME)
    h = getipnodebyname(name, md_af, tmp_flags, &error_num);
#else
#error "Unknown name-to-address translation function"
#endif	
#elif defined(_PR_INET6_PROBE) && defined(_PR_HAVE_GETIPNODEBYNAME)
    if (_pr_ipv6_is_present())
    {
#ifdef PR_GETIPNODE_NOT_THREADSAFE
        LOCK_DNS();
#endif
    	h = (*((_pr_getipnodebyname_t)_pr_getipnodebyname_fp))(name, md_af, tmp_flags, &error_num);
    }
    else
    {
        LOCK_DNS();
    	h = GETHOSTBYNAME(name);
    }
#else 
    LOCK_DNS();
    h = GETHOSTBYNAME(name);
#endif 
    
	if (NULL == h)
	{
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
	    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, error_num);
#elif defined(_PR_INET6_PROBE) && defined(_PR_HAVE_GETIPNODEBYNAME)
    	if (_pr_ipv6_is_present())
	    	PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, error_num);
		else
	    	PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
#else
	    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
#endif
	}
	else
	{
		_PRIPAddrConversion conversion = _PRIPAddrNoConversion;

		if (af == PR_AF_INET6) conversion = _PRIPAddrIPv4Mapped;
		rv = CopyHostent(h, &buf, &bufsize, conversion, hp);
		if (PR_SUCCESS != rv)
		    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
		freehostent(h);
#elif defined(_PR_INET6_PROBE) && defined(_PR_HAVE_GETIPNODEBYNAME)
    	if (_pr_ipv6_is_present())
			(*((_pr_freehostent_t)_pr_freehostent_fp))(h);
#endif
#if defined(_PR_INET6) && defined(_PR_HAVE_GETHOSTBYNAME2)
		if ((PR_SUCCESS == rv) && (flags & PR_AI_V4MAPPED)
				&& ((flags & PR_AI_ALL)
				|| ((flags & PR_AI_ADDRCONFIG) && _pr_have_inet_if))
				&& !did_af_inet && (h = GETHOSTBYNAME2(name, AF_INET)) != 0) {
			rv = AppendV4AddrsToHostent(h, &buf, &bufsize, hp);
			if (PR_SUCCESS != rv)
				PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
		}
#endif
	}

    
#ifdef _PR_INET6
#ifdef _PR_HAVE_GETHOSTBYNAME2
    UNLOCK_DNS();
#endif	
#elif defined(_PR_INET6_PROBE) && defined(_PR_HAVE_GETIPNODEBYNAME)
#ifdef PR_GETIPNODE_NOT_THREADSAFE
    UNLOCK_DNS();
#else
    if (!_pr_ipv6_is_present())
        UNLOCK_DNS();
#endif
#else 
    UNLOCK_DNS();
#endif 

#if defined(_PR_HAVE_GETHOST_R)
    if (tmpbuf != localbuf)
        PR_Free(tmpbuf);
#endif

	return rv;
}

PR_IMPLEMENT(PRStatus) PR_GetHostByAddr(
    const PRNetAddr *hostaddr, char *buf, PRIntn bufsize, PRHostEnt *hostentry)
{
	struct hostent *h;
	PRStatus rv = PR_FAILURE;
	const void *addr;
	PRUint32 tmp_ip;
	int addrlen;
	PRInt32 af;
#if defined(_PR_HAVE_GETHOST_R)
    char localbuf[PR_NETDB_BUF_SIZE];
    char *tmpbuf;
    struct hostent tmphe;
    int h_err;
#endif
#if defined(_PR_HAVE_GETIPNODEBYADDR)
	int error_num;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

	if (hostaddr->raw.family == PR_AF_INET6)
	{
#if defined(_PR_INET6_PROBE)
		af = _pr_ipv6_is_present() ? AF_INET6 : AF_INET;
#elif defined(_PR_INET6)
		af = AF_INET6;
#else
		af = AF_INET;
#endif
#if defined(_PR_GHBA_DISALLOW_V4MAPPED)
		if (_PR_IN6_IS_ADDR_V4MAPPED(&hostaddr->ipv6.ip))
			af = AF_INET;
#endif
	}
	else
	{
		PR_ASSERT(hostaddr->raw.family == AF_INET);
		af = AF_INET;
	}
	if (hostaddr->raw.family == PR_AF_INET6) {
#if defined(_PR_INET6) || defined(_PR_INET6_PROBE)
		if (af == AF_INET6) {
			addr = &hostaddr->ipv6.ip;
			addrlen = sizeof(hostaddr->ipv6.ip);
		}
		else
#endif
		{
			PR_ASSERT(af == AF_INET);
			if (!_PR_IN6_IS_ADDR_V4MAPPED(&hostaddr->ipv6.ip)) {
				PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
				return rv;
			}
			tmp_ip = _PR_IN6_V4MAPPED_TO_IPADDR((PRIPv6Addr *)
												&hostaddr->ipv6.ip);
			addr = &tmp_ip;
			addrlen = sizeof(tmp_ip);
		}
	} else {
		PR_ASSERT(hostaddr->raw.family == AF_INET);
		PR_ASSERT(af == AF_INET);
		addr = &hostaddr->inet.ip;
		addrlen = sizeof(hostaddr->inet.ip);
	}

#if defined(_PR_HAVE_GETHOST_R)
    tmpbuf = localbuf;
    if (bufsize > sizeof(localbuf))
    {
        tmpbuf = (char *)PR_Malloc(bufsize);
        if (NULL == tmpbuf)
        {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            return rv;
        }
    }
#endif

    
#if defined(_PR_HAVE_GETIPNODEBYADDR) && defined(_PR_INET6)
	h = getipnodebyaddr(addr, addrlen, af, &error_num);
#elif defined(_PR_HAVE_GETIPNODEBYADDR) && defined(_PR_INET6_PROBE)
    if (_pr_ipv6_is_present())
    {
#ifdef PR_GETIPNODE_NOT_THREADSAFE
        LOCK_DNS();
#endif
    	h = (*((_pr_getipnodebyaddr_t)_pr_getipnodebyaddr_fp))(addr, addrlen,
				af, &error_num);
    }
	else
    {
        LOCK_DNS();
		h = GETHOSTBYADDR(addr, addrlen, af);
    }
#else	
    LOCK_DNS();
	h = GETHOSTBYADDR(addr, addrlen, af);
#endif 
	if (NULL == h)
	{
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYADDR)
		PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, error_num);
#elif defined(_PR_INET6_PROBE) && defined(_PR_HAVE_GETIPNODEBYADDR)
    	if (_pr_ipv6_is_present())
	    	PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, error_num);
		else
	    	PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
#else
		PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_GETHOST_ERRNO());
#endif
	}
	else
	{
		_PRIPAddrConversion conversion = _PRIPAddrNoConversion;
		if (hostaddr->raw.family == PR_AF_INET6) {
			if (af == AF_INET) {
				if (_PR_IN6_IS_ADDR_V4MAPPED((PRIPv6Addr*)
												&hostaddr->ipv6.ip)) {
					conversion = _PRIPAddrIPv4Mapped;
				} else if (_PR_IN6_IS_ADDR_V4COMPAT((PRIPv6Addr *)
													&hostaddr->ipv6.ip)) {
					conversion = _PRIPAddrIPv4Compat;
				}
			}
		}
		rv = CopyHostent(h, &buf, &bufsize, conversion, hostentry);
		if (PR_SUCCESS != rv) {
		    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
		}
#if defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYADDR)
		freehostent(h);
#elif defined(_PR_INET6_PROBE) && defined(_PR_HAVE_GETIPNODEBYADDR)
    	if (_pr_ipv6_is_present())
			(*((_pr_freehostent_t)_pr_freehostent_fp))(h);
#endif
	}

    
#if defined(_PR_HAVE_GETIPNODEBYADDR) && defined(_PR_INET6)
#elif defined(_PR_HAVE_GETIPNODEBYADDR) && defined(_PR_INET6_PROBE)
#ifdef PR_GETIPNODE_NOT_THREADSAFE
    UNLOCK_DNS();
#else
    if (!_pr_ipv6_is_present())
        UNLOCK_DNS();
#endif
#else	
    UNLOCK_DNS();
#endif 

#if defined(_PR_HAVE_GETHOST_R)
    if (tmpbuf != localbuf)
        PR_Free(tmpbuf);
#endif

	return rv;
}









#if !defined(_PR_HAVE_GETPROTO_R)







#if defined(ANDROID)





#define getprotobyname_r _pr_getprotobyname_r
#define getprotobynumber_r _pr_getprotobynumber_r
#endif

static struct protoent *getprotobyname_r(const char* name)
{
	return getprotobyname(name);
} 

static struct protoent *getprotobynumber_r(PRInt32 number)
{
	return getprotobynumber(number);
} 

#endif 

PR_IMPLEMENT(PRStatus) PR_GetProtoByName(
    const char* name, char* buffer, PRInt32 buflen, PRProtoEnt* result)
{
	PRStatus rv = PR_SUCCESS;
#if defined(_PR_HAVE_GETPROTO_R)
	struct protoent* res = (struct protoent*)result;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

#if defined(_PR_HAVE_GETPROTO_R_INT)
    {
        




        PRUptrdiff aligned = (PRUptrdiff)buffer;
        if (0 != (aligned & (sizeof(struct protoent_data*) - 1)))
        {
            aligned += sizeof(struct protoent_data*) - 1;
            aligned &= ~(sizeof(struct protoent_data*) - 1);
            buflen -= (aligned - (PRUptrdiff)buffer);
            buffer = (char*)aligned;
        }
    }
#endif  

    if (PR_NETDB_BUF_SIZE > buflen)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

#if defined(_PR_HAVE_GETPROTO_R_POINTER)
    if (NULL == getprotobyname_r(name, res, buffer, buflen))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#elif defined(_PR_HAVE_GETPROTO_R_INT)
    



    memset(buffer, 0, buflen);
	if (-1 == getprotobyname_r(name, res, (struct protoent_data*)buffer))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#elif defined(_PR_HAVE_5_ARG_GETPROTO_R)
    
    if (-1 == getprotobyname_r(name, res, buffer, buflen, &res))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#else  
	{
		struct protoent *staticBuf;
		PR_Lock(_getproto_lock);
		staticBuf = getprotobyname_r(name);
		if (NULL == staticBuf)
		{
		    rv = PR_FAILURE;
		    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        }
		else
		{
#if defined(SYMBIAN)
			char* aliases[2];
			AssignAliases(staticBuf, aliases);
#endif
			rv = CopyProtoent(staticBuf, buffer, buflen, result);
			if (PR_FAILURE == rv)
			    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
        }
		PR_Unlock(_getproto_lock);
	}
#endif  
    return rv;
}

PR_IMPLEMENT(PRStatus) PR_GetProtoByNumber(
    PRInt32 number, char* buffer, PRInt32 buflen, PRProtoEnt* result)
{
	PRStatus rv = PR_SUCCESS;
#if defined(_PR_HAVE_GETPROTO_R)
	struct protoent* res = (struct protoent*)result;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

#if defined(_PR_HAVE_GETPROTO_R_INT)
    {
        




        PRUptrdiff aligned = (PRUptrdiff)buffer;
        if (0 != (aligned & (sizeof(struct protoent_data*) - 1)))
        {
            aligned += sizeof(struct protoent_data*) - 1;
            aligned &= ~(sizeof(struct protoent_data*) - 1);
            buflen -= (aligned - (PRUptrdiff)buffer);
            buffer = (char*)aligned;
        }
    }
#endif 

    if (PR_NETDB_BUF_SIZE > buflen)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

#if defined(_PR_HAVE_GETPROTO_R_POINTER)
    if (NULL == getprotobynumber_r(number, res, buffer, buflen))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }

#elif defined(_PR_HAVE_GETPROTO_R_INT)
    


    memset(buffer, 0, buflen);
	if (-1 == getprotobynumber_r(number, res, (struct protoent_data*)buffer))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#elif defined(_PR_HAVE_5_ARG_GETPROTO_R)
    
    if (-1 == getprotobynumber_r(number, res, buffer, buflen, &res))
    {
        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        return PR_FAILURE;
    }
#else  
	{
		struct protoent *staticBuf;
		PR_Lock(_getproto_lock);
		staticBuf = getprotobynumber_r(number);
		if (NULL == staticBuf)
		{
		    rv = PR_FAILURE;
		    PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, _MD_ERRNO());
        }
		else
		{
#if defined(SYMBIAN)
			char* aliases[2];
			AssignAliases(staticBuf, aliases);
#endif
			rv = CopyProtoent(staticBuf, buffer, buflen, result);
			if (PR_FAILURE == rv)
			    PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
        }
		PR_Unlock(_getproto_lock);
	}
#endif  
    return rv;

}

PRUintn _PR_NetAddrSize(const PRNetAddr* addr)
{
    PRUintn addrsize;

    







    if (AF_INET == addr->raw.family)
        addrsize = sizeof(addr->inet);
    else if (PR_AF_INET6 == addr->raw.family)
#if defined(_PR_INET6)
        addrsize = sizeof(struct sockaddr_in6);
#else
        addrsize = sizeof(addr->ipv6);
#endif
#if defined(XP_UNIX) || defined(XP_OS2)
    else if (AF_UNIX == addr->raw.family)
        addrsize = sizeof(addr->local);
#endif
    else addrsize = 0;

    return addrsize;
}  

PR_IMPLEMENT(PRIntn) PR_EnumerateHostEnt(
    PRIntn enumIndex, const PRHostEnt *hostEnt, PRUint16 port, PRNetAddr *address)
{
    void *addr = hostEnt->h_addr_list[enumIndex++];
    memset(address, 0, sizeof(PRNetAddr));
    if (NULL == addr) enumIndex = 0;
    else
    {
        address->raw.family = hostEnt->h_addrtype;
        if (PR_AF_INET6 == hostEnt->h_addrtype)
        {
            address->ipv6.port = htons(port);
        	address->ipv6.flowinfo = 0;
        	address->ipv6.scope_id = 0;
            memcpy(&address->ipv6.ip, addr, hostEnt->h_length);
        }
        else
        {
            PR_ASSERT(AF_INET == hostEnt->h_addrtype);
            address->inet.port = htons(port);
            memcpy(&address->inet.ip, addr, hostEnt->h_length);
        }
    }
    return enumIndex;
}  

PR_IMPLEMENT(PRStatus) PR_InitializeNetAddr(
    PRNetAddrValue val, PRUint16 port, PRNetAddr *addr)
{
    PRStatus rv = PR_SUCCESS;
    if (!_pr_initialized) _PR_ImplicitInitialization();

	if (val != PR_IpAddrNull) memset(addr, 0, sizeof(addr->inet));
	addr->inet.family = AF_INET;
	addr->inet.port = htons(port);
	switch (val)
	{
	case PR_IpAddrNull:
		break;  
	case PR_IpAddrAny:
		addr->inet.ip = htonl(INADDR_ANY);
		break;
	case PR_IpAddrLoopback:
		addr->inet.ip = htonl(INADDR_LOOPBACK);
		break;
	default:
		PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
		rv = PR_FAILURE;
	}
    return rv;
}  

PR_IMPLEMENT(PRStatus) PR_SetNetAddr(
    PRNetAddrValue val, PRUint16 af, PRUint16 port, PRNetAddr *addr)
{
    PRStatus rv = PR_SUCCESS;
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (af == PR_AF_INET6)
    {
        if (val != PR_IpAddrNull) memset(addr, 0, sizeof(addr->ipv6));
        addr->ipv6.family = af;
        addr->ipv6.port = htons(port);
        addr->ipv6.flowinfo = 0;
        addr->ipv6.scope_id = 0;
        switch (val)
        {
        case PR_IpAddrNull:
            break;  
        case PR_IpAddrAny:
            addr->ipv6.ip = _pr_in6addr_any;
            break;
        case PR_IpAddrLoopback:
            addr->ipv6.ip = _pr_in6addr_loopback;
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = PR_FAILURE;
        }
    }
    else
    {
        if (val != PR_IpAddrNull) memset(addr, 0, sizeof(addr->inet));
        addr->inet.family = af;
        addr->inet.port = htons(port);
        switch (val)
        {
        case PR_IpAddrNull:
            break;  
        case PR_IpAddrAny:
            addr->inet.ip = htonl(INADDR_ANY);
            break;
        case PR_IpAddrLoopback:
            addr->inet.ip = htonl(INADDR_LOOPBACK);
            break;
        default:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            rv = PR_FAILURE;
        }
    }
    return rv;
}  

PR_IMPLEMENT(PRBool)
PR_IsNetAddrType(const PRNetAddr *addr, PRNetAddrValue val)
{
    if (addr->raw.family == PR_AF_INET6) {
        if (val == PR_IpAddrAny) {
			if (_PR_IN6_IS_ADDR_UNSPECIFIED((PRIPv6Addr *)&addr->ipv6.ip)) {
            	return PR_TRUE;
			} else if (_PR_IN6_IS_ADDR_V4MAPPED((PRIPv6Addr *)&addr->ipv6.ip)
					&& _PR_IN6_V4MAPPED_TO_IPADDR((PRIPv6Addr *)&addr->ipv6.ip)
							== htonl(INADDR_ANY)) {
            	return PR_TRUE;
			}
        } else if (val == PR_IpAddrLoopback) {
            if (_PR_IN6_IS_ADDR_LOOPBACK((PRIPv6Addr *)&addr->ipv6.ip)) {
            	return PR_TRUE;
			} else if (_PR_IN6_IS_ADDR_V4MAPPED((PRIPv6Addr *)&addr->ipv6.ip)
					&& _PR_IN6_V4MAPPED_TO_IPADDR((PRIPv6Addr *)&addr->ipv6.ip)
							== htonl(INADDR_LOOPBACK)) {
            	return PR_TRUE;
			}
        } else if (val == PR_IpAddrV4Mapped
                && _PR_IN6_IS_ADDR_V4MAPPED((PRIPv6Addr *)&addr->ipv6.ip)) {
            return PR_TRUE;
        }
    } else {
        if (addr->raw.family == AF_INET) {
            if (val == PR_IpAddrAny && addr->inet.ip == htonl(INADDR_ANY)) {
                return PR_TRUE;
            } else if (val == PR_IpAddrLoopback
                    && addr->inet.ip == htonl(INADDR_LOOPBACK)) {
                return PR_TRUE;
            }
        }
    }
    return PR_FALSE;
}

extern int pr_inet_aton(const char *cp, PRUint32 *addr);

#define XX 127
static const unsigned char index_hex[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};






static int StringToV6Addr(const char *string, PRIPv6Addr *addr)
{
    const unsigned char *s = (const unsigned char *)string;
    int section = 0;        

    int double_colon = -1;  


    unsigned int val;
    int len;

    
    if (*s == ':') {
        if (s[1] != ':') return 0;
        s += 2;
        addr->pr_s6_addr16[0] = 0;
        section = double_colon = 1;
    }

    while (*s) {
        if (section == 8) return 0; 
        if (*s == ':') {
            if (double_colon != -1) return 0; 
            addr->pr_s6_addr16[section++] = 0;
            double_colon = section;
            s++;
            continue;
        }
        for (len = val = 0; len < 4 && index_hex[*s] != XX; len++) {
            val = (val << 4) + index_hex[*s++];
        }
        if (*s == '.') {
            if (len == 0) return 0; 
            break;
        }
        if (*s == ':') {
            s++;
            if (!*s) return 0; 
        } else if (*s) {
            return 0; 
        }
        addr->pr_s6_addr16[section++] = htons((unsigned short)val);
    }
    
    if (*s == '.') {
        
        if (section > 6) return 0; 

        




        if (val > 0x0255 || (val & 0xf0) > 0x90 || (val & 0xf) > 9) return 0;
        val = (val >> 8) * 100 + ((val >> 4) & 0xf) * 10 + (val & 0xf);
        addr->pr_s6_addr[2 * section] = val;

        s++;
        val = index_hex[*s++];
        if (val > 9) return 0;
        while (*s >= '0' && *s <= '9') {
            val = val * 10 + *s++ - '0';
            if (val > 255) return 0;
        }
        if (*s != '.') return 0; 
        addr->pr_s6_addr[2 * section + 1] = val;
        section++;

        s++;
        val = index_hex[*s++];
        if (val > 9) return 0;
        while (*s >= '0' && *s <= '9') {
            val = val * 10 + *s++ - '0';
            if (val > 255) return 0;
        }
        if (*s != '.') return 0; 
        addr->pr_s6_addr[2 * section] = val;

        s++;
        val = index_hex[*s++];
        if (val > 9) return 0;
        while (*s >= '0' && *s <= '9') {
            val = val * 10 + *s++ - '0';
            if (val > 255) return 0;
        }
        if (*s) return 0; 
        addr->pr_s6_addr[2 * section + 1] = val;
        section++;
    }
    
    if (double_colon != -1) {
        
        int tosection;
        int ncopy = section - double_colon;
        for (tosection = 7; ncopy--; tosection--) {
            addr->pr_s6_addr16[tosection] = 
                addr->pr_s6_addr16[double_colon + ncopy];
        }
        while (tosection >= double_colon) {
            addr->pr_s6_addr16[tosection--] = 0;
        }
    } else if (section != 8) {
        return 0; 
    }
    return 1;
}
#undef XX

#ifndef _PR_HAVE_INET_NTOP
static const char *basis_hex = "0123456789abcdef";







static const char *V6AddrToString(
    const PRIPv6Addr *addr, char *buf, PRUint32 size)
{
#define STUFF(c) do { \
    if (!size--) return NULL; \
    *buf++ = (c); \
} while (0)

    int double_colon = -1;          


    int double_colon_length = 1;    


    int zero_length;
    int section;
    unsigned int val;
    const char *bufcopy = buf;

    
    for (section = 0; section < 8; section++) {
        if (addr->pr_s6_addr16[section] == 0) {
            zero_length = 1;
            section++;
            while (section < 8 && addr->pr_s6_addr16[section] == 0) {
                zero_length++;
                section++;
            }
            
            if (zero_length > double_colon_length) {
                double_colon = section - zero_length;
                double_colon_length = zero_length;
            }
        }
    }

    
    section = 0;

    if (double_colon == 0) {
        if (double_colon_length == 6 ||
            (double_colon_length == 5 && addr->pr_s6_addr16[5] == 0xffff)) {
            
            STUFF(':');
            STUFF(':');
            if (double_colon_length == 5) {
                STUFF('f');
                STUFF('f');
                STUFF('f');
                STUFF('f');
                STUFF(':');
            }
            if (addr->pr_s6_addr[12] > 99) STUFF(addr->pr_s6_addr[12]/100 + '0');
            if (addr->pr_s6_addr[12] > 9) STUFF((addr->pr_s6_addr[12]%100)/10 + '0');
            STUFF(addr->pr_s6_addr[12]%10 + '0');
            STUFF('.');
            if (addr->pr_s6_addr[13] > 99) STUFF(addr->pr_s6_addr[13]/100 + '0');
            if (addr->pr_s6_addr[13] > 9) STUFF((addr->pr_s6_addr[13]%100)/10 + '0');
            STUFF(addr->pr_s6_addr[13]%10 + '0');
            STUFF('.');
            if (addr->pr_s6_addr[14] > 99) STUFF(addr->pr_s6_addr[14]/100 + '0');
            if (addr->pr_s6_addr[14] > 9) STUFF((addr->pr_s6_addr[14]%100)/10 + '0');
            STUFF(addr->pr_s6_addr[14]%10 + '0');
            STUFF('.');
            if (addr->pr_s6_addr[15] > 99) STUFF(addr->pr_s6_addr[15]/100 + '0');
            if (addr->pr_s6_addr[15] > 9) STUFF((addr->pr_s6_addr[15]%100)/10 + '0');
            STUFF(addr->pr_s6_addr[15]%10 + '0');
            STUFF('\0');
            return bufcopy;
        }
    }

    while (section < 8) {
        if (section == double_colon) {
            STUFF(':');
            STUFF(':');
            section += double_colon_length;
            continue;
        }
        val = ntohs(addr->pr_s6_addr16[section]);
        if (val > 0xfff) {
            STUFF(basis_hex[val >> 12]);
        }
        if (val > 0xff) {
            STUFF(basis_hex[(val >> 8) & 0xf]);
        }
        if (val > 0xf) {
            STUFF(basis_hex[(val >> 4) & 0xf]);
        }
        STUFF(basis_hex[val & 0xf]);
        section++;
        if (section < 8 && section != double_colon) STUFF(':');
    }
    STUFF('\0');
    return bufcopy;
#undef STUFF    
}
#endif 




PR_IMPLEMENT(void) PR_ConvertIPv4AddrToIPv6(PRUint32 v4addr, PRIPv6Addr *v6addr)
{
    PRUint8 *dstp;
    dstp = v6addr->pr_s6_addr;
    memset(dstp, 0, 10);
    memset(dstp + 10, 0xff, 2);
    memcpy(dstp + 12,(char *) &v4addr, 4);
}

PR_IMPLEMENT(PRUint16) PR_ntohs(PRUint16 n) { return ntohs(n); }
PR_IMPLEMENT(PRUint32) PR_ntohl(PRUint32 n) { return ntohl(n); }
PR_IMPLEMENT(PRUint16) PR_htons(PRUint16 n) { return htons(n); }
PR_IMPLEMENT(PRUint32) PR_htonl(PRUint32 n) { return htonl(n); }
PR_IMPLEMENT(PRUint64) PR_ntohll(PRUint64 n)
{
#ifdef IS_BIG_ENDIAN
    return n;
#else
    PRUint64 tmp;
    PRUint32 hi, lo;
    LL_L2UI(lo, n);
    LL_SHR(tmp, n, 32);
    LL_L2UI(hi, tmp);
    hi = PR_ntohl(hi);
    lo = PR_ntohl(lo);
    LL_UI2L(n, lo);
    LL_SHL(n, n, 32);
    LL_UI2L(tmp, hi);
    LL_ADD(n, n, tmp);
    return n;
#endif
}  

PR_IMPLEMENT(PRUint64) PR_htonll(PRUint64 n)
{
#ifdef IS_BIG_ENDIAN
    return n;
#else
    PRUint64 tmp;
    PRUint32 hi, lo;
    LL_L2UI(lo, n);
    LL_SHR(tmp, n, 32);
    LL_L2UI(hi, tmp);
    hi = htonl(hi);
    lo = htonl(lo);
    LL_UI2L(n, lo);
    LL_SHL(n, n, 32);
    LL_UI2L(tmp, hi);
    LL_ADD(n, n, tmp);
    return n;
#endif
}  




















#if defined(_PR_HAVE_GETADDRINFO)

#if defined(_PR_INET6)

typedef struct addrinfo PRADDRINFO;
#define GETADDRINFO getaddrinfo
#define FREEADDRINFO freeaddrinfo
#define GETNAMEINFO getnameinfo

#elif defined(_PR_INET6_PROBE)

typedef struct addrinfo PRADDRINFO;

 
#if defined(WIN32)
#define FUNC_MODIFIER __stdcall
#else
#define FUNC_MODIFIER
#endif
typedef int (FUNC_MODIFIER * FN_GETADDRINFO)
    (const char *nodename,
     const char *servname,
     const PRADDRINFO *hints,
     PRADDRINFO **res);
typedef int (FUNC_MODIFIER * FN_FREEADDRINFO)
    (PRADDRINFO *ai);
typedef int (FUNC_MODIFIER * FN_GETNAMEINFO)
    (const struct sockaddr *addr, int addrlen,
     char *host, int hostlen,
     char *serv, int servlen, int flags);


static FN_GETADDRINFO   _pr_getaddrinfo   = NULL;
static FN_FREEADDRINFO  _pr_freeaddrinfo  = NULL;
static FN_GETNAMEINFO   _pr_getnameinfo   = NULL;

#define GETADDRINFO_SYMBOL "getaddrinfo"
#define FREEADDRINFO_SYMBOL "freeaddrinfo"
#define GETNAMEINFO_SYMBOL "getnameinfo"

PRStatus
_pr_find_getaddrinfo(void)
{
    PRLibrary *lib;
#ifdef WIN32
    





    const char *libname[] = { "ws2_32.dll", "wship6.dll" };
    int i;

    for (i = 0; i < sizeof(libname)/sizeof(libname[0]); i++) {
        lib = PR_LoadLibrary(libname[i]);
        if (!lib) {
            continue;
        }
        _pr_getaddrinfo = (FN_GETADDRINFO)
            PR_FindFunctionSymbol(lib, GETADDRINFO_SYMBOL);
        if (!_pr_getaddrinfo) {
            PR_UnloadLibrary(lib);
            continue;
        }
        _pr_freeaddrinfo = (FN_FREEADDRINFO)
            PR_FindFunctionSymbol(lib, FREEADDRINFO_SYMBOL);
        _pr_getnameinfo = (FN_GETNAMEINFO)
            PR_FindFunctionSymbol(lib, GETNAMEINFO_SYMBOL);
        if (!_pr_freeaddrinfo || !_pr_getnameinfo) {
            PR_UnloadLibrary(lib);
            continue;
        }
        
        return PR_SUCCESS;
    }
    return PR_FAILURE;
#else
    



    _pr_getaddrinfo = (FN_GETADDRINFO)
        PR_FindFunctionSymbolAndLibrary(GETADDRINFO_SYMBOL, &lib);
    if (!_pr_getaddrinfo) {
        return PR_FAILURE;
    }
    _pr_freeaddrinfo = (FN_FREEADDRINFO)
        PR_FindFunctionSymbol(lib, FREEADDRINFO_SYMBOL);
    _pr_getnameinfo = (FN_GETNAMEINFO)
        PR_FindFunctionSymbol(lib, GETNAMEINFO_SYMBOL);
    PR_UnloadLibrary(lib);
    if (!_pr_freeaddrinfo || !_pr_getnameinfo) {
        return PR_FAILURE;
    }
    return PR_SUCCESS;
#endif
}

#define GETADDRINFO (*_pr_getaddrinfo)
#define FREEADDRINFO (*_pr_freeaddrinfo)
#define GETNAMEINFO (*_pr_getnameinfo)

#endif 

#endif 

#if !defined(_PR_HAVE_GETADDRINFO) || defined(_PR_INET6_PROBE)





typedef struct PRAddrInfoFB {
    char      buf[PR_NETDB_BUF_SIZE];
    PRHostEnt hostent;
    PRBool    has_cname;
} PRAddrInfoFB;

static PRAddrInfo *
pr_GetAddrInfoByNameFB(const char  *hostname,
                       PRUint16     af,
                       PRIntn       flags)
{
    PRStatus rv;
    PRAddrInfoFB *ai;
    
    ai = PR_NEW(PRAddrInfoFB);
    if (!ai) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }
    rv = PR_GetHostByName(hostname, ai->buf, sizeof ai->buf, &ai->hostent);
    if (rv == PR_FAILURE) {
        PR_Free(ai);
        return NULL;
    }
    ai->has_cname = !(flags & PR_AI_NOCANONNAME);

    return (PRAddrInfo *) ai;
}
#endif 

PR_IMPLEMENT(PRAddrInfo *) PR_GetAddrInfoByName(const char  *hostname,
                                                PRUint16     af,
                                                PRIntn       flags)
{
    
    if ((af != PR_AF_INET && af != PR_AF_UNSPEC) ||
        (flags & ~ PR_AI_NOCANONNAME) != PR_AI_ADDRCONFIG) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return NULL;
    }

    if (!_pr_initialized) _PR_ImplicitInitialization();

#if !defined(_PR_HAVE_GETADDRINFO)
    return pr_GetAddrInfoByNameFB(hostname, af, flags);
#else
#if defined(_PR_INET6_PROBE)
    if (!_pr_ipv6_is_present()) {
        return pr_GetAddrInfoByNameFB(hostname, af, flags);
    }
#endif
    {
        PRADDRINFO *res, hints;
        PRStatus rv;

        





        memset(&hints, 0, sizeof(hints));
        if (!(flags & PR_AI_NOCANONNAME))
            hints.ai_flags |= AI_CANONNAME;
#ifdef AI_ADDRCONFIG
        














        if ((flags & PR_AI_ADDRCONFIG) &&
            strcmp(hostname, "localhost") != 0 &&
            strcmp(hostname, "localhost.localdomain") != 0 &&
            strcmp(hostname, "localhost6") != 0 &&
            strcmp(hostname, "localhost6.localdomain6") != 0) {
            hints.ai_flags |= AI_ADDRCONFIG;
        }
#endif
        hints.ai_family = (af == PR_AF_INET) ? AF_INET : AF_UNSPEC;

        






        hints.ai_socktype = SOCK_STREAM;

        rv = GETADDRINFO(hostname, NULL, &hints, &res);
#ifdef AI_ADDRCONFIG
        if (rv == EAI_BADFLAGS && (hints.ai_flags & AI_ADDRCONFIG)) {
            hints.ai_flags &= ~AI_ADDRCONFIG;
            rv = GETADDRINFO(hostname, NULL, &hints, &res);
        }
#endif
        if (rv == 0)
            return (PRAddrInfo *) res;

        PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, rv);
    }
    return NULL;
#endif
}

PR_IMPLEMENT(void) PR_FreeAddrInfo(PRAddrInfo *ai)
{
#if defined(_PR_HAVE_GETADDRINFO)
#if defined(_PR_INET6_PROBE)
    if (!_pr_ipv6_is_present())
        PR_Free((PRAddrInfoFB *) ai);
    else
#endif
        FREEADDRINFO((PRADDRINFO *) ai);
#else
    PR_Free((PRAddrInfoFB *) ai);
#endif
}

PR_IMPLEMENT(void *) PR_EnumerateAddrInfo(void             *iterPtr,
                                          const PRAddrInfo *base,
                                          PRUint16          port,
                                          PRNetAddr        *result)
{
#if defined(_PR_HAVE_GETADDRINFO)
    PRADDRINFO *ai;
#if defined(_PR_INET6_PROBE)
    if (!_pr_ipv6_is_present()) {
        
        PRIntn iter = (PRIntn)(PRPtrdiff) iterPtr;
        iter = PR_EnumerateHostEnt(iter, &((PRAddrInfoFB *) base)->hostent, port, result);
        if (iter < 0)
            iter = 0;
        return (void *)(PRPtrdiff) iter;
    }
#endif

    if (iterPtr)
        ai = ((PRADDRINFO *) iterPtr)->ai_next;
    else
        ai = (PRADDRINFO *) base;

    while (ai && ai->ai_addrlen > sizeof(PRNetAddr))
        ai = ai->ai_next;

    if (ai) {
        
        memcpy(result, ai->ai_addr, ai->ai_addrlen);
        result->raw.family = ai->ai_addr->sa_family;
#ifdef _PR_INET6
        if (AF_INET6 == result->raw.family)
            result->raw.family = PR_AF_INET6;
#endif
        if (ai->ai_addrlen < sizeof(PRNetAddr))
            memset(((char*)result)+ai->ai_addrlen, 0, sizeof(PRNetAddr) - ai->ai_addrlen);

        if (result->raw.family == PR_AF_INET)
            result->inet.port = htons(port);
        else
            result->ipv6.port = htons(port);
    }

    return ai;
#else
    
    PRIntn iter = (PRIntn) iterPtr;
    iter = PR_EnumerateHostEnt(iter, &((PRAddrInfoFB *) base)->hostent, port, result);
    if (iter < 0)
        iter = 0;
    return (void *) iter;
#endif
}

PR_IMPLEMENT(const char *) PR_GetCanonNameFromAddrInfo(const PRAddrInfo *ai)
{
#if defined(_PR_HAVE_GETADDRINFO)
#if defined(_PR_INET6_PROBE)
    if (!_pr_ipv6_is_present()) {
        const PRAddrInfoFB *fb = (const PRAddrInfoFB *) ai;
        return fb->has_cname ? fb->hostent.h_name : NULL;
    } 
#endif
    return ((const PRADDRINFO *) ai)->ai_canonname;
#else
    const PRAddrInfoFB *fb = (const PRAddrInfoFB *) ai;
    return fb->has_cname ? fb->hostent.h_name : NULL;
#endif
}

#if defined(_PR_HAVE_GETADDRINFO)
static PRStatus pr_StringToNetAddrGAI(const char *string, PRNetAddr *addr)
{
    PRADDRINFO *res, hints;
    int rv;  
    PRNetAddr laddr;
    PRStatus status = PR_SUCCESS;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rv = GETADDRINFO(string, NULL, &hints, &res);
    if (rv != 0)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, rv);
        return PR_FAILURE;
    }

    
    memcpy(&laddr, res->ai_addr, res->ai_addrlen);
    if (AF_INET6 == res->ai_addr->sa_family)
    {
        addr->ipv6.family = PR_AF_INET6;
        addr->ipv6.ip = laddr.ipv6.ip;
        addr->ipv6.scope_id = laddr.ipv6.scope_id;
    }
    else if (AF_INET == res->ai_addr->sa_family)
    {
        addr->inet.family = PR_AF_INET;
        addr->inet.ip = laddr.inet.ip;
    }
    else
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        status = PR_FAILURE;
    }

    FREEADDRINFO(res);
    return status;
}
#endif  

static PRStatus pr_StringToNetAddrFB(const char *string, PRNetAddr *addr)
{
    PRIntn rv;

    rv = pr_inet_aton(string, &addr->inet.ip);
    if (1 == rv)
    {
        addr->raw.family = AF_INET;
        return PR_SUCCESS;
    }

    PR_ASSERT(0 == rv);
    
    memset(&addr->inet.ip, 0, sizeof(addr->inet.ip));

    rv = StringToV6Addr(string, &addr->ipv6.ip);
    if (1 == rv)
    {
        addr->raw.family = PR_AF_INET6;
        return PR_SUCCESS;
    }

    PR_ASSERT(0 == rv);
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return PR_FAILURE;
}

PR_IMPLEMENT(PRStatus) PR_StringToNetAddr(const char *string, PRNetAddr *addr)
{
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (!addr || !string || !*string)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

#if !defined(_PR_HAVE_GETADDRINFO)
    return pr_StringToNetAddrFB(string, addr);
#else
#if defined(_PR_INET6_PROBE)
    if (!_pr_ipv6_is_present())
        return pr_StringToNetAddrFB(string, addr);
#endif
    





    if (!strchr(string, '%'))
        return pr_StringToNetAddrFB(string, addr);

    return pr_StringToNetAddrGAI(string, addr);
#endif
}

#if defined(_PR_HAVE_GETADDRINFO)
static PRStatus pr_NetAddrToStringGNI(
    const PRNetAddr *addr, char *string, PRUint32 size)
{
    int addrlen;
    const PRNetAddr *addrp = addr;
#if defined(_PR_HAVE_SOCKADDR_LEN) || defined(_PR_INET6)
    PRUint16 md_af = addr->raw.family;
    PRNetAddr addrcopy;
#endif
    int rv;  

#ifdef _PR_INET6
    if (addr->raw.family == PR_AF_INET6)
    {
        md_af = AF_INET6;
#ifndef _PR_HAVE_SOCKADDR_LEN
        addrcopy = *addr;
        addrcopy.raw.family = AF_INET6;
        addrp = &addrcopy;
#endif
    }
#endif

    addrlen = PR_NETADDR_SIZE(addr);
#ifdef _PR_HAVE_SOCKADDR_LEN
    addrcopy = *addr;
    ((struct sockaddr*)&addrcopy)->sa_len = addrlen;
    ((struct sockaddr*)&addrcopy)->sa_family = md_af;
    addrp = &addrcopy;
#endif
    rv = GETNAMEINFO((const struct sockaddr *)addrp, addrlen,
        string, size, NULL, 0, NI_NUMERICHOST);
    if (rv != 0)
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, rv);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}
#endif  

#if !defined(_PR_HAVE_GETADDRINFO) || defined(_PR_INET6_PROBE)
static PRStatus pr_NetAddrToStringFB(
    const PRNetAddr *addr, char *string, PRUint32 size)
{
    if (PR_AF_INET6 == addr->raw.family)
    {
#if defined(_PR_HAVE_INET_NTOP)
        if (NULL == inet_ntop(AF_INET6, &addr->ipv6.ip, string, size))
#else
        if (NULL == V6AddrToString(&addr->ipv6.ip, string, size))
#endif
        {
            
            PR_SetError(PR_BUFFER_OVERFLOW_ERROR, 0);
            return PR_FAILURE;
        }
    }
    else
    {
        if (size < 16) goto failed;
        if (AF_INET != addr->raw.family) goto failed;
        else
        {
            unsigned char *byte = (unsigned char*)&addr->inet.ip;
            PR_snprintf(string, size, "%u.%u.%u.%u",
                byte[0], byte[1], byte[2], byte[3]);
        }
    }

    return PR_SUCCESS;

failed:
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return PR_FAILURE;

}  
#endif  

PR_IMPLEMENT(PRStatus) PR_NetAddrToString(
    const PRNetAddr *addr, char *string, PRUint32 size)
{
    if (!_pr_initialized) _PR_ImplicitInitialization();

#if !defined(_PR_HAVE_GETADDRINFO)
    return pr_NetAddrToStringFB(addr, string, size);
#else
#if defined(_PR_INET6_PROBE)
    if (!_pr_ipv6_is_present())
        return pr_NetAddrToStringFB(addr, string, size);
#endif
    return pr_NetAddrToStringGNI(addr, string, size);
#endif
}  
