



#include <errno.h>

#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"
#include "configmgr.h"
#include "prot_configmgr.h"
#include "ccapi.h"
#include "CSFLog.h"
#include "prprf.h"

static const char *logTag = "sdp_token";

#define MCAST_STRING_LEN 4


sdp_result_e sdp_parse_version (sdp_t *sdp_p, u16 level, const char *ptr)
{
    sdp_result_e result = SDP_FAILURE;

    sdp_p->version = (u16)sdp_getnextnumtok(ptr, &ptr, " \t", &result);
    if ((result != SDP_SUCCESS) || (sdp_p->version != SDP_CURRENT_VERSION)) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Invalid version (%lu) found, parse failed.",
            sdp_p->debug_str, sdp_p->version);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse version line successful, version %u",
                  sdp_p->debug_str, (u16)sdp_p->version);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_version (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    if (sdp_p->version == SDP_INVALID_VALUE) {
        if (sdp_p->conf_p->version_reqd == TRUE) {
            CSFLogError(logTag, "%s Invalid version for v= line, "
                        "build failed.", sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        } else {
            
            return (SDP_SUCCESS);
        }
    }

    flex_string_sprintf(fs, "v=%u\r\n", (u16)sdp_p->version);

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Built v= version line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

static sdp_result_e sdp_verify_unsigned(const char *ptr, uint64_t max_value)
{
    uint64_t numeric_value;
    

    size_t end = strspn(ptr, "0123456789");

    if (ptr[end] != '\0')
        return SDP_INVALID_PARAMETER;

    if (PR_sscanf(ptr, "%llu", &numeric_value) != 1)
        return SDP_INVALID_PARAMETER;

    if (numeric_value > max_value)
        return SDP_INVALID_PARAMETER;

    return SDP_SUCCESS;
}

sdp_result_e sdp_parse_owner (sdp_t *sdp_p, u16 level, const char *ptr)
{
    int          i;
    char        *tmpptr;
    sdp_result_e result;
    char         tmp[SDP_MAX_STRING_LEN];
    






    uint64_t     max_value_sessid_version = ((((uint64_t) 1) << 62) - 2);

    if (sdp_p->owner_name[0] != '\0') {
        sdp_p->conf_p->num_invalid_token_order++;
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: More than one o= line specified.",
            sdp_p->debug_str);
    }

    
    ptr = sdp_getnextstrtok(ptr, sdp_p->owner_name, sizeof(sdp_p->owner_name), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No owner name specified for o=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    


    ptr = sdp_getnextstrtok(ptr, sdp_p->owner_sessid, sizeof(sdp_p->owner_sessid), " \t", &result);
    if (result == SDP_SUCCESS) {
        


        result = sdp_verify_unsigned(sdp_p->owner_sessid, max_value_sessid_version);
    }
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Invalid owner session id specified for o=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, sdp_p->owner_version, sizeof(sdp_p->owner_version), " \t", &result);
    if (result == SDP_SUCCESS) {
        


        result = sdp_verify_unsigned(sdp_p->owner_version, max_value_sessid_version);
    }
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Invalid owner version specified for o=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No owner network type specified for o=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    sdp_p->owner_network_type = SDP_NT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_NETWORK_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_nettype[i].name,
                        sdp_nettype[i].strlen) == 0) {
            if (sdp_p->conf_p->nettype_supported[i] == TRUE) {
                sdp_p->owner_network_type = (sdp_nettype_e)i;
            }
        }
    }
    if (sdp_p->owner_network_type == SDP_NT_UNSUPPORTED) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Owner network type unsupported (%s)",
            sdp_p->debug_str, tmp);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No owner address type specified for o=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    sdp_p->owner_addr_type = SDP_AT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_ADDR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_addrtype[i].name,
                        sdp_addrtype[i].strlen) == 0) {
            if (sdp_p->conf_p->addrtype_supported[i] == TRUE) {
                sdp_p->owner_addr_type = (sdp_addrtype_e)i;
            }
        }
    }
    if ((sdp_p->owner_addr_type == SDP_AT_UNSUPPORTED) &&
        (sdp_p->owner_network_type != SDP_NT_ATM)) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Owner address type unsupported (%s)",
            sdp_p->debug_str, tmp);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    ptr = sdp_getnextstrtok(ptr, sdp_p->owner_addr, sizeof(sdp_p->owner_addr), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No owner address specified.", sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse owner: name %s, session id %s, version %s",
                  sdp_p->debug_str, sdp_p->owner_name, sdp_p->owner_sessid,
                  sdp_p->owner_version);
        SDP_PRINT("%s              network %s, address type %s, "
                  "address %s", sdp_p->debug_str,
                  sdp_get_network_name(sdp_p->owner_network_type),
                  sdp_get_address_name(sdp_p->owner_addr_type),
                  sdp_p->owner_addr);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_owner (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    if ((sdp_p->owner_name[0] == '\0') ||
        (sdp_p->owner_network_type >= SDP_MAX_NETWORK_TYPES) ||
        (sdp_p->owner_addr_type >= SDP_MAX_ADDR_TYPES) ||
        (sdp_p->owner_addr[0] == '\0')) {

        if((sdp_p->owner_network_type == SDP_NT_ATM) &&
           (sdp_p->owner_addr_type == SDP_AT_INVALID)) {
          flex_string_sprintf(fs, "o=%s %s %s %s - -\r\n",
                    sdp_p->owner_name, sdp_p->owner_sessid,
                    sdp_p->owner_version,
                    sdp_get_network_name(sdp_p->owner_network_type));
        }

        if (sdp_p->conf_p->owner_reqd == TRUE) {
            CSFLogError(logTag, "%s Invalid params for o= owner line, "
                        "build failed.", sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        } else {
            
            return (SDP_SUCCESS);
        }
    }

    flex_string_sprintf(fs, "o=%s %s %s %s %s %s\r\n",
                    sdp_p->owner_name, sdp_p->owner_sessid,
                    sdp_p->owner_version,
                    sdp_get_network_name(sdp_p->owner_network_type),
                    sdp_get_address_name(sdp_p->owner_addr_type),
                    sdp_p->owner_addr);

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Built o= owner line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_sessname (sdp_t *sdp_p, u16 level, const char *ptr)
{
    int   str_len;
    char *endptr;

    if (sdp_p->sessname[0] != '\0') {
        sdp_p->conf_p->num_invalid_token_order++;
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: More than one s= line specified.",
            sdp_p->debug_str);
    }

    endptr = sdp_findchar(ptr, "\r\n");
    if (ptr == endptr) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No session name specified.",
            sdp_p->debug_str);
    }
    str_len = MIN(endptr - ptr, SDP_MAX_STRING_LEN);
    sstrncpy(sdp_p->sessname, ptr, str_len+1);

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse session name, %s",
                  sdp_p->debug_str, sdp_p->sessname);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_sessname (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    if (sdp_p->sessname[0] == '\0') {
        if (sdp_p->conf_p->session_name_reqd == TRUE) {
            CSFLogError(logTag, "%s No param defined for s= session name line, "
                        "build failed.", sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        } else {
            
            return (SDP_SUCCESS);
        }
    }

    flex_string_sprintf(fs, "s=%s\r\n", sdp_p->sessname);

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Built s= session name line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}





sdp_result_e sdp_parse_sessinfo (sdp_t *sdp_p, u16 level, const char *ptr)
{
    char *endptr;
    sdp_mca_t *mca_p;

    if (level == SDP_SESSION_LEVEL) {
        if (sdp_p->sessinfo_found == TRUE) {
            sdp_p->conf_p->num_invalid_token_order++;
            sdp_parse_error(sdp_p->peerconnection,
                "%s Warning: More than one i= line specified.",
                sdp_p->debug_str);
        }
        sdp_p->sessinfo_found = TRUE;
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        if (mca_p->sessinfo_found == TRUE) {
            sdp_p->conf_p->num_invalid_token_order++;
            sdp_parse_error(sdp_p->peerconnection,
                "%s Warning: More than one i= line specified"
                " for media line %d.", sdp_p->debug_str, level);
        }
        mca_p->sessinfo_found = TRUE;
    }

    endptr = sdp_findchar(ptr, "\n");
    if (ptr == endptr) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No session info specified.",
            sdp_p->debug_str);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed session info line.", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_sessinfo (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_uri (sdp_t *sdp_p, u16 level, const char *ptr)
{
    char *endptr;

    if (sdp_p->uri_found == TRUE) {
        sdp_p->conf_p->num_invalid_token_order++;
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: More than one u= line specified.",
            sdp_p->debug_str);
    }
    sdp_p->uri_found = TRUE;

    endptr = sdp_findchar(ptr, "\n");
    if (ptr == endptr) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No URI info specified.", sdp_p->debug_str);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed URI line.", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_uri (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_email (sdp_t *sdp_p, u16 level, const char *ptr)
{
    char *endptr;

    endptr = sdp_findchar(ptr, "\n");
    if (ptr == endptr) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No email info specified.", sdp_p->debug_str);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse email line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_email (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_phonenum (sdp_t *sdp_p, u16 level, const char *ptr)
{
    char *endptr;

    endptr = sdp_findchar(ptr, "\n");
    if (ptr == endptr) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No phone number info specified.",
            sdp_p->debug_str);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse phone number line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_phonenum (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_connection (sdp_t *sdp_p, u16 level, const char *ptr)
{
    int           i;
    const char   *slash_ptr;
    sdp_result_e  result;
    sdp_conn_t   *conn_p;
    sdp_mca_t    *mca_p;
    char          tmp[SDP_MAX_STRING_LEN];
    char mcast_str[MCAST_STRING_LEN];
    int  mcast_bits;
    unsigned long strtoul_result;
    char *strtoul_end;

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        conn_p = &(mca_p->conn);
    }

    



    if (conn_p->nettype != SDP_NT_INVALID) {
        sdp_p->conf_p->num_invalid_token_order++;
        sdp_parse_error(sdp_p->peerconnection,
            "%s c= line specified twice at same level, "
            "parse failed.", sdp_p->debug_str);
        return (SDP_INVALID_TOKEN_ORDERING);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No connection network type specified for c=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    conn_p->nettype = SDP_NT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_NETWORK_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_nettype[i].name,
                        sdp_nettype[i].strlen) == 0) {
            if (sdp_p->conf_p->nettype_supported[i] == TRUE) {
                conn_p->nettype = (sdp_nettype_e)i;
            }
        }
    }
    if (conn_p->nettype == SDP_NT_UNSUPPORTED) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: Connection network type unsupported "
            "(%s) for c=.", sdp_p->debug_str, tmp);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        if (conn_p->nettype == SDP_NT_ATM) {
            
            if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
                SDP_PRINT("%s Parse connection: network %s", sdp_p->debug_str,
                          sdp_get_network_name(conn_p->nettype));
            }
            return (SDP_SUCCESS);
        } else {
            sdp_parse_error(sdp_p->peerconnection,
                "%s No connection address type specified for "
                "c=.", sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    }
    conn_p->addrtype = SDP_AT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_ADDR_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_addrtype[i].name,
                        sdp_addrtype[i].strlen) == 0) {
            if (sdp_p->conf_p->addrtype_supported[i] == TRUE) {
                conn_p->addrtype = (sdp_addrtype_e)i;
            }
        }
    }
    if (conn_p->addrtype == SDP_AT_UNSUPPORTED) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: Connection address type unsupported "
            "(%s) for c=.", sdp_p->debug_str, tmp);
    }

    
    ptr = sdp_getnextstrtok(ptr, conn_p->conn_addr, sizeof(conn_p->conn_addr), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No connection address specified for c=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    

    



    
    sstrncpy (mcast_str, conn_p->conn_addr, MCAST_STRING_LEN);

    errno = 0;
    strtoul_result = strtoul(mcast_str, &strtoul_end, 10);

    if (errno || mcast_str == strtoul_end || strtoul_result > 255) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Error parsing address %s for mcast.",
            sdp_p->debug_str, mcast_str);
        sdp_p->conf_p->num_invalid_param++;
        return SDP_INVALID_PARAMETER;
    }


    mcast_bits = (int) strtoul_result;
    if ((mcast_bits >= SDP_MIN_MCAST_ADDR_HI_BIT_VAL ) &&
        (mcast_bits <= SDP_MAX_MCAST_ADDR_HI_BIT_VAL)) {
        SDP_PRINT("%s Parsed to be a multicast address with mcast bits %d",
                  sdp_p->debug_str, mcast_bits);
        conn_p->is_multicast = TRUE;
    }

    if (conn_p->addrtype != SDP_AT_EPN) {
        slash_ptr = sdp_findchar(conn_p->conn_addr, "/");
        if (slash_ptr[0] != '\0') {
            if (conn_p->is_multicast) {
                SDP_PRINT("%s A multicast address with slash %s",
                          sdp_p->debug_str, conn_p->conn_addr);
                slash_ptr++;
                slash_ptr = sdp_getnextstrtok(slash_ptr, tmp, sizeof(tmp), "/", &result);
                if (result != SDP_SUCCESS) {
                    sdp_parse_error(sdp_p->peerconnection,
                        "%s No ttl value specified for this multicast addr with a slash",
                        sdp_p->debug_str);
                    sdp_p->conf_p->num_invalid_param++;
                    return (SDP_INVALID_PARAMETER);
                }

                errno = 0;
                strtoul_result = strtoul(tmp, &strtoul_end, 10);

                if (errno || tmp == strtoul_end || conn_p->ttl > SDP_MAX_TTL_VALUE) {
                    sdp_parse_error(sdp_p->peerconnection,
                        "%s Invalid TTL: Value must be in the range 0-255 ",
                        sdp_p->debug_str);
                    sdp_p->conf_p->num_invalid_param++;
                    return (SDP_INVALID_PARAMETER);
                }

                conn_p->ttl = (int) strtoul_result;

                
                


                slash_ptr = sdp_findchar(slash_ptr, "/");
                if (slash_ptr != NULL &&
                      slash_ptr[0] != '\0') {
                    SDP_PRINT("%s Found a num addr field for multicast addr %s ",
                              sdp_p->debug_str,slash_ptr);
                    slash_ptr++;

                    errno = 0;
                    strtoul_result = strtoul(slash_ptr, &strtoul_end, 10);

                    if (errno || slash_ptr == strtoul_end || strtoul_result == 0) {
                        sdp_parse_error(sdp_p->peerconnection,
                            "%s Invalid Num of addresses: Value must be > 0 ",
                            sdp_p->debug_str);
                        sdp_p->conf_p->num_invalid_param++;
                        return SDP_INVALID_PARAMETER;
                    }

                    conn_p->num_of_addresses = (int) strtoul_result;
                }
	        } else {
                sdp_p->conf_p->num_invalid_param++;
                SDP_PRINT("%s Only multicast addresses allowed with slashes",
                          sdp_p->debug_str);
                return (SDP_INVALID_PARAMETER);
            }
        }
    }

    
    if ((sdp_p->conf_p->allow_choose[SDP_CHOOSE_CONN_ADDR] == FALSE) &&
        (strcmp(conn_p->conn_addr, "$") == 0)) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: Choose parameter for connection "
            "address specified but not allowed.", sdp_p->debug_str);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse connection: network %s, address type %s, "
                  "address %s ttl= %d num of addresses = %d",
                  sdp_p->debug_str,
                  sdp_get_network_name(conn_p->nettype),
                  sdp_get_address_name(conn_p->addrtype),
                  conn_p->conn_addr, conn_p->ttl, conn_p->num_of_addresses);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_connection (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    sdp_mca_t  *mca_p;
    sdp_conn_t *conn_p;

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        conn_p = &(mca_p->conn);
    }

    if((conn_p->nettype == SDP_NT_ATM ) &&
       (conn_p->addrtype == SDP_AT_INVALID)) {
        

        flex_string_sprintf(fs, "c=%s\r\n",
                    sdp_get_network_name(conn_p->nettype));
        return SDP_SUCCESS;
    }
    if ((conn_p->nettype >= SDP_MAX_NETWORK_TYPES) ||
        (conn_p->addrtype >= SDP_MAX_ADDR_TYPES) ||
        (conn_p->conn_addr[0] == '\0')) {
        
        return (SDP_SUCCESS);
    }

    if (conn_p->is_multicast) {
        if (conn_p->num_of_addresses > 1) {
            flex_string_sprintf(fs, "c=%s %s %s/%d/%d\r\n",
                             sdp_get_network_name(conn_p->nettype),
                             sdp_get_address_name(conn_p->addrtype),
                             conn_p->conn_addr, conn_p->ttl,
                             conn_p->num_of_addresses);
        } else {
            flex_string_sprintf(fs, "c=%s %s %s/%d\r\n",
                             sdp_get_network_name(conn_p->nettype),
                             sdp_get_address_name(conn_p->addrtype),
                             conn_p->conn_addr, conn_p->ttl);
        }
    } else {

        flex_string_sprintf(fs, "c=%s %s %s\r\n",
                         sdp_get_network_name(conn_p->nettype),
                         sdp_get_address_name(conn_p->addrtype),
                         conn_p->conn_addr);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Built c= connection line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}













