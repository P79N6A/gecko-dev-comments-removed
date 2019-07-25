


















































































#define ANDROID_CHANGES 1

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#ifdef notdef
static const char sccsid[] = "@(#)res_send.c	8.1 (Berkeley) 6/4/93";
static const char rcsid[] = "Id: res_send.c,v 1.5.2.2.4.5 2004/08/10 02:19:56 marka Exp";
#else
__RCSID("$NetBSD: res_send.c,v 1.9 2006/01/24 17:41:25 christos Exp $");
#endif
#endif 


#define  USE_RESOLV_CACHE  1





#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include "arpa_nameser.h"
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#ifdef ANDROID_CHANGES
#include "resolv_private.h"
#else
#include <resolv.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "eventlib.h"

#if USE_RESOLV_CACHE
#  include <resolv_cache.h>
#endif

#ifndef DE_CONST
#define DE_CONST(c,v)   v = ((c) ? \
    strchr((const void *)(c), *(const char *)(const void *)(c)) : NULL)
#endif


#ifndef DEBUG
#define DEBUG
#endif
#include "res_debug.h"
#include "res_private.h"

#define EXT(res) ((res)->_u._ext)

static const int highestFD = FD_SETSIZE - 1;



static int		get_salen __P((const struct sockaddr *));
static struct sockaddr * get_nsaddr __P((res_state, size_t));
static int		send_vc(res_state, const u_char *, int,
				u_char *, int, int *, int);
static int		send_dg(res_state, const u_char *, int,
				u_char *, int, int *, int,
				int *, int *);
static void		Aerror(const res_state, FILE *, const char *, int,
			       const struct sockaddr *, int);
static void		Perror(const res_state, FILE *, const char *, int);
static int		sock_eq(struct sockaddr *, struct sockaddr *);
#ifdef NEED_PSELECT
static int		pselect(int, void *, void *, void *,
				struct timespec *,
				const sigset_t *);
#endif
void res_pquery(const res_state, const u_char *, int, FILE *);



typedef union {
    struct sockaddr      sa;
    struct sockaddr_in   sin;
    struct sockaddr_in6  sin6;
} _sockaddr_union;

static int
random_bind( int  s, int  family )
{
    _sockaddr_union  u;
    int              j;
    socklen_t        slen;

    
    memset( &u, 0, sizeof u );

    switch (family) {
        case AF_INET:
            u.sin.sin_family = family;
            slen             = sizeof u.sin;
            break;
        case AF_INET6:
            u.sin6.sin6_family = family;
            slen               = sizeof u.sin6;
            break;
        default:
            errno = EPROTO;
            return -1;
    }

    
    for (j = 0; j < 10; j++) {
        
        int  port = 1025 + (res_randomid() % (65535-1025));
        if (family == AF_INET)
            u.sin.sin_port = htons(port);
        else
            u.sin6.sin6_port = htons(port);

        if ( !bind( s, &u.sa, slen ) )
            return 0;
    }

    
    
    if (family == AF_INET)
        u.sin.sin_port = 0;
    else
        u.sin6.sin6_port = 0;

    return bind( s, &u.sa, slen );
}


static const int niflags = NI_NUMERICHOST | NI_NUMERICSERV;












int
res_ourserver_p(const res_state statp, const struct sockaddr *sa) {
	const struct sockaddr_in *inp, *srv;
	const struct sockaddr_in6 *in6p, *srv6;
	int ns;

	switch (sa->sa_family) {
	case AF_INET:
		inp = (const struct sockaddr_in *)(const void *)sa;
		for (ns = 0;  ns < statp->nscount;  ns++) {
			srv = (struct sockaddr_in *)(void *)get_nsaddr(statp, (size_t)ns);
			if (srv->sin_family == inp->sin_family &&
			    srv->sin_port == inp->sin_port &&
			    (srv->sin_addr.s_addr == INADDR_ANY ||
			     srv->sin_addr.s_addr == inp->sin_addr.s_addr))
				return (1);
		}
		break;
	case AF_INET6:
		if (EXT(statp).ext == NULL)
			break;
		in6p = (const struct sockaddr_in6 *)(const void *)sa;
		for (ns = 0;  ns < statp->nscount;  ns++) {
			srv6 = (struct sockaddr_in6 *)(void *)get_nsaddr(statp, (size_t)ns);
			if (srv6->sin6_family == in6p->sin6_family &&
			    srv6->sin6_port == in6p->sin6_port &&
#ifdef HAVE_SIN6_SCOPE_ID
			    (srv6->sin6_scope_id == 0 ||
			     srv6->sin6_scope_id == in6p->sin6_scope_id) &&
#endif
			    (IN6_IS_ADDR_UNSPECIFIED(&srv6->sin6_addr) ||
			     IN6_ARE_ADDR_EQUAL(&srv6->sin6_addr, &in6p->sin6_addr)))
				return (1);
		}
		break;
	default:
		break;
	}
	return (0);
}













