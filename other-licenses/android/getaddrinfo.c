








































#define ANDROID_CHANGES 1




















































#include <fcntl.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "arpa_nameser.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include "resolv_private.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <syslog.h>
#include <stdarg.h>
#include "nsswitch.h"

#ifdef MOZ_GETADDRINFO_LOG_VERBOSE
#include <android/log.h>
#endif

#ifdef ANDROID_CHANGES
#include <sys/system_properties.h>
#endif 

typedef struct _pseudo_FILE {
    int fd;
    off_t maplen;
    void* mapping;
    off_t offset;
} _pseudo_FILE;

#define _PSEUDO_FILE_INITIALIZER { -1, 0, MAP_FAILED, 0 }

static void
_pseudo_fclose(_pseudo_FILE * __restrict__ fp)
{
    assert(fp);
    fp->offset = 0;
    if (fp->mapping != MAP_FAILED) {
        (void) munmap(fp->mapping, fp->maplen);
        fp->mapping = MAP_FAILED;
    }
    fp->maplen = 0;
    if (fp->fd != -1) {
        (void) close(fp->fd);
        fp->fd = -1;
    }
}

static _pseudo_FILE *
_pseudo_fopen_r(_pseudo_FILE * __restrict__ fp, const char* fname)
{
    struct stat statbuf;
    assert(fp);
    fp->fd = open(fname, O_RDONLY);
    if (fp->fd < 0) {
        fp->fd = -1;
        return NULL;
    }
    if ((0 != fstat(fp->fd, &statbuf)) || (statbuf.st_size <= 0)) {
        close(fp->fd);
        fp->fd = -1;
        return NULL;
    }
    fp->maplen = statbuf.st_size;
    fp->mapping = mmap(NULL, fp->maplen, PROT_READ, MAP_PRIVATE, fp->fd, 0);
    if (fp->mapping == MAP_FAILED) {
        close(fp->fd);
        fp->fd = -1;
        return NULL;
    }
    fp->offset = 0;
    return fp;
}

static void
_pseudo_rewind(_pseudo_FILE * __restrict__ fp)
{
    assert(fp);
    fp->offset = 0;
}

static char*
_pseudo_fgets(char* buf, int bufsize, _pseudo_FILE * __restrict__ fp)
{
    char* current;
    char* endp;
    int maxcopy;
    assert(fp);
    maxcopy = fp->maplen - fp->offset;
    if (fp->mapping == MAP_FAILED)
        return NULL;
    if (maxcopy > bufsize - 1)
        maxcopy = bufsize - 1;
    if (maxcopy <= 0)
        return NULL;
    current = ((char*) fp->mapping) + fp->offset;
    endp = memccpy(buf, current, '\n', maxcopy);
    if (endp)
        maxcopy = endp - buf;
    buf[maxcopy] = '\0';
    fp->offset += maxcopy;
    return buf;
}

typedef union sockaddr_union {
    struct sockaddr     generic;
    struct sockaddr_in  in;
    struct sockaddr_in6 in6;
} sockaddr_union;

#define SUCCESS 0
#define ANY 0
#define YES 1
#define NO  0