sdp_result_e sdp_parse_bandwidth (sdp_t *sdp_p, u16 level, const char *ptr)
{
    int                  i;
    sdp_mca_t            *mca_p;
    sdp_bw_t             *bw_p;
    sdp_bw_data_t        *bw_data_p;
    sdp_bw_data_t        *new_bw_data_p;
    sdp_result_e         result;
    char                 tmp[SDP_MAX_STRING_LEN];
    sdp_bw_modifier_e    bw_modifier = SDP_BW_MODIFIER_UNSUPPORTED;
    int                  bw_val = 0;

    if (level == SDP_SESSION_LEVEL) {
        bw_p = &(sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        bw_p = &(mca_p->bw);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse bandwidth line", sdp_p->debug_str);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), ":", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No bandwidth type specified for b= ",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    for (i=0; i < SDP_MAX_BW_MODIFIER_VAL; i++) {
        if (cpr_strncasecmp(tmp, sdp_bw_modifier_val[i].name,
                        sdp_bw_modifier_val[i].strlen) == 0) {
            bw_modifier  = (sdp_bw_modifier_e)i;
            break;
        }
    }

    if (bw_modifier == SDP_BW_MODIFIER_UNSUPPORTED) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Error: BW Modifier type unsupported (%s).",
            sdp_p->debug_str, tmp);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    


    if (*ptr == ':') {
        ptr++;
        bw_val = sdp_getnextnumtok(ptr, &ptr, " \t", &result);
        if ((result != SDP_SUCCESS)) {
            sdp_parse_error(sdp_p->peerconnection,
                "%s Error: No BW Value specified ",
                sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    }

    



    new_bw_data_p = (sdp_bw_data_t*)SDP_MALLOC(sizeof(sdp_bw_data_t));
    if (new_bw_data_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_NO_RESOURCE);
    }
    new_bw_data_p->next_p = NULL;
    new_bw_data_p->bw_modifier = bw_modifier;
    new_bw_data_p->bw_val = bw_val;

    



    if (bw_p->bw_data_list == NULL) {
        bw_p->bw_data_list = new_bw_data_p;
    } else {
        for (bw_data_p = bw_p->bw_data_list;
             bw_data_p->next_p != NULL;
             bw_data_p = bw_data_p->next_p) {
            ; 
        }
        bw_data_p->next_p = new_bw_data_p;
    }
    bw_p->bw_data_count++;

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed bw type %s, value %d", sdp_p->debug_str,
                     sdp_get_bw_modifier_name(new_bw_data_p->bw_modifier),
		     new_bw_data_p->bw_val);
    }

    return (SDP_SUCCESS);
}






