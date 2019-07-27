



#include <errno.h>
#include <limits.h>

#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "cpr_socket.h"
#include "cpr_in.h"
#include "util_string.h"












void
ipaddr2dotted (char *addr_str, cpr_ip_addr_t *addr)
{
    if (addr_str)
    {
        switch (addr->type) {
        case CPR_IP_ADDR_IPV4 :
        sprintf(addr_str, "%u.%u.%u.%u",
                (addr->u.ip4 >> 24) & 0xFF,
                (addr->u.ip4 >> 16) & 0xFF,
                (addr->u.ip4 >> 8)  & 0xFF,
                (addr->u.ip4)       & 0xFF);
            break;
        case CPR_IP_ADDR_IPV6:
        sprintf(addr_str, "[%x:%x:%x:%x:%x:%x:%x:%x]",
                (addr->u.ip6.addr.base16[7]),
                (addr->u.ip6.addr.base16[6]),
                (addr->u.ip6.addr.base16[5]),
                (addr->u.ip6.addr.base16[4]),
                (addr->u.ip6.addr.base16[3]),
                (addr->u.ip6.addr.base16[2]),
                (addr->u.ip6.addr.base16[1]),
                (addr->u.ip6.addr.base16[0]));

            break;
        default:
            break;
        }


    }
}












uint32_t
dotted2ipaddr (const char *addr_str)
{
    uint32_t    address = 0;
    const char *p = NULL;
    char        section_str[3];
    int         sections[4];
    int         section;
    int         i;
    long        strtoul_result;
    char       *strtoul_end;

    
    if ((!addr_str) || (addr_str[0] == '\0'))
    {
        return 0xFFFFFFFF;
    }

    
    for (i = 0; i < 4; i++)
    {
        sections[i] = 0;
    }
    p = addr_str;

    
    while ((*p == ' ') || (*p == '\t') || (*p == '\n') || (*p == '\r'))
    {
        p++;
    }

    for (section = 0; section < 4; section++)
    {
        i = 0;
        section_str[0] = '\0';
        section_str[1] = '\0';
        section_str[2] = '\0';
        while ((*p != '.') && (i < 3)) {
            section_str[i] = *p;
            i++;
            p++;
        }

        errno = 0;
        strtoul_result = strtoul(section_str, &strtoul_end, 10);

        if (errno || section_str == strtoul_end ||
            strtoul_result > 255) {
            return 0xFFFFFFFF;
        }

        sections[section] = (int) strtoul_result;

        address = address | (sections[section]<<((3-section)*8));
        p++;
    }

    return address;
}












void util_ntohl (cpr_ip_addr_t *ip_addr_out, cpr_ip_addr_t *ip_addr_in)
{
    int i,j;
    unsigned char tmp;

    ip_addr_out->type = ip_addr_in->type;

    if (ip_addr_in->type == CPR_IP_ADDR_IPV4) {

        ip_addr_out->u.ip4 = ntohl(ip_addr_in->u.ip4);

    } else {
        

        if (ip_addr_out == ip_addr_in) {
            for (i=0, j=15; i<8; i++, j--) {
                tmp  = ip_addr_out->u.ip6.addr.base8[j];
                ip_addr_out->u.ip6.addr.base8[j] = ip_addr_in->u.ip6.addr.base8[i];
                ip_addr_in->u.ip6.addr.base8[i] = tmp;
            }
        } else {
            for (i=0, j=15; i<16; i++, j--) {
                ip_addr_out->u.ip6.addr.base8[j] = ip_addr_in->u.ip6.addr.base8[i];
            }
        }
    }
}















