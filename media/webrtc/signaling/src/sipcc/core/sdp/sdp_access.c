



#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"
#include "ccsip_sdp.h"
#include "rtp_defs.h"
#include "CSFLog.h"

static const char* logTag = "sdp_access";










sdp_mca_t *sdp_find_media_level (sdp_t *sdp_p, u16 level)
{
    int i;
    sdp_mca_t *mca_p = NULL;

    if ((level >= 1) && (level <= sdp_p->mca_count)) {
        for (i=1, mca_p = sdp_p->mca_p;
             ((i < level) && (mca_p != NULL));
             mca_p = mca_p->next_p, i++) {

             
             ; 
        }
    }

    return (mca_p);
}








tinybool sdp_version_valid (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    if (sdp_p->version == SDP_INVALID_VALUE) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}






int32 sdp_get_version (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    return (sdp_p->version);
}








sdp_result_e sdp_set_version (void *sdp_ptr, int32 version)
{
    sdp_t *sdp_p = (sdp_t*)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sdp_p->version = version;
    return (SDP_SUCCESS);
}







tinybool sdp_owner_valid (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    if ((sdp_p->owner_name[0] == '\0') ||
        (sdp_p->owner_network_type == SDP_NT_INVALID) ||
        (sdp_p->owner_addr_type == SDP_AT_INVALID) ||
        (sdp_p->owner_addr[0] == '\0')) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}








const char *sdp_get_owner_username (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    return (sdp_p->owner_name);
}










const char *sdp_get_owner_sessionid (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    return (sdp_p->owner_sessid);
}










const char *sdp_get_owner_version (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    return (sdp_p->owner_version);
}








sdp_nettype_e sdp_get_owner_network_type (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_NT_INVALID);
    }

    return (sdp_p->owner_network_type);
}








sdp_addrtype_e sdp_get_owner_address_type (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_AT_INVALID);
    }

    return (sdp_p->owner_addr_type);
}








const char *sdp_get_owner_address (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    return (sdp_p->owner_addr);
}









sdp_result_e sdp_set_owner_username (void *sdp_ptr, const char *username)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sstrncpy(sdp_p->owner_name, username, sizeof(sdp_p->owner_name));
    return (SDP_SUCCESS);
}









sdp_result_e sdp_set_owner_sessionid (void *sdp_ptr, const char *sessionid)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sstrncpy(sdp_p->owner_sessid, sessionid, sizeof(sdp_p->owner_sessid));
    return (SDP_SUCCESS);
}









sdp_result_e sdp_set_owner_version (void *sdp_ptr, const char *version)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sstrncpy(sdp_p->owner_version, version, sizeof(sdp_p->owner_version));
    return (SDP_SUCCESS);
}








sdp_result_e sdp_set_owner_network_type (void *sdp_ptr,
                                         sdp_nettype_e network_type)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sdp_p->owner_network_type = network_type;
    return (SDP_SUCCESS);
}








sdp_result_e sdp_set_owner_address_type (void *sdp_ptr,
                                         sdp_addrtype_e address_type)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sdp_p->owner_addr_type = address_type;
    return (SDP_SUCCESS);
}









sdp_result_e sdp_set_owner_address (void *sdp_ptr, const char *address)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sstrncpy(sdp_p->owner_addr, address, sizeof(sdp_p->owner_addr));
    return (SDP_SUCCESS);
}







tinybool sdp_session_name_valid (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    if (sdp_p->sessname[0] == '\0') {
        return (FALSE);
    } else {
        return (TRUE);
    }
}








const char *sdp_get_session_name (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    return (sdp_p->sessname);
}










sdp_result_e sdp_set_session_name (void *sdp_ptr, const char *sessname)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    sstrncpy(sdp_p->sessname, sessname, sizeof(sdp_p->sessname));
    return (SDP_SUCCESS);
}







tinybool sdp_timespec_valid (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    if ((sdp_p->timespec_p == NULL) ||
        (sdp_p->timespec_p->start_time == '\0') ||
        (sdp_p->timespec_p->stop_time == '\0')) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}










const char *sdp_get_time_start (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    if (sdp_p->timespec_p != NULL) {
        return (sdp_p->timespec_p->start_time);
    } else {
        return (NULL);
    }
}










const char *sdp_get_time_stop (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    if (sdp_p->timespec_p != NULL) {
        return (sdp_p->timespec_p->stop_time);
    } else {
        return (NULL);
    }
}