static const char in_addrany[] = { 0, 0, 0, 0 };
static const char in_loopback[] = { 127, 0, 0, 1 };
#ifdef INET6
static const char in6_addrany[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const char in6_loopback[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};
#endif

static const struct afd {
	int a_af;
	int a_addrlen;
	int a_socklen;
	int a_off;
	const char *a_addrany;
	const char *a_loopback;
	int a_scoped;
} afdl [] = {
#ifdef INET6
	{PF_INET6, sizeof(struct in6_addr),
	 sizeof(struct sockaddr_in6),
	 offsetof(struct sockaddr_in6, sin6_addr),
	 in6_addrany, in6_loopback, 1},
#endif
	{PF_INET, sizeof(struct in_addr),
	 sizeof(struct sockaddr_in),
	 offsetof(struct sockaddr_in, sin_addr),
	 in_addrany, in_loopback, 0},
	{0, 0, 0, 0, NULL, NULL, 0},
};

struct explore {
	int e_af;
	int e_socktype;
	int e_protocol;
	const char *e_protostr;
	int e_wild;
#define WILD_AF(ex)		((ex)->e_wild & 0x01)
#define WILD_SOCKTYPE(ex)	((ex)->e_wild & 0x02)
#define WILD_PROTOCOL(ex)	((ex)->e_wild & 0x04)
};

static const struct explore explore[] = {
#if 0
	{ PF_LOCAL, 0, ANY, ANY, NULL, 0x01 },
#endif
#ifdef INET6
	{ PF_INET6, SOCK_DGRAM, IPPROTO_UDP, "udp", 0x07 },
	{ PF_INET6, SOCK_STREAM, IPPROTO_TCP, "tcp", 0x07 },
	{ PF_INET6, SOCK_RAW, ANY, NULL, 0x05 },
#endif
	{ PF_INET, SOCK_DGRAM, IPPROTO_UDP, "udp", 0x07 },
	{ PF_INET, SOCK_STREAM, IPPROTO_TCP, "tcp", 0x07 },
	{ PF_INET, SOCK_RAW, ANY, NULL, 0x05 },
	{ PF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, "udp", 0x07 },
	{ PF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, "tcp", 0x07 },
	{ PF_UNSPEC, SOCK_RAW, ANY, NULL, 0x05 },
	{ -1, 0, 0, NULL, 0 },
};

#ifdef INET6
#define PTON_MAX	16
#else
#define PTON_MAX	4
#endif

static const ns_src default_dns_files[] = {
	{ NSSRC_FILES, 	NS_SUCCESS },
	{ NSSRC_DNS, 	NS_SUCCESS },
	{ 0, 0 }
};

#define MAXPACKET	(64*1024)

typedef union {
	HEADER hdr;
	u_char buf[MAXPACKET];
} querybuf;

struct res_target {
	struct res_target *next;
	const char *name;	
	int qclass, qtype;	
	u_char *answer;		
	int anslen;		
	int n;			
};

static int str2number(const char *);
static int explore_fqdn(const struct addrinfo *, const char *,
	const char *, struct addrinfo **);
static int explore_null(const struct addrinfo *,
	const char *, struct addrinfo **);
static int explore_numeric(const struct addrinfo *, const char *,
	const char *, struct addrinfo **, const char *);
static int explore_numeric_scope(const struct addrinfo *, const char *,
	const char *, struct addrinfo **);
static int get_canonname(const struct addrinfo *,
	struct addrinfo *, const char *);
static struct addrinfo *get_ai(const struct addrinfo *,
	const struct afd *, const char *);
static int get_portmatch(const struct addrinfo *, const char *);
static int get_port(const struct addrinfo *, const char *, int);
static const struct afd *find_afd(int);
#ifdef INET6
static int ip6_str2scopeid(char *, struct sockaddr_in6 *, u_int32_t *);
#endif

static struct addrinfo *getanswer(const querybuf *, int, const char *, int,
	const struct addrinfo *);
static int _dns_getaddrinfo(void *, void *, va_list);
static void _sethtent(_pseudo_FILE * __restrict__);
static void _endhtent(_pseudo_FILE * __restrict__);
static struct addrinfo *_gethtent(_pseudo_FILE * __restrict__, const char *,
    const struct addrinfo *);
static int _files_getaddrinfo(void *, void *, va_list);

static int res_queryN(const char *, struct res_target *, res_state);
static int res_searchN(const char *, struct res_target *, res_state);
static int res_querydomainN(const char *, const char *,
	struct res_target *, res_state);

static const char * const ai_errlist[] = {
	"Success",
	"Address family for hostname not supported",	
	"Temporary failure in name resolution",		
	"Invalid value for ai_flags",		       	
	"Non-recoverable failure in name resolution", 	
	"ai_family not supported",			
	"Memory allocation failure", 			
	"No address associated with hostname", 		
	"hostname nor servname provided, or not known",	
	"servname not supported for ai_socktype",	
	"ai_socktype not supported", 			
	"System error returned in errno", 		
	"Invalid value for hints",			
	"Resolved protocol is unknown",			
	"Argument buffer overflow",			
	"Unknown error", 				
};



#define GET_AI(ai, afd, addr) 					\
do { 								\
	/* external reference: pai, error, and label free */ 	\
	(ai) = get_ai(pai, (afd), (addr)); 			\
	if ((ai) == NULL) { 					\
		error = EAI_MEMORY; 				\
		goto free; 					\
	} 							\
} while (/*CONSTCOND*/0)

#define GET_PORT(ai, serv) 					\
do { 								\
	/* external reference: error and label free */ 		\
	error = get_port((ai), (serv), 0); 			\
	if (error != 0) 					\
		goto free; 					\
} while (/*CONSTCOND*/0)

#define GET_CANONNAME(ai, str) 					\
do { 								\
	/* external reference: pai, error and label free */ 	\
	error = get_canonname(pai, (ai), (str)); 		\
	if (error != 0) 					\
		goto free; 					\
} while (/*CONSTCOND*/0)

#define ERR(err) 						\
do { 								\
	/* external reference: error, and label bad */ 		\
	error = (err); 						\
	goto bad; 						\
	/*NOTREACHED*/ 						\
} while (/*CONSTCOND*/0)

#define MATCH_FAMILY(x, y, w) 						\
	((x) == (y) || (/*CONSTCOND*/(w) && ((x) == PF_UNSPEC || 	\
	    (y) == PF_UNSPEC)))
#define MATCH(x, y, w) 							\
	((x) == (y) || (/*CONSTCOND*/(w) && ((x) == ANY || (y) == ANY)))

#pragma GCC visibility push(default)

extern const char *
__wrap_gai_strerror(int ecode);
extern void
__wrap_freeaddrinfo(struct addrinfo *ai);
extern int
__wrap_getaddrinfo(const char *hostname, const char *servname,
    const struct addrinfo *hints, struct addrinfo **res);

int android_sdk_version;

#pragma GCC visibility pop

int android_sdk_version = -1;

static int honeycomb_or_later()
{
#ifdef MOZ_GETADDRINFO_LOG_VERBOSE
	__android_log_print(ANDROID_LOG_INFO, "getaddrinfo",
		"I am%s Honeycomb\n",
		(android_sdk_version >= 11) ? "" : " not");
#endif
	return android_sdk_version >= 11;
}

const char *
__wrap_gai_strerror(int ecode)
{
	if (honeycomb_or_later())
		return gai_strerror(ecode);
	if (ecode < 0 || ecode > EAI_MAX)
		ecode = EAI_MAX;
	return ai_errlist[ecode];
}

void
__wrap_freeaddrinfo(struct addrinfo *ai)
{
	struct addrinfo *next;

	if (honeycomb_or_later()) {
		freeaddrinfo(ai);
		return;
	}

	assert(ai != NULL);

	do {
		next = ai->ai_next;
		if (ai->ai_canonname)
			free(ai->ai_canonname);
		
		free(ai);
		ai = next;
	} while (ai);
}

static int
str2number(const char *p)
{
	char *ep;
	unsigned long v;

	assert(p != NULL);

	if (*p == '\0')
		return -1;
	ep = NULL;
	errno = 0;
	v = strtoul(p, &ep, 10);
	if (errno == 0 && ep && *ep == '\0' && v <= UINT_MAX)
		return v;
	else
		return -1;
}






static int
_test_connect(int pf, struct sockaddr *addr, size_t addrlen) {
	int s = socket(pf, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0)
		return 0;
	int ret;
	do {
		ret = connect(s, addr, addrlen);
	} while (ret < 0 && errno == EINTR);
	int success = (ret == 0);
	do {
		ret = close(s);
	} while (ret < 0 && errno == EINTR);
	return success;
}










static int
_have_ipv6() {
	static const struct sockaddr_in6 sin6_test = {
		.sin6_family = AF_INET6,
		.sin6_addr.s6_addr = {  
			0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
		};
        sockaddr_union addr = { .in6 = sin6_test };
	return _test_connect(PF_INET6, &addr.generic, sizeof(addr.in6));
}

static int
_have_ipv4() {
	static const struct sockaddr_in sin_test = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = __constant_htonl(0x08080808L)  
	};
        sockaddr_union addr = { .in = sin_test };
        return _test_connect(PF_INET, &addr.generic, sizeof(addr.in));
}

int
__wrap_getaddrinfo(const char *hostname, const char *servname,
    const struct addrinfo *hints, struct addrinfo **res)
{
	struct addrinfo sentinel;
	struct addrinfo *cur;
	int error = 0;
	struct addrinfo ai;
	struct addrinfo ai0;
	struct addrinfo *pai;
	const struct explore *ex;

	if (honeycomb_or_later())
		return getaddrinfo(hostname, servname, hints, res);

	
	
	
	assert(res != NULL);

	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;
	pai = &ai;
	pai->ai_flags = 0;
	pai->ai_family = PF_UNSPEC;
	pai->ai_socktype = ANY;
	pai->ai_protocol = ANY;
	pai->ai_addrlen = 0;
	pai->ai_canonname = NULL;
	pai->ai_addr = NULL;
	pai->ai_next = NULL;

	if (hostname == NULL && servname == NULL)
		return EAI_NONAME;
	if (hints) {
		
		if (hints->ai_addrlen || hints->ai_canonname ||
		    hints->ai_addr || hints->ai_next)
			ERR(EAI_BADHINTS); 
		if (hints->ai_flags & ~AI_MASK)
			ERR(EAI_BADFLAGS);
		switch (hints->ai_family) {
		case PF_UNSPEC:
		case PF_INET:
#ifdef INET6
		case PF_INET6:
#endif
			break;
		default:
			ERR(EAI_FAMILY);
		}
		memcpy(pai, hints, sizeof(*pai));

		



		if (pai->ai_socktype != ANY && pai->ai_protocol != ANY) {
			for (ex = explore; ex->e_af >= 0; ex++) {
				if (pai->ai_family != ex->e_af)
					continue;
				if (ex->e_socktype == ANY)
					continue;
				if (ex->e_protocol == ANY)
					continue;
				if (pai->ai_socktype == ex->e_socktype
				 && pai->ai_protocol != ex->e_protocol) {
					ERR(EAI_BADHINTS);
				}
			}
		}
	}

	




	if (MATCH_FAMILY(pai->ai_family, PF_INET, 1)
#ifdef PF_INET6
	 || MATCH_FAMILY(pai->ai_family, PF_INET6, 1)
#endif
	    ) {
		ai0 = *pai;	

		if (pai->ai_family == PF_UNSPEC) {
#ifdef PF_INET6
			pai->ai_family = PF_INET6;
#else
			pai->ai_family = PF_INET;
#endif
		}
		error = get_portmatch(pai, servname);
		if (error)
			ERR(error);

		*pai = ai0;
	}

	ai0 = *pai;

	
	for (ex = explore; ex->e_af >= 0; ex++) {
		*pai = ai0;

		
		if (ex->e_af == PF_UNSPEC)
			continue;

		if (!MATCH_FAMILY(pai->ai_family, ex->e_af, WILD_AF(ex)))
			continue;
		if (!MATCH(pai->ai_socktype, ex->e_socktype, WILD_SOCKTYPE(ex)))
			continue;
		if (!MATCH(pai->ai_protocol, ex->e_protocol, WILD_PROTOCOL(ex)))
			continue;

		if (pai->ai_family == PF_UNSPEC)
			pai->ai_family = ex->e_af;
		if (pai->ai_socktype == ANY && ex->e_socktype != ANY)
			pai->ai_socktype = ex->e_socktype;
		if (pai->ai_protocol == ANY && ex->e_protocol != ANY)
			pai->ai_protocol = ex->e_protocol;

		if (hostname == NULL)
			error = explore_null(pai, servname, &cur->ai_next);
		else
			error = explore_numeric_scope(pai, hostname, servname,
			    &cur->ai_next);

		if (error)
			goto free;

		while (cur->ai_next)
			cur = cur->ai_next;
	}

	




	if (sentinel.ai_next)
		goto good;

	if (hostname == NULL)
		ERR(EAI_NODATA);
	if (pai->ai_flags & AI_NUMERICHOST)
		ERR(EAI_NONAME);

	




	for (ex = explore; ex->e_af >= 0; ex++) {
		*pai = ai0;

		
		if (pai->ai_family != ex->e_af)
			continue;

		if (!MATCH(pai->ai_socktype, ex->e_socktype,
				WILD_SOCKTYPE(ex))) {
			continue;
		}
		if (!MATCH(pai->ai_protocol, ex->e_protocol,
				WILD_PROTOCOL(ex))) {
			continue;
		}

		if (pai->ai_socktype == ANY && ex->e_socktype != ANY)
			pai->ai_socktype = ex->e_socktype;
		if (pai->ai_protocol == ANY && ex->e_protocol != ANY)
			pai->ai_protocol = ex->e_protocol;

		error = explore_fqdn(pai, hostname, servname,
			&cur->ai_next);

		while (cur && cur->ai_next)
			cur = cur->ai_next;
	}

	
	if (sentinel.ai_next)
		error = 0;

	if (error)
		goto free;
	if (error == 0) {
		if (sentinel.ai_next) {
 good:
			*res = sentinel.ai_next;
			return SUCCESS;
		} else
			error = EAI_FAIL;
	}
 free:
 bad:
	if (sentinel.ai_next)
		__wrap_freeaddrinfo(sentinel.ai_next);
	*res = NULL;
	return error;
}




static int
explore_fqdn(const struct addrinfo *pai, const char *hostname,
    const char *servname, struct addrinfo **res)
{
	struct addrinfo *result;
	struct addrinfo *cur;
	int error = 0;
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_getaddrinfo, NULL)
		{ NSSRC_DNS, _dns_getaddrinfo, NULL },	
		NS_NIS_CB(_yp_getaddrinfo, NULL)
		{ 0, 0, 0 }
	};

	assert(pai != NULL);
	
	
	assert(res != NULL);

	result = NULL;

	


	if (get_portmatch(pai, servname) != 0)
		return 0;

	switch (nsdispatch(&result, dtab, NSDB_HOSTS, "getaddrinfo",
			default_dns_files, hostname, pai)) {
	case NS_TRYAGAIN:
		error = EAI_AGAIN;
		goto free;
	case NS_UNAVAIL:
		error = EAI_FAIL;
		goto free;
	case NS_NOTFOUND:
		error = EAI_NODATA;
		goto free;
	case NS_SUCCESS:
		error = 0;
		for (cur = result; cur; cur = cur->ai_next) {
			GET_PORT(cur, servname);
			
		}
		break;
	}

	*res = result;

	return 0;