sdp_result_e sdp_build_bandwidth (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    sdp_bw_t            *bw_p;
    sdp_bw_data_t       *bw_data_p;
    sdp_mca_t           *mca_p;

    if (level == SDP_SESSION_LEVEL) {
        bw_p = &(sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        bw_p = &(mca_p->bw);
    }

    bw_data_p = bw_p->bw_data_list;
    while (bw_data_p) {
        flex_string_sprintf(fs, "b=%s:%d\r\n",
                         sdp_get_bw_modifier_name(bw_data_p->bw_modifier),
                         bw_data_p->bw_val);

        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
           SDP_PRINT("%s Built b=%s:%d bandwidth line", sdp_p->debug_str,
                     sdp_get_bw_modifier_name(bw_data_p->bw_modifier),
                     bw_data_p->bw_val);
        }

        bw_data_p = bw_data_p->next_p;
    }

    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_timespec (sdp_t *sdp_p, u16 level, const char *ptr)
{
    char            *tmpptr;
    sdp_result_e     result;
    sdp_timespec_t  *timespec_p;
    sdp_timespec_t  *next_timespec_p;

    timespec_p = (sdp_timespec_t *)SDP_MALLOC(sizeof(sdp_timespec_t));
    if (timespec_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        return (SDP_NO_RESOURCE);
    }

    
    ptr = sdp_getnextstrtok(ptr, timespec_p->start_time, sizeof(timespec_p->start_time), " \t", &result);
    if (result == SDP_SUCCESS) {
        


        (void)sdp_getnextnumtok(timespec_p->start_time,
                                (const char **)&tmpptr, " \t", &result);
    }
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Invalid timespec start time specified.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        SDP_FREE(timespec_p);
        return (SDP_INVALID_PARAMETER);
    }

    ptr = sdp_getnextstrtok(ptr, timespec_p->stop_time, sizeof(timespec_p->stop_time), " \t", &result);
    if (result == SDP_SUCCESS) {
        


        (void)sdp_getnextnumtok(timespec_p->stop_time,
                                (const char **)&tmpptr, " \t", &result);
    }
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Invalid timespec stop time specified.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        SDP_FREE(timespec_p);
        return (SDP_INVALID_PARAMETER);
    }

    
    if (sdp_p->timespec_p == NULL) {
        sdp_p->timespec_p = timespec_p;
    } else {
        next_timespec_p = sdp_p->timespec_p;
        while (next_timespec_p->next_p != NULL) {
            next_timespec_p = next_timespec_p->next_p;
        }
        next_timespec_p->next_p = timespec_p;
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed timespec line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_timespec (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    if ((sdp_p->timespec_p == NULL) ||
        (sdp_p->timespec_p->start_time == '\0') ||
        (sdp_p->timespec_p->stop_time == '\0')) {
        if (sdp_p->conf_p->timespec_reqd == TRUE) {
            CSFLogError(logTag, "%s Invalid params for t= time spec line, "
                        "build failed.", sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        } else {
            
            return (SDP_SUCCESS);
        }
    }

    
    flex_string_sprintf(fs, "t=%s %s\r\n", sdp_p->timespec_p->start_time,
                    sdp_p->timespec_p->stop_time);

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Built t= timespec line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_repeat_time (sdp_t *sdp_p, u16 level, const char *ptr)
{
    char *endptr;

    endptr = sdp_findchar(ptr, "\n");
    if (ptr == endptr) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No repeat time parameters "
            "specified.", sdp_p->debug_str);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parsed repeat time line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_repeat_time (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_timezone_adj (sdp_t *sdp_p, u16 level, const char *ptr)
{
    char *endptr;

    endptr = sdp_findchar(ptr, "\n");
    if (ptr == endptr) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No timezone parameters specified.",
            sdp_p->debug_str);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse timezone adustment line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_timezone_adj (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_encryption (sdp_t *sdp_p, u16 level, const char *ptr)
{
    int                  i;
    sdp_result_e         result;
    sdp_encryptspec_t   *encrypt_p;
    sdp_mca_t           *mca_p;
    char                 tmp[SDP_MAX_STRING_LEN];

    if (level == SDP_SESSION_LEVEL) {
        encrypt_p = &(sdp_p->encrypt);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        encrypt_p = &(mca_p->encrypt);
    }
    encrypt_p->encrypt_key[0] = '\0';

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), ":", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No encryption type specified for k=.",
            sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    encrypt_p->encrypt_type = SDP_ENCRYPT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_ENCRYPT_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_encrypt[i].name,
                        sdp_encrypt[i].strlen) == 0) {
            encrypt_p->encrypt_type = (sdp_encrypt_type_e)i;
            break;
        }
    }
    if (encrypt_p->encrypt_type == SDP_ENCRYPT_UNSUPPORTED) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: Encryption type unsupported (%s).",
            sdp_p->debug_str, tmp);
    }

    
    encrypt_p->encrypt_key[0] = '\0';
    


    if (*ptr == ':')
        ptr++;
    if (encrypt_p->encrypt_type != SDP_ENCRYPT_PROMPT) {
        ptr = sdp_getnextstrtok(ptr, encrypt_p->encrypt_key, sizeof(encrypt_p->encrypt_key), " \t", &result);
        if ((result != SDP_SUCCESS) &&
            ((encrypt_p->encrypt_type == SDP_ENCRYPT_CLEAR) ||
             (encrypt_p->encrypt_type == SDP_ENCRYPT_BASE64) ||
             (encrypt_p->encrypt_type == SDP_ENCRYPT_URI))) {
            sdp_parse_error(sdp_p->peerconnection,
                "%s Warning: No encryption key specified "
                "as required.", sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Parse encryption type %s, key %s", sdp_p->debug_str,
                   sdp_get_encrypt_name(encrypt_p->encrypt_type),
                   encrypt_p->encrypt_key);
    }
    return (SDP_SUCCESS);
}


