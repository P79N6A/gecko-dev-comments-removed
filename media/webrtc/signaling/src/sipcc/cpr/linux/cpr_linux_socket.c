



#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_assert.h"
#include "cpr_socket.h"
#include "cpr_debug.h"
#include "cpr_rand.h"
#include "cpr_timers.h"
#include "cpr_errno.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#if defined(WEBRTC_GONK)
#include <syslog.h>
#include <linux/fcntl.h>
#else
#include <sys/syslog.h>
#include <sys/fcntl.h>
#endif
#include <ctype.h>



const cpr_ip_addr_t ip_addr_invalid = {0};

#define IN6ADDRSZ   16
#define INT16SZ     2
#define	INADDRSZ	4

#define MAX_RETRY_FOR_EAGAIN 10


static int cpr_inet_pton4(const char *src, uint8_t *dst, int pton);
static int cpr_inet_pton6(const char *src, uint8_t *dst);
























cpr_status_e
cprBind (cpr_socket_t soc,
         CONST cpr_sockaddr_t * RESTRICT addr,
         cpr_socklen_t addr_len)
{
    cprAssert(addr != NULL, CPR_FAILURE);

    return ((bind(soc, (struct sockaddr *)addr, addr_len) != 0) ?
            CPR_FAILURE : CPR_SUCCESS);
}



















cpr_status_e
cprCloseSocket (cpr_socket_t soc)
{
    return ((close(soc) != 0) ? CPR_FAILURE : CPR_SUCCESS);
}




































cpr_status_e
cprConnect (cpr_socket_t soc,
            SUPPORT_CONNECT_CONST cpr_sockaddr_t * RESTRICT addr,
            cpr_socklen_t addr_len)
{
    int retry = 0, retval;

    cprAssert(addr != NULL, CPR_FAILURE);

    retval = connect(soc, (struct sockaddr *)addr, addr_len);

    while( retval == -1 && errno == EAGAIN && retry < MAX_RETRY_FOR_EAGAIN ) {
      cprSleep(100);
      retry++;
      retval = connect(soc, (struct sockaddr *)addr, addr_len);
    }

    return ((retval!=0) ? CPR_FAILURE : CPR_SUCCESS);
}





























cpr_status_e
cprGetSockName (cpr_socket_t soc,
                cpr_sockaddr_t * RESTRICT addr,
                cpr_socklen_t * RESTRICT addr_len)
{
    cprAssert(addr != NULL, CPR_FAILURE);
    cprAssert(addr_len != NULL, CPR_FAILURE);

    return ((getsockname(soc, (struct sockaddr *)addr, addr_len) != 0) ?
            CPR_FAILURE : CPR_SUCCESS);
}





























cpr_status_e
cprListen (cpr_socket_t soc,
           uint16_t backlog)
{
    return ((listen(soc, backlog) != 0) ? CPR_FAILURE : CPR_SUCCESS);
}













































ssize_t
cprRecv (cpr_socket_t soc,
         void * RESTRICT buf,
         size_t len,
         int32_t flags)
{
    ssize_t rc;
    int retry = 0;

    cprAssert(buf != NULL, CPR_FAILURE);

    rc = recv(soc, buf, len, flags);
    while( rc == -1 && errno == EAGAIN && retry < MAX_RETRY_FOR_EAGAIN ) {
      cprSleep(100);
      retry++;
      rc = recv(soc, buf, len, flags);
    }
    if (rc == -1) {
        return SOCKET_ERROR;
    }
    return rc;
}















































