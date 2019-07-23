


















































#if defined(WINNT) || defined(__MINGW32__)
#include <winsock.h>
#endif


#ifdef __MINGW32__
#ifndef IP_TTL
#define IP_TTL 7
#endif
#ifndef IP_TOS
#define IP_TOS 8
#endif
#endif

#include "primpl.h"

#if defined(NEXTSTEP)

#include <netinet/in_systm.h>  
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>  
#endif

#ifndef _PR_PTHREADS

PRStatus PR_CALLBACK _PR_SocketGetSocketOption(PRFileDesc *fd, PRSocketOptionData *data)
{
    PRStatus rv;
    PRInt32 length;
    PRInt32 level, name;

    



    if (PR_SockOpt_Nonblocking == data->option)
    {
        data->value.non_blocking = fd->secret->nonblocking;
        return PR_SUCCESS;
    }

    rv = _PR_MapOptionName(data->option, &level, &name);
    if (PR_SUCCESS == rv)
    {
        switch (data->option)
        {
            case PR_SockOpt_Linger:
            {
#if !defined(XP_BEOS) || defined(BONE_VERSION)
                struct linger linger;
                length = sizeof(linger);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name, (char *) &linger, &length);
                if (PR_SUCCESS == rv)
                {
                    PR_ASSERT(sizeof(linger) == length);
                    data->value.linger.polarity =
                        (linger.l_onoff) ? PR_TRUE : PR_FALSE;
                    data->value.linger.linger =
                        PR_SecondsToInterval(linger.l_linger);
                }
                break;
#else
                PR_SetError( PR_NOT_IMPLEMENTED_ERROR, 0 );
                return PR_FAILURE;
#endif
            }
            case PR_SockOpt_Reuseaddr:
            case PR_SockOpt_Keepalive:
            case PR_SockOpt_NoDelay:
            case PR_SockOpt_Broadcast:
            {
#ifdef WIN32 
                BOOL value;
#else
                PRIntn value;
#endif
                length = sizeof(value);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name, (char*)&value, &length);
                if (PR_SUCCESS == rv)
                    data->value.reuse_addr = (0 == value) ? PR_FALSE : PR_TRUE;
                break;
            }
            case PR_SockOpt_McastLoopback:
            {
#ifdef WIN32 
                BOOL bool;
#else
                PRUint8 bool;
#endif
                length = sizeof(bool);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name, (char*)&bool, &length);
                if (PR_SUCCESS == rv)
                    data->value.mcast_loopback = (0 == bool) ? PR_FALSE : PR_TRUE;
                break;
            }
            case PR_SockOpt_RecvBufferSize:
            case PR_SockOpt_SendBufferSize:
            case PR_SockOpt_MaxSegment:
            {
                PRIntn value;
                length = sizeof(value);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name, (char*)&value, &length);
                if (PR_SUCCESS == rv)
                    data->value.recv_buffer_size = value;
                break;
            }
            case PR_SockOpt_IpTimeToLive:
            case PR_SockOpt_IpTypeOfService:
            {
                
                length = sizeof(PRUintn);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name, (char*)&data->value.ip_ttl, &length);
                break;
            }
            case PR_SockOpt_McastTimeToLive:
            {
#ifdef WIN32 
                int ttl;
#else
                PRUint8 ttl;
#endif
                length = sizeof(ttl);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name, (char*)&ttl, &length);
                if (PR_SUCCESS == rv)
                    data->value.mcast_ttl = ttl;
                break;
            }
#ifdef IP_ADD_MEMBERSHIP
            case PR_SockOpt_AddMember:
            case PR_SockOpt_DropMember:
            {
                struct ip_mreq mreq;
                length = sizeof(mreq);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name, (char*)&mreq, &length);
                if (PR_SUCCESS == rv)
                {
                    data->value.add_member.mcaddr.inet.ip =
                        mreq.imr_multiaddr.s_addr;
                    data->value.add_member.ifaddr.inet.ip =
                        mreq.imr_interface.s_addr;
                }
                break;
            }