sdp_result_e sdp_build_encryption (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    sdp_encryptspec_t   *encrypt_p;
    sdp_mca_t           *mca_p;

    if (level == SDP_SESSION_LEVEL) {
        encrypt_p = &(sdp_p->encrypt);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_FAILURE);
        }
        encrypt_p = &(mca_p->encrypt);
    }

    if ((encrypt_p->encrypt_type >= SDP_MAX_ENCRYPT_TYPES) ||
        ((encrypt_p->encrypt_type != SDP_ENCRYPT_PROMPT) &&
         (encrypt_p->encrypt_key[0] == '\0'))) {
        
        return (SDP_SUCCESS);
    }

    flex_string_sprintf(fs, "k=%s",
                     sdp_get_encrypt_name(encrypt_p->encrypt_type));

    if (encrypt_p->encrypt_type == SDP_ENCRYPT_PROMPT) {
        
        flex_string_sprintf(fs, "\r\n");
    } else {
        flex_string_sprintf(fs, ":%s\r\n", encrypt_p->encrypt_key);
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Built k= encryption line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_parse_media (sdp_t *sdp_p, u16 level, const char *ptr)
{
    u16                   i;
    u16                   num_port_params=0;
    int32                 num[SDP_MAX_PORT_PARAMS];
    tinybool              valid_param = FALSE;
    sdp_result_e          result;
    sdp_mca_t            *mca_p;
    sdp_mca_t            *next_mca_p;
    char                  tmp[SDP_MAX_STRING_LEN];
    char                  port[SDP_MAX_STRING_LEN];
    const char           *port_ptr;
    int32                 sctp_port;

    
    mca_p = sdp_alloc_mca();
    if (mca_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        return (SDP_NO_RESOURCE);
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No media type specified, parse failed.",
            sdp_p->debug_str);
        SDP_FREE(mca_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    mca_p->media = SDP_MEDIA_UNSUPPORTED;
    for (i=0; i < SDP_MAX_MEDIA_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_media[i].name,
                        sdp_media[i].strlen) == 0) {
            mca_p->media = (sdp_media_e)i;
        }
    }
    if (mca_p->media == SDP_MEDIA_UNSUPPORTED) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: Media type unsupported (%s).",
            sdp_p->debug_str, tmp);
    }

    



    ptr = sdp_getnextstrtok(ptr, port, sizeof(port), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No port specified in m= media line, "
            "parse failed.", sdp_p->debug_str);
        SDP_FREE(mca_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    port_ptr = port;
    for (i=0; i < SDP_MAX_PORT_PARAMS; i++) {
        if (sdp_getchoosetok(port_ptr, &port_ptr, "/ \t", &result) == TRUE) {
            num[i] = SDP_CHOOSE_PARAM;
        } else {
            num[i] = sdp_getnextnumtok(port_ptr, (const char **)&port_ptr,
                                       "/ \t", &result);
            if (result != SDP_SUCCESS) {
                break;
            }
        }
        num_port_params++;
    }

    
    ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
    if (result != SDP_SUCCESS) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s No transport protocol type specified, "
            "parse failed.", sdp_p->debug_str);
        SDP_FREE(mca_p);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    mca_p->transport = SDP_TRANSPORT_UNSUPPORTED;
    for (i=0; i < SDP_MAX_TRANSPORT_TYPES; i++) {
        if (cpr_strncasecmp(tmp, sdp_transport[i].name,
                        sdp_transport[i].strlen) == 0) {
            mca_p->transport = (sdp_transport_e)i;
            break;
        }
    }
    if (mca_p->transport == SDP_TRANSPORT_UNSUPPORTED) {
        


        mca_p->port = num[0];
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: Transport protocol type unsupported "
            "(%s).", sdp_p->debug_str, tmp);
    }

    


    valid_param = FALSE;
    switch (num_port_params) {
    case 1:
        if ((mca_p->transport == SDP_TRANSPORT_RTPAVP) ||
	    (mca_p->transport == SDP_TRANSPORT_RTPSAVP) ||
	    (mca_p->transport == SDP_TRANSPORT_RTPSAVPF) ||
            (mca_p->transport == SDP_TRANSPORT_UDP) ||
            (mca_p->transport == SDP_TRANSPORT_TCP) ||
            (mca_p->transport == SDP_TRANSPORT_UDPTL) ||
            (mca_p->transport == SDP_TRANSPORT_UDPSPRT) ||
            (mca_p->transport == SDP_TRANSPORT_LOCAL) ||
            (mca_p->transport == SDP_TRANSPORT_SCTPDTLS)) {
            



            if ((sdp_p->conf_p->allow_choose[SDP_CHOOSE_PORTNUM]) ||
                (num[0] != SDP_CHOOSE_PARAM)) {
                mca_p->port        = num[0];
                mca_p->port_format = SDP_PORT_NUM_ONLY;
                valid_param        = TRUE;
            }
        } else if (mca_p->transport == SDP_TRANSPORT_AAL1AVP) {
            

            if (num[0] != SDP_CHOOSE_PARAM) {
                mca_p->vcci        = num[0];
                mca_p->port_format = SDP_PORT_VCCI;
                valid_param        = TRUE;
            }
        } else if ((mca_p->transport == SDP_TRANSPORT_AAL2_ITU) ||
            (mca_p->transport == SDP_TRANSPORT_AAL2_ATMF) ||
            (mca_p->transport == SDP_TRANSPORT_AAL2_CUSTOM)) {
            


            mca_p->port        = num[0];
            mca_p->port_format = SDP_PORT_NUM_ONLY;
            valid_param        = TRUE;
        }
        break;
    case 2:
        if ((mca_p->transport == SDP_TRANSPORT_RTPAVP) ||
	    (mca_p->transport == SDP_TRANSPORT_RTPSAVP) ||
	    (mca_p->transport == SDP_TRANSPORT_RTPSAVPF) ||
            (mca_p->transport == SDP_TRANSPORT_UDP) ||
            (mca_p->transport == SDP_TRANSPORT_LOCAL)) {
            


            if ((num[0] != SDP_CHOOSE_PARAM) &&
                (num[1] != SDP_CHOOSE_PARAM)) {
                mca_p->port        = num[0];
                mca_p->num_ports   = num[1];
                mca_p->port_format = SDP_PORT_NUM_COUNT;
                valid_param        = TRUE;
            }
        } else if (mca_p->transport == SDP_TRANSPORT_UDPTL) {
            



            if ((num[0] != SDP_CHOOSE_PARAM) &&
                (num[1] == 1)) {
                mca_p->port        = num[0];
                mca_p->num_ports   = 1;
                mca_p->port_format = SDP_PORT_NUM_COUNT;
                valid_param        = TRUE;
            }
        } else if (mca_p->transport == SDP_TRANSPORT_CES10) {
            


            if ((num[0] != SDP_CHOOSE_PARAM) &&
                (num[1] != SDP_CHOOSE_PARAM)) {
                mca_p->vpi         = num[0];
                mca_p->vci         = num[1];
                mca_p->port_format = SDP_PORT_VPI_VCI;
                valid_param        = TRUE;
            }
        } else if ((mca_p->transport == SDP_TRANSPORT_AAL2_ITU) ||
                   (mca_p->transport == SDP_TRANSPORT_AAL2_ATMF) ||
                   (mca_p->transport == SDP_TRANSPORT_AAL2_CUSTOM)) {
            




            if (((num[0] != SDP_CHOOSE_PARAM) &&
                 (num[1] != SDP_CHOOSE_PARAM)) ||
                ((num[0] == SDP_CHOOSE_PARAM) &&
                 (num[1] == SDP_CHOOSE_PARAM))) {
                mca_p->vcci        = num[0];
                mca_p->cid         = num[1];
                mca_p->port_format = SDP_PORT_VCCI_CID;
                valid_param        = TRUE;
            }
        }
        break;
    case 3:
        if (mca_p->transport == SDP_TRANSPORT_AAL1AVP) {
            


            if ((num[0] != SDP_CHOOSE_PARAM) &&
                (num[1] != SDP_CHOOSE_PARAM) &&
                (num[2] != SDP_CHOOSE_PARAM)) {
                mca_p->port        = num[0];
                mca_p->vpi         = num[1];
                mca_p->vci         = num[2];
                mca_p->port_format = SDP_PORT_NUM_VPI_VCI;
                valid_param        = TRUE;
            }
        }
        break;
    case 4:
        if ((mca_p->transport == SDP_TRANSPORT_AAL2_ITU) ||
            (mca_p->transport == SDP_TRANSPORT_AAL2_ATMF) ||
            (mca_p->transport == SDP_TRANSPORT_AAL2_CUSTOM)) {
            


            if ((num[0] != SDP_CHOOSE_PARAM) &&
                (num[1] != SDP_CHOOSE_PARAM) &&
                (num[2] != SDP_CHOOSE_PARAM) &&
                (num[3] != SDP_CHOOSE_PARAM)) {
                mca_p->port        = num[0];
                mca_p->vpi         = num[1];
                mca_p->vci         = num[2];
                mca_p->cid         = num[3];
                mca_p->port_format = SDP_PORT_NUM_VPI_VCI_CID;
                valid_param        = TRUE;
            }
        }
        break;
    }
    if (valid_param == FALSE) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Invalid port format (%s) specified for transport "
            "protocol (%s), parse failed.", sdp_p->debug_str,
            port, sdp_get_transport_name(mca_p->transport));
        sdp_p->conf_p->num_invalid_param++;
        SDP_FREE(mca_p);
        return (SDP_INVALID_PARAMETER);
    }

    

    if ((mca_p->transport == SDP_TRANSPORT_AAL2_ITU) ||
        (mca_p->transport == SDP_TRANSPORT_AAL2_ATMF) ||
        (mca_p->transport == SDP_TRANSPORT_AAL2_CUSTOM)) {

        if (sdp_parse_multiple_profile_payload_types(sdp_p, mca_p, ptr) !=
            SDP_SUCCESS) {
            sdp_p->conf_p->num_invalid_param++;
	    SDP_FREE(mca_p);
            return (SDP_INVALID_PARAMETER);
        }
    } else {
        
        sdp_parse_payload_types(sdp_p, mca_p, ptr);
    }

    
    if (mca_p->transport == SDP_TRANSPORT_SCTPDTLS) {
        ptr = sdp_getnextstrtok(ptr, port, sizeof(port), " \t", &result);
        if (result != SDP_SUCCESS) {
            sdp_parse_error(sdp_p->peerconnection,
                "%s No sctp port specified in m= media line, "
                "parse failed.", sdp_p->debug_str);
            SDP_FREE(mca_p);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        port_ptr = port;

        if (sdp_getchoosetok(port_ptr, &port_ptr, "/ \t", &result)) {
        	sctp_port = SDP_CHOOSE_PARAM;
        } else {
        	sctp_port = sdp_getnextnumtok(port_ptr, (const char **)&port_ptr,
                                           "/ \t", &result);
            if (result != SDP_SUCCESS) {
            	return (SDP_INVALID_PARAMETER);
            }
            mca_p->sctpport = sctp_port;
        }
    }

    
    sdp_p->mca_count++;
    if (sdp_p->mca_p == NULL) {
        sdp_p->mca_p = mca_p;
    } else {
        for (next_mca_p = sdp_p->mca_p; next_mca_p->next_p != NULL;
             next_mca_p = next_mca_p->next_p) {
            ; 
        }
        next_mca_p->next_p = mca_p;
    }

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {

        SDP_PRINT("%s Parsed media type %s, ", sdp_p->debug_str,
                  sdp_get_media_name(mca_p->media));
        switch (mca_p->port_format) {
        case SDP_PORT_NUM_ONLY:
            SDP_PRINT("Port num %ld, ", mca_p->port);
            break;

        case SDP_PORT_NUM_COUNT:
            SDP_PRINT("Port num %ld, count %ld, ",
                      mca_p->port, mca_p->num_ports);
            break;
        case SDP_PORT_VPI_VCI:
            SDP_PRINT("VPI/VCI %ld/%lu, ", mca_p->vpi, mca_p->vci);
            break;
        case SDP_PORT_VCCI:
            SDP_PRINT("VCCI %ld, ", mca_p->vcci);
            break;
        case SDP_PORT_NUM_VPI_VCI:
            SDP_PRINT("Port %ld, VPI/VCI %ld/%lu, ", mca_p->port,
                      mca_p->vpi, mca_p->vci);
            break;
        case SDP_PORT_VCCI_CID:
            SDP_PRINT("VCCI %ld, CID %ld, ", mca_p->vcci, mca_p->cid);
            break;
        case SDP_PORT_NUM_VPI_VCI_CID:
            SDP_PRINT("Port %ld, VPI/VCI %ld/%lu, CID %ld, ", mca_p->port,
                      mca_p->vpi, mca_p->vci, mca_p->cid);
            break;
        default:
            SDP_PRINT("Port format not valid, ");
            break;
        }

        if ((mca_p->transport >= SDP_TRANSPORT_AAL2_ITU) &&
            (mca_p->transport <= SDP_TRANSPORT_AAL2_CUSTOM)) {
            for (i=0; i < mca_p->media_profiles_p->num_profiles; i++) {
                SDP_PRINT("Profile %s, Num payloads %u ",
                   sdp_get_transport_name(mca_p->media_profiles_p->profile[i]),
                   mca_p->media_profiles_p->num_payloads[i]);
            }
        } else {
            SDP_PRINT("Transport %s, Num payloads %u",
                      sdp_get_transport_name(mca_p->transport),
                      mca_p->num_payloads);
        }
    }
    return (SDP_SUCCESS);
}