sdp_result_e sdp_set_time_start (void *sdp_ptr, const char *start_time)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (sdp_p->timespec_p == NULL) {
        sdp_p->timespec_p = (sdp_timespec_t *)SDP_MALLOC(sizeof(sdp_timespec_t));
        if (sdp_p->timespec_p == NULL) {
            sdp_p->conf_p->num_no_resource++;
            return (SDP_NO_RESOURCE);
        }
        sdp_p->timespec_p->start_time[0] = '\0';
        sdp_p->timespec_p->stop_time[0] = '\0';
    }
    sstrncpy(sdp_p->timespec_p->start_time, start_time,
             sizeof(sdp_p->timespec_p->start_time));
    return (SDP_SUCCESS);
}










sdp_result_e sdp_set_time_stop (void *sdp_ptr, const char *stop_time)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (sdp_p->timespec_p == NULL) {
        sdp_p->timespec_p = (sdp_timespec_t *)SDP_MALLOC(sizeof(sdp_timespec_t));
        if (sdp_p->timespec_p == NULL) {
            sdp_p->conf_p->num_no_resource++;
            return (SDP_NO_RESOURCE);
        }
        sdp_p->timespec_p->start_time[0] = '\0';
        sdp_p->timespec_p->stop_time[0] = '\0';
    }
    sstrncpy(sdp_p->timespec_p->stop_time, stop_time,
             sizeof(sdp_p->timespec_p->stop_time));
    return (SDP_SUCCESS);
}










tinybool sdp_encryption_valid (void *sdp_ptr, u16 level)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_encryptspec_t   *encrypt_p;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    if (level == SDP_SESSION_LEVEL) {
        encrypt_p = &(sdp_p->encrypt);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (FALSE);
        }
        encrypt_p = &(mca_p->encrypt);
    }

    if ((encrypt_p->encrypt_type == SDP_ENCRYPT_INVALID) ||
        ((encrypt_p->encrypt_type != SDP_ENCRYPT_PROMPT) &&
         (encrypt_p->encrypt_key[0] == '\0'))) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}











sdp_encrypt_type_e sdp_get_encryption_method (void *sdp_ptr, u16 level)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_encryptspec_t   *encrypt_p;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_ENCRYPT_INVALID);
    }

    if (level == SDP_SESSION_LEVEL) {
        encrypt_p = &(sdp_p->encrypt);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_ENCRYPT_INVALID);
        }
        encrypt_p = &(mca_p->encrypt);
    }

    return (encrypt_p->encrypt_type);
}











const char *sdp_get_encryption_key (void *sdp_ptr, u16 level)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_encryptspec_t   *encrypt_p;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    if (level == SDP_SESSION_LEVEL) {
        encrypt_p = &(sdp_p->encrypt);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (NULL);
        }
        encrypt_p = &(mca_p->encrypt);
    }

    return (encrypt_p->encrypt_key);
}











sdp_result_e sdp_set_encryption_method (void *sdp_ptr, u16 level,
                                        sdp_encrypt_type_e type)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_encryptspec_t   *encrypt_p;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        encrypt_p = &(sdp_p->encrypt);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        encrypt_p = &(mca_p->encrypt);
    }

    encrypt_p->encrypt_type = type;
    return (SDP_SUCCESS);
}













sdp_result_e sdp_set_encryption_key (void *sdp_ptr, u16 level, const char *key)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_encryptspec_t   *encrypt_p;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        encrypt_p = &(sdp_p->encrypt);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        encrypt_p = &(mca_p->encrypt);
    }

    sstrncpy(encrypt_p->encrypt_key, key, sizeof(encrypt_p->encrypt_key));
    return (SDP_SUCCESS);
}










tinybool sdp_connection_valid (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (FALSE);
        }
        conn_p = &(mca_p->conn);
    }

    


    if (conn_p->nettype == SDP_NT_ATM &&
        conn_p->addrtype == SDP_AT_INVALID) {
        return TRUE;
    }

    if ((conn_p->nettype >= SDP_MAX_NETWORK_TYPES) ||
        (conn_p->addrtype >= SDP_MAX_ADDR_TYPES) ||
        (conn_p->conn_addr[0] == '\0')) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}












tinybool sdp_bandwidth_valid (void *sdp_ptr, u16 level, u16 inst_num)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_data_t *bw_data_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return FALSE;
    }

    bw_data_p = sdp_find_bw_line(sdp_p, level, inst_num);
    if (bw_data_p != NULL) {
        if ((bw_data_p->bw_modifier < SDP_BW_MODIFIER_AS) ||
            (bw_data_p->bw_modifier >= SDP_MAX_BW_MODIFIER_VAL)) {
            return FALSE;
        } else {
            return TRUE;
        }
    } else {
        return FALSE;
    }
}














