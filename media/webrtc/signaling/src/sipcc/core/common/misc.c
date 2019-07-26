



#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "cpr.h"
#include "phone_debug.h"
#include "cc_debug.h"
#include "phone.h"
#include "cpr_socket.h"
#include "prot_configmgr.h"
#include "debug.h"
#include "cpr_string.h"
#include "cpr_stdlib.h"







#define MAX_NTP_MONTH_STR_LEN     4
#define MAX_NTP_MONTH_ARRAY_SIZE  12
#define MAX_NTP_DATE_HDR_STR_LEN  128
#define MAX_NTP_TOKEN_BUF_LEN     16


#define NUM_OF_SHOW_ARGUMENTS 2

static int last_month = 99;
static char last_month_str[MAX_NTP_MONTH_STR_LEN] = "";
static const char *month_ar[MAX_NTP_MONTH_ARRAY_SIZE] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};






extern void platform_set_time(int32_t gmt_time);
void SipNtpUpdateClockFromCCM(void);













static boolean
set_month_from_str (char *month_str)
{
    boolean ret_val = FALSE;
    const char * fname = "set_month_from_str";
    int i;

    if (month_str) {
        if (strncmp(month_str, last_month_str, 3) != 0) {
            for (i = 0; i < 12; i++) {
                if (strncmp(month_str, month_ar[i], 3) == 0) {
                    sstrncpy(last_month_str, month_str, sizeof(last_month_str));
                    last_month = i;
                    ret_val = TRUE;
                    break;
                }
            }
        } else {
            ret_val = TRUE;
        }
    } else {
        TNP_DEBUG(DEB_F_PREFIX "Input month_str is NULL!!!! \n", DEB_F_PREFIX_ARGS(PLAT_API, fname));
    }
    return (ret_val);
}









static uint16_t PHNState = STATE_CONNECTED;


uint16_t
PHNGetState (void)
{
    return (PHNState);
}

void
PHNChangeState (uint16_t state)
{
    PHNState = state;
}


void
phone_reset (DeviceResetType resetType)
{
    return;
}






extern void config_get_value (int id, void *buffer, int length);












void
log_clear (int msg)
{
}












boolean plat_is_network_interface_changed (void)
{
    return(FALSE);
}











void
platform_get_ipv6_address (cpr_ip_addr_t *ip_addr)
{
    
    
    
    
    ip_addr->type = CPR_IP_ADDR_IPV6;
    ip_addr->u.ip6.addr.base8[15] = 0x65;
    ip_addr->u.ip6.addr.base8[14] = 0xfb;
    ip_addr->u.ip6.addr.base8[13] = 0xb1;
    ip_addr->u.ip6.addr.base8[12] = 0xfe;
    ip_addr->u.ip6.addr.base8[11] = 0xff;
    ip_addr->u.ip6.addr.base8[10] = 0x11;
    ip_addr->u.ip6.addr.base8[9] = 0x11;
    ip_addr->u.ip6.addr.base8[8] = 0x02;
    ip_addr->u.ip6.addr.base8[7] = 0x01;
    ip_addr->u.ip6.addr.base8[6] = 0x00;
    ip_addr->u.ip6.addr.base8[5] = 0x18;
    ip_addr->u.ip6.addr.base8[4] = 0x0c;
    ip_addr->u.ip6.addr.base8[3] = 0xb8;
    ip_addr->u.ip6.addr.base8[2] = 0x0d;
    ip_addr->u.ip6.addr.base8[1] = 0x01;
    ip_addr->u.ip6.addr.base8[0] = 0x20;

    return;
}








void
platform_get_wired_mac_address (unsigned char *addr)
{
    config_get_value(CFGID_MY_MAC_ADDR, addr, 6);
    TNP_DEBUG(DEB_F_PREFIX"Wired MacAddr:from Get Val: %04x:%04x:%04x",
            DEB_F_PREFIX_ARGS(PLAT_API, "platform_get_wired_mac_address"),
              addr[0] * 256 + addr[1], addr[2] * 256 + addr[3],
              addr[4] * 256 + addr[5]);
}








void
platform_get_active_mac_address (unsigned char *addr)
{
    config_get_value(CFGID_MY_ACTIVE_MAC_ADDR, addr, 6);
    TNP_DEBUG(DEB_F_PREFIX"ActiveMacAddr:from Get Val: %04x:%04x:%04x",
            DEB_F_PREFIX_ARGS(PLAT_API, "platform_get_mac_address"),
              addr[0] * 256 + addr[1], addr[2] * 256 + addr[3],
              addr[4] * 256 + addr[5]);
}










void
platform_get_ipv4_address (cpr_ip_addr_t *ip_addr)
{
    config_get_value(CFGID_MY_IP_ADDR, ip_addr, sizeof(cpr_ip_addr_t));
    ip_addr->type = CPR_IP_ADDR_IPV4;

    return;
}

uint32_t
IPNameCk (char *name, char *addr_error)
{
    char *namePtr = name;
    char string[4] = { 0, 0, 0, 0 };
    int x = 0;
    int i = 0;
    uint32_t temp, ip_addr = 0;
    char ip_addr_out[MAX_IPADDR_STR_LEN];
    unsigned long strtoul_result;
    char *strtoul_end;

    
    if (cpr_inet_pton(AF_INET6, name, ip_addr_out)) {
        *addr_error = FALSE;
        return TRUE;
    }
    *addr_error = TRUE;
    while (*namePtr != 0) {
        if ((*namePtr >= 0x30) && (*namePtr <= 0x39)) {
            if (x > 2)
                return (0);
            string[x++] = *namePtr++;
        } else {
            if (*namePtr == 0x2e) {
                if (i > 3)
                    return (0);
                namePtr++;
                x = 0;

                errno = 0;
                strtoul_result = strtoul(string, &strtoul_end, 10);

                if (errno || string == strtoul_end || strtoul_result > 255) {
                    return 0;
                }

                temp = (uint32_t) strtoul_result;

                ip_addr |= temp << (24 - (i * 8));
                string[0] = 0;
                string[1] = 0;
                string[2] = 0;
                i++;
            } else
                return (0);     
        }
    }

    if (i == 3) {
        errno = 0;
        strtoul_result = strtoul(string, &strtoul_end, 10);

        if (errno || string == strtoul_end || strtoul_result > 255) {
            return 0;
        }

        temp = (uint32_t) strtoul_result;

        ip_addr |= temp;
        *addr_error = FALSE;
        return (ntohl(ip_addr));
    } else {
        return 0;
    }
}












void *
cprGetSysHeader (void *buffer)
{
    phn_syshdr_t *syshdr;

    



    syshdr = cpr_calloc(1, sizeof(phn_syshdr_t));
    if (syshdr) {
        syshdr->Data = buffer;
    }
    return (void *)syshdr;
}










void
cprReleaseSysHeader (void *syshdr)
{
    if (syshdr == NULL) {
        CPR_ERROR("cprReleaseSysHeader: Sys header pointer is NULL\n");
        return;
    }

    cpr_free(syshdr);
}


















void
fillInSysHeader (void *buffer, uint16_t cmd, uint16_t len, void *timerMsg)
{
    phn_syshdr_t *syshdr;

    syshdr = (phn_syshdr_t *) buffer;
    syshdr->Cmd = cmd;
    syshdr->Len = len;
    syshdr->Usr.UsrPtr = timerMsg;
    return;
}

