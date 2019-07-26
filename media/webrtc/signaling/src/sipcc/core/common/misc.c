






































#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "cpr.h"
#include "phone_debug.h"
#include "cc_debug.h"
#include "phone.h"
#include "cpr_socket.h"

#include "prot_configmgr.h"
#include "debug.h"






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

extern boolean ccsip_get_ccm_date(char *date_value);






debugStruct_t debugBindTable[] = {
    {CC_DEBUG_CCAPP, "ccapp", &g_CCAppDebug},
    {CC_DEBUG_CONFIG_CACHE, "config-cache", &ConfigDebug},
    {CC_DEBUG_SIP_ADAPTER, "sip-adapter", &TNPDebug},
    {CC_DEBUG_CCAPI, "cc", &CCDebug},
    {CC_DEBUG_CC_MSG, "cc-msg", &CCDebugMsg},
    {CC_DEBUG_FIM, "fim", &FIMDebug},
    {CC_DEBUG_FSM, "fsm", &FSMDebugSM},
    {CC_DEBUG_AUTH, "auth", &AuthDebug},
    {CC_DEBUG_GSM, "gsm", &GSMDebug},
    {CC_DEBUG_LSM, "lsm", &LSMDebug},
    {CC_DEBUG_FSM_CAC, "fsm-cac", &g_cacDebug},
    {CC_DEBUG_DCSM, "dcsm", &g_dcsmDebug},
    {CC_DEBUG_SIP_TASK, "sip-task",      &SipDebugTask},
    {CC_DEBUG_SIP_STATE, "sip-state",     &SipDebugState},
    {CC_DEBUG_SIP_MSG, "sip-messages",  &SipDebugMessage},
    {CC_DEBUG_SIP_REG_STATE, "sip-reg-state", &SipDebugRegState},
    {CC_DEBUG_SIP_TRX, "sip-trx",       &SipDebugTrx},
    {CC_DEBUG_TIMERS, "timers",        &TMRDebug},
    {CC_DEBUG_CCDEFAULT, "ccdefault",     &g_DEFDebug},
    {CC_DEBUG_DIALPLAN, "dialplan", &DpintDebug},
    {CC_DEBUG_KPML, "kpml", &KpmlDebug},
    {CC_DEBUG_SIP_PRESENCE, "sip-presence",  &g_blfDebug},
    {CC_DEBUG_CONFIG_APP, "config-app", &g_configappDebug},
    {CC_DEBUG_CALL_EVENT, "call-event", &CCEVENTDebug},
    {CC_DEBUG_PLAT, "plat", &PLATDebug},
    {CC_DEBUG_NOTIFY, "cc-notify", NULL},
    {CC_DEBUG_CPR_MEMORY, "cpr-memory", NULL}, 
    {CC_DEBUG_MAX, "not-used", NULL} 
};





extern cc_int32_t fsmcnf_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t fsmdef_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t fsmxfr_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t fsmb2bcnf_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t dcsm_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t fim_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t fsm_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t lsm_show_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t show_kpmlmap_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t show_config_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t show_subsmanager_stats(cc_int32_t argc, const char *argv[]);
extern cc_int32_t show_publish_stats(cc_int32_t argc, const char *argv[]);
extern cc_int32_t show_register_cmd(cc_int32_t argc, const char *argv[]);
extern cc_int32_t show_dialplan_cmd(cc_int32_t argc, const char *argv[]);

extern int32_t cpr_show_memory(int32_t argc, const char *argv[]);
extern int32_t cpr_clear_memory (int32_t argc, const char *argv[]);
extern void debugCprMem(cc_debug_cpr_mem_options_e category, cc_debug_flag_e flag);
extern void debugClearCprMem(cc_debug_clear_cpr_options_e category);
void debugShowCprMem(cc_debug_show_cpr_options_e category);