tinybool sdp_bw_line_exists (void *sdp_ptr, u16 level, u16 inst_num)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_data_t *bw_data_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return FALSE;
    }

    bw_data_p = sdp_find_bw_line(sdp_p, level, inst_num);
    if (bw_data_p != NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}











sdp_nettype_e sdp_get_conn_nettype (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_NT_INVALID);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_NT_INVALID);
        }
        conn_p = &(mca_p->conn);
    }

    return (conn_p->nettype);
}











sdp_addrtype_e sdp_get_conn_addrtype (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_AT_INVALID);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (SDP_AT_INVALID);
        }
        conn_p = &(mca_p->conn);
    }

    return (conn_p->addrtype);
}











const char *sdp_get_conn_address (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            return (NULL);
        }
        conn_p = &(mca_p->conn);
    }

    return (conn_p->conn_addr);
}











tinybool sdp_is_mcast_addr (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p != NULL) {
            conn_p = &(mca_p->conn);
	} else {
            return (FALSE);
	}
    }

    if ((conn_p) && (conn_p->is_multicast)) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}











int32 sdp_get_mcast_ttl (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;
    u16 ttl=0;

    if (sdp_verify_sdp_ptr(sdp_p) != FALSE) {
        if (level == SDP_SESSION_LEVEL) {
            conn_p = &(sdp_p->default_conn);
        } else {
            mca_p = sdp_find_media_level(sdp_p, level);
            if (mca_p != NULL) {
                conn_p = &(mca_p->conn);
            } else {
                return SDP_INVALID_VALUE;
            }
        }
    } else {
        return SDP_INVALID_VALUE;
    }

    if (conn_p) {
	ttl = conn_p->ttl;
    }
    return ttl;
}











int32 sdp_get_mcast_num_of_addresses (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;
    u16 num_addr = 0;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    } else {
        if (level == SDP_SESSION_LEVEL) {
            conn_p = &(sdp_p->default_conn);
        } else {
            mca_p = sdp_find_media_level(sdp_p, level);
            if (mca_p != NULL) {
                conn_p = &(mca_p->conn);
            } else {
                return (SDP_INVALID_VALUE);
	    }
        }
    }

    if (conn_p) {
	num_addr = conn_p->num_of_addresses;
    }
    return num_addr;
}










sdp_result_e sdp_set_conn_nettype (void *sdp_ptr, u16 level,
                                   sdp_nettype_e nettype)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        conn_p = &(mca_p->conn);
    }

    conn_p->nettype = nettype;
    return (SDP_SUCCESS);
}











sdp_result_e sdp_set_conn_addrtype (void *sdp_ptr, u16 level,
                                    sdp_addrtype_e addrtype)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        conn_p = &(mca_p->conn);
    }

    conn_p->addrtype = addrtype;
    return (SDP_SUCCESS);
}













sdp_result_e sdp_set_conn_address (void *sdp_ptr, u16 level,
                                   const char *address)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        conn_p = &(mca_p->conn);
    }

    sstrncpy(conn_p->conn_addr, address, sizeof(conn_p->conn_addr));
    return (SDP_SUCCESS);
}













sdp_result_e sdp_set_mcast_addr_fields(void *sdp_ptr, u16 level,
				       u16 ttl, u16 num_of_addresses)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_conn_t *conn_p;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        conn_p = &(sdp_p->default_conn);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        conn_p = &(mca_p->conn);
    }

    if (conn_p) {
       conn_p->is_multicast = TRUE;
       if ((conn_p->ttl >0) && (conn_p->ttl <= SDP_MAX_TTL_VALUE)) {
          conn_p->ttl = ttl;
       }
       conn_p->num_of_addresses = num_of_addresses;
    } else {
       return (SDP_FAILURE);
    }
    return (SDP_SUCCESS);
}












tinybool sdp_media_line_valid (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (FALSE);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (FALSE);
    }

    
    if ((mca_p->media >= SDP_MAX_MEDIA_TYPES) ||
        (mca_p->port_format >= SDP_MAX_PORT_FORMAT_TYPES) ||
        (mca_p->transport >= SDP_MAX_TRANSPORT_TYPES) ||
        (mca_p->num_payloads == 0)) {
        return (FALSE);
    } else {
        return (TRUE);
    }
}






u16 sdp_get_num_media_lines (void *sdp_ptr)
{
    sdp_t *sdp_p = (sdp_t *)sdp_ptr;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    return (sdp_p->mca_count);
}









sdp_media_e sdp_get_media_type (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_MEDIA_INVALID);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_MEDIA_INVALID);
    }

    return (mca_p->media);
}









sdp_port_format_e sdp_get_media_port_format (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_PORT_FORMAT_INVALID);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_PORT_FORMAT_INVALID);
    }

    return (mca_p->port_format);
}









