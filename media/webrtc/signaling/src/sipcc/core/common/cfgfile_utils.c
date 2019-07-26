






































#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "cpr_socket.h"
#include "cpr_in.h"
#include <text_strings.h>
#include <cfgfile_utils.h>
#include <config.h>
#include <phone_debug.h>
#include "util_string.h"

#define IN6ADDRSZ   16 
#define INT16SZ     2 
#define	INADDRSZ	4
#define IS_DIGIT(ch)   ((ch >= '0') && (ch <= '9'))











int
str2ip (const char *str, cpr_ip_addr_t *cpr_addr)
{
    uint32_t ip_addr;
    unsigned int num;
    int dot_cnt;
    char ch;
    int digit_flag;
    uint32_t *addr = (uint32_t *)&(cpr_addr->u.ip4);

    dot_cnt = 0;
    num = 0;
    ip_addr = 0;
    digit_flag = 0;
    cpr_addr->type = CPR_IP_ADDR_INVALID;

    while (1) {
        ch = *str++;
        if (!ch)
            break;              
        


        if (IS_DIGIT(ch)) {
            digit_flag = 1;
            num = num * 10 + (ch - '0');
            if (num > 255) {
                return (1);
            }
            continue;
        } else if (ch == ':') {
            
            cpr_addr->type = CPR_IP_ADDR_IPV6;
            return(cpr_inet_pton(AF_INET6, str, addr));
        }

        


        if ((ch == '.') && (digit_flag)) {
            dot_cnt++;
            ip_addr = ((ip_addr << 8) | num);
            num = 0;
            digit_flag = 0;
            continue;
        } 

        
        return (1);
    }

    


    if ((dot_cnt != 3) || (!digit_flag)) {
        return (1);
    }

    ip_addr = ((ip_addr << 8) | num);

    ip_addr = ntohl(ip_addr);   
    cpr_addr->type = CPR_IP_ADDR_IPV4;
    *addr = ip_addr;
    return (0);
}







int
cfgfile_parse_ip (const var_t *entry, const char *value)
{





    return (str2ip(value, (cpr_ip_addr_t *) entry->addr));

}






int
cfgfile_print_ip (const var_t *entry, char *buf, int len)
{    
    
    cpr_ip_addr_t *cprIpAddrPtr = (cpr_ip_addr_t *)entry->addr;
    
    if (cprIpAddrPtr->type == CPR_IP_ADDR_IPV4) {
        sprint_ip(buf, cprIpAddrPtr->u.ip4);
        return 1;
    }
    
    return 0;
}






int
cfgfile_print_ip_ntohl (const var_t *entry, char *buf, int len)
{
    uint32_t ip;

    ip = *(uint32_t *) entry->addr;
    return (snprintf(buf, len, get_debug_string(DEBUG_IP_PRINT),
                     ((ip >> 24) & (0xff)), ((ip >> 16) & (0xff)),
                     ((ip >> 8) & (0xff)), ((ip >> 0) & (0xff))));
}




int
cfgfile_parse_str (const var_t *entry, const char *value)
{
    int str_len;

    
    
    
    

    str_len = strlen(value);
    if (str_len + 1 > entry->length) {
        err_msg(get_debug_string(DEBUG_PARSER_STRING_TOO_LARGE),
                entry->length, str_len);
        return (1);
    }

    


    sstrncpy((char *)entry->addr, value, entry->length);
    return (0);


}




int
cfgfile_print_str (const var_t *entry, char *buf, int len)
{
    return (snprintf(buf, len, "%s", (char *)entry->addr));
}