int
res_nameinquery(const char *name, int type, int class,
		const u_char *buf, const u_char *eom)
{
	const u_char *cp = buf + HFIXEDSZ;
	int qdcount = ntohs(((const HEADER*)(const void *)buf)->qdcount);

	while (qdcount-- > 0) {
		char tname[MAXDNAME+1];
		int n, ttype, tclass;

		n = dn_expand(buf, eom, cp, tname, sizeof tname);
		if (n < 0)
			return (-1);
		cp += n;
		if (cp + 2 * INT16SZ > eom)
			return (-1);
		ttype = ns_get16(cp); cp += INT16SZ;
		tclass = ns_get16(cp); cp += INT16SZ;
		if (ttype == type && tclass == class &&
		    ns_samename(tname, name) == 1)
			return (1);
	}
	return (0);
}












int
res_queriesmatch(const u_char *buf1, const u_char *eom1,
		 const u_char *buf2, const u_char *eom2)
{
	const u_char *cp = buf1 + HFIXEDSZ;
	int qdcount = ntohs(((const HEADER*)(const void *)buf1)->qdcount);

	if (buf1 + HFIXEDSZ > eom1 || buf2 + HFIXEDSZ > eom2)
		return (-1);

	



	if ((((const HEADER *)(const void *)buf1)->opcode == ns_o_update) &&
	    (((const HEADER *)(const void *)buf2)->opcode == ns_o_update))
		return (1);

	if (qdcount != ntohs(((const HEADER*)(const void *)buf2)->qdcount))
		return (0);
	while (qdcount-- > 0) {
		char tname[MAXDNAME+1];
		int n, ttype, tclass;

		n = dn_expand(buf1, eom1, cp, tname, sizeof tname);
		if (n < 0)
			return (-1);
		cp += n;
		if (cp + 2 * INT16SZ > eom1)
			return (-1);
		ttype = ns_get16(cp);	cp += INT16SZ;
		tclass = ns_get16(cp); cp += INT16SZ;
		if (!res_nameinquery(tname, ttype, tclass, buf2, eom2))
			return (0);
	}
	return (1);
}