int32 sdp_get_media_portnum (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_INVALID_VALUE);
    }

    
    if ((mca_p->port_format != SDP_PORT_NUM_ONLY) &&
        (mca_p->port_format != SDP_PORT_NUM_COUNT) &&
        (mca_p->port_format != SDP_PORT_NUM_VPI_VCI) &&
        (mca_p->port_format != SDP_PORT_NUM_VPI_VCI_CID)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Port num not valid for media line %u",
                      sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    }

    return (mca_p->port);
}









int32 sdp_get_media_portcount (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_INVALID_VALUE);
    }

    
    if (mca_p->port_format != SDP_PORT_NUM_COUNT) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Port count not valid for media line %u",
                      sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    }

    return (mca_p->num_ports);
}









int32 sdp_get_media_vpi (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_INVALID_VALUE);
    }

    
    if ((mca_p->port_format != SDP_PORT_VPI_VCI) &&
        (mca_p->port_format != SDP_PORT_NUM_VPI_VCI) &&
        (mca_p->port_format != SDP_PORT_NUM_VPI_VCI_CID)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s VPI not valid for media line %u",
                      sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    }

    return (mca_p->vpi);
}









u32 sdp_get_media_vci (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (0);
    }

    
    if ((mca_p->port_format != SDP_PORT_VPI_VCI) &&
        (mca_p->port_format != SDP_PORT_NUM_VPI_VCI) &&
        (mca_p->port_format != SDP_PORT_NUM_VPI_VCI_CID)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s VCI not valid for media line %u",
                      sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (0);
    }

    return (mca_p->vci);
}









int32 sdp_get_media_vcci (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_INVALID_VALUE);
    }

    
    if ((mca_p->port_format != SDP_PORT_VCCI) &&
        (mca_p->port_format != SDP_PORT_VCCI_CID)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s VCCI not valid for media line %u",
                      sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    }

    return (mca_p->vcci);
}









int32 sdp_get_media_cid (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_INVALID_VALUE);
    }

    
    if ((mca_p->port_format != SDP_PORT_VCCI_CID) &&
        (mca_p->port_format != SDP_PORT_NUM_VPI_VCI_CID)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s CID not valid for media line %u",
                      sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    }

    return (mca_p->cid);
}












sdp_transport_e sdp_get_media_transport (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_TRANSPORT_INVALID);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_TRANSPORT_INVALID);
    }

    return (mca_p->transport);
}














u16 sdp_get_media_num_profiles (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (0);
    }

    if (mca_p->media_profiles_p == NULL) {
        return (0);
    } else {
        return (mca_p->media_profiles_p->num_profiles);
    }
}












sdp_transport_e sdp_get_media_profile (void *sdp_ptr, u16 level,
                                       u16 profile_num)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_TRANSPORT_INVALID);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (SDP_TRANSPORT_INVALID);
    }

    if ((profile_num < 1) ||
        (profile_num > mca_p->media_profiles_p->num_profiles)) {
        return (SDP_TRANSPORT_INVALID);
    } else {
        return (mca_p->media_profiles_p->profile[profile_num-1]);
    }
}










u16 sdp_get_media_num_payload_types (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (0);
    }

    return (mca_p->num_payloads);
}












u16 sdp_get_media_profile_num_payload_types (void *sdp_ptr, u16 level,
                                             u16 profile_num)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (0);
    }

    if ((profile_num < 1) ||
        (profile_num > mca_p->media_profiles_p->num_profiles)) {
        return (0);
    } else {
        return (mca_p->media_profiles_p->num_payloads[profile_num-1]);
    }
}