free:
	if (result)
		__wrap_freeaddrinfo(result);
	return error;
}






static int
explore_null(const struct addrinfo *pai, const char *servname,
    struct addrinfo **res)
{
	int s;
	const struct afd *afd;
	struct addrinfo *cur;
	struct addrinfo sentinel;
	int error;

	assert(pai != NULL);
	
	assert(res != NULL);

	*res = NULL;
	sentinel.ai_next = NULL;
	cur = &sentinel;

	



	s = socket(pai->ai_family, SOCK_DGRAM, 0);
	if (s < 0) {
		if (errno != EMFILE)
			return 0;
	} else
		close(s);

	


	if (get_portmatch(pai, servname) != 0)
		return 0;

	afd = find_afd(pai->ai_family);
	if (afd == NULL)
		return 0;

	if (pai->ai_flags & AI_PASSIVE) {
		GET_AI(cur->ai_next, afd, afd->a_addrany);
		


		GET_PORT(cur->ai_next, servname);
	} else {
		GET_AI(cur->ai_next, afd, afd->a_loopback);
		


		GET_PORT(cur->ai_next, servname);
	}
	cur = cur->ai_next;

	*res = sentinel.ai_next;
	return 0;

free:
	if (sentinel.ai_next)
		__wrap_freeaddrinfo(sentinel.ai_next);
	return error;
}