int
res_nsend(res_state statp,
	  const u_char *buf, int buflen, u_char *ans, int anssiz)
{
	int gotsomewhere, terrno, try, v_circuit, resplen, ns, n;
	char abuf[NI_MAXHOST];
#if USE_RESOLV_CACHE
        struct resolv_cache*  cache;
        ResolvCacheStatus     cache_status = RESOLV_CACHE_UNSUPPORTED;
#endif

	if (statp->nscount == 0) {
		errno = ESRCH;
		return (-1);
	}
	if (anssiz < HFIXEDSZ) {
		errno = EINVAL;
		return (-1);
	}
	DprintQ((statp->options & RES_DEBUG) || (statp->pfcode & RES_PRF_QUERY),
		(stdout, ";; res_send()\n"), buf, buflen);
	v_circuit = (statp->options & RES_USEVC) || buflen > PACKETSZ;
	gotsomewhere = 0;
	terrno = ETIMEDOUT;

#if USE_RESOLV_CACHE
        cache = __get_res_cache();
        if (cache != NULL) {
            int  anslen = 0;
            cache_status = _resolv_cache_lookup(
                                cache, buf, buflen,
                                ans, anssiz, &anslen);

            if (cache_status == RESOLV_CACHE_FOUND) {
                return anslen;
            }
        }
#endif

	



	if (EXT(statp).nscount != 0) {
		int needclose = 0;
		struct sockaddr_storage peer;
		socklen_t peerlen;

		if (EXT(statp).nscount != statp->nscount)
			needclose++;
		else
			for (ns = 0; ns < statp->nscount; ns++) {
				if (statp->nsaddr_list[ns].sin_family &&
				    !sock_eq((struct sockaddr *)(void *)&statp->nsaddr_list[ns],
					     (struct sockaddr *)(void *)&EXT(statp).ext->nsaddrs[ns])) {
					needclose++;
					break;
				}

				if (EXT(statp).nssocks[ns] == -1)
					continue;
				peerlen = sizeof(peer);
				if (getsockname(EXT(statp).nssocks[ns],
				    (struct sockaddr *)(void *)&peer, &peerlen) < 0) {
					needclose++;
					break;
				}
				if (!sock_eq((struct sockaddr *)(void *)&peer,
				    get_nsaddr(statp, (size_t)ns))) {
					needclose++;
					break;
				}
			}
		if (needclose) {
			res_nclose(statp);
			EXT(statp).nscount = 0;
		}
	}

	


	if (EXT(statp).nscount == 0) {
		for (ns = 0; ns < statp->nscount; ns++) {
			EXT(statp).nstimes[ns] = RES_MAXTIME;
			EXT(statp).nssocks[ns] = -1;
			if (!statp->nsaddr_list[ns].sin_family)
				continue;
			EXT(statp).ext->nsaddrs[ns].sin =
				 statp->nsaddr_list[ns];
		}
		EXT(statp).nscount = statp->nscount;
	}

	



	if ((statp->options & RES_ROTATE) != 0U &&
	    (statp->options & RES_BLAST) == 0U) {
		union res_sockaddr_union inu;
		struct sockaddr_in ina;
		int lastns = statp->nscount - 1;
		int fd;
		u_int16_t nstime;

		if (EXT(statp).ext != NULL)
			inu = EXT(statp).ext->nsaddrs[0];
		ina = statp->nsaddr_list[0];
		fd = EXT(statp).nssocks[0];
		nstime = EXT(statp).nstimes[0];
		for (ns = 0; ns < lastns; ns++) {
			if (EXT(statp).ext != NULL)
                                EXT(statp).ext->nsaddrs[ns] =
					EXT(statp).ext->nsaddrs[ns + 1];
			statp->nsaddr_list[ns] = statp->nsaddr_list[ns + 1];
			EXT(statp).nssocks[ns] = EXT(statp).nssocks[ns + 1];
			EXT(statp).nstimes[ns] = EXT(statp).nstimes[ns + 1];
		}
		if (EXT(statp).ext != NULL)
			EXT(statp).ext->nsaddrs[lastns] = inu;
		statp->nsaddr_list[lastns] = ina;
		EXT(statp).nssocks[lastns] = fd;
		EXT(statp).nstimes[lastns] = nstime;
	}

	


	for (try = 0; try < statp->retry; try++) {
	    for (ns = 0; ns < statp->nscount; ns++) {
		struct sockaddr *nsap;
		int nsaplen;
		nsap = get_nsaddr(statp, (size_t)ns);
		nsaplen = get_salen(nsap);
		statp->_flags &= ~RES_F_LASTMASK;
		statp->_flags |= (ns << RES_F_LASTSHIFT);
 same_ns:
		if (statp->qhook) {
			int done = 0, loops = 0;

			do {
				res_sendhookact act;

				act = (*statp->qhook)(&nsap, &buf, &buflen,
						      ans, anssiz, &resplen);
				switch (act) {
				case res_goahead:
					done = 1;
					break;
				case res_nextns:
					res_nclose(statp);
					goto next_ns;
				case res_done:
					return (resplen);
				case res_modified:
					
					if (++loops < 42) 
						break;
					
				case res_error:
					
				default:
					goto fail;
				}
			} while (!done);
		}

		Dprint(((statp->options & RES_DEBUG) &&
			getnameinfo(nsap, (socklen_t)nsaplen, abuf, sizeof(abuf),
				    NULL, 0, niflags) == 0),
		       (stdout, ";; Querying server (# %d) address = %s\n",
			ns + 1, abuf));


		if (v_circuit) {
			
			try = statp->retry;
			n = send_vc(statp, buf, buflen, ans, anssiz, &terrno,
				    ns);
			if (n < 0)
				goto fail;
			if (n == 0)
				goto next_ns;
			resplen = n;
		} else {
			
			n = send_dg(statp, buf, buflen, ans, anssiz, &terrno,
				    ns, &v_circuit, &gotsomewhere);
			if (n < 0)
				goto fail;
			if (n == 0)
				goto next_ns;
			if (v_circuit)
				goto same_ns;
			resplen = n;
		}

		Dprint((statp->options & RES_DEBUG) ||
		       ((statp->pfcode & RES_PRF_REPLY) &&
			(statp->pfcode & RES_PRF_HEAD1)),
		       (stdout, ";; got answer:\n"));

		DprintQ((statp->options & RES_DEBUG) ||
			(statp->pfcode & RES_PRF_REPLY),
			(stdout, "%s", ""),
			ans, (resplen > anssiz) ? anssiz : resplen);

#if USE_RESOLV_CACHE
                if (cache_status == RESOLV_CACHE_NOTFOUND) {
                    _resolv_cache_add(cache, buf, buflen,
                                      ans, resplen);
                }
#endif
		




		if ((v_circuit && (statp->options & RES_USEVC) == 0U) ||
		    (statp->options & RES_STAYOPEN) == 0U) {
			res_nclose(statp);
		}
		if (statp->rhook) {
			int done = 0, loops = 0;

			do {
				res_sendhookact act;

				act = (*statp->rhook)(nsap, buf, buflen,
						      ans, anssiz, &resplen);
				switch (act) {
				case res_goahead:
				case res_done:
					done = 1;
					break;
				case res_nextns:
					res_nclose(statp);
					goto next_ns;
				case res_modified:
					
					if (++loops < 42) 
						break;
					
				case res_error:
					
				default:
					goto fail;
				}
			} while (!done);

		}
		return (resplen);
 next_ns: ;
	   } 
	} 
	res_nclose(statp);
	if (!v_circuit) {
		if (!gotsomewhere)
			errno = ECONNREFUSED;	
		else
			errno = ETIMEDOUT;	
	} else
		errno = terrno;
	return (-1);
 fail:
	res_nclose(statp);
	return (-1);
}