debugShowStruct_t debugShowTable[] = {
    {CC_DEBUG_SHOW_FSMCNF, "fsmcnf", fsmcnf_show_cmd, FALSE},
    {CC_DEBUG_SHOW_FSMDEF, "fsmdef", fsmdef_show_cmd, FALSE},
    {CC_DEBUG_SHOW_FSMXFR, "fsmxfr", fsmxfr_show_cmd, FALSE},
    {CC_DEBUG_SHOW_FSMB2BCNF, "fsmb2bcnf", fsmb2bcnf_show_cmd, FALSE},
    {CC_DEBUG_SHOW_DCSM, "dcsm", dcsm_show_cmd, FALSE},
    {CC_DEBUG_SHOW_FIM, "fim", fim_show_cmd, FALSE},
    {CC_DEBUG_SHOW_FSM, "fsm", fsm_show_cmd, FALSE},
    {CC_DEBUG_SHOW_LSM, "lsm", lsm_show_cmd, FALSE},
    {CC_DEBUG_SHOW_KPML, "kpml", show_kpmlmap_cmd, FALSE},
    {CC_DEBUG_SHOW_CONFIG_CACHE, "config-cache", show_config_cmd, TRUE},
    {CC_DEBUG_SHOW_SUBS_STATS, "sip-subscription-statistics", show_subsmanager_stats, TRUE},
    {CC_DEBUG_SHOW_PUBLISH_STATS, "sip-publish-statistics", show_publish_stats, TRUE},
    {CC_DEBUG_SHOW_REGISTER, "register", show_register_cmd, TRUE},
    {CC_DEBUG_SHOW_DIALPLAN, "dialplan", show_dialplan_cmd, TRUE},
    {CC_DEBUG_SHOW_CPR_MEMORY, "cpr-memory", cpr_show_memory, FALSE},
    {CC_DEBUG_SHOW_MAX, "not-used", NULL, FALSE} 
};





debugClearStruct_t debugClearTable[] = {
    {CC_DEBUG_CLEAR_CPR_MEMORY, "cpr-memory", cpr_clear_memory},
    {CC_DEBUG_CLEAR_MAX, "not-used", NULL} 
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
                    strncpy(last_month_str, month_str, 3);
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














void
SipNtpUpdateClockFromCCM (void)
{
    const char *fname = "SipNtpUpdateClockFromCCM";
    struct tm tm_s, *tm_tmp_ptr;
    time_t epoch = 0;
    time_t epoch_tmp = 0;
    time_t tz_diff_sec = 0;
    char   date_hdr_str[MAX_NTP_DATE_HDR_STR_LEN];
    char  *token[MAX_NTP_TOKEN_BUF_LEN];
    char  *curr_token;
    int    count = 0;
#ifndef _WIN32
    char  *last;
#endif

    if (ccsip_get_ccm_date(date_hdr_str) == TRUE) {

        


#ifndef _WIN32
        curr_token = (char *) strtok_r(date_hdr_str, " ,:\t\r\n", &last);
#else
        curr_token = (char *) strtok(date_hdr_str, " ,:\t\r\n");
#endif

        while (curr_token) {
            token[count++] = curr_token;
            if (count > 8) {
                break;
            }

            


#ifndef _WIN32
            curr_token = (char *) strtok_r(NULL, " ,:\t\r\n", &last);
#else
            curr_token = (char *) strtok(NULL, " ,:\t\r\n");
#endif

        }
        
        if ((count == 8) && (strcmp(token[7], "GMT") == 0)) {
            if (set_month_from_str(token[2])) {
                tm_s.tm_mon = last_month;
                (void) sscanf(token[6], "%d", &tm_s.tm_sec);
                (void) sscanf(token[5], "%d", &tm_s.tm_min);
                (void) sscanf(token[4], "%d", &tm_s.tm_hour);
                (void) sscanf(token[1], "%d", &tm_s.tm_mday);
                (void) sscanf(token[3], "%d", &tm_s.tm_year);

                
                tm_s.tm_year = tm_s.tm_year - 1900;

                
                tm_s.tm_isdst = -1;

                
                epoch = (time_t) mktime(&tm_s);
                if (epoch == (time_t) - 1) {
                    TNP_DEBUG(DEB_F_PREFIX "mktime() returned -1... Not Good\n", DEB_F_PREFIX_ARGS(PLAT_API, fname));
                } else {
                    
                    
                    tm_tmp_ptr = gmtime(&epoch);
                    tm_tmp_ptr->tm_isdst = -1;

                    
                    
                    
                    epoch_tmp = (time_t) mktime(tm_tmp_ptr);
                    if (epoch == (time_t) - 1) {
                        TNP_DEBUG(DEB_F_PREFIX "mktime() returned -1... Not Good\n",
                                  DEB_F_PREFIX_ARGS(PLAT_API, fname));
                    } else {
                        
                        tz_diff_sec = epoch_tmp - epoch;
                        
                        epoch -= tz_diff_sec;

                        
                        platform_set_time((long) (epoch));
                    }
                }
            }
        }
    }
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
                if ((temp = atoi(string)) > 255)
                    return (0);
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
        if ((temp = atoi(string)) > 255)
            return (0);
        ip_addr |= temp;
        *addr_error = FALSE;
        return (ntohl(ip_addr));
    } else {
        return 0;
    }
}