static int
explore_numeric(const struct addrinfo *pai, const char *hostname,
    const char *servname, struct addrinfo **res, const char *canonname)
{
	const struct afd *afd;
	struct addrinfo *cur;
	struct addrinfo sentinel;
	int error;
	char pton[PTON_MAX];

	assert(pai != NULL);
	
	
	assert(res != NULL);

	*res = NULL;
	sentinel.ai_next = NULL;
	cur = &sentinel;

	


	if (get_portmatch(pai, servname) != 0)
		return 0;

	afd = find_afd(pai->ai_family);
	if (afd == NULL)
		return 0;

	switch (afd->a_af) {
#if 0 
	case AF_INET:
		if (inet_aton(hostname, (struct in_addr *)pton) == 1) {
			if (pai->ai_family == afd->a_af ||
			    pai->ai_family == PF_UNSPEC ) {
				GET_AI(cur->ai_next, afd, pton);
				GET_PORT(cur->ai_next, servname);
				if ((pai->ai_flags & AI_CANONNAME)) {
					




					GET_CANONNAME(cur->ai_next, canonname);
				}
				while (cur && cur->ai_next)
					cur = cur->ai_next;
			} else
				ERR(EAI_FAMILY);	
		}
		break;
#endif
	default:
		if (inet_pton(afd->a_af, hostname, pton) == 1) {
			if (pai->ai_family == afd->a_af ||
			    pai->ai_family == PF_UNSPEC ) {
				GET_AI(cur->ai_next, afd, pton);
				GET_PORT(cur->ai_next, servname);
				if ((pai->ai_flags & AI_CANONNAME)) {
					




					GET_CANONNAME(cur->ai_next, canonname);
				}
				while (cur->ai_next)
					cur = cur->ai_next;
			} else
				ERR(EAI_FAMILY);	
		}
		break;
	}

	*res = sentinel.ai_next;
	return 0;

free:
bad:
	if (sentinel.ai_next)
		__wrap_freeaddrinfo(sentinel.ai_next);
	return error;
}




static int
explore_numeric_scope(const struct addrinfo *pai, const char *hostname,
    const char *servname, struct addrinfo **res)
{
#if !defined(SCOPE_DELIMITER) || !defined(INET6)
	return explore_numeric(pai, hostname, servname, res, hostname);
#else
	const struct afd *afd;
	struct addrinfo *cur;
	int error;
	char *cp, *hostname2 = NULL, *scope, *addr;
	struct sockaddr_in6 *sin6;

	assert(pai != NULL);
	
	
	assert(res != NULL);

	


	if (get_portmatch(pai, servname) != 0)
		return 0;

	afd = find_afd(pai->ai_family);
	if (afd == NULL)
		return 0;

	if (!afd->a_scoped)
		return explore_numeric(pai, hostname, servname, res, hostname);

	cp = strchr(hostname, SCOPE_DELIMITER);
	if (cp == NULL)
		return explore_numeric(pai, hostname, servname, res, hostname);

	


	hostname2 = strdup(hostname);
	if (hostname2 == NULL)
		return EAI_MEMORY;
	
	hostname2[cp - hostname] = '\0';
	addr = hostname2;
	scope = cp + 1;

	error = explore_numeric(pai, addr, servname, res, hostname);
	if (error == 0) {
		u_int32_t scopeid;

		for (cur = *res; cur; cur = cur->ai_next) {
			if (cur->ai_family != AF_INET6)
				continue;
			sin6 = (struct sockaddr_in6 *)(void *)cur->ai_addr;
			if (ip6_str2scopeid(scope, sin6, &scopeid) == -1) {
				free(hostname2);
				return(EAI_NODATA); 
			}
			sin6->sin6_scope_id = scopeid;
		}
	}

	free(hostname2);

	return error;
#endif
}

static int
get_canonname(const struct addrinfo *pai, struct addrinfo *ai, const char *str)
{

	assert(pai != NULL);
	assert(ai != NULL);
	assert(str != NULL);

	if ((pai->ai_flags & AI_CANONNAME) != 0) {
		ai->ai_canonname = strdup(str);
		if (ai->ai_canonname == NULL)
			return EAI_MEMORY;
	}
	return 0;
}

static struct addrinfo *
get_ai(const struct addrinfo *pai, const struct afd *afd, const char *addr)
{
	char *p;
	struct addrinfo *ai;

	assert(pai != NULL);
	assert(afd != NULL);
	assert(addr != NULL);

	ai = (struct addrinfo *)malloc(sizeof(struct addrinfo)
		+ (afd->a_socklen));
	if (ai == NULL)
		return NULL;

	memcpy(ai, pai, sizeof(struct addrinfo));
	ai->ai_addr = (struct sockaddr *)(void *)(ai + 1);
	memset(ai->ai_addr, 0, (size_t)afd->a_socklen);

#ifdef HAVE_SA_LEN
	ai->ai_addr->sa_len = afd->a_socklen;
#endif

	ai->ai_addrlen = afd->a_socklen;
#if defined (__alpha__) || (defined(__i386__) && defined(_LP64)) || defined(__sparc64__)
	ai->__ai_pad0 = 0;
#endif
	ai->ai_addr->sa_family = ai->ai_family = afd->a_af;
	p = (char *)(void *)(ai->ai_addr);
	memcpy(p + afd->a_off, addr, (size_t)afd->a_addrlen);
	return ai;
}

static int
get_portmatch(const struct addrinfo *ai, const char *servname)
{

	assert(ai != NULL);
	

	return get_port(ai, servname, 1);
}

static int
get_port(const struct addrinfo *ai, const char *servname, int matchonly)
{
	const char *proto;
	struct servent *sp;
	int port;
	int allownumeric;

	assert(ai != NULL);
	

	if (servname == NULL)
		return 0;
	switch (ai->ai_family) {
	case AF_INET:
#ifdef AF_INET6
	case AF_INET6:
#endif
		break;
	default:
		return 0;
	}

	switch (ai->ai_socktype) {
	case SOCK_RAW:
		return EAI_SERVICE;
	case SOCK_DGRAM:
	case SOCK_STREAM:
		allownumeric = 1;
		break;
	case ANY:
#if 1  
		allownumeric = 1;
#else
		allownumeric = 0;
#endif
		break;
	default:
		return EAI_SOCKTYPE;
	}

	port = str2number(servname);
	if (port >= 0) {
		if (!allownumeric)
			return EAI_SERVICE;
		if (port < 0 || port > 65535)
			return EAI_SERVICE;
		port = htons(port);
	} else {
		if (ai->ai_flags & AI_NUMERICSERV)
			return EAI_NONAME;

		switch (ai->ai_socktype) {
		case SOCK_DGRAM:
			proto = "udp";
			break;
		case SOCK_STREAM:
			proto = "tcp";
			break;
		default:
			proto = NULL;
			break;
		}

		if ((sp = getservbyname(servname, proto)) == NULL)
			return EAI_SERVICE;
		port = sp->s_port;
	}

	if (!matchonly) {
		switch (ai->ai_family) {
		case AF_INET:
			((struct sockaddr_in *)(void *)
			    ai->ai_addr)->sin_port = port;
			break;
#ifdef INET6
		case AF_INET6:
			((struct sockaddr_in6 *)(void *)
			    ai->ai_addr)->sin6_port = port;
			break;
#endif
		}
	}

	return 0;
}

static const struct afd *
find_afd(int af)
{
	const struct afd *afd;

	if (af == PF_UNSPEC)
		return NULL;
	for (afd = afdl; afd->a_af; afd++) {
		if (afd->a_af == af)
			return afd;
	}
	return NULL;
}

#ifdef INET6