static int
get_salen(sa)
	const struct sockaddr *sa;
{

#ifdef HAVE_SA_LEN
	
	if (sa->sa_len)
		return (sa->sa_len);
#endif

	if (sa->sa_family == AF_INET)
		return (sizeof(struct sockaddr_in));
	else if (sa->sa_family == AF_INET6)
		return (sizeof(struct sockaddr_in6));
	else
		return (0);	
}




static struct sockaddr *
get_nsaddr(statp, n)
	res_state statp;
	size_t n;
{

	if (!statp->nsaddr_list[n].sin_family && EXT(statp).ext) {
		




		return (struct sockaddr *)(void *)&EXT(statp).ext->nsaddrs[n];
	} else {
		




		return (struct sockaddr *)(void *)&statp->nsaddr_list[n];
	}
}

static int
send_vc(res_state statp,
	const u_char *buf, int buflen, u_char *ans, int anssiz,
	int *terrno, int ns)
{
	const HEADER *hp = (const HEADER *)(const void *)buf;
	HEADER *anhp = (HEADER *)(void *)ans;
	struct sockaddr *nsap;
	int nsaplen;
	int truncating, connreset, resplen, n;
	struct iovec iov[2];
	u_short len;
	u_char *cp;
	void *tmp;

	nsap = get_nsaddr(statp, (size_t)ns);
	nsaplen = get_salen(nsap);

	connreset = 0;
 same_ns:
	truncating = 0;

	
	if (statp->_vcsock >= 0 && (statp->_flags & RES_F_VC) != 0) {
		struct sockaddr_storage peer;
		socklen_t size = sizeof peer;

		if (getpeername(statp->_vcsock,
				(struct sockaddr *)(void *)&peer, &size) < 0 ||
		    !sock_eq((struct sockaddr *)(void *)&peer, nsap)) {
			res_nclose(statp);
			statp->_flags &= ~RES_F_VC;
		}
	}

	if (statp->_vcsock < 0 || (statp->_flags & RES_F_VC) == 0) {
		if (statp->_vcsock >= 0)
			res_nclose(statp);

		statp->_vcsock = socket(nsap->sa_family, SOCK_STREAM, 0);
		if (statp->_vcsock > highestFD) {
			res_nclose(statp);
			errno = ENOTSOCK;
		}
		if (statp->_vcsock < 0) {
			switch (errno) {
			case EPROTONOSUPPORT:
#ifdef EPFNOSUPPORT
			case EPFNOSUPPORT:
#endif
			case EAFNOSUPPORT:
				Perror(statp, stderr, "socket(vc)", errno);
				return (0);
			default:
				*terrno = errno;
				Perror(statp, stderr, "socket(vc)", errno);
				return (-1);
			}
		}
		errno = 0;
		if (random_bind(statp->_vcsock,nsap->sa_family) < 0) {
			*terrno = errno;
			Aerror(statp, stderr, "bind/vc", errno, nsap,
			    nsaplen);
			res_nclose(statp);
			return (0);
		}
		if (connect(statp->_vcsock, nsap, (socklen_t)nsaplen) < 0) {
			*terrno = errno;
			Aerror(statp, stderr, "connect/vc", errno, nsap,
			    nsaplen);
			res_nclose(statp);
			return (0);
		}
		statp->_flags |= RES_F_VC;
	}

	


	ns_put16((u_short)buflen, (u_char*)(void *)&len);
	iov[0] = evConsIovec(&len, INT16SZ);
	DE_CONST(buf, tmp);
	iov[1] = evConsIovec(tmp, (size_t)buflen);
	if (writev(statp->_vcsock, iov, 2) != (INT16SZ + buflen)) {
		*terrno = errno;
		Perror(statp, stderr, "write failed", errno);
		res_nclose(statp);
		return (0);
	}
	


 read_len:
	cp = ans;
	len = INT16SZ;
	while ((n = read(statp->_vcsock, (char *)cp, (size_t)len)) > 0) {
		cp += n;
		if ((len -= n) == 0)
			break;
	}
	if (n <= 0) {
		*terrno = errno;
		Perror(statp, stderr, "read failed", errno);
	  res_nclose(statp);
		








		if (*terrno == ECONNRESET && !connreset) {
			connreset = 1;
			res_nclose(statp);
			goto same_ns;
		}
		res_nclose(statp);
		return (0);
	}
	resplen = ns_get16(ans);
	if (resplen > anssiz) {
		Dprint(statp->options & RES_DEBUG,
		       (stdout, ";; response truncated\n")
		       );
		truncating = 1;
		len = anssiz;
	} else
		len = resplen;
	if (len < HFIXEDSZ) {
		


		Dprint(statp->options & RES_DEBUG,
		       (stdout, ";; undersized: %d\n", len));
		*terrno = EMSGSIZE;
		res_nclose(statp);
		return (0);
	}
	cp = ans;
	while (len != 0 && (n = read(statp->_vcsock, (char *)cp, (size_t)len)) > 0){
		cp += n;
		len -= n;
	}
	if (n <= 0) {
		*terrno = errno;
		Perror(statp, stderr, "read(vc)", errno);
		res_nclose(statp);
		return (0);
	}
	if (truncating) {
		


		anhp->tc = 1;
		len = resplen - anssiz;
		while (len != 0) {
			char junk[PACKETSZ];

			n = read(statp->_vcsock, junk,
				 (len > sizeof junk) ? sizeof junk : len);
			if (n > 0)
				len -= n;
			else
				break;
		}
	}
	






	if (hp->id != anhp->id) {
		DprintQ((statp->options & RES_DEBUG) ||
			(statp->pfcode & RES_PRF_REPLY),
			(stdout, ";; old answer (unexpected):\n"),
			ans, (resplen > anssiz) ? anssiz: resplen);
		goto read_len;
	}

	



	return (resplen);
}