u32 sdp_get_media_payload_type (void *sdp_ptr, u16 level, u16 payload_num,
                                sdp_payload_ind_e *indicator)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;
    uint16_t    num_a_lines = 0;
    int         i;
    uint16_t    ptype;
    uint16_t    pack_mode = 0; 
    const char *encname = NULL;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (0);
    }

    if ((payload_num < 1) || (payload_num > mca_p->num_payloads)) {
        return (0);
    }

    *indicator = mca_p->payload_indicator[payload_num-1];
    if ((mca_p->payload_type[payload_num-1] >= SDP_MIN_DYNAMIC_PAYLOAD) &&
        (mca_p->payload_type[payload_num-1] <= SDP_MAX_DYNAMIC_PAYLOAD)) {
        


        (void) sdp_attr_num_instances(sdp_p, level, 0, SDP_ATTR_RTPMAP,
                                      &num_a_lines);
        



        for (i = 0; i < num_a_lines; i++) {
            ptype = sdp_attr_get_rtpmap_payload_type(sdp_p, level, 0,
                                                     (uint16_t) (i + 1));
            if (ptype == mca_p->payload_type[payload_num-1] ) {
                encname = sdp_attr_get_rtpmap_encname(sdp_p, level, 0,
                                                  (uint16_t) (i + 1));
                if (encname) {
                    if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_ILBC) == 0) {
                        return (SET_PAYLOAD_TYPE_WITH_DYNAMIC(ptype, RTP_ILBC));
                    }
                    if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_L16_256K) == 0) {
                        return (SET_PAYLOAD_TYPE_WITH_DYNAMIC(ptype, RTP_L16));
                    }
                    if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_ISAC) == 0) {
                        return (SET_PAYLOAD_TYPE_WITH_DYNAMIC(ptype, RTP_ISAC));
                    }
                    if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_OPUS) == 0) {
                        return (SET_PAYLOAD_TYPE_WITH_DYNAMIC(ptype, RTP_OPUS));
                    }
                    if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_H264) == 0) {
                        sdp_attr_get_fmtp_pack_mode(sdp_p, level, 0, (uint16_t) (i + 1), &pack_mode);
                        if (pack_mode == SDP_DEFAULT_PACKETIZATION_MODE_VALUE) {
                            return (SET_PAYLOAD_TYPE_WITH_DYNAMIC(ptype, RTP_H264_P0));
                        } else {
                            return (SET_PAYLOAD_TYPE_WITH_DYNAMIC(ptype, RTP_H264_P1));
                        }
                    }
                    if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_VP8) == 0) {
                        return (SET_PAYLOAD_TYPE_WITH_DYNAMIC(ptype, RTP_VP8));
                    }
                }
            }
        }
    }
    return (mca_p->payload_type[payload_num-1]);
}














u32 sdp_get_media_profile_payload_type (void *sdp_ptr, u16 level, u16 prof_num,
                                        u16 payload_num,
                                        sdp_payload_ind_e *indicator)
{
    sdp_t                *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t            *mca_p;
    sdp_media_profiles_t *prof_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (0);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        return (0);
    }

    prof_p = mca_p->media_profiles_p;
    if ((prof_num < 1) ||
        (prof_num > prof_p->num_profiles)) {
        return (0);
    }

    if ((payload_num < 1) ||
        (payload_num > prof_p->num_payloads[prof_num-1])) {
        return (0);
    }

    *indicator = prof_p->payload_indicator[prof_num-1][payload_num-1];
    return (prof_p->payload_type[prof_num-1][payload_num-1]);
}








sdp_result_e sdp_insert_media_line (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;
    sdp_mca_t  *new_mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if ((level < 1) || (level > (sdp_p->mca_count+1))) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Invalid media line (%u) to insert, max is "
                      "(%u).", sdp_p->debug_str, level, sdp_p->mca_count);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    
    new_mca_p = sdp_alloc_mca();
    if (new_mca_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        return (SDP_NO_RESOURCE);
    }

    if (level == 1) {
        
        new_mca_p->next_p = sdp_p->mca_p;
        sdp_p->mca_p = new_mca_p;
    } else {
        


        mca_p = sdp_find_media_level(sdp_p, (u16)(level-1));
        if (mca_p == NULL) {
            SDP_FREE(new_mca_p);
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }

        new_mca_p->next_p = mca_p->next_p;
        mca_p->next_p = new_mca_p;
    }

    sdp_p->mca_count++;
    return (SDP_SUCCESS);
}








void sdp_delete_media_line (void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;
    sdp_mca_t  *prev_mca_p = NULL;
    sdp_attr_t *attr_p;
    sdp_attr_t *next_attr_p;
    sdp_bw_t        *bw_p;
    sdp_bw_data_t   *bw_data_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return;
    }

    

    if (level == 1) {
        mca_p = sdp_find_media_level(sdp_p, level);
    } else {
        prev_mca_p = sdp_find_media_level(sdp_p, (u16)(level-1));
        if (prev_mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return;
        }
        mca_p = prev_mca_p->next_p;
    }
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return;
    }

    
    for (attr_p = mca_p->media_attrs_p; attr_p != NULL;) {
        next_attr_p = attr_p->next_p;
        sdp_free_attr(attr_p);
        attr_p = next_attr_p;
    }

     
     bw_p = &(mca_p->bw);
     bw_data_p = bw_p->bw_data_list;
     while (bw_data_p != NULL) {
         bw_p->bw_data_list = bw_data_p->next_p;
         SDP_FREE(bw_data_p);
         bw_data_p = bw_p->bw_data_list;
     }

    
    if (prev_mca_p == NULL) {
        sdp_p->mca_p = mca_p->next_p;
    } else {
        prev_mca_p->next_p = mca_p->next_p;
    }
    SDP_FREE(mca_p);
    sdp_p->mca_count--;
    return;
}