static int
ip6_str2scopeid(char *scope, struct sockaddr_in6 *sin6, u_int32_t *scopeid)
{
	u_long lscopeid;
	struct in6_addr *a6;
	char *ep;

	assert(scope != NULL);
	assert(sin6 != NULL);
	assert(scopeid != NULL);

	a6 = &sin6->sin6_addr;

	
	if (*scope == '\0')
		return -1;

	if (IN6_IS_ADDR_LINKLOCAL(a6) || IN6_IS_ADDR_MC_LINKLOCAL(a6)) {
		




		*scopeid = if_nametoindex(scope);
		if (*scopeid == 0)
			goto trynumeric;
		return 0;
	}

	
	if (IN6_IS_ADDR_SITELOCAL(a6) || IN6_IS_ADDR_MC_SITELOCAL(a6))
		goto trynumeric;
	if (IN6_IS_ADDR_MC_ORGLOCAL(a6))
		goto trynumeric;
	else
		goto trynumeric;	

	
  trynumeric:
	errno = 0;
	lscopeid = strtoul(scope, &ep, 10);
	*scopeid = (u_int32_t)(lscopeid & 0xffffffffUL);
	if (errno == 0 && ep && *ep == '\0' && *scopeid == lscopeid)
		return 0;
	else
		return -1;
}
#endif



static const char AskedForGot[] =
	"gethostby*.getanswer: asked for \"%s\", got \"%s\"";

static struct addrinfo *
getanswer(const querybuf *answer, int anslen, const char *qname, int qtype,
    const struct addrinfo *pai)
{
	struct addrinfo sentinel, *cur;
	struct addrinfo ai;
	const struct afd *afd;
	char *canonname;
	const HEADER *hp;
	const u_char *cp;
	int n;
	const u_char *eom;
	char *bp, *ep;
	int type, class, ancount, qdcount;
	int haveanswer, had_error;
	char tbuf[MAXDNAME];
	int (*name_ok) (const char *);
	char hostbuf[8*1024];

	assert(answer != NULL);
	assert(qname != NULL);
	assert(pai != NULL);

	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

	canonname = NULL;
	eom = answer->buf + anslen;
	switch (qtype) {
	case T_A:
	case T_AAAA:
	case T_ANY:	
		name_ok = res_hnok;
		break;
	default:
		return NULL;	
	}
	


	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = hostbuf;
	ep = hostbuf + sizeof hostbuf;
	cp = answer->buf + HFIXEDSZ;
	if (qdcount != 1) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	n = dn_expand(answer->buf, eom, cp, bp, ep - bp);
	if ((n < 0) || !(*name_ok)(bp)) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	cp += n + QFIXEDSZ;
	if (qtype == T_A || qtype == T_AAAA || qtype == T_ANY) {
		



		n = strlen(bp) + 1;		
		if (n >= MAXHOSTNAMELEN) {
			h_errno = NO_RECOVERY;
			return (NULL);
		}
		canonname = bp;
		bp += n;
		
		qname = canonname;
	}
	haveanswer = 0;
	had_error = 0;
	while (ancount-- > 0 && cp < eom && !had_error) {
		n = dn_expand(answer->buf, eom, cp, bp, ep - bp);
		if ((n < 0) || !(*name_ok)(bp)) {
			had_error++;
			continue;
		}
		cp += n;			
		type = _getshort(cp);
 		cp += INT16SZ;			
		class = _getshort(cp);
 		cp += INT16SZ + INT32SZ;	
		n = _getshort(cp);
		cp += INT16SZ;			
		if (class != C_IN) {
			
			cp += n;
			continue;		
		}
		if ((qtype == T_A || qtype == T_AAAA || qtype == T_ANY) &&
		    type == T_CNAME) {
			n = dn_expand(answer->buf, eom, cp, tbuf, sizeof tbuf);
			if ((n < 0) || !(*name_ok)(tbuf)) {
				had_error++;
				continue;
			}
			cp += n;
			
			n = strlen(tbuf) + 1;	
			if (n > ep - bp || n >= MAXHOSTNAMELEN) {
				had_error++;
				continue;
			}
			strlcpy(bp, tbuf, (size_t)(ep - bp));
			canonname = bp;
			bp += n;
			continue;
		}
		if (qtype == T_ANY) {
			if (!(type == T_A || type == T_AAAA)) {
				cp += n;
				continue;
			}
		} else if (type != qtype) {
			if (type != T_KEY && type != T_SIG)
				syslog(LOG_NOTICE|LOG_AUTH,
	       "gethostby*.getanswer: asked for \"%s %s %s\", got type \"%s\"",
				       qname, p_class(C_IN), p_type(qtype),
				       p_type(type));
			cp += n;
			continue;		
		}
		switch (type) {
		case T_A:
		case T_AAAA:
			if (strcasecmp(canonname, bp) != 0) {
				syslog(LOG_NOTICE|LOG_AUTH,
				       AskedForGot, canonname, bp);
				cp += n;
				continue;	
			}
			if (type == T_A && n != INADDRSZ) {
				cp += n;
				continue;
			}
			if (type == T_AAAA && n != IN6ADDRSZ) {
				cp += n;
				continue;
			}
			if (type == T_AAAA) {
				struct in6_addr in6;
				memcpy(&in6, cp, IN6ADDRSZ);
				if (IN6_IS_ADDR_V4MAPPED(&in6)) {
					cp += n;
					continue;
				}
			}
			if (!haveanswer) {
				int nn;

				canonname = bp;
				nn = strlen(bp) + 1;	
				bp += nn;
			}

			
			ai = *pai;
			ai.ai_family = (type == T_A) ? AF_INET : AF_INET6;
			afd = find_afd(ai.ai_family);
			if (afd == NULL) {
				cp += n;
				continue;
			}
			cur->ai_next = get_ai(&ai, afd, (const char *)cp);
			if (cur->ai_next == NULL)
				had_error++;
			while (cur && cur->ai_next)
				cur = cur->ai_next;
			cp += n;
			break;
		default:
			abort();
		}
		if (!had_error)
			haveanswer++;
	}
	if (haveanswer) {
		if (!canonname)
			(void)get_canonname(pai, sentinel.ai_next, qname);
		else
			(void)get_canonname(pai, sentinel.ai_next, canonname);
		h_errno = NETDB_SUCCESS;
		return sentinel.ai_next;
	}

	h_errno = NO_RECOVERY;
	return NULL;
}

struct addrinfo_sort_elem {
	struct addrinfo *ai;
	int has_src_addr;
	sockaddr_union src_addr;
	int original_order;
};