sdp_result_e sdp_build_media (sdp_t *sdp_p, u16 level, flex_string *fs)
{
    int                   i, j;
    sdp_mca_t            *mca_p;
    tinybool              invalid_params=FALSE;
    sdp_media_profiles_t *profile_p;

    
    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_FAILURE);
    }

    
    if ((mca_p->media >= SDP_MAX_MEDIA_TYPES) ||
        (mca_p->port_format >= SDP_MAX_PORT_FORMAT_TYPES) ||
        (mca_p->transport >= SDP_MAX_TRANSPORT_TYPES)) {
        invalid_params = TRUE;
    }

    if (invalid_params == TRUE) {
        CSFLogError(logTag, "%s Invalid params for m= media line, "
                    "build failed.", sdp_p->debug_str);
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    flex_string_sprintf(fs, "m=%s ", sdp_get_media_name(mca_p->media));

    
    if (mca_p->port_format == SDP_PORT_NUM_ONLY) {
        if (mca_p->port == SDP_CHOOSE_PARAM) {
            flex_string_sprintf(fs, "$ ");
        } else {
            flex_string_sprintf(fs, "%u ", (u16)mca_p->port);
        }
    } else if (mca_p->port_format == SDP_PORT_NUM_COUNT) {
        flex_string_sprintf(fs, "%u/%u ", (u16)mca_p->port,
                        (u16)mca_p->num_ports);
    } else if (mca_p->port_format == SDP_PORT_VPI_VCI) {
        flex_string_sprintf(fs, "%u/%u ",
                         (u16)mca_p->vpi, (u16)mca_p->vci);
    } else if (mca_p->port_format == SDP_PORT_VCCI) {
        flex_string_sprintf(fs, "%u ", (u16)mca_p->vcci);
    } else if (mca_p->port_format == SDP_PORT_NUM_VPI_VCI) {
        flex_string_sprintf(fs, "%u/%u/%u ", (u16)mca_p->port,
                         (u16)mca_p->vpi, (u16)mca_p->vci);
    } else if (mca_p->port_format == SDP_PORT_VCCI_CID) {
        if ((mca_p->vcci == SDP_CHOOSE_PARAM) &&
            (mca_p->cid == SDP_CHOOSE_PARAM)) {
            flex_string_sprintf(fs, "$/$ ");
        } else if ((mca_p->vcci == SDP_CHOOSE_PARAM) ||
                   (mca_p->cid == SDP_CHOOSE_PARAM)) {
            
            CSFLogError(logTag, "%s Invalid params for m= port parameter, "
                        "build failed.", sdp_p->debug_str);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        } else {
            flex_string_sprintf(fs, "%u/%u ",
                             (u16)mca_p->vcci, (u16)mca_p->cid);
        }
    } else if (mca_p->port_format == SDP_PORT_NUM_VPI_VCI_CID) {
        flex_string_sprintf(fs, "%u/%u/%u/%u ", (u16)mca_p->port,
                        (u16)mca_p->vpi, (u16)mca_p->vci, (u16)mca_p->cid);
    }

    
    if ((mca_p->transport == SDP_TRANSPORT_AAL2_ITU) ||
        (mca_p->transport == SDP_TRANSPORT_AAL2_ATMF) ||
        (mca_p->transport == SDP_TRANSPORT_AAL2_CUSTOM)) {
        profile_p = mca_p->media_profiles_p;
        for (i=0; i < profile_p->num_profiles; i++) {
            flex_string_sprintf(fs, "%s",
                             sdp_get_transport_name(profile_p->profile[i]));

            for (j=0; j < profile_p->num_payloads[i]; j++) {
                flex_string_sprintf(fs, " %u",
                                 profile_p->payload_type[i][j]);
            }
            flex_string_sprintf(fs, " ");
        }
        flex_string_sprintf(fs, "\n");
        if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
            SDP_PRINT("%s Built m= media line", sdp_p->debug_str);
        }
        return (SDP_SUCCESS);
    }

    
    flex_string_sprintf(fs, "%s",
                     sdp_get_transport_name(mca_p->transport));

    if(mca_p->transport != SDP_TRANSPORT_SCTPDTLS) {

        
        for (i=0; i < mca_p->num_payloads; i++) {
            if (mca_p->payload_indicator[i] == SDP_PAYLOAD_ENUM) {
                flex_string_sprintf(fs, " %s",
                                 sdp_get_payload_name((sdp_payload_e)mca_p->payload_type[i]));
            } else {
                flex_string_sprintf(fs, " %u", mca_p->payload_type[i]);
            }
        }
    } else {
        
    	flex_string_sprintf(fs, " %u ", (u32)mca_p->sctpport);
    }

    flex_string_sprintf(fs, "\r\n");

    if (sdp_p->debug_flag[SDP_DEBUG_TRACE]) {
        SDP_PRINT("%s Built m= media line", sdp_p->debug_str);
    }
    return (SDP_SUCCESS);
}











