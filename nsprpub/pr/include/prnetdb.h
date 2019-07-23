




































#ifndef prnetdb_h___
#define prnetdb_h___

#include "prtypes.h"
#include "prio.h"

PR_BEGIN_EXTERN_C







NSPR_API(PRStatus) PR_StringToNetAddr(
    const char *string, PRNetAddr *addr);

NSPR_API(PRStatus) PR_NetAddrToString(
    const PRNetAddr *addr, char *string, PRUint32 size);











typedef struct PRHostEnt {
    char *h_name;       
    char **h_aliases;   
#ifdef WIN32
    PRInt16 h_addrtype; 
    PRInt16 h_length;   
#else
    PRInt32 h_addrtype; 
    PRInt32 h_length;   
#endif
    char **h_addr_list; 
} PRHostEnt;


#if (defined(AIX) && defined(_THREAD_SAFE)) || defined(OSF1)
#define PR_NETDB_BUF_SIZE sizeof(struct protoent_data)
#else
#define PR_NETDB_BUF_SIZE 1024
#endif






















NSPR_API(PRStatus) PR_GetHostByName(
    const char *hostname, char *buf, PRIntn bufsize, PRHostEnt *hostentry);





























#define PR_AI_ALL         0x08
#define PR_AI_V4MAPPED    0x10
#define PR_AI_ADDRCONFIG  0x20
#define PR_AI_NOCANONNAME 0x8000
#define PR_AI_DEFAULT     (PR_AI_V4MAPPED | PR_AI_ADDRCONFIG)

NSPR_API(PRStatus) PR_GetIPNodeByName(
    const char *hostname,
    PRUint16 af,
    PRIntn flags,
    char *buf,
    PRIntn bufsize,
    PRHostEnt *hostentry);






















NSPR_API(PRStatus) PR_GetHostByAddr(
    const PRNetAddr *hostaddr, char *buf, PRIntn bufsize, PRHostEnt *hostentry);
































NSPR_API(PRIntn) PR_EnumerateHostEnt(
    PRIntn enumIndex, const PRHostEnt *hostEnt, PRUint16 port, PRNetAddr *address);























typedef enum PRNetAddrValue
{
    PR_IpAddrNull,      
    PR_IpAddrAny,       
    PR_IpAddrLoopback,  
    PR_IpAddrV4Mapped   
} PRNetAddrValue;

NSPR_API(PRStatus) PR_InitializeNetAddr(
    PRNetAddrValue val, PRUint16 port, PRNetAddr *addr);


























NSPR_API(PRStatus) PR_SetNetAddr(
    PRNetAddrValue val, PRUint16 af, PRUint16 port, PRNetAddr *addr);














NSPR_API(PRBool) PR_IsNetAddrType(const PRNetAddr *addr, PRNetAddrValue val);
















NSPR_API(void) PR_ConvertIPv4AddrToIPv6(PRUint32 v4addr, PRIPv6Addr *v6addr);












#define PR_NetAddrFamily(addr) ((addr)->raw.family)












#define PR_NetAddrInetPort(addr) \
    ((addr)->raw.family == PR_AF_INET6 ? (addr)->ipv6.port : (addr)->inet.port)























typedef struct PRProtoEnt {
    char *p_name;       
    char **p_aliases;   
#ifdef WIN32
    PRInt16 p_num;      
#else
    PRInt32 p_num;      
#endif
} PRProtoEnt;

NSPR_API(PRStatus) PR_GetProtoByName(
    const char* protocolname, char* buffer, PRInt32 bufsize, PRProtoEnt* result);























NSPR_API(PRStatus) PR_GetProtoByNumber(
    PRInt32 protocolnumber, char* buffer, PRInt32 bufsize, PRProtoEnt* result);






















typedef struct PRAddrInfo PRAddrInfo;

NSPR_API(PRAddrInfo*) PR_GetAddrInfoByName(
    const char *hostname, PRUint16 af, PRIntn flags);













NSPR_API(void) PR_FreeAddrInfo(PRAddrInfo *addrInfo);
























NSPR_API(void *) PR_EnumerateAddrInfo(
    void *enumPtr, const PRAddrInfo *addrInfo, PRUint16 port, PRNetAddr *result);

















NSPR_API(const char *) PR_GetCanonNameFromAddrInfo(
    const PRAddrInfo *addrInfo);














NSPR_API(PRUint16) PR_ntohs(PRUint16);
NSPR_API(PRUint32) PR_ntohl(PRUint32);
NSPR_API(PRUint64) PR_ntohll(PRUint64);
NSPR_API(PRUint16) PR_htons(PRUint16);
NSPR_API(PRUint32) PR_htonl(PRUint32);
NSPR_API(PRUint64) PR_htonll(PRUint64);

PR_END_EXTERN_C

#endif 