static int
_get_scope(const struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET6) {
		const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *)addr;
		if (IN6_IS_ADDR_MULTICAST(&addr6->sin6_addr)) {
			return IPV6_ADDR_MC_SCOPE(&addr6->sin6_addr);
		} else if (IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr) ||
			   IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr)) {
			



			return IPV6_ADDR_SCOPE_LINKLOCAL;
		} else if (IN6_IS_ADDR_SITELOCAL(&addr6->sin6_addr)) {
			return IPV6_ADDR_SCOPE_SITELOCAL;
		} else {
			return IPV6_ADDR_SCOPE_GLOBAL;
		}
	} else if (addr->sa_family == AF_INET) {
		const struct sockaddr_in *addr4 = (const struct sockaddr_in *)addr;
		unsigned long int na = ntohl(addr4->sin_addr.s_addr);

		if (IN_LOOPBACK(na) ||                          
		    (na & 0xffff0000) == 0xa9fe0000) {          
			return IPV6_ADDR_SCOPE_LINKLOCAL;
		} else {
			





			return IPV6_ADDR_SCOPE_GLOBAL;
		}
	} else {
		



		return IPV6_ADDR_SCOPE_NODELOCAL;
	}
}




#define IN6_IS_ADDR_TEREDO(a)	 \
	((*(const uint32_t *)(const void *)(&(a)->s6_addr[0]) == ntohl(0x20010000)))


#define IN6_IS_ADDR_6TO4(a)	 \
	(((a)->s6_addr[0] == 0x20) && ((a)->s6_addr[1] == 0x02))


#define IN6_IS_ADDR_6BONE(a)      \
	(((a)->s6_addr[0] == 0x3f) && ((a)->s6_addr[1] == 0xfe))







static int
_get_label(const struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		return 3;
	} else if (addr->sa_family == AF_INET6) {
		const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *)addr;
		if (IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr)) {
			return 0;
		} else if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
			return 3;
		} else if (IN6_IS_ADDR_6TO4(&addr6->sin6_addr)) {
			return 4;
		} else if (IN6_IS_ADDR_TEREDO(&addr6->sin6_addr)) {
			return 5;
		} else if (IN6_IS_ADDR_V4COMPAT(&addr6->sin6_addr)) {
			return 10;
		} else if (IN6_IS_ADDR_SITELOCAL(&addr6->sin6_addr)) {
			return 11;
		} else if (IN6_IS_ADDR_6BONE(&addr6->sin6_addr)) {
			return 12;
		} else {
			return 2;
		}
	} else {
		



		return 1;
	}
}







static int
_get_precedence(const struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		return 30;
	} else if (addr->sa_family == AF_INET6) {
		const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *)addr;
		if (IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr)) {
			return 60;
		} else if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
			return 30;
		} else if (IN6_IS_ADDR_6TO4(&addr6->sin6_addr)) {
			return 20;
		} else if (IN6_IS_ADDR_TEREDO(&addr6->sin6_addr)) {
			return 10;
		} else if (IN6_IS_ADDR_V4COMPAT(&addr6->sin6_addr) ||
		           IN6_IS_ADDR_SITELOCAL(&addr6->sin6_addr) ||
		           IN6_IS_ADDR_6BONE(&addr6->sin6_addr)) {
			return 1;
		} else {
			return 40;
		}
	} else {
		return 1;
	}
}






static int
_common_prefix_len(const struct in6_addr *a1, const struct in6_addr *a2)
{
	const char *p1 = (const char *)a1;
	const char *p2 = (const char *)a2;
	unsigned i;

	for (i = 0; i < sizeof(*a1); ++i) {
		int x, j;

		if (p1[i] == p2[i]) {
			continue;
		}
		x = p1[i] ^ p2[i];
		for (j = 0; j < CHAR_BIT; ++j) {
			if (x & (1 << (CHAR_BIT - 1))) {
				return i * CHAR_BIT + j;
			}
			x <<= 1;
		}
	}
	return sizeof(*a1) * CHAR_BIT;
}







static int
_rfc3484_compare(const void *ptr1, const void* ptr2)
{
	const struct addrinfo_sort_elem *a1 = (const struct addrinfo_sort_elem *)ptr1;
	const struct addrinfo_sort_elem *a2 = (const struct addrinfo_sort_elem *)ptr2;
	int scope_src1, scope_dst1, scope_match1;
	int scope_src2, scope_dst2, scope_match2;
	int label_src1, label_dst1, label_match1;
	int label_src2, label_dst2, label_match2;
	int precedence1, precedence2;
	int prefixlen1, prefixlen2;

	
	if (a1->has_src_addr != a2->has_src_addr) {
		return a2->has_src_addr - a1->has_src_addr;
	}

	
	scope_src1 = _get_scope(&a1->src_addr.generic);
	scope_dst1 = _get_scope(a1->ai->ai_addr);
	scope_match1 = (scope_src1 == scope_dst1);

	scope_src2 = _get_scope(&a2->src_addr.generic);
	scope_dst2 = _get_scope(a2->ai->ai_addr);
	scope_match2 = (scope_src2 == scope_dst2);

	if (scope_match1 != scope_match2) {
		return scope_match2 - scope_match1;
	}

	




	




	
	label_src1 = _get_label(&a1->src_addr.generic);
	label_dst1 = _get_label(a1->ai->ai_addr);
	label_match1 = (label_src1 == label_dst1);

	label_src2 = _get_label(&a2->src_addr.generic);
	label_dst2 = _get_label(a2->ai->ai_addr);
	label_match2 = (label_src2 == label_dst2);

	if (label_match1 != label_match2) {
		return label_match2 - label_match1;
	}

	
	precedence1 = _get_precedence(a1->ai->ai_addr);
	precedence2 = _get_precedence(a2->ai->ai_addr);
	if (precedence1 != precedence2) {
		return precedence2 - precedence1;
	}

	




	
	if (scope_dst1 != scope_dst2) {
		return scope_dst1 - scope_dst2;
	}

	





	if (a1->has_src_addr && a1->ai->ai_addr->sa_family == AF_INET6 &&
	    a2->has_src_addr && a2->ai->ai_addr->sa_family == AF_INET6) {
		const struct sockaddr_in6 *a1_src = &a1->src_addr.in6;
		const struct sockaddr_in6 *a1_dst = (const struct sockaddr_in6 *)a1->ai->ai_addr;
		const struct sockaddr_in6 *a2_src = &a2->src_addr.in6;
		const struct sockaddr_in6 *a2_dst = (const struct sockaddr_in6 *)a2->ai->ai_addr;
		prefixlen1 = _common_prefix_len(&a1_src->sin6_addr, &a1_dst->sin6_addr);
		prefixlen2 = _common_prefix_len(&a2_src->sin6_addr, &a2_dst->sin6_addr);
		if (prefixlen1 != prefixlen2) {
			return prefixlen2 - prefixlen1;
		}
	}

	



	return a1->original_order - a2->original_order;
}











static int
_find_src_addr(const struct sockaddr *addr, struct sockaddr *src_addr)
{
	int sock;
	int ret;
	socklen_t len;

	switch (addr->sa_family) {
	case AF_INET:
		len = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		len = sizeof(struct sockaddr_in6);
		break;
	default:
		
		return 0;
	}

	sock = socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		if (errno == EAFNOSUPPORT) {
			return 0;
		} else {
			return -1;
		}
	}

	do {
		ret = connect(sock, addr, len);
	} while (ret == -1 && errno == EINTR);

	if (ret == -1) {
		close(sock);
		return 0;
	}

	if (getsockname(sock, src_addr, &len) == -1) {
		close(sock);
		return -1;
	}
	close(sock);
	return 1;
}