ssize_t
cprRecvFrom (cpr_socket_t soc,
             void * RESTRICT buf,
             size_t len,
             int32_t flags,
             cpr_sockaddr_t * RESTRICT from,
             cpr_socklen_t * RESTRICT fromlen)
{
    ssize_t rc;
    int retry = 0;

    cprAssert(buf != NULL, CPR_FAILURE);
    cprAssert(from != NULL, CPR_FAILURE);
    cprAssert(fromlen != NULL, CPR_FAILURE);

    rc = recvfrom(soc, buf, len, flags, (struct sockaddr *)from, fromlen);
    while( rc == -1 && errno == EAGAIN && retry < MAX_RETRY_FOR_EAGAIN ) {
      cprSleep(100);
      retry++;
      rc = recvfrom(soc, buf, len, flags, (struct sockaddr *)from, fromlen);
    }

    if (rc == -1) {
        CPR_INFO("error in recvfrom buf=%x fromlen=%d\n", buf, *fromlen);
        return SOCKET_ERROR;
    }
    return rc;
}






















































int16_t
cprSelect (uint32_t nfds,
           fd_set * RESTRICT read_fds,
           fd_set * RESTRICT write_fds,
           fd_set * RESTRICT except_fds,
           struct cpr_timeval * RESTRICT timeout)
{
    int16_t rc;
    struct timeval t, *t_p;

    if (timeout != NULL) {
        t.tv_sec  = timeout->tv_sec;
        t.tv_usec = timeout->tv_usec;
        t_p       = &t;
    } else {
        t_p       = NULL;
    }

    rc = (int16_t) select(nfds, read_fds, write_fds, except_fds, t_p);
    if (rc == -1) {
        return SOCKET_ERROR;
    }
    return rc;
}







































ssize_t
cprSend (cpr_socket_t soc,
         CONST void *buf,
         size_t len,
         int32_t flags)
{
    ssize_t rc;
    int retry = 0;

    cprAssert(buf != NULL, CPR_FAILURE);

    rc = send(soc, buf, len, flags);
    while( rc == -1 && errno == EAGAIN && retry < MAX_RETRY_FOR_EAGAIN ) {
      cprSleep(100);
      retry++;
      rc = send(soc, buf, len, flags);
    }

    if (rc == -1) {
        return SOCKET_ERROR;
    }
    return rc;
}











































ssize_t
cprSendTo (cpr_socket_t soc,
           CONST void *msg,
           size_t len,
           int32_t flags,
           CONST cpr_sockaddr_t *dest_addr,
           cpr_socklen_t dest_len)
{
    ssize_t rc;
    int retry = 0;

    cprAssert(msg != NULL, CPR_FAILURE);
    cprAssert(dest_addr != NULL, CPR_FAILURE);

    rc = sendto(soc, msg, len, flags, (struct sockaddr *)dest_addr, dest_len);
    while( rc == -1 && errno == EAGAIN && retry < MAX_RETRY_FOR_EAGAIN ) {
      cprSleep(100);
      retry++;
      rc = sendto(soc, msg, len, flags, (struct sockaddr *)dest_addr, dest_len);
    }

    if (rc == -1) {
        return SOCKET_ERROR;
    }
    return rc;
}








































cpr_status_e
cprSetSockOpt (cpr_socket_t soc,
               uint32_t level,
               uint32_t opt_name,
               CONST void *opt_val,
               cpr_socklen_t opt_len)
{
    cprAssert(opt_val != NULL, CPR_FAILURE);

    return ((setsockopt(soc, (int)level, (int)opt_name, opt_val, opt_len) != 0)
            ? CPR_FAILURE : CPR_SUCCESS);
}

















cpr_status_e
cprSetSockNonBlock (cpr_socket_t soc)
{

    return ((fcntl(soc, F_SETFL, O_NONBLOCK) != 0) ? CPR_FAILURE : CPR_SUCCESS);
}












































cpr_socket_t
cprSocket (uint32_t domain,
           uint32_t type,
           uint32_t protocol)
{
    cpr_socket_t s;

    s = socket((int)domain, (int)type, (int)protocol);
    if (s == -1) {
        return INVALID_SOCKET;
    }
    return s;
}


















int
cpr_inet_pton (int af, const char *src, void *dst)
{

	switch (af) {
	case AF_INET:
		return (cpr_inet_pton4(src, dst, 1));
	case AF_INET6:
		return (cpr_inet_pton6(src, dst));
	default:
		return (-1);
	}
	
}












