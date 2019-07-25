




























#define ANDROID_CHANGES 1
#define MOZILLA_NECKO_EXCLUDE_CODE 1 

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static const char rcsid[] = "Id: ns_print.c,v 1.3.2.1.4.5 2004/07/28 20:16:45 marka Exp";
#else
__RCSID("$NetBSD: ns_print.c,v 1.5 2004/11/07 02:19:49 christos Exp $");
#endif
#endif



#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include "arpa_nameser.h"
#include <arpa/inet.h>

#include "assertions.h"
#include "dst.h"
#include <errno.h>
#ifdef ANDROID_CHANGES
#include "resolv_private.h"
#else
#include <resolv.h>
#endif
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef SPRINTF_CHAR
# define SPRINTF(x) strlen(sprintf/**/x)
#else
# define SPRINTF(x) ((size_t)sprintf x)
#endif

#ifndef MIN
#define	MIN(x,y)	((x)<(y)?(x):(y))
#endif



static size_t	prune_origin(const char *name, const char *origin);
static int	charstr(const u_char *rdata, const u_char *edata,
			char **buf, size_t *buflen);
static int	addname(const u_char *msg, size_t msglen,
			const u_char **p, const char *origin,
			char **buf, size_t *buflen);
static void	addlen(size_t len, char **buf, size_t *buflen);
static int	addstr(const char *src, size_t len,
		       char **buf, size_t *buflen);
static int	addtab(size_t len, size_t target, int spaced,
		       char **buf, size_t *buflen);



#define	T(x) \
	do { \
		if ((x) < 0) \
			return (-1); \
	} while (/*CONSTCOND*/0)










int
ns_sprintrr(const ns_msg *handle, const ns_rr *rr,
	    const char *name_ctx, const char *origin,
	    char *buf, size_t buflen)
{
	int n;

	n = ns_sprintrrf(ns_msg_base(*handle), ns_msg_size(*handle),
			 ns_rr_name(*rr), ns_rr_class(*rr), ns_rr_type(*rr),
			 ns_rr_ttl(*rr), ns_rr_rdata(*rr), ns_rr_rdlen(*rr),
			 name_ctx, origin, buf, buflen);
	return (n);
}