void debugSet (cc_debug_category_e category, cc_debug_flag_e flag, ...)
{
    int i = 0; 
    va_list ap;
    int32_t data = -1;

    va_start(ap, flag);

    while (debugBindTable[i].category != CC_DEBUG_MAX) {
        if (debugBindTable[i].category == category) {
            if (debugBindTable[i].category == CC_DEBUG_CPR_MEMORY) {
                data = va_arg(ap, int32_t);
                if (data != -1) {
                    
                    switch(data) {
                        case CC_DEBUG_CPR_MEM_TRACKING:
                        case CC_DEBUG_CPR_MEM_POISON:
                            debugCprMem(data, flag);
                            break;
                        default:
                            debugif_printf("Error: Unknown CPR debug sub-category passed in\n");
                    }
                } else {
                    debugif_printf("Error: CPR debug sub-category NOT passed in\n");
                }
            } else {
                *(debugBindTable[i].key) = flag;
            }
            va_end(ap);
            return;
        }
        i++;
    }
    debugif_printf("Error: Unknown debug category passed in\n");
    va_end(ap);
    return;
}











int debugShow(cc_debug_show_options_e category, ...)
{
    const char *showArgc[NUM_OF_SHOW_ARGUMENTS];
    int i = 0, returnCode = 0;
    va_list ap;
    int32_t data = -1;

    va_start(ap, category);

    while (debugShowTable[i].category != CC_DEBUG_SHOW_MAX) {
        if (debugShowTable[i].category == category) {
            if (category == CC_DEBUG_SHOW_FSMDEF) {
                
                showArgc[0] = "all";
                returnCode = debugShowTable[i].callbackFunc(NUM_OF_SHOW_ARGUMENTS-1, showArgc);
            } else if (category == CC_DEBUG_SHOW_CPR_MEMORY) {
                data = va_arg(ap, int32_t);
                if (data != -1) {
                    switch (data) {
                        case CC_DEBUG_SHOW_CPR_CONFIG:
                        case CC_DEBUG_SHOW_CPR_HEAP_GUARD:
                        case CC_DEBUG_SHOW_CPR_STATISTICS:
                        case CC_DEBUG_SHOW_CPR_TRACKING:
                            debugShowCprMem((cc_debug_show_cpr_options_e) data);
                            break;

                        default:
                            debugif_printf("Error: Unknown CPR show sub-category passed in\n");
                    }
                } else {
                            debugif_printf("Error: CPR show sub-category NOT passed in\n");
                }
            } else {
                showArgc[0] = "show";
                showArgc[1] = debugShowTable[i].showName;
                returnCode = debugShowTable[i].callbackFunc(NUM_OF_SHOW_ARGUMENTS, showArgc);
            }
            debugif_printf("\n<EOT>\n");
            va_end(ap);
            return returnCode;
        }
        i++;
    }
    debugif_printf("Error: Unknown show category passed in\n");

    va_end(ap);
    return -1;
}








void debugShowTech()
{
    const char *showArgc[NUM_OF_SHOW_ARGUMENTS];
    int i = 0;

    showArgc[0] = "show";

    while (debugShowTable[i].category != CC_DEBUG_SHOW_MAX) {
        if (debugShowTable[i].showTech == TRUE) {
            showArgc[0] = debugShowTable[i].showName;
            debugShowTable[i].callbackFunc(NUM_OF_SHOW_ARGUMENTS, showArgc);
        }
        i++;
    }
    debugif_printf("\n<EOT>\n");
}

















int debugClear(cc_debug_clear_options_e category, ...)
{
    int i = 0, returnCode = 0 ;
    va_list ap;
    int32_t data = -1;

    va_start(ap, category);

    
    if (debugClearTable[i].category == category) {
        data = va_arg(ap, int32_t);
        if (data != -1) {
            switch (data) {
                case CC_DEBUG_CLEAR_CPR_TRACKING:
                case CC_DEBUG_CLEAR_CPR_STATISTICS:
                    debugClearCprMem((cc_debug_clear_cpr_options_e) data);
                    break;
                default:
                    debugif_printf("Error: Unknown CPR clear sub-category passed in\n");
            }
            debugif_printf("\n<EOT>\n");
        } else {
            debugif_printf("Error: CPR clear sub-category NOT passed in\n");
        }
    } else {
        debugif_printf("Error: Unknown show category passed in\n");
        returnCode = -1;
    }

    va_end(ap);
    return returnCode;
}
