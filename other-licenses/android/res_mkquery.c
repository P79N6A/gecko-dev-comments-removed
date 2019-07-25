


















































































#define ANDROID_CHANGES 1

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#ifdef notdef
static const char sccsid[] = "@(#)res_mkquery.c	8.1 (Berkeley) 6/4/93";
static const char rcsid[] = "Id: res_mkquery.c,v 1.1.2.2.4.2 2004/03/16 12:34:18 marka Exp";
#else
__RCSID("$NetBSD: res_mkquery.c,v 1.6 2006/01/24 17:40:32 christos Exp $");
#endif
#endif 



#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include "arpa_nameser.h"
#include <netdb.h>
#ifdef ANDROID_CHANGES
#include "resolv_private.h"
#else
#include <resolv.h>
#endif
#include <stdio.h>
#include <string.h>


#ifndef DEBUG
#define DEBUG
#endif

#ifndef lint
#define UNUSED(a)	(void)&a
#else
#define UNUSED(a)	a = a
#endif

extern const char *_res_opcodes[];





int
res_nmkquery(res_state statp,
	     int op,			
	     const char *dname,		
	     int class, int type,	
	     const u_char *data,	
	     int datalen,		
	     const u_char *newrr_in,	
	     u_char *buf,		
	     int buflen)		
{
	register HEADER *hp;
	register u_char *cp, *ep;
	register int n;
	u_char *dnptrs[20], **dpp, **lastdnptr;

	UNUSED(newrr_in);

#ifdef DEBUG
	if (statp->options & RES_DEBUG)
		printf(";; res_nmkquery(%s, %s, %s, %s)\n",
		       _res_opcodes[op], dname, p_class(class), p_type(type));
#endif
	


	if ((buf == NULL) || (buflen < HFIXEDSZ))
		return (-1);
	memset(buf, 0, HFIXEDSZ);
	hp = (HEADER *)(void *)buf;
	hp->id = htons(res_randomid());
	hp->opcode = op;
	hp->rd = (statp->options & RES_RECURSE) != 0U;
	hp->rcode = NOERROR;
	cp = buf + HFIXEDSZ;
	ep = buf + buflen;
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof dnptrs / sizeof dnptrs[0];
	


	switch (op) {
	case QUERY:	
	case NS_NOTIFY_OP:
		if (ep - cp < QFIXEDSZ)
			return (-1);
		if ((n = dn_comp(dname, cp, ep - cp - QFIXEDSZ, dnptrs,
		    lastdnptr)) < 0)
			return (-1);
		cp += n;
		ns_put16(type, cp);
		cp += INT16SZ;
		ns_put16(class, cp);
		cp += INT16SZ;
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		


		if ((ep - cp) < RRFIXEDSZ)
			return (-1);
		n = dn_comp((const char *)data, cp, ep - cp - RRFIXEDSZ,
			    dnptrs, lastdnptr);
		if (n < 0)
			return (-1);
		cp += n;
		ns_put16(T_NULL, cp);
		cp += INT16SZ;
		ns_put16(class, cp);
		cp += INT16SZ;
		ns_put32(0, cp);
		cp += INT32SZ;
		ns_put16(0, cp);
		cp += INT16SZ;
		hp->arcount = htons(1);
		break;

	case IQUERY:
		


		if (ep - cp < 1 + RRFIXEDSZ + datalen)
			return (-1);
		*cp++ = '\0';	
		ns_put16(type, cp);
		cp += INT16SZ;
		ns_put16(class, cp);
		cp += INT16SZ;
		ns_put32(0, cp);
		cp += INT32SZ;
		ns_put16(datalen, cp);
		cp += INT16SZ;
		if (datalen) {
			memcpy(cp, data, (size_t)datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

	default:
		return (-1);
	}
	return (cp - buf);
}

#ifdef RES_USE_EDNS0

#ifndef T_OPT
#define T_OPT	41
#endif

int
res_nopt(res_state statp,
	 int n0,		
	 u_char *buf,		
	 int buflen,		
	 int anslen)		
{
	register HEADER *hp;
	register u_char *cp, *ep;
	u_int16_t flags = 0;

#ifdef DEBUG
	if ((statp->options & RES_DEBUG) != 0U)
		printf(";; res_nopt()\n");
#endif

	hp = (HEADER *)(void *)buf;
	cp = buf + n0;
	ep = buf + buflen;

	if ((ep - cp) < 1 + RRFIXEDSZ)
		return (-1);

	*cp++ = 0;	

	ns_put16(T_OPT, cp);	
	cp += INT16SZ;
	ns_put16(anslen & 0xffff, cp);	
	cp += INT16SZ;
	*cp++ = NOERROR;	
	*cp++ = 0;		
	if (statp->options & RES_USE_DNSSEC) {
#ifdef DEBUG
		if (statp->options & RES_DEBUG)
			printf(";; res_opt()... ENDS0 DNSSEC\n");
#endif
		flags |= NS_OPT_DNSSEC_OK;
	}
	ns_put16(flags, cp);
	cp += INT16SZ;
	ns_put16(0, cp);	
	cp += INT16SZ;
	hp->arcount = htons(ntohs(hp->arcount) + 1);

	return (cp - buf);
}
#endif