int
ns_sprintrrf(const u_char *msg, size_t msglen,
	    const char *name, ns_class class, ns_type type,
	    u_long ttl, const u_char *rdata, size_t rdlen,
	    const char *name_ctx, const char *origin,
	    char *buf, size_t buflen)
{
	const char *obuf = buf;
	const u_char *edata = rdata + rdlen;
	int spaced = 0;

	const char *comment;
	char tmp[100];
	int len, x;

	


	if (name_ctx != NULL && ns_samename(name_ctx, name) == 1) {
		T(addstr("\t\t\t", (size_t)3, &buf, &buflen));
	} else {
		len = prune_origin(name, origin);
		if (*name == '\0') {
			goto root;
		} else if (len == 0) {
			T(addstr("@\t\t\t", (size_t)4, &buf, &buflen));
		} else {
			T(addstr(name, (size_t)len, &buf, &buflen));
			
			if (((origin == NULL || origin[0] == '\0') ||
			    (origin[0] != '.' && origin[1] != '\0' &&
			    name[len] == '\0')) && name[len - 1] != '.') {
 root:
				T(addstr(".", (size_t)1, &buf, &buflen));
				len++;
			}
			T(spaced = addtab((size_t)len, 24, spaced, &buf, &buflen));
		}
	}

	


	T(x = ns_format_ttl(ttl, buf, buflen));
	addlen((size_t)x, &buf, &buflen);
	len = SPRINTF((tmp, " %s %s", p_class(class), p_type(type)));
	T(addstr(tmp, (size_t)len, &buf, &buflen));
	T(spaced = addtab((size_t)(x + len), (size_t)16, spaced, &buf, &buflen));

	


	switch (type) {
	case ns_t_a:
		if (rdlen != (size_t)NS_INADDRSZ)
			goto formerr;
		(void) inet_ntop(AF_INET, rdata, buf, buflen);
		addlen(strlen(buf), &buf, &buflen);
		break;

	case ns_t_cname:
	case ns_t_mb:
	case ns_t_mg:
	case ns_t_mr:
	case ns_t_ns:
	case ns_t_ptr:
	case ns_t_dname:
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		break;

	case ns_t_hinfo:
	case ns_t_isdn:
		
		T(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		T(addstr(" ", (size_t)1, &buf, &buflen));


		
		if (type == ns_t_isdn && rdata == edata)
			break;

		T(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		break;

	case ns_t_soa: {
		u_long t;

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		T(addstr(" ", (size_t)1, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		T(addstr(" (\n", (size_t)3, &buf, &buflen));
		spaced = 0;

		if ((edata - rdata) != 5*NS_INT32SZ)
			goto formerr;

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		T(addstr("\t\t\t\t\t", (size_t)5, &buf, &buflen));
		len = SPRINTF((tmp, "%lu", t));
		T(addstr(tmp, (size_t)len, &buf, &buflen));
		T(spaced = addtab((size_t)len, (size_t)16, spaced, &buf, &buflen));
		T(addstr("; serial\n", (size_t)9, &buf, &buflen));
		spaced = 0;

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		T(addstr("\t\t\t\t\t", (size_t)5, &buf, &buflen));
		T(len = ns_format_ttl(t, buf, buflen));
		addlen((size_t)len, &buf, &buflen);
		T(spaced = addtab((size_t)len, (size_t)16, spaced, &buf, &buflen));
		T(addstr("; refresh\n", (size_t)10, &buf, &buflen));
		spaced = 0;

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		T(addstr("\t\t\t\t\t", (size_t)5, &buf, &buflen));
		T(len = ns_format_ttl(t, buf, buflen));
		addlen((size_t)len, &buf, &buflen);
		T(spaced = addtab((size_t)len, (size_t)16, spaced, &buf, &buflen));
		T(addstr("; retry\n", (size_t)8, &buf, &buflen));
		spaced = 0;

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		T(addstr("\t\t\t\t\t", (size_t)5, &buf, &buflen));
		T(len = ns_format_ttl(t, buf, buflen));
		addlen((size_t)len, &buf, &buflen);
		T(spaced = addtab((size_t)len, (size_t)16, spaced, &buf, &buflen));
		T(addstr("; expiry\n", (size_t)9, &buf, &buflen));
		spaced = 0;

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		T(addstr("\t\t\t\t\t", (size_t)5, &buf, &buflen));
		T(len = ns_format_ttl(t, buf, buflen));
		addlen((size_t)len, &buf, &buflen);
		T(addstr(" )", (size_t)2, &buf, &buflen));
		T(spaced = addtab((size_t)len, (size_t)16, spaced, &buf, &buflen));
		T(addstr("; minimum\n", (size_t)10, &buf, &buflen));

		break;
	    }

	case ns_t_mx:
	case ns_t_afsdb:
	case ns_t_rt: {
		u_int t;

		if (rdlen < (size_t)NS_INT16SZ)
			goto formerr;

		
		t = ns_get16(rdata);
		rdata += NS_INT16SZ;
		len = SPRINTF((tmp, "%u ", t));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		break;
	    }

	case ns_t_px: {
		u_int t;

		if (rdlen < (size_t)NS_INT16SZ)
			goto formerr;

		
		t = ns_get16(rdata);
		rdata += NS_INT16SZ;
		len = SPRINTF((tmp, "%u ", t));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		T(addstr(" ", (size_t)1, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		break;
	    }

	case ns_t_x25:
		T(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		break;

	case ns_t_txt:
		while (rdata < edata) {
			T(len = charstr(rdata, edata, &buf, &buflen));
			if (len == 0)
				goto formerr;
			rdata += len;
			if (rdata < edata)
				T(addstr(" ", (size_t)1, &buf, &buflen));
		}
		break;

	case ns_t_nsap: {
		char t[2+255*3];

		(void) inet_nsap_ntoa((int)rdlen, rdata, t);
		T(addstr(t, strlen(t), &buf, &buflen));
		break;
	    }

	case ns_t_aaaa:
		if (rdlen != (size_t)NS_IN6ADDRSZ)
			goto formerr;
		(void) inet_ntop(AF_INET6, rdata, buf, buflen);
		addlen(strlen(buf), &buf, &buflen);
		break;

	case ns_t_loc: {
		char t[255];

		
		(void) loc_ntoa(rdata, t);
		T(addstr(t, strlen(t), &buf, &buflen));
		break;
	    }

	case ns_t_naptr: {
		u_int order, preference;
		char t[50];

		if (rdlen < 2U*NS_INT16SZ)
			goto formerr;

		
		order = ns_get16(rdata);	rdata += NS_INT16SZ;
		preference = ns_get16(rdata);	rdata += NS_INT16SZ;
		len = SPRINTF((t, "%u %u ", order, preference));
		T(addstr(t, (size_t)len, &buf, &buflen));

		
		T(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		T(addstr(" ", (size_t)1, &buf, &buflen));

		
		T(len = charstr(rdata, edata, &buf, &buflen));
		if (len == 0)
			goto formerr;
		rdata += len;
		T(addstr(" ", (size_t)1, &buf, &buflen));

		
		T(len = charstr(rdata, edata, &buf, &buflen));
		if (len < 0)
			return (-1);
		if (len == 0)
			goto formerr;
		rdata += len;
		T(addstr(" ", (size_t)1, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		break;
	    }

	case ns_t_srv: {
		u_int priority, weight, port;
		char t[50];

		if (rdlen < 3U*NS_INT16SZ)
			goto formerr;

		
		priority = ns_get16(rdata);  rdata += NS_INT16SZ;
		weight   = ns_get16(rdata);  rdata += NS_INT16SZ;
		port     = ns_get16(rdata);  rdata += NS_INT16SZ;
		len = SPRINTF((t, "%u %u %u ", priority, weight, port));
		T(addstr(t, (size_t)len, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		break;
	    }

	case ns_t_minfo:
	case ns_t_rp:
		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		T(addstr(" ", (size_t)1, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		break;

	case ns_t_wks: {
		int n, lcnt;

		if (rdlen < 1U + NS_INT32SZ)
			goto formerr;

		
		(void) inet_ntop(AF_INET, rdata, buf, buflen);
		addlen(strlen(buf), &buf, &buflen);
		rdata += NS_INADDRSZ;

		
		len = SPRINTF((tmp, " %u ( ", *rdata));
		T(addstr(tmp, (size_t)len, &buf, &buflen));
		rdata += NS_INT8SZ;

		
		n = 0;
		lcnt = 0;
		while (rdata < edata) {
			u_int c = *rdata++;
			do {
				if (c & 0200) {
					if (lcnt == 0) {
						T(addstr("\n\t\t\t\t", (size_t)5,
							 &buf, &buflen));
						lcnt = 10;
						spaced = 0;
					}
					len = SPRINTF((tmp, "%d ", n));
					T(addstr(tmp, (size_t)len, &buf, &buflen));
					lcnt--;
				}
				c <<= 1;
			} while (++n & 07);
		}
		T(addstr(")", (size_t)1, &buf, &buflen));

		break;
	    }

	case ns_t_key: {
		char base64_key[NS_MD5RSA_MAX_BASE64];
		u_int keyflags, protocol, algorithm, key_id;
		const char *leader;
		int n;

		if (rdlen < 0U + NS_INT16SZ + NS_INT8SZ + NS_INT8SZ)
			goto formerr;

		
#if !defined(MOZILLA_NECKO_EXCLUDE_CODE) && !defined(_LIBC)
		key_id = dst_s_dns_key_id(rdata, edata-rdata);
#else
		key_id = 0;
#endif
		keyflags = ns_get16(rdata);  rdata += NS_INT16SZ;
		protocol = *rdata++;
		algorithm = *rdata++;
		len = SPRINTF((tmp, "0x%04x %u %u",
			       keyflags, protocol, algorithm));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		len = b64_ntop(rdata, (size_t)(edata - rdata),
			       base64_key, sizeof base64_key);
		if (len < 0)
			goto formerr;
		if (len > 15) {
			T(addstr(" (", (size_t)2, &buf, &buflen));
			leader = "\n\t\t";
			spaced = 0;
		} else
			leader = " ";
		for (n = 0; n < len; n += 48) {
			T(addstr(leader, strlen(leader), &buf, &buflen));
			T(addstr(base64_key + n, (size_t)MIN(len - n, 48),
				 &buf, &buflen));
		}
		if (len > 15)
			T(addstr(" )", (size_t)2, &buf, &buflen));
		n = SPRINTF((tmp, " ; key_tag= %u", key_id));
		T(addstr(tmp, (size_t)n, &buf, &buflen));

		break;
	    }

	case ns_t_sig: {
		char base64_key[NS_MD5RSA_MAX_BASE64];
		u_int typ, algorithm, labels, footprint;
		const char *leader;
		u_long t;
		int n;

		if (rdlen < 22U)
			goto formerr;

		
	        typ = ns_get16(rdata);  rdata += NS_INT16SZ;
		algorithm = *rdata++;
		labels = *rdata++;
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		len = SPRINTF((tmp, "%s %d %d %lu ",
			       p_type((int)typ), algorithm, labels, t));
		T(addstr(tmp, (size_t)len, &buf, &buflen));
		if (labels > (u_int)dn_count_labels(name))
			goto formerr;

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		len = SPRINTF((tmp, "%s ", p_secstodate(t)));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		len = SPRINTF((tmp, "%s ", p_secstodate(t)));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		footprint = ns_get16(rdata);  rdata += NS_INT16SZ;
		len = SPRINTF((tmp, "%u ", footprint));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		
		len = b64_ntop(rdata, (size_t)(edata - rdata),
			       base64_key, sizeof base64_key);
		if (len > 15) {
			T(addstr(" (", (size_t)2, &buf, &buflen));
			leader = "\n\t\t";
			spaced = 0;
		} else
			leader = " ";
		if (len < 0)
			goto formerr;
		for (n = 0; n < len; n += 48) {
			T(addstr(leader, strlen(leader), &buf, &buflen));
			T(addstr(base64_key + n, (size_t)MIN(len - n, 48),
				 &buf, &buflen));
		}
		if (len > 15)
			T(addstr(" )", (size_t)2, &buf, &buflen));
		break;
	    }

	case ns_t_nxt: {
		int n, c;

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		
		n = edata - rdata;
		for (c = 0; c < n*8; c++)
			if (NS_NXT_BIT_ISSET(c, rdata)) {
				len = SPRINTF((tmp, " %s", p_type(c)));
				T(addstr(tmp, (size_t)len, &buf, &buflen));
			}
		break;
	    }

	case ns_t_cert: {
		u_int c_type, key_tag, alg;
		int n;
		unsigned int siz;
		char base64_cert[8192], tmp1[40];
		const char *leader;

		c_type  = ns_get16(rdata); rdata += NS_INT16SZ;
		key_tag = ns_get16(rdata); rdata += NS_INT16SZ;
		alg = (u_int) *rdata++;

		len = SPRINTF((tmp1, "%d %d %d ", c_type, key_tag, alg));
		T(addstr(tmp1, (size_t)len, &buf, &buflen));
		siz = (edata-rdata)*4/3 + 4; 
		if (siz > sizeof(base64_cert) * 3/4) {
			const char *str = "record too long to print";
			T(addstr(str, strlen(str), &buf, &buflen));
		}
		else {
			len = b64_ntop(rdata, (size_t)(edata-rdata),
			    base64_cert, siz);

			if (len < 0)
				goto formerr;
			else if (len > 15) {
				T(addstr(" (", (size_t)2, &buf, &buflen));
				leader = "\n\t\t";
				spaced = 0;
			}
			else
				leader = " ";

			for (n = 0; n < len; n += 48) {
				T(addstr(leader, strlen(leader),
					 &buf, &buflen));
				T(addstr(base64_cert + n, (size_t)MIN(len - n, 48),
					 &buf, &buflen));
			}
			if (len > 15)
				T(addstr(" )", (size_t)2, &buf, &buflen));
		}
		break;
	    }

	case ns_t_tkey: {
		
		u_long t;
		int mode, err, keysize;

		
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));
		T(addstr(" ", (size_t)1, &buf, &buflen));

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		len = SPRINTF((tmp, "%s ", p_secstodate(t)));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		t = ns_get32(rdata);  rdata += NS_INT32SZ;
		len = SPRINTF((tmp, "%s ", p_secstodate(t)));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		
		mode = ns_get16(rdata);  rdata += NS_INT16SZ;
		err  = ns_get16(rdata);  rdata += NS_INT16SZ;
		keysize  = ns_get16(rdata);  rdata += NS_INT16SZ;
		len = SPRINTF((tmp, "%u %u %u ", mode, err, keysize));
		T(addstr(tmp, (size_t)len, &buf, &buflen));

		
		break;
	    }

	case ns_t_tsig: {
		
		int n;

		T(len = addname(msg, msglen, &rdata, origin, &buf, &buflen));
		T(addstr(" ", (size_t)1, &buf, &buflen));
		rdata += 8; 
		n = ns_get16(rdata); rdata += INT16SZ;
		rdata += n; 
		n = ns_get16(rdata); rdata += INT16SZ; 
		sprintf(buf, "%d", ns_get16(rdata));
		rdata += INT16SZ;
		addlen(strlen(buf), &buf, &buflen);
		break;
	    }

	case ns_t_a6: {
		struct in6_addr a;
		int pbyte, pbit;

		
		if (rdlen == 0U) goto formerr;
		len = SPRINTF((tmp, "%d ", *rdata));
		T(addstr(tmp, (size_t)len, &buf, &buflen));
		pbit = *rdata;
		if (pbit > 128) goto formerr;
		pbyte = (pbit & ~7) / 8;
		rdata++;

		
		if (pbit < 128) {
			if (rdata + pbyte >= edata) goto formerr;
			memset(&a, 0, sizeof(a));
			memcpy(&a.s6_addr[pbyte], rdata, sizeof(a) - pbyte);
			(void) inet_ntop(AF_INET6, &a, buf, buflen);
			addlen(strlen(buf), &buf, &buflen);
			rdata += sizeof(a) - pbyte;
		}

		
		if (pbit == 0)
			break;
		if (rdata >= edata) goto formerr;
		T(addstr(" ", (size_t)1, &buf, &buflen));
		T(addname(msg, msglen, &rdata, origin, &buf, &buflen));

		break;
	    }

	case ns_t_opt: {
		len = SPRINTF((tmp, "%u bytes", class));
		T(addstr(tmp, (size_t)len, &buf, &buflen));
		break;
	    }

	default:
		comment = "unknown RR type";
		goto hexify;
	}
	return (buf - obuf);
 formerr:
	comment = "RR format error";
 hexify: {
	int n, m;
	char *p;

	len = SPRINTF((tmp, "\\# %tu%s\t; %s", edata - rdata,
		       rdlen != 0 ? " (" : "", comment));
	T(addstr(tmp, (size_t)len, &buf, &buflen));
	while (rdata < edata) {
		p = tmp;
		p += SPRINTF((p, "\n\t"));
		spaced = 0;
		n = MIN(16, edata - rdata);
		for (m = 0; m < n; m++)
			p += SPRINTF((p, "%02x ", rdata[m]));
		T(addstr(tmp, (size_t)(p - tmp), &buf, &buflen));
		if (n < 16) {
			T(addstr(")", (size_t)1, &buf, &buflen));
			T(addtab((size_t)(p - tmp + 1), (size_t)48, spaced, &buf, &buflen));
		}
		p = tmp;
		p += SPRINTF((p, "; "));
		for (m = 0; m < n; m++)
			*p++ = (isascii(rdata[m]) && isprint(rdata[m]))
				? rdata[m]
				: '.';
		T(addstr(tmp, (size_t)(p - tmp), &buf, &buflen));
		rdata += n;
	}
	return (buf - obuf);
    }
}













static size_t
prune_origin(const char *name, const char *origin) {
	const char *oname = name;

	while (*name != '\0') {
		if (origin != NULL && ns_samename(name, origin) == 1)
			return (name - oname - (name > oname));
		while (*name != '\0') {
			if (*name == '\\') {
				name++;
				
				if (*name == '\0')
					break;
			} else if (*name == '.') {
				name++;
				break;
			}
			name++;
		}
	}
	return (name - oname);
}












static int
charstr(const u_char *rdata, const u_char *edata, char **buf, size_t *buflen) {
	const u_char *odata = rdata;
	size_t save_buflen = *buflen;
	char *save_buf = *buf;

	if (addstr("\"", (size_t)1, buf, buflen) < 0)
		goto enospc;
	if (rdata < edata) {
		int n = *rdata;

		if (rdata + 1 + n <= edata) {
			rdata++;
			while (n-- > 0) {
				if (strchr("\n\"\\", *rdata) != NULL)
					if (addstr("\\", (size_t)1, buf, buflen) < 0)
						goto enospc;
				if (addstr((const char *)rdata, (size_t)1,
					   buf, buflen) < 0)
					goto enospc;
				rdata++;
			}
		}
	}
	if (addstr("\"", (size_t)1, buf, buflen) < 0)
		goto enospc;
	return (rdata - odata);
 enospc:
	errno = ENOSPC;
	*buf = save_buf;
	*buflen = save_buflen;
	return (-1);
}

static int
addname(const u_char *msg, size_t msglen,
	const u_char **pp, const char *origin,
	char **buf, size_t *buflen)
{
	size_t newlen, save_buflen = *buflen;
	char *save_buf = *buf;
	int n;

	n = dn_expand(msg, msg + msglen, *pp, *buf, (int)*buflen);
	if (n < 0)
		goto enospc;	
	newlen = prune_origin(*buf, origin);
	if (**buf == '\0') {
		goto root;
	} else if (newlen == 0U) {
		
		if (newlen + 2 > *buflen)
			goto enospc;        
		(*buf)[newlen++] = '@';
		(*buf)[newlen] = '\0';
	} else {
		if (((origin == NULL || origin[0] == '\0') ||
		    (origin[0] != '.' && origin[1] != '\0' &&
		    (*buf)[newlen] == '\0')) && (*buf)[newlen - 1] != '.') {
			
 root:
			if (newlen + 2 > *buflen)
				goto enospc;	
			(*buf)[newlen++] = '.';
			(*buf)[newlen] = '\0';
		}
	}
	*pp += n;
	addlen(newlen, buf, buflen);
	**buf = '\0';
	return (newlen);
 enospc:
	errno = ENOSPC;
	*buf = save_buf;
	*buflen = save_buflen;
	return (-1);
}

static void
addlen(size_t len, char **buf, size_t *buflen) {
	assert(len <= *buflen);
	*buf += len;
	*buflen -= len;
}

static int
addstr(const char *src, size_t len, char **buf, size_t *buflen) {
	if (len >= *buflen) {
		errno = ENOSPC;
		return (-1);
	}
	memcpy(*buf, src, len);
	addlen(len, buf, buflen);
	**buf = '\0';
	return (0);
}

static int
addtab(size_t len, size_t target, int spaced, char **buf, size_t *buflen) {
	size_t save_buflen = *buflen;
	char *save_buf = *buf;
	int t;

	if (spaced || len >= target - 1) {
		T(addstr("  ", (size_t)2, buf, buflen));
		spaced = 1;
	} else {
		for (t = (target - len - 1) / 8; t >= 0; t--)
			if (addstr("\t", (size_t)1, buf, buflen) < 0) {
				*buflen = save_buflen;
				*buf = save_buf;
				return (-1);
			}
		spaced = 0;
	}
	return (spaced);
}