#endif 
            case PR_SockOpt_McastInterface:
            {
                
                length = sizeof(data->value.mcast_if.inet.ip);
                rv = _PR_MD_GETSOCKOPT(
                    fd, level, name,
                    (char*)&data->value.mcast_if.inet.ip, &length);
                break;
            }
            default:
                PR_NOT_REACHED("Unknown socket option");
                break;
        }  
    }
    return rv;
}  

PRStatus PR_CALLBACK _PR_SocketSetSocketOption(PRFileDesc *fd, const PRSocketOptionData *data)
{
    PRStatus rv;
    PRInt32 level, name;

    



    if (PR_SockOpt_Nonblocking == data->option)
    {
#ifdef WINNT
        PR_ASSERT((fd->secret->md.io_model_committed == PR_FALSE)
            || (fd->secret->nonblocking == data->value.non_blocking));
        if (fd->secret->md.io_model_committed
            && (fd->secret->nonblocking != data->value.non_blocking))
        {
            





            PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
            return PR_FAILURE;
        }
#endif
        fd->secret->nonblocking = data->value.non_blocking;
        return PR_SUCCESS;
    }

    rv = _PR_MapOptionName(data->option, &level, &name);
    if (PR_SUCCESS == rv)
    {
        switch (data->option)
        {
            case PR_SockOpt_Linger:
            {
#if !defined(XP_BEOS) || defined(BONE_VERSION)
                struct linger linger;
                linger.l_onoff = data->value.linger.polarity;
                linger.l_linger = PR_IntervalToSeconds(data->value.linger.linger);
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&linger, sizeof(linger));
                break;
#else
                PR_SetError( PR_NOT_IMPLEMENTED_ERROR, 0 );
                return PR_FAILURE;
#endif
            }
            case PR_SockOpt_Reuseaddr:
            case PR_SockOpt_Keepalive:
            case PR_SockOpt_NoDelay:
            case PR_SockOpt_Broadcast:
            {
#ifdef WIN32 
                BOOL value;
#else
                PRIntn value;
#endif
                value = (data->value.reuse_addr) ? 1 : 0;
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&value, sizeof(value));
                break;
            }
            case PR_SockOpt_McastLoopback:
            {
#ifdef WIN32 
                BOOL bool;
#else
                PRUint8 bool;
#endif
                bool = data->value.mcast_loopback ? 1 : 0;
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&bool, sizeof(bool));
                break;
            }
            case PR_SockOpt_RecvBufferSize:
            case PR_SockOpt_SendBufferSize:
            case PR_SockOpt_MaxSegment:
            {
                PRIntn value = data->value.recv_buffer_size;
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&value, sizeof(value));
                break;
            }
            case PR_SockOpt_IpTimeToLive:
            case PR_SockOpt_IpTypeOfService:
            {
                
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&data->value.ip_ttl, sizeof(PRUintn));
                break;
            }
            case PR_SockOpt_McastTimeToLive:
            {
#ifdef WIN32 
                int ttl;
#else
                PRUint8 ttl;
#endif
                ttl = data->value.mcast_ttl;
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&ttl, sizeof(ttl));
                break;
            }
#ifdef IP_ADD_MEMBERSHIP
            case PR_SockOpt_AddMember:
            case PR_SockOpt_DropMember:
            {
                struct ip_mreq mreq;
                mreq.imr_multiaddr.s_addr =
                    data->value.add_member.mcaddr.inet.ip;
                mreq.imr_interface.s_addr =
                    data->value.add_member.ifaddr.inet.ip;
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&mreq, sizeof(mreq));
                break;
            }