int
cfgfile_parse_int (const var_t *entry, const char *value)
{
    unsigned int num;
    char ch;

    num = 0;

    if (strcmp(value, "UNPROVISIONED") == 0) {
        num = 0;
    } else {
        while (1) {
            ch = *value++;
            if (!ch)
                break;          
            


            if (IS_DIGIT(ch)) {
                num = num * 10 + (ch - '0');
                continue;
            }

            
            return (1);
        }
    }
    switch (entry->length) {
    case 1:
        *(uint8_t *) entry->addr = (uint8_t) num;
        break;
    case 2:
        *(uint16_t *) entry->addr = (uint16_t) num;
        break;
    case 4:
        *(uint32_t *) entry->addr = num;
        break;
    default:
        *(unsigned int *) entry->addr = num;
        break;
    }

    return (0);
}




int
cfgfile_print_int (const var_t *entry, char *buf, int len)
{
    unsigned int value;

    switch (entry->length) {
    case 1:
        value = *(uint8_t *) entry->addr;
        break;
    case 2:
        value = *(uint16_t *) entry->addr;
        break;
    case 4:
        value = *(uint32_t *) entry->addr;
        break;
    default:
        value = *(unsigned int *) entry->addr;
        break;
    }
    return (snprintf(buf, len, "%u", value));
}







int
cfgfile_parse_key (const var_t *entry, const char *value)
{
    const key_table_entry_t *keytable;

    keytable = entry->key_table;

    if (keytable == NULL) {
        err_msg(get_debug_string(DEBUG_PARSER_NULL_KEY_TABLE));
        return (1);
    }











    while (keytable->name) {
        if (cpr_strcasecmp(value, keytable->name) == 0) {
            *(unsigned int *) entry->addr = keytable->value;
            return (0);
        }
        keytable++;
    }

    err_msg(get_debug_string(DEBUG_PARSER_UNKNOWN_KEY), value);
    return (1);
}





int
cfgfile_print_key (const var_t *entry, char *buf, int len)
{
    const key_table_entry_t *keytable;
    int value;

    keytable = entry->key_table;
    value = *(int *) entry->addr;

    while (keytable->name) {
        if (value == keytable->value) {
            return (snprintf(buf, len, "%s", keytable->name));
        }
        keytable++;
    }

    err_msg(get_debug_string(DEBUG_PARSER_UNKNOWN_KEY_ENUM), value);
    return (0);
}




int
sprint_ip (char *buf, uint32_t ip)
{
    return (sprintf(buf, get_debug_string(DEBUG_IP_PRINT),
                    ((ip >> 0) & (0xff)), ((ip >> 8) & (0xff)),
                    ((ip >> 16) & (0xff)), ((ip >> 24) & (0xff))));
}




int
cfgfile_print_mac (const var_t *entry, char *buf, int len)
{
    return (snprintf(buf, len, get_debug_string(DEBUG_MAC_PRINT),
                     ((uint8_t *) entry->addr)[0] * 256 +
                     ((uint8_t *) entry->addr)[1],
                     ((uint8_t *) entry->addr)[2] * 256 +
                     ((uint8_t *) entry->addr)[3],
                     ((uint8_t *) entry->addr)[4] * 256 +
                     ((uint8_t *) entry->addr)[5]));
}
















int
cfgfile_parse_key_entry (const var_t *entry, const char *value)
{
    const key_table_entry_t *keytable;

    keytable = entry->key_table;

    if (keytable == NULL) {
        err_msg(get_debug_string(DEBUG_PARSER_NULL_KEY_TABLE));
        return (1);
    }

    while (keytable->name) {
        if (cpr_strcasecmp(value, keytable->name) == 0) {
            
            *(key_table_entry_t *)entry->addr = *keytable;
            return (0);
        }
        keytable++;
    }

    err_msg(get_debug_string(DEBUG_PARSER_UNKNOWN_KEY), value);
    return (1);
}












int 
cfgfile_print_key_entry (const var_t *entry, char *buf, int len)
{
    key_table_entry_t *key;

    key = (key_table_entry_t *) entry->addr;
    if (key->name != NULL) {
        return (snprintf(buf, len, "%s", key->name));
    } else {
         
        return (0);
    }
}