static void
_rfc3484_sort(struct addrinfo *list_sentinel)
{
	struct addrinfo *cur;
	int nelem = 0, i;
	struct addrinfo_sort_elem *elems;

	cur = list_sentinel->ai_next;
	while (cur) {
		++nelem;
		cur = cur->ai_next;
	}

	elems = (struct addrinfo_sort_elem *)malloc(nelem * sizeof(struct addrinfo_sort_elem));
	if (elems == NULL) {
		goto error;
	}

	



	for (i = 0, cur = list_sentinel->ai_next; i < nelem; ++i, cur = cur->ai_next) {
		int has_src_addr;
		assert(cur != NULL);
		elems[i].ai = cur;
		elems[i].original_order = i;

		has_src_addr = _find_src_addr(cur->ai_addr, &elems[i].src_addr.generic);
		if (has_src_addr == -1) {
			goto error;
		}
		elems[i].has_src_addr = has_src_addr;
	}

	
	qsort((void *)elems, nelem, sizeof(struct addrinfo_sort_elem), _rfc3484_compare);

	list_sentinel->ai_next = elems[0].ai;
	for (i = 0; i < nelem - 1; ++i) {
		elems[i].ai->ai_next = elems[i + 1].ai;
	}
	elems[nelem - 1].ai->ai_next = NULL;

error:
	free(elems);
}


static int
_dns_getaddrinfo(void *rv, void	*cb_data, va_list ap)
{
	struct addrinfo *ai;
	querybuf *buf, *buf2;
	const char *name;
	const struct addrinfo *pai;
	struct addrinfo sentinel, *cur;
	struct res_target q, q2;
	res_state res;

	name = va_arg(ap, char *);
	pai = va_arg(ap, const struct addrinfo *);
	

	memset(&q, 0, sizeof(q));
	memset(&q2, 0, sizeof(q2));
	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

	buf = malloc(sizeof(*buf));
	if (buf == NULL) {
		h_errno = NETDB_INTERNAL;
		return NS_NOTFOUND;
	}
	buf2 = malloc(sizeof(*buf2));
	if (buf2 == NULL) {
		free(buf);
		h_errno = NETDB_INTERNAL;
		return NS_NOTFOUND;
	}

	switch (pai->ai_family) {
	case AF_UNSPEC:
		
		q.name = name;
		q.qclass = C_IN;
		q.answer = buf->buf;
		q.anslen = sizeof(buf->buf);
		int query_ipv6 = 1, query_ipv4 = 1;
		if (pai->ai_flags & AI_ADDRCONFIG) {
			query_ipv6 = _have_ipv6();
			query_ipv4 = _have_ipv4();
		}
		if (query_ipv6) {
			q.qtype = T_AAAA;
			if (query_ipv4) {
				q.next = &q2;
				q2.name = name;
				q2.qclass = C_IN;
				q2.qtype = T_A;
				q2.answer = buf2->buf;
				q2.anslen = sizeof(buf2->buf);
			}
		} else if (query_ipv4) {
			q.qtype = T_A;
		} else {
			free(buf);
			free(buf2);
			return NS_NOTFOUND;
		}
		break;
	case AF_INET:
		q.name = name;
		q.qclass = C_IN;
		q.qtype = T_A;
		q.answer = buf->buf;
		q.anslen = sizeof(buf->buf);
		break;
	case AF_INET6:
		q.name = name;
		q.qclass = C_IN;
		q.qtype = T_AAAA;
		q.answer = buf->buf;
		q.anslen = sizeof(buf->buf);
		break;
	default:
		free(buf);
		free(buf2);
		return NS_UNAVAIL;
	}

	res = __res_get_state();
	if (res == NULL) {
		free(buf);
		free(buf2);
		return NS_NOTFOUND;
	}

	if (res_searchN(name, &q, res) < 0) {
		__res_put_state(res);
		free(buf);
		free(buf2);
		return NS_NOTFOUND;
	}
	ai = getanswer(buf, q.n, q.name, q.qtype, pai);
	if (ai) {
		cur->ai_next = ai;
		while (cur && cur->ai_next)
			cur = cur->ai_next;
	}
	if (q.next) {
		ai = getanswer(buf2, q2.n, q2.name, q2.qtype, pai);
		if (ai)
			cur->ai_next = ai;
	}
	free(buf);
	free(buf2);
	if (sentinel.ai_next == NULL) {
		__res_put_state(res);
		switch (h_errno) {
		case HOST_NOT_FOUND:
			return NS_NOTFOUND;
		case TRY_AGAIN:
			return NS_TRYAGAIN;
		default:
			return NS_UNAVAIL;
		}
	}

	_rfc3484_sort(&sentinel);

	__res_put_state(res);

	*((struct addrinfo **)rv) = sentinel.ai_next;
	return NS_SUCCESS;
}

static void
_sethtent(_pseudo_FILE * __restrict__ hostf)
{
	assert(hostf);
	if (hostf->mapping == MAP_FAILED)
		(void) _pseudo_fopen_r(hostf, _PATH_HOSTS);
	else
		_pseudo_rewind(hostf);
}

static void
_endhtent(_pseudo_FILE * __restrict__ hostf)
{
	assert(hostf);
	(void) _pseudo_fclose(hostf);
}

static struct addrinfo *
_gethtent(_pseudo_FILE * __restrict__ hostf, const char *name, const struct addrinfo *pai)
{
	char *p;
	char *cp, *tname, *cname;
	struct addrinfo hints, *res0, *res;
	int error;
	const char *addr;
	char hostbuf[8*1024];

	assert(hostf);

	assert(name != NULL);
	assert(pai != NULL);

	if (hostf->mapping == MAP_FAILED)
		(void) _pseudo_fopen_r(hostf, _PATH_HOSTS);
	if (hostf->mapping == MAP_FAILED)
		return (NULL);
 again:
	if (!(p = _pseudo_fgets(hostbuf, sizeof hostbuf, hostf)))
		return (NULL);
	if (*p == '#')
		goto again;
	if (!(cp = strpbrk(p, "#\n")))
		goto again;
	*cp = '\0';
	if (!(cp = strpbrk(p, " \t")))
		goto again;
	*cp++ = '\0';
	addr = p;
	
	cname = NULL;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (!cname)
			cname = cp;
		tname = cp;
		if ((cp = strpbrk(cp, " \t")) != NULL)
			*cp++ = '\0';

		if (strcasecmp(name, tname) == 0)
			goto found;
	}
	goto again;