sdp_result_e sdp_set_media_type (void *sdp_ptr, u16 level, sdp_media_e media)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->media = media;
    return (SDP_SUCCESS);
}












sdp_result_e sdp_set_media_port_format (void *sdp_ptr, u16 level,
                                        sdp_port_format_e port_format)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->port_format = port_format;
    return (SDP_SUCCESS);
}












sdp_result_e sdp_set_media_portnum (void *sdp_ptr, u16 level, int32 portnum, int32 sctp_port)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->port = portnum;
    mca_p->sctpport = sctp_port;
    return (SDP_SUCCESS);
}








int32 sdp_get_media_sctp_port(void *sdp_ptr, u16 level)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (!sdp_verify_sdp_ptr(sdp_p)) {
        return -1;
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (!mca_p) {
        sdp_p->conf_p->num_invalid_param++;
        return -1;
    }

    return mca_p->sctpport;
}











sdp_result_e sdp_set_media_portcount (void *sdp_ptr, u16 level,
                                      int32 num_ports)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->num_ports = num_ports;
    return (SDP_SUCCESS);
}











sdp_result_e sdp_set_media_vpi (void *sdp_ptr, u16 level, int32 vpi)
{
    sdp_t      *sdp_p = (sdp_t*)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->vpi = vpi;
    return (SDP_SUCCESS);
}











sdp_result_e sdp_set_media_vci (void *sdp_ptr, u16 level, u32 vci)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->vci = vci;
    return (SDP_SUCCESS);
}











sdp_result_e sdp_set_media_vcci (void *sdp_ptr, u16 level, int32 vcci)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->vcci = vcci;
    return (SDP_SUCCESS);
}











sdp_result_e sdp_set_media_cid (void *sdp_ptr, u16 level, int32 cid)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->cid = cid;
    return (SDP_SUCCESS);
}









sdp_result_e sdp_set_media_transport (void *sdp_ptr, u16 level,
                                      sdp_transport_e transport)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->transport = transport;
    return (SDP_SUCCESS);
}











sdp_result_e sdp_add_media_profile (void *sdp_ptr, u16 level,
                                    sdp_transport_e profile)
{
    u16         prof_num;
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (mca_p->media_profiles_p == NULL) {
        mca_p->media_profiles_p = (sdp_media_profiles_t *) \
            SDP_MALLOC(sizeof(sdp_media_profiles_t));
        if (mca_p->media_profiles_p == NULL) {
            sdp_p->conf_p->num_no_resource++;
            return (SDP_NO_RESOURCE);
        } else {
            mca_p->media_profiles_p->num_profiles = 0;
            
            mca_p->transport = profile;
        }
    }

    if (mca_p->media_profiles_p->num_profiles >= SDP_MAX_PROFILES) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Max number of media profiles already specified"
                      " for media level %u", sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    prof_num = mca_p->media_profiles_p->num_profiles++;
    mca_p->media_profiles_p->profile[prof_num] = profile;
    mca_p->media_profiles_p->num_payloads[prof_num] = 0;
    return (SDP_SUCCESS);
}












sdp_result_e sdp_add_media_payload_type (void *sdp_ptr, u16 level,
                                         u16 payload_type,
                                         sdp_payload_ind_e indicator)
{
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (mca_p->num_payloads == SDP_MAX_PAYLOAD_TYPES) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Max number of payload types already defined "
                      "for media line %u", sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    mca_p->payload_indicator[mca_p->num_payloads] = indicator;
    mca_p->payload_type[mca_p->num_payloads++] = payload_type;
    return (SDP_SUCCESS);
}













sdp_result_e sdp_add_media_profile_payload_type (void *sdp_ptr, u16 level,
                                                u16 prof_num, u16 payload_type,
                                                sdp_payload_ind_e indicator)
{
    u16         num_payloads;
    sdp_t      *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t  *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if ((prof_num < 1) ||
        (prof_num > mca_p->media_profiles_p->num_profiles)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Invalid profile number (%u) for set profile "
                      " payload type", sdp_p->debug_str, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (mca_p->media_profiles_p->num_payloads[prof_num-1] ==
        SDP_MAX_PAYLOAD_TYPES) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Max number of profile payload types already "
                      "defined profile %u on media line %u",
                      sdp_p->debug_str, prof_num, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    

    num_payloads = mca_p->media_profiles_p->num_payloads[prof_num-1]++;
    mca_p->media_profiles_p->payload_indicator[prof_num-1][num_payloads] =
                                                           indicator;
    mca_p->media_profiles_p->payload_type[prof_num-1][num_payloads] =
                                                           payload_type;
    return (SDP_SUCCESS);
}









sdp_bw_data_t* sdp_find_bw_line (void *sdp_ptr, u16 level, u16 inst_num)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_t            *bw_p;
    sdp_bw_data_t       *bw_data_p;
    sdp_mca_t           *mca_p;
    int                 bw_attr_count=0;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (NULL);
    }

    if (level == SDP_SESSION_LEVEL) {
        bw_p = &(sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (NULL);
        }
        bw_p = &(mca_p->bw);
    }

    for (bw_data_p = bw_p->bw_data_list;
         bw_data_p != NULL;
         bw_data_p = bw_data_p->next_p) {
        bw_attr_count++;
        if (bw_attr_count == inst_num) {
            return bw_data_p;
        }
    }

    return NULL;
}