#endif 
            case PR_SockOpt_McastInterface:
            {
                
                rv = _PR_MD_SETSOCKOPT(
                    fd, level, name, (char*)&data->value.mcast_if.inet.ip,
                    sizeof(data->value.mcast_if.inet.ip));
                break;
            }
            default:
                PR_NOT_REACHED("Unknown socket option");
                break;
        }  
    }
    return rv;
}  

#endif 































#if !defined(SO_LINGER)
#error "SO_LINGER is not defined"
#endif





#if !defined(NCR)
#if !defined(TCP_NODELAY)
#error "TCP_NODELAY is not defined"
#endif
#endif





#define _PR_NO_SUCH_SOCKOPT -1

#ifndef SO_KEEPALIVE
#define SO_KEEPALIVE        _PR_NO_SUCH_SOCKOPT
#endif

#ifndef SO_SNDBUF
#define SO_SNDBUF           _PR_NO_SUCH_SOCKOPT
#endif

#ifndef SO_RCVBUF
#define SO_RCVBUF           _PR_NO_SUCH_SOCKOPT
#endif

#ifndef IP_MULTICAST_IF                 
#define IP_MULTICAST_IF     _PR_NO_SUCH_SOCKOPT
#endif

#ifndef IP_MULTICAST_TTL                
#define IP_MULTICAST_TTL    _PR_NO_SUCH_SOCKOPT
#endif

#ifndef IP_MULTICAST_LOOP               
#define IP_MULTICAST_LOOP   _PR_NO_SUCH_SOCKOPT
#endif

#ifndef IP_ADD_MEMBERSHIP               
#define IP_ADD_MEMBERSHIP   _PR_NO_SUCH_SOCKOPT
#endif

#ifndef IP_DROP_MEMBERSHIP              
#define IP_DROP_MEMBERSHIP  _PR_NO_SUCH_SOCKOPT
#endif

#ifndef IP_TTL                          
#define IP_TTL              _PR_NO_SUCH_SOCKOPT
#endif

#ifndef IP_TOS                          
#define IP_TOS              _PR_NO_SUCH_SOCKOPT
#endif

#ifndef TCP_NODELAY                     
#define TCP_NODELAY         _PR_NO_SUCH_SOCKOPT
#endif

#ifndef TCP_MAXSEG                      
#define TCP_MAXSEG          _PR_NO_SUCH_SOCKOPT
#endif

#ifndef SO_BROADCAST                 
#define SO_BROADCAST        _PR_NO_SUCH_SOCKOPT
#endif

PRStatus _PR_MapOptionName(
    PRSockOption optname, PRInt32 *level, PRInt32 *name)
{
    static PRInt32 socketOptions[PR_SockOpt_Last] =
    {
        0, SO_LINGER, SO_REUSEADDR, SO_KEEPALIVE, SO_RCVBUF, SO_SNDBUF,
        IP_TTL, IP_TOS, IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP,
        IP_MULTICAST_IF, IP_MULTICAST_TTL, IP_MULTICAST_LOOP,
        TCP_NODELAY, TCP_MAXSEG, SO_BROADCAST
    };
    static PRInt32 socketLevels[PR_SockOpt_Last] =
    {
        0, SOL_SOCKET, SOL_SOCKET, SOL_SOCKET, SOL_SOCKET, SOL_SOCKET,
        IPPROTO_IP, IPPROTO_IP, IPPROTO_IP, IPPROTO_IP,
        IPPROTO_IP, IPPROTO_IP, IPPROTO_IP,
        IPPROTO_TCP, IPPROTO_TCP, SOL_SOCKET
    };

    if ((optname < PR_SockOpt_Linger)
    || (optname >= PR_SockOpt_Last))
    {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }

    if (socketOptions[optname] == _PR_NO_SUCH_SOCKOPT)
    {
        PR_SetError(PR_OPERATION_NOT_SUPPORTED_ERROR, 0);
        return PR_FAILURE;
    }
    *name = socketOptions[optname];
    *level = socketLevels[optname];
    return PR_SUCCESS;
}  