static int
send_dg(res_state statp,
	const u_char *buf, int buflen, u_char *ans, int anssiz,
	int *terrno, int ns, int *v_circuit, int *gotsomewhere)
{
	const HEADER *hp = (const HEADER *)(const void *)buf;
	HEADER *anhp = (HEADER *)(void *)ans;
	const struct sockaddr *nsap;
	int nsaplen;
	struct timespec now, timeout, finish;
	fd_set dsmask;
	struct sockaddr_storage from;
	socklen_t fromlen;
	int resplen, seconds, n, s;

	nsap = get_nsaddr(statp, (size_t)ns);
	nsaplen = get_salen(nsap);
	if (EXT(statp).nssocks[ns] == -1) {
		EXT(statp).nssocks[ns] = socket(nsap->sa_family, SOCK_DGRAM, 0);
		if (EXT(statp).nssocks[ns] > highestFD) {
			res_nclose(statp);
			errno = ENOTSOCK;
		}
		if (EXT(statp).nssocks[ns] < 0) {
			switch (errno) {
			case EPROTONOSUPPORT:
#ifdef EPFNOSUPPORT
			case EPFNOSUPPORT:
#endif
			case EAFNOSUPPORT:
				Perror(statp, stderr, "socket(dg)", errno);
				return (0);
			default:
				*terrno = errno;
				Perror(statp, stderr, "socket(dg)", errno);
				return (-1);
			}
		}
#ifndef CANNOT_CONNECT_DGRAM
		










		if (random_bind(EXT(statp).nssocks[ns], nsap->sa_family) < 0) {
			Aerror(statp, stderr, "bind(dg)", errno, nsap,
			    nsaplen);
			res_nclose(statp);
			return (0);
		}
		if (connect(EXT(statp).nssocks[ns], nsap, (socklen_t)nsaplen) < 0) {
			Aerror(statp, stderr, "connect(dg)", errno, nsap,
			    nsaplen);
			res_nclose(statp);
			return (0);
		}
#endif 
		Dprint(statp->options & RES_DEBUG,
		       (stdout, ";; new DG socket\n"))
	}
	s = EXT(statp).nssocks[ns];
#ifndef CANNOT_CONNECT_DGRAM
	if (send(s, (const char*)buf, (size_t)buflen, 0) != buflen) {
		Perror(statp, stderr, "send", errno);
		res_nclose(statp);
		return (0);
	}
#else 
	if (sendto(s, (const char*)buf, buflen, 0, nsap, nsaplen) != buflen)
	{
		Aerror(statp, stderr, "sendto", errno, nsap, nsaplen);
		res_nclose(statp);
		return (0);
	}
#endif 

	


	seconds = (statp->retrans << ns);
	if (ns > 0)
		seconds /= statp->nscount;
	if (seconds <= 0)
		seconds = 1;
	now = evNowTime();
	timeout = evConsTime((long)seconds, 0L);
	finish = evAddTime(now, timeout);
	goto nonow;
 wait:
	now = evNowTime();
 nonow:
	FD_ZERO(&dsmask);
	FD_SET(s, &dsmask);
	if (evCmpTime(finish, now) > 0)
		timeout = evSubTime(finish, now);
	else
		timeout = evConsTime(0L, 0L);
	n = pselect(s + 1, &dsmask, NULL, NULL, &timeout, NULL);
	if (n == 0) {
		Dprint(statp->options & RES_DEBUG, (stdout, ";; timeout\n"));
		*gotsomewhere = 1;
		return (0);
	}
	if (n < 0) {
		if (errno == EINTR)
			goto wait;
		Perror(statp, stderr, "select", errno);
		res_nclose(statp);
		return (0);
	}
	errno = 0;
	fromlen = sizeof(from);
	resplen = recvfrom(s, (char*)ans, (size_t)anssiz,0,
			   (struct sockaddr *)(void *)&from, &fromlen);
	if (resplen <= 0) {
		Perror(statp, stderr, "recvfrom", errno);
		res_nclose(statp);
		return (0);
	}
	*gotsomewhere = 1;
	if (resplen < HFIXEDSZ) {
		


		Dprint(statp->options & RES_DEBUG,
		       (stdout, ";; undersized: %d\n",
			resplen));
		*terrno = EMSGSIZE;
		res_nclose(statp);
		return (0);
	}
	if (hp->id != anhp->id) {
		




		DprintQ((statp->options & RES_DEBUG) ||
			(statp->pfcode & RES_PRF_REPLY),
			(stdout, ";; old answer:\n"),
			ans, (resplen > anssiz) ? anssiz : resplen);
		goto wait;
	}
	if (!(statp->options & RES_INSECURE1) &&
	    !res_ourserver_p(statp, (struct sockaddr *)(void *)&from)) {
		




		DprintQ((statp->options & RES_DEBUG) ||
			(statp->pfcode & RES_PRF_REPLY),
			(stdout, ";; not our server:\n"),
			ans, (resplen > anssiz) ? anssiz : resplen);
		goto wait;
	}
#ifdef RES_USE_EDNS0
	if (anhp->rcode == FORMERR && (statp->options & RES_USE_EDNS0) != 0U) {
		




		DprintQ(statp->options & RES_DEBUG,
			(stdout, "server rejected query with EDNS0:\n"),
			ans, (resplen > anssiz) ? anssiz : resplen);
		
		statp->_flags |= RES_F_EDNS0ERR;
		res_nclose(statp);
		return (0);
	}
#endif
	if (!(statp->options & RES_INSECURE2) &&
	    !res_queriesmatch(buf, buf + buflen,
			      ans, ans + anssiz)) {
		




		DprintQ((statp->options & RES_DEBUG) ||
			(statp->pfcode & RES_PRF_REPLY),
			(stdout, ";; wrong query name:\n"),
			ans, (resplen > anssiz) ? anssiz : resplen);
		goto wait;
	}
	if (anhp->rcode == SERVFAIL ||
	    anhp->rcode == NOTIMP ||
	    anhp->rcode == REFUSED) {
		DprintQ(statp->options & RES_DEBUG,
			(stdout, "server rejected query:\n"),
			ans, (resplen > anssiz) ? anssiz : resplen);
		res_nclose(statp);
		
		if (!statp->pfcode)
			return (0);
	}
	if (!(statp->options & RES_IGNTC) && anhp->tc) {
		



		Dprint(statp->options & RES_DEBUG,
		       (stdout, ";; truncated answer\n"));
		*v_circuit = 1;
		res_nclose(statp);
		return (1);
	}
	



	return (resplen);
}