found:
	hints = *pai;
	hints.ai_flags = AI_NUMERICHOST;
	error = __wrap_getaddrinfo(addr, NULL, &hints, &res0);
	if (error)
		goto again;
	for (res = res0; res; res = res->ai_next) {
		
		res->ai_flags = pai->ai_flags;

		if (pai->ai_flags & AI_CANONNAME) {
			if (get_canonname(pai, res, cname) != 0) {
				__wrap_freeaddrinfo(res0);
				goto again;
			}
		}
	}
	return res0;
}


static int
_files_getaddrinfo(void *rv, void *cb_data, va_list ap)
{
	const char *name;
	const struct addrinfo *pai;
	struct addrinfo sentinel, *cur;
	struct addrinfo *p;
	_pseudo_FILE hostf = _PSEUDO_FILE_INITIALIZER;

	name = va_arg(ap, char *);
	pai = va_arg(ap, struct addrinfo *);


	memset(&sentinel, 0, sizeof(sentinel));
	cur = &sentinel;

	_sethtent(&hostf);
	while ((p = _gethtent(&hostf, name, pai)) != NULL) {
		cur->ai_next = p;
		while (cur && cur->ai_next)
			cur = cur->ai_next;
	}
	_endhtent(&hostf);

	*((struct addrinfo **)rv) = sentinel.ai_next;
	if (sentinel.ai_next == NULL)
		return NS_NOTFOUND;
	return NS_SUCCESS;
}













static int
res_queryN(const char *name,  struct res_target *target,
    res_state res)
{
	u_char buf[MAXPACKET];
	HEADER *hp;
	int n;
	struct res_target *t;
	int rcode;
	int ancount;

	assert(name != NULL);
	

	rcode = NOERROR;
	ancount = 0;

	for (t = target; t; t = t->next) {
		int class, type;
		u_char *answer;
		int anslen;

		hp = (HEADER *)(void *)t->answer;
		hp->rcode = NOERROR;	

		
		class = t->qclass;
		type = t->qtype;
		answer = t->answer;
		anslen = t->anslen;
#ifdef DEBUG
		if (res->options & RES_DEBUG)
			printf(";; res_nquery(%s, %d, %d)\n", name, class, type);
#endif

		n = res_nmkquery(res, QUERY, name, class, type, NULL, 0, NULL,
		    buf, sizeof(buf));
#ifdef RES_USE_EDNS0
		if (n > 0 && (res->options & RES_USE_EDNS0) != 0)
			n = res_nopt(res, n, buf, sizeof(buf), anslen);
#endif
		if (n <= 0) {
#ifdef DEBUG
			if (res->options & RES_DEBUG)
				printf(";; res_nquery: mkquery failed\n");
#endif
			h_errno = NO_RECOVERY;
			return n;
		}
		n = res_nsend(res, buf, n, answer, anslen);
#if 0
		if (n < 0) {
#ifdef DEBUG
			if (res->options & RES_DEBUG)
				printf(";; res_query: send error\n");
#endif
			h_errno = TRY_AGAIN;
			return n;
		}
#endif

		if (n < 0 || hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
			rcode = hp->rcode;	
#ifdef DEBUG
			if (res->options & RES_DEBUG)
				printf(";; rcode = %u, ancount=%u\n", hp->rcode,
				    ntohs(hp->ancount));
#endif
			continue;
		}

		ancount += ntohs(hp->ancount);

		t->n = n;
	}

	if (ancount == 0) {
		switch (rcode) {
		case NXDOMAIN:
			h_errno = HOST_NOT_FOUND;
			break;
		case SERVFAIL:
			h_errno = TRY_AGAIN;
			break;
		case NOERROR:
			h_errno = NO_DATA;
			break;
		case FORMERR:
		case NOTIMP:
		case REFUSED:
		default:
			h_errno = NO_RECOVERY;
			break;
		}
		return -1;
	}
	return ancount;
}







static int
res_searchN(const char *name, struct res_target *target, res_state res)
{
	const char *cp, * const *domain;
	HEADER *hp;
	u_int dots;
	int trailing_dot, ret, saved_herrno;
	int got_nodata = 0, got_servfail = 0, tried_as_is = 0;

	assert(name != NULL);
	assert(target != NULL);

	hp = (HEADER *)(void *)target->answer;	

	errno = 0;
	h_errno = HOST_NOT_FOUND;	
	dots = 0;
	for (cp = name; *cp; cp++)
		dots += (*cp == '.');
	trailing_dot = 0;
	if (cp > name && *--cp == '.')
		trailing_dot++;


        

	


	if (!dots && (cp = __hostalias(name)) != NULL) {
		ret = res_queryN(cp, target, res);
		return ret;
	}

	



	saved_herrno = -1;
	if (dots >= res->ndots) {
		ret = res_querydomainN(name, NULL, target, res);
		if (ret > 0)
			return (ret);
		saved_herrno = h_errno;
		tried_as_is++;
	}

	





	if ((!dots && (res->options & RES_DEFNAMES)) ||
	    (dots && !trailing_dot && (res->options & RES_DNSRCH))) {
		int done = 0;

		for (domain = (const char * const *)res->dnsrch;
		   *domain && !done;
		   domain++) {

			ret = res_querydomainN(name, *domain, target, res);
			if (ret > 0)
				return ret;

			












			if (errno == ECONNREFUSED) {
				h_errno = TRY_AGAIN;
				return -1;
			}

			switch (h_errno) {
			case NO_DATA:
				got_nodata++;
				
			case HOST_NOT_FOUND:
				
				break;
			case TRY_AGAIN:
				if (hp->rcode == SERVFAIL) {
					
					got_servfail++;
					break;
				}
				
			default:
				
				done++;
			}
			



			if (!(res->options & RES_DNSRCH))
			        done++;
		}
	}

	




	if (!tried_as_is) {
		ret = res_querydomainN(name, NULL, target, res);
		if (ret > 0)
			return ret;
	}

	







	if (saved_herrno != -1)
		h_errno = saved_herrno;
	else if (got_nodata)
		h_errno = NO_DATA;
	else if (got_servfail)
		h_errno = TRY_AGAIN;
	return -1;
}





static int
res_querydomainN(const char *name, const char *domain,
    struct res_target *target, res_state res)
{
	char nbuf[MAXDNAME];
	const char *longname = nbuf;
	size_t n, d;

	assert(name != NULL);
	

#ifdef DEBUG
	if (res->options & RES_DEBUG)
		printf(";; res_querydomain(%s, %s)\n",
			name, domain?domain:"<Nil>");
#endif
	if (domain == NULL) {
		



		n = strlen(name);
		if (n + 1 > sizeof(nbuf)) {
			h_errno = NO_RECOVERY;
			return -1;
		}
		if (n > 0 && name[--n] == '.') {
			strncpy(nbuf, name, n);
			nbuf[n] = '\0';
		} else
			longname = name;
	} else {
		n = strlen(name);
		d = strlen(domain);
		if (n + 1 + d + 1 > sizeof(nbuf)) {
			h_errno = NO_RECOVERY;
			return -1;
		}
		snprintf(nbuf, sizeof(nbuf), "%s.%s", name, domain);
	}
	return res_queryN(longname, target, res);
}