sdp_result_e sdp_copy_all_bw_lines (void *src_sdp_ptr, void *dst_sdp_ptr,
                                    u16 src_level, u16 dst_level)
{
    sdp_t                *src_sdp_p = (sdp_t *)src_sdp_ptr;
    sdp_t                *dst_sdp_p = (sdp_t *)dst_sdp_ptr;
    sdp_bw_data_t        *orig_bw_data_p;
    sdp_bw_data_t        *new_bw_data_p;
    sdp_bw_data_t        *bw_data_p;
    sdp_bw_t             *src_bw_p;
    sdp_bw_t             *dst_bw_p;
    sdp_mca_t            *mca_p;

    if (sdp_verify_sdp_ptr(src_sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (sdp_verify_sdp_ptr(dst_sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    
    if (src_level == SDP_SESSION_LEVEL) {
        src_bw_p = &(src_sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(src_sdp_p, src_level);
        if (mca_p == NULL) {
            if (src_sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s Invalid src media level (%u) for copy all "
                          "attrs ", src_sdp_p->debug_str, src_level);
            }
            return (SDP_INVALID_PARAMETER);
        }
        src_bw_p = &(mca_p->bw);
    }

    
    if (dst_level == SDP_SESSION_LEVEL) {
        dst_bw_p = &(dst_sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(dst_sdp_p, dst_level);
        if (mca_p == NULL) {
            if (src_sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
                CSFLogError(logTag, "%s Invalid dst media level (%u) for copy all "
                          "attrs ", src_sdp_p->debug_str, dst_level);
            }
            return (SDP_INVALID_PARAMETER);
        }
        dst_bw_p = &(mca_p->bw);
    }

    orig_bw_data_p = src_bw_p->bw_data_list;
    while (orig_bw_data_p) {
        
        new_bw_data_p = (sdp_bw_data_t*)SDP_MALLOC(sizeof(sdp_bw_data_t));
        if (new_bw_data_p == NULL) {
            return (SDP_NO_RESOURCE);
        }
        new_bw_data_p->next_p = NULL;
        new_bw_data_p->bw_modifier = orig_bw_data_p->bw_modifier;
        new_bw_data_p->bw_val = orig_bw_data_p->bw_val;

        



        if (dst_bw_p->bw_data_list == NULL) {
            dst_bw_p->bw_data_list = new_bw_data_p;
        } else {
            for (bw_data_p = dst_bw_p->bw_data_list;
                 bw_data_p->next_p != NULL;
                 bw_data_p = bw_data_p->next_p) {

                
                ; 
            }

            bw_data_p->next_p = new_bw_data_p;
        }
        dst_bw_p->bw_data_count++;

        orig_bw_data_p = orig_bw_data_p->next_p;
    }

    return (SDP_SUCCESS);
}











sdp_bw_modifier_e sdp_get_bw_modifier (void *sdp_ptr, u16 level, u16 inst_num)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_data_t       *bw_data_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_BW_MODIFIER_UNSUPPORTED);
    }

    bw_data_p = sdp_find_bw_line(sdp_p, level, inst_num);

    if (bw_data_p) {
        return (bw_data_p->bw_modifier);
    } else {
        return (SDP_BW_MODIFIER_UNSUPPORTED);
    }
}










int32 sdp_get_bw_value (void *sdp_ptr, u16 level, u16 inst_num)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_data_t       *bw_data_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    bw_data_p = sdp_find_bw_line(sdp_p, level, inst_num);

    if (bw_data_p) {
        return (bw_data_p->bw_val);
    } else {
        return (SDP_INVALID_VALUE);
    }
}











int32 sdp_get_num_bw_lines (void *sdp_ptr, u16 level)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_t            *bw_p;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    if (level == SDP_SESSION_LEVEL) {
        bw_p = &(sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_VALUE);
        }
        bw_p = &(mca_p->bw);
    }

    return (bw_p->bw_data_count);
}



