void sdp_parse_payload_types (sdp_t *sdp_p, sdp_mca_t *mca_p, const char *ptr)
{
    u16           i;
    u16           num_payloads;
    sdp_result_e  result;
    tinybool      valid_payload;
    char          tmp[SDP_MAX_STRING_LEN];
    char         *tmp2;

    for (num_payloads = 0; (num_payloads < SDP_MAX_PAYLOAD_TYPES); ) {
        ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
        if (result != SDP_SUCCESS) {
            
            break;
        }
        mca_p->payload_type[num_payloads] = (u16)sdp_getnextnumtok(tmp,
                                                        (const char **)&tmp2,
                                                        " \t", &result);
        if (result == SDP_SUCCESS) {
            if ((mca_p->media == SDP_MEDIA_IMAGE) &&
                (mca_p->transport == SDP_TRANSPORT_UDPTL)) {
                sdp_parse_error(sdp_p->peerconnection,
                    "%s Warning: Numeric payload type not "
                    "valid for media %s with transport %s.",
                    sdp_p->debug_str,
                    sdp_get_media_name(mca_p->media),
                    sdp_get_transport_name(mca_p->transport));
            } else {
                mca_p->payload_indicator[num_payloads] = SDP_PAYLOAD_NUMERIC;
                mca_p->num_payloads++;
                num_payloads++;
            }
            continue;
        }

        valid_payload = FALSE;
        for (i=0; i < SDP_MAX_STRING_PAYLOAD_TYPES; i++) {
            if (cpr_strncasecmp(tmp, sdp_payload[i].name,
                            sdp_payload[i].strlen) == 0) {
                valid_payload = TRUE;
                break;
            }
        }
        if (valid_payload == TRUE) {
            

            valid_payload = FALSE;
            if ((mca_p->media == SDP_MEDIA_IMAGE) &&
                (mca_p->transport == SDP_TRANSPORT_UDPTL) &&
                (i == SDP_PAYLOAD_T38)) {
                valid_payload = TRUE;
            } else if ((mca_p->media == SDP_MEDIA_APPLICATION) &&
                       (mca_p->transport == SDP_TRANSPORT_UDP) &&
                       (i == SDP_PAYLOAD_XTMR)) {
                valid_payload = TRUE;
            } else if ((mca_p->media == SDP_MEDIA_APPLICATION) &&
                       (mca_p->transport == SDP_TRANSPORT_TCP) &&
                       (i == SDP_PAYLOAD_T120)) {
                valid_payload = TRUE;
            }

            if (valid_payload == TRUE) {
                mca_p->payload_indicator[num_payloads] = SDP_PAYLOAD_ENUM;
                mca_p->payload_type[num_payloads] = i;
                mca_p->num_payloads++;
                num_payloads++;
            } else {
                sdp_parse_error(sdp_p->peerconnection,
                    "%s Warning: Payload type %s not valid for "
                    "media %s with transport %s.",
                    sdp_p->debug_str,
                    sdp_get_payload_name((sdp_payload_e)i),
                    sdp_get_media_name(mca_p->media),
                    sdp_get_transport_name(mca_p->transport));
            }
        } else {
            
            sdp_parse_error(sdp_p->peerconnection,
                "%s Warning: Payload type "
                "unsupported (%s).", sdp_p->debug_str, tmp);
        }
    }
    if (mca_p->num_payloads == 0) {
        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: No payload types specified.",
            sdp_p->debug_str);
    }
}