static void
Aerror(const res_state statp, FILE *file, const char *string, int error,
       const struct sockaddr *address, int alen)
{
	int save = errno;
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXSERV];

	alen = alen;

	if ((statp->options & RES_DEBUG) != 0U) {
		if (getnameinfo(address, (socklen_t)alen, hbuf, sizeof(hbuf),
		    sbuf, sizeof(sbuf), niflags)) {
			strncpy(hbuf, "?", sizeof(hbuf) - 1);
			hbuf[sizeof(hbuf) - 1] = '\0';
			strncpy(sbuf, "?", sizeof(sbuf) - 1);
			sbuf[sizeof(sbuf) - 1] = '\0';
		}
		fprintf(file, "res_send: %s ([%s].%s): %s\n",
			string, hbuf, sbuf, strerror(error));
	}
	errno = save;
}

static void
Perror(const res_state statp, FILE *file, const char *string, int error) {
	int save = errno;

	if ((statp->options & RES_DEBUG) != 0U)
		fprintf(file, "res_send: %s: %s\n",
			string, strerror(error));
	errno = save;
}

static int
sock_eq(struct sockaddr *a, struct sockaddr *b) {
	struct sockaddr_in *a4, *b4;
	struct sockaddr_in6 *a6, *b6;

	if (a->sa_family != b->sa_family)
		return 0;
	switch (a->sa_family) {
	case AF_INET:
		a4 = (struct sockaddr_in *)(void *)a;
		b4 = (struct sockaddr_in *)(void *)b;
		return a4->sin_port == b4->sin_port &&
		    a4->sin_addr.s_addr == b4->sin_addr.s_addr;
	case AF_INET6:
		a6 = (struct sockaddr_in6 *)(void *)a;
		b6 = (struct sockaddr_in6 *)(void *)b;
		return a6->sin6_port == b6->sin6_port &&
#ifdef HAVE_SIN6_SCOPE_ID
		    a6->sin6_scope_id == b6->sin6_scope_id &&
#endif
		    IN6_ARE_ADDR_EQUAL(&a6->sin6_addr, &b6->sin6_addr);
	default:
		return 0;
	}
}

#ifdef NEED_PSELECT

static int
pselect(int nfds, void *rfds, void *wfds, void *efds,
	struct timespec *tsp, const sigset_t *sigmask)
{
	struct timeval tv, *tvp;
	sigset_t sigs;
	int n;

	if (tsp) {
		tvp = &tv;
		tv = evTimeVal(*tsp);
	} else
		tvp = NULL;
	if (sigmask)
		sigprocmask(SIG_SETMASK, sigmask, &sigs);
	n = select(nfds, rfds, wfds, efds, tvp);
	if (sigmask)
		sigprocmask(SIG_SETMASK, &sigs, NULL);
	if (tsp)
		*tsp = evTimeSpec(tv);
	return (n);
}
#endif