boolean util_check_if_ip_valid (cpr_ip_addr_t *ip_addr)
{
    if (ip_addr->type == CPR_IP_ADDR_INVALID) {
        return(FALSE);
    }
    if (ip_addr->type == CPR_IP_ADDR_IPV4 &&
        ip_addr->u.ip4 == 0) {

        return(FALSE);
    }

    if ((ip_addr->type == CPR_IP_ADDR_IPV6) &&
        (ip_addr->u.ip6.addr.base16[7] == 0) &&
        (ip_addr->u.ip6.addr.base16[6] == 0) &&
        (ip_addr->u.ip6.addr.base16[5] == 0) &&
        (ip_addr->u.ip6.addr.base16[4] == 0) &&
        (ip_addr->u.ip6.addr.base16[3] == 0) &&
        (ip_addr->u.ip6.addr.base16[2] == 0) &&
        (ip_addr->u.ip6.addr.base16[1] == 0) &&
        (ip_addr->u.ip6.addr.base16[0] == 0)) {

        return(FALSE);
    }

    if ((ip_addr->type != CPR_IP_ADDR_INVALID) &&
        (ip_addr->type != CPR_IP_ADDR_IPV4) &&
        (ip_addr->type != CPR_IP_ADDR_IPV6)) {

        return(FALSE);
    }

    return(TRUE);
}












boolean util_compare_ip (cpr_ip_addr_t *ip_addr1, cpr_ip_addr_t *ip_addr2)
{
    if (ip_addr1->type != ip_addr2->type) {

        return(FALSE);
    }

    if (ip_addr1->type == CPR_IP_ADDR_IPV4 &&
        ip_addr2->type == CPR_IP_ADDR_IPV4) {

        return((boolean) (ip_addr1->u.ip4 == ip_addr2->u.ip4));

    } else if (ip_addr1->type == CPR_IP_ADDR_IPV6 &&
            ip_addr2->type == CPR_IP_ADDR_IPV6) {

        return((boolean)memcmp((void *)&(ip_addr1->u.ip6.addr.base8),
                    (void *)&(ip_addr2->u.ip6.addr.base8), 16));
    }

    return(FALSE);
}












void util_extract_ip (cpr_ip_addr_t *ip_addr,
                        cpr_sockaddr_storage *from)
{
    switch (from->ss_family) {
    case AF_INET6:
        ip_addr->type = CPR_IP_ADDR_IPV6;
        ip_addr->u.ip6 = ((cpr_sockaddr_in6_t *)from)->sin6_addr;
        break;
    case AF_INET:
        ip_addr->type = CPR_IP_ADDR_IPV4;
        ip_addr->u.ip4 = ((cpr_sockaddr_in_t *)from)->sin_addr.s_addr;
        break;
    default:
        break;
    }
}












uint16_t util_get_port (cpr_sockaddr_storage *sock_storage)
{
    switch (sock_storage->ss_family) {
    case AF_INET6:
        return(((cpr_sockaddr_in6_t *)sock_storage)->sin6_port);
    case AF_INET:
        return(((cpr_sockaddr_in_t *)sock_storage)->sin_port);
    default:
        break;
    }
    return(0);
}

void util_get_ip_using_mode (cpr_ip_addr_t *ip_addr, cpr_ip_mode_e ip_mode,
                             uint32_t  ip4, char *ip6)
{
    *ip_addr = ip_addr_invalid;

    switch (ip_mode) {
    case CPR_IP_MODE_IPV4:
        ip_addr->type = CPR_IP_ADDR_IPV4;
        ip_addr->u.ip4 = ip4;
        break;
    case CPR_IP_MODE_IPV6:
        ip_addr->type = CPR_IP_ADDR_IPV6;
        if (ip6 != NULL) {
            memcpy((void *)&(ip_addr->u.ip6.addr.base8[16]), (void *)ip6, 16);
        }
        break;
    case CPR_IP_MODE_DUAL:
    default:
        break;
    }

}

unsigned long
gmt_string_to_seconds (char *gmt_string, unsigned long *seconds)
{
    char *token;

    
    
    
    
    
    *seconds = strtoul(gmt_string, &token, 10);
    if ((token == NULL) || (token[0] == '\0')) {
        return 1;
    }
    *seconds = 0;
    return 0;
}

long
diff_current_time (unsigned long t1, unsigned long *difference)
{
    *difference = 1;
    return 0;
}

boolean
is_empty_str (char *str)
{
    if (str == NULL) {
        return TRUE;
    }
    if (strncmp(str, EMPTY_STR, EMPTY_STR_LEN) == 0) {
        return TRUE;
    }
    return FALSE;
}

void
init_empty_str (char *str)
{
    strcpy(str,EMPTY_STR);
}