sdp_result_e sdp_add_new_bw_line (void *sdp_ptr, u16 level, sdp_bw_modifier_e bw_modifier, u16 *inst_num)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_t            *bw_p;
    sdp_mca_t           *mca_p;
    sdp_bw_data_t       *new_bw_data_p;
    sdp_bw_data_t       *bw_data_p = NULL;

    *inst_num = 0;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        bw_p = &(sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        bw_p = &(mca_p->bw);
    }

    
    for(bw_data_p = bw_p->bw_data_list; bw_data_p != NULL; bw_data_p = bw_data_p->next_p) {
        ++(*inst_num);
        if (bw_data_p->bw_modifier == bw_modifier) {
            return (SDP_SUCCESS);
        }
    }

    



    new_bw_data_p = (sdp_bw_data_t*)SDP_MALLOC(sizeof(sdp_bw_data_t));
    if (new_bw_data_p == NULL) {
        sdp_p->conf_p->num_no_resource++;
        return (SDP_NO_RESOURCE);
    }
    new_bw_data_p->next_p = NULL;
    new_bw_data_p->bw_modifier = SDP_BW_MODIFIER_UNSUPPORTED;
    new_bw_data_p->bw_val = 0;

    



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
    *inst_num = ++bw_p->bw_data_count;

    return (SDP_SUCCESS);
}










sdp_result_e sdp_delete_bw_line (void *sdp_ptr, u16 level, u16 inst_num)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_t            *bw_p;
    sdp_mca_t           *mca_p;
    sdp_bw_data_t       *bw_data_p = NULL;
    sdp_bw_data_t       *prev_bw_data_p = NULL;
    int                 bw_data_count = 0;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if (level == SDP_SESSION_LEVEL) {
        bw_p = &(sdp_p->bw);
    } else {
        mca_p = sdp_find_media_level(sdp_p, level);
        if (mca_p == NULL) {
            sdp_p->conf_p->num_invalid_param++;
            return (SDP_INVALID_PARAMETER);
        }
        bw_p = &(mca_p->bw);
    }

    bw_data_p = bw_p->bw_data_list;
    while (bw_data_p != NULL) {
        bw_data_count++;
        if (bw_data_count == inst_num) {
            break;
        }

        prev_bw_data_p = bw_data_p;
        bw_data_p = bw_data_p->next_p;
    }

    if (bw_data_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s bw line instance %d not found.",
                      sdp_p->debug_str, inst_num);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    if (prev_bw_data_p == NULL) {
        bw_p->bw_data_list = bw_data_p->next_p;
    } else {
        prev_bw_data_p->next_p = bw_data_p->next_p;
    }
    bw_p->bw_data_count--;

    SDP_FREE(bw_data_p);
    return (SDP_SUCCESS);
}

















sdp_result_e sdp_set_bw (void *sdp_ptr, u16 level, u16 inst_num,
                         sdp_bw_modifier_e bw_modifier, u32 bw_val)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_bw_data_t       *bw_data_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    if ((bw_modifier < SDP_BW_MODIFIER_AS) ||
        (bw_modifier >= SDP_MAX_BW_MODIFIER_VAL)) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s Invalid bw modifier type: %d.",
                      sdp_p->debug_str, bw_modifier);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }

    bw_data_p = sdp_find_bw_line(sdp_p, level, inst_num);
    if (bw_data_p == NULL) {
        if (sdp_p->debug_flag[SDP_DEBUG_ERRORS]) {
            CSFLogError(logTag, "%s The %u instance of a b= line was not found at level %u.",
                      sdp_p->debug_str, inst_num, level);
        }
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    bw_data_p->bw_modifier = bw_modifier;
    bw_data_p->bw_val = bw_val;

    return (SDP_SUCCESS);
}







int32 sdp_get_mid_value (void *sdp_ptr, u16 level)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_VALUE);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_VALUE);
    }
    return (mca_p->mid);
}









sdp_result_e sdp_set_mid_value (void *sdp_ptr, u16 level, u32 mid_val)
{
    sdp_t               *sdp_p = (sdp_t *)sdp_ptr;
    sdp_mca_t           *mca_p;

    if (sdp_verify_sdp_ptr(sdp_p) == FALSE) {
        return (SDP_INVALID_SDP_PTR);
    }

    mca_p = sdp_find_media_level(sdp_p, level);
    if (mca_p == NULL) {
        sdp_p->conf_p->num_invalid_param++;
        return (SDP_INVALID_PARAMETER);
    }
    mca_p->mid = mid_val;
    return (SDP_SUCCESS);
}