void cpr_set_sockun_addr (cpr_sockaddr_un_t *addr, const char *name, pid_t pid)
{
    
    memset(addr, 0, sizeof(cpr_sockaddr_un_t));
    addr->sun_family = AF_LOCAL;
    snprintf(addr->sun_path, sizeof(addr->sun_path), name, pid);
}










static int
cpr_inet_pton4(const char *src, uint8_t *dst, int pton)
{
	uint32_t val;
	uint32_t digit;
	int base, n;
	unsigned char c;
	uint32_t parts[4];
	uint32_t *pp = parts;

	c = *src;
	for (;;) {
		




		if (!isdigit(c))
			return (0);
		val = 0; base = 10;
		if (c == '0') {
			c = *++src;
			if (c == 'x' || c == 'X')
				base = 16, c = *++src;
			else if (isdigit(c) && c != '9')
				base = 8;
		}
		
		if (pton && base != 10)
			return (0);
		for (;;) {
			if (isdigit(c)) {
				digit = c - '0';
				if (digit >= (uint16_t)base)
					break;
				val = (val * base) + digit;
				c = *++src;
			} else if (base == 16 && isxdigit(c)) {
				digit = c + 10 - (islower(c) ? 'a' : 'A');
				if (digit >= 16)
					break;
				val = (val << 4) | digit;
				c = *++src;
			} else
				break;
		}
		if (c == '.') {
			






			if (pp >= parts + 3)
				return (0);
			*pp++ = val;
			c = *++src;
		} else
			break;
	}
	


	if (c != '\0' && !isspace(c))
		return (0);
	



	n = pp - parts + 1;
	
	if (pton && n != 4)
		return (0);
	switch (n) {

	case 0:
		return (0);		

	case 1:				
		break;

	case 2:				
		if (parts[0] > 0xff || val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:				
		if ((parts[0] | parts[1]) > 0xff || val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				
		if ((parts[0] | parts[1] | parts[2] | val) > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (dst) {
		val = htonl(val);
		memcpy(dst, &val, INADDRSZ);
	}
	return (1);
}










static int
cpr_inet_pton6(const char *src, uint8_t *dst)
{
	static const char xdigits_l[] = "0123456789abcdef",
			  xdigits_u[] = "0123456789ABCDEF";
	uint8_t tmp[IN6ADDRSZ], *tp, *endp, *colonp;
	const char *xdigits, *curtok;
	int ch, saw_xdigit;
	unsigned int val;

	memset((tp = tmp), '\0', IN6ADDRSZ);
	endp = tp + IN6ADDRSZ;
	colonp = NULL;
	
	if (*src == ':')
		if (*++src != ':')
			return (0);
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
			pch = strchr((xdigits = xdigits_u), ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (val > 0xffff)
				return (0);
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					return (0);
				colonp = tp;
				continue;
			} else if (*src == '\0')
				return (0);
			if (tp + INT16SZ > endp)
				return (0);
			*tp++ = (uint8_t) (val >> 8) & 0xff;
			*tp++ = (uint8_t) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && ((tp + INADDRSZ) <= endp) &&
		    cpr_inet_pton4(curtok, tp, 1) > 0) {
			tp += INADDRSZ;
			saw_xdigit = 0;
			break;	
		}
		return (0);
	}
	if (saw_xdigit) {
		if (tp + INT16SZ > endp)
			return (0);
		*tp++ = (uint8_t) (val >> 8) & 0xff;
		*tp++ = (uint8_t) val & 0xff;
	}
	if (colonp != NULL) {
		



		const int n = tp - colonp;
		int i;

		if (tp == endp)
			return (0);
		for (i = 1; i <= n; i++) {
			endp[- i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		return (0);
	memcpy(dst, tmp, IN6ADDRSZ);
	return (1);
}