sdp_result_e sdp_parse_multiple_profile_payload_types (sdp_t *sdp_p,
                                                 sdp_mca_t *mca_p,
                                                 const char *ptr)
{
    u16                   i;
    u16                   prof;
    u16                   payload;
    sdp_result_e          result;
    sdp_media_profiles_t *profile_p;
    char                  tmp[SDP_MAX_STRING_LEN];
    char                 *tmp2;

    


    mca_p->media_profiles_p = (sdp_media_profiles_t *) \
        SDP_MALLOC(sizeof(sdp_media_profiles_t));
    if (mca_p->media_profiles_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        SDP_FREE(mca_p);
        return (SDP_NO_RESOURCE);
    }
    profile_p = mca_p->media_profiles_p;
    
    profile_p->num_profiles = 1;
    prof = 0;
    payload = 0;
    profile_p->profile[prof] = mca_p->transport;
    profile_p->num_payloads[prof] = 0;

    
    while (TRUE) {
        ptr = sdp_getnextstrtok(ptr, tmp, sizeof(tmp), " \t", &result);
        if (result != SDP_SUCCESS) {
            
            break;
        }

        
        if (prof < SDP_MAX_PROFILES) {
            profile_p->profile[prof+1] = SDP_TRANSPORT_UNSUPPORTED;
            for (i=SDP_TRANSPORT_AAL2_ITU;
                 i <= SDP_TRANSPORT_AAL2_CUSTOM; i++) {
                if (cpr_strncasecmp(tmp, sdp_transport[i].name,
                                sdp_transport[i].strlen) == 0) {
                    profile_p->profile[prof+1] = (sdp_transport_e)i;
                    break;
                }
            }
            

            if (profile_p->profile[prof+1] != SDP_TRANSPORT_UNSUPPORTED) {
                
                payload = 0;
                prof++;
                profile_p->num_profiles++;
                if (prof < SDP_MAX_PROFILES) {
                    profile_p->num_payloads[prof] = 0;
                }
                continue;
            }
        }

        

        if (payload >= SDP_MAX_PAYLOAD_TYPES) {
            sdp_parse_error(sdp_p->peerconnection,
                "%s Warning: Too many payload types "
                "found, truncating.", sdp_p->debug_str);
            continue;
        }

        
        if (prof < SDP_MAX_PROFILES && payload < SDP_MAX_PAYLOAD_TYPES) {
            profile_p->payload_type[prof][payload] = (u16)sdp_getnextnumtok(tmp,
                                                             (const char **)&tmp2,
                                                             " \t", &result);
            if (result == SDP_SUCCESS) {
                profile_p->payload_indicator[prof][payload] = SDP_PAYLOAD_NUMERIC;
                profile_p->num_payloads[prof]++;
                payload++;
                continue;
            }
        }

        

        sdp_parse_error(sdp_p->peerconnection,
            "%s Warning: Unsupported payload type "
            "found (%s).", sdp_p->debug_str, tmp);
    }
    for (i=0; i < profile_p->num_profiles; i++) {
        
        if (profile_p->num_payloads[i] == 0) {
            sdp_parse_error(sdp_p->peerconnection,
                "%s Warning: No payload types specified "
                "for AAL2 profile %s.", sdp_p->debug_str,
                sdp_get_transport_name(profile_p->profile[i]));
        }
    }
    return (SDP_SUCCESS);
}
