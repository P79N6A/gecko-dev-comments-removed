



#ifndef _CPR_TYPES_H_
#define _CPR_TYPES_H_

#if defined SIP_OS_LINUX
#include "cpr_linux_types.h"
#elif defined SIP_OS_WINDOWS
#include "cpr_win_types.h"
#elif defined SIP_OS_OSX
#include "cpr_darwin_types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif




typedef enum
{
    CPR_SUCCESS,
    CPR_FAILURE
} cpr_status_e;
typedef cpr_status_e cprRC_t;




typedef uint32_t cpr_in_addr_t;

struct in_addr_s
{
#ifdef s_addr
    
    union {
        struct {
            unsigned char s_b1, s_b2, sb_b3, s_b4;
        } S_un_b;
        cpr_in_addr_t S_addr;
    } S_un;
#else
    cpr_in_addr_t s_addr;
#endif
};




typedef struct
{
    union
    {
        uint8_t  base8[16];
        uint16_t base16[8];
        uint32_t base32[4];
    } addr;
} cpr_in6_addr_t;

#ifndef s6_addr
#define s6_addr   addr.base8
#endif
#ifndef s6_addr16
#define s6_addr16 addr.base16
#endif
#ifndef s6_addr32
#define s6_addr32 addr.base32
#endif

typedef enum
{
     CPR_IP_ADDR_INVALID=0,
     CPR_IP_ADDR_IPV4,
     CPR_IP_ADDR_IPV6
} cpr_ip_type;

typedef enum
{
     CPR_IP_MODE_IPV4 = 0,
     CPR_IP_MODE_IPV6,
     CPR_IP_MODE_DUAL
}
cpr_ip_mode_e;



typedef struct
{
    cpr_ip_type type;
    union
    {
        cpr_in_addr_t  ip4;
        cpr_in6_addr_t ip6;
    } u;
} cpr_ip_addr_t;

extern const cpr_ip_addr_t ip_addr_invalid;

#define MAX_IPADDR_STR_LEN 48


#define CPR_IP_ADDR_INIT(a) a.type = CPR_IP_ADDR_INVALID;











typedef const char *string_t;

#ifdef __cplusplus
}
#endif

#endif
