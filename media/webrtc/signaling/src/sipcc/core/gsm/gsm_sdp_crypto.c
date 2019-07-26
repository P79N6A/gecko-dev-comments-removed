



#include <errno.h>
#include <limits.h>

#include "cpr_types.h"
#include "cpr_rand.h"
#include "sdp.h"
#include "fsm.h"
#include "gsm_sdp.h"
#include "util_string.h"
#include "lsm.h"
#include "sip_interface_regmgr.h"
#include "plat_api.h"

static const char *gsmsdp_crypto_suite_name[SDP_SRTP_MAX_NUM_CRYPTO_SUITES] =
{
    "SDP_SRTP_UNKNOWN_CRYPTO_SUITE",
    "SDP_SRTP_AES_CM_128_HMAC_SHA1_32",
    "SDP_SRTP_AES_CM_128_HMAC_SHA1_80",
    "SDP_SRTP_F8_128_HMAC_SHA1_80"
};






#define RAND_POOL_SIZE ((VCM_SRTP_MAX_KEY_SIZE+VCM_SRTP_MAX_SALT_SIZE)*10)
#define RAND_REQ_LIMIT 256      /* limit random number request */
static unsigned char rand_pool[RAND_POOL_SIZE]; 
static int rand_pool_bytes = 0; 






#define GSMSDP_DEFALT_KEY_LIFETIME NULL


#define GSMSDP_DEFAULT_ALGORITHM_ID VCM_AES_128_COUNTER
















void
gsmsdp_cache_crypto_keys (void)
{
    int number_to_fill;
    int accumulate_bytes;
    int bytes;

    



    if ((rand_pool_bytes == RAND_POOL_SIZE) || !lsm_is_phone_idle()) {
        return;
    }

    number_to_fill = RAND_POOL_SIZE - rand_pool_bytes;
    accumulate_bytes = 0;

    while (accumulate_bytes < number_to_fill) {
        bytes = number_to_fill - accumulate_bytes;
        




        if (bytes > RAND_REQ_LIMIT) {
            bytes = RAND_REQ_LIMIT;
        }

        if (platGenerateCryptoRand(&rand_pool[accumulate_bytes], &bytes)) {
            accumulate_bytes += bytes;
        } else {
            



            rand_pool[accumulate_bytes] = (uint8_t) (cpr_rand() & 0xff);
            accumulate_bytes++;
        }
    }
    rand_pool_bytes = RAND_POOL_SIZE;
}















static int
gsmsdp_get_rand_from_cached_pool (unsigned char *dst_buf, int req_bytes)
{
    int bytes;

    if (rand_pool_bytes == 0) {
        
        return 0;
    }

    if (rand_pool_bytes >= req_bytes) {
        
        bytes = req_bytes;
    } else {
        



        bytes = rand_pool_bytes;
    }

    memcpy(dst_buf, &rand_pool[RAND_POOL_SIZE - rand_pool_bytes], bytes);
    rand_pool_bytes -= bytes;

    
    return (bytes);
}














static void
gsmsdp_generate_key (uint32_t algorithmID, vcm_crypto_key_t * key)
{
    int     accumulate_len, len, total_bytes, bytes_from_cache;
    uint8_t random[sizeof(key->key) + sizeof(key->salt)];
    uint8_t key_len, salt_len;

    if (algorithmID == VCM_AES_128_COUNTER) {
        key_len  = VCM_AES_128_COUNTER_KEY_SIZE;
        salt_len = VCM_AES_128_COUNTER_SALT_SIZE;
    } else {
        
        key_len  = VCM_SRTP_MAX_KEY_SIZE;
        salt_len = VCM_SRTP_MAX_SALT_SIZE;
    }

    






    accumulate_len = 0;
    total_bytes    = key_len + salt_len;

    while (accumulate_len < total_bytes) {
        len = total_bytes - accumulate_len;

        
        bytes_from_cache =
            gsmsdp_get_rand_from_cached_pool(&random[accumulate_len], len);
        if (bytes_from_cache) {
            
            accumulate_len += bytes_from_cache;
        } else {
            






            if (len > RAND_REQ_LIMIT) {
                len = RAND_REQ_LIMIT;
            }
            if (platGenerateCryptoRand(&random[accumulate_len], &len)) {
                accumulate_len += len;
            } else {
                



                random[accumulate_len] = (uint8_t) (cpr_rand() & 0xff);
                accumulate_len++;
            }
        }
    }

    


    key->key_len = key_len;
    memcpy(&key->key[0], &random[0], key->key_len);

    key->salt_len = salt_len;
    memcpy(&key->salt[0], &random[key->key_len], key->salt_len);
}















static boolean
gsmsdp_is_supported_crypto_suite (sdp_srtp_crypto_suite_t crypto_suite)
{
    switch (crypto_suite) {
    case SDP_SRTP_AES_CM_128_HMAC_SHA1_32:
        
        return (TRUE);
    default:
        return (FALSE);
    }
}















static boolean
gsmsdp_is_valid_key_size (sdp_srtp_crypto_suite_t crypto_suite,
                          unsigned char key_size)
{
    




    if (key_size > VCM_SRTP_MAX_KEY_SIZE) {
        return (FALSE);
    }

    
    switch (crypto_suite) {
    case SDP_SRTP_AES_CM_128_HMAC_SHA1_32:
        if (key_size == VCM_AES_128_COUNTER_KEY_SIZE) {
            return (TRUE);
        }
        break;

    default:
        break;
    }
    return (FALSE);
}















static boolean
gsmsdp_is_valid_salt_size (sdp_srtp_crypto_suite_t crypto_suite,
                           unsigned char salt_size)
{
    




    if (salt_size > VCM_SRTP_MAX_SALT_SIZE) {
        return (FALSE);
    }

    
    switch (crypto_suite) {
    case SDP_SRTP_AES_CM_128_HMAC_SHA1_32:
        if (salt_size == VCM_AES_128_COUNTER_SALT_SIZE) {
            return (TRUE);
        }
        break;

    default:
        break;
    }
    return (FALSE);
}















static boolean
gsmdsp_cmp_key (vcm_crypto_key_t *key1, vcm_crypto_key_t *key2)
{
    if ((key1 == NULL) && (key2 != NULL)) {
        
        return (TRUE);
    }
    if ((key1 != NULL) && (key2 == NULL)) {
        
        return (TRUE);
    }
    if ((key1 == NULL) && (key2 == NULL)) {
        
        return (FALSE);
    }
    



    if ((key1->key_len != key2->key_len) || (key1->salt_len != key2->salt_len)) {
        
        return (TRUE);
    }
    if (key1->key_len != 0) {
        if (memcmp(key1->key, key2->key, key1->key_len)) {
            return (TRUE);
        }
    }
    if (key1->salt_len != 0) {
        if (memcmp(key1->salt, key2->salt, key1->salt_len)) {
            return (TRUE);
        }
    }
    
    return (FALSE);
}















static boolean
gsmsdp_is_supported_session_parm (const char *session_parms)
{
    int         len, wsh;
    const char *parm_ptr;
    long strtol_result;
    char *strtol_end;

    if (session_parms == NULL) {
        
        return (TRUE);
    }
    



    len = strlen(session_parms);
    if (strcmp(session_parms, "WSH=") && (len == 6)) {
        parm_ptr = &session_parms[sizeof("WSH=") - 1]; 

        errno = 0;
        strtol_result = strtol(parm_ptr, &strtol_end, 10);

        
        if (errno || parm_ptr == strtol_end || strtol_result < 64 || strtol_result > INT_MAX) {
            return FALSE;
        }

        return TRUE;
    }
    
    return (FALSE);
}















static vcm_crypto_algorithmID
gsmsdp_crypto_suite_to_algorithmID (sdp_srtp_crypto_suite_t crypto_suite)
{
    
    switch (crypto_suite) {
    case SDP_SRTP_AES_CM_128_HMAC_SHA1_32:
        return (VCM_AES_128_COUNTER);
    default:
        return (VCM_INVLID_ALGORITM_ID);
    }
}














static sdp_srtp_crypto_suite_t
gsmsdp_algorithmID_to_crypto_suite (vcm_crypto_algorithmID algorithmID)
{
    
    switch (algorithmID) {
    case VCM_AES_128_COUNTER:
        return (SDP_SRTP_AES_CM_128_HMAC_SHA1_32);
    default:
        return (SDP_SRTP_UNKNOWN_CRYPTO_SUITE);
    }
}














static const char *
gsmsdp_crypto_suite_string (sdp_srtp_crypto_suite_t crypto_suite)
{
    if (crypto_suite >= SDP_SRTP_MAX_NUM_CRYPTO_SUITES) {
        return (gsmsdp_crypto_suite_name[SDP_SRTP_UNKNOWN_CRYPTO_SUITE]);
    }
    return (gsmsdp_crypto_suite_name[crypto_suite]);
}



















static boolean
gsmsdp_get_key_from_sdp (fsmdef_dcb_t *dcb_p, void *sdp_p, uint16_t level,
                         uint16_t inst_num, vcm_crypto_key_t *key_st)
{
    const char    *fname = "gsmsdp_get_key_from_sdp";
    unsigned char  key_size;
    unsigned char  salt_size;
    const char    *key, *salt;
    sdp_srtp_crypto_suite_t crypto_suite;

    
    crypto_suite = sdp_attr_get_sdescriptions_crypto_suite(sdp_p,
                                                           level, 0, inst_num);

    
    key_size = sdp_attr_get_sdescriptions_key_size(sdp_p, level, 0, inst_num);
    if (!gsmsdp_is_valid_key_size(crypto_suite, key_size)) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "SDP has invalid key size %d at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, key_size, level);
        return (FALSE);
    }

    key = sdp_attr_get_sdescriptions_key(sdp_p, level, 0, inst_num);
    if (key == NULL) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "SDP has no key at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, level);
        return (FALSE);
    }

    
    salt_size = sdp_attr_get_sdescriptions_salt_size(sdp_p, level, 0, inst_num);
    if (!gsmsdp_is_valid_salt_size(crypto_suite, salt_size)) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "SDP has invalid salt size %d at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, salt_size, level);
        return (FALSE);
    }
    salt = sdp_attr_get_sdescriptions_salt(sdp_p, level, 0, inst_num);
    if (salt == NULL) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "SDP has no salt at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, level);
        return (FALSE);
    }

    



    if (key_st != NULL) {
        
        key_st->key_len = key_size;
        memcpy(key_st->key, key, key_size);
        key_st->salt_len = salt_size;
        memcpy(key_st->salt, salt, salt_size);
    }
    return (TRUE);
}














static boolean
gsmsdp_local_offer_srtp (fsmdef_media_t *media)
{
    



    if (media->local_crypto.tag == SDP_INVALID_VALUE) {
        return (FALSE);
    }
    return (TRUE);
}














static void
gsmsdp_clear_local_offer_srtp (fsmdef_media_t *media)
{
    



    media->local_crypto.tag = SDP_INVALID_VALUE;
}




















static boolean
gsmsdp_check_common_crypto_param (fsmdef_dcb_t *dcb_p, void *sdp_p,
                                  uint16_t level, uint16_t inst, boolean offer)
{
    const char *fname = "gsmsdp_check_common_crypto_param";
    const char *dir_str; 
    const char *session_parms;
    const char *mki_value = NULL;
    uint16_t    mki_length = 0;

    if (offer) {
        dir_str = "Offer";      
    } else {
        dir_str = "Answer";     
    }

    
    if (!gsmsdp_get_key_from_sdp(dcb_p, sdp_p, level, inst, NULL)) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "%s SDP has invalid key at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, dir_str, level);
        return (FALSE);
    }

    
    if (sdp_attr_get_sdescriptions_mki(sdp_p, level, 0, inst,
                                       &mki_value, &mki_length)
        != SDP_SUCCESS) {
        
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "Fail to obtain MKI from %s SDP at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, dir_str, level);
        return (FALSE);
    }
    if (mki_length) {
        
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "%s SDP has MKI %d (not supported) at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, dir_str, mki_length,
                  level);
        return (FALSE);
    }

    
    session_parms = sdp_attr_get_sdescriptions_session_params(sdp_p,
                                                              level, 0, inst);
    if (!gsmsdp_is_supported_session_parm(session_parms)) {
        
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "%s SDP has unsupported session param at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, dir_str, level);
        return (FALSE);
    }

    
    return (TRUE);
}


















static boolean
gsmsdp_select_offer_crypto (fsmdef_dcb_t *dcb_p, void *sdp_p, uint16_t level,
                            uint16_t *crypto_inst)
{
    const char  *fname = "gsmsdp_select_offer_crypto";
    uint16_t     num_attrs = 0; 
    uint16_t     attr;
    int32_t      tag;
    sdp_attr_e   attr_type;
    sdp_result_e rc;
    sdp_srtp_crypto_suite_t crypto_suite;

    
    rc = sdp_get_total_attrs(sdp_p, level, 0, &num_attrs);
    if (rc != SDP_SUCCESS) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "Failed finding attributes for media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname, level);
        return (FALSE);
    }

    







    for (attr = 1; attr <= num_attrs; attr++) {
        rc = sdp_get_attr_type(sdp_p, level, 0, attr, &attr_type, crypto_inst);
        if ((rc != SDP_SUCCESS) || (attr_type != SDP_ATTR_SDESCRIPTIONS)) {
            
            continue;
        }
        


        crypto_suite = sdp_attr_get_sdescriptions_crypto_suite(sdp_p,
                                                               level, 0,
                                                               *crypto_inst);
        if (!gsmsdp_is_supported_crypto_suite(crypto_suite)) {
            
            continue;
        }
        
        tag = sdp_attr_get_sdescriptions_tag(sdp_p, level, 0, *crypto_inst);
        if (tag == SDP_INVALID_VALUE) {
            
            continue;
        }

        
        if (!gsmsdp_check_common_crypto_param(dcb_p, sdp_p, level,
                                              *crypto_inst, TRUE)) {
            
            continue;
        }

        
        return (TRUE);
    }

    GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
              "Failed finding supported crypto attribute for media level %d\n",
              dcb_p->line, dcb_p->call_id, fname, level);
    return (FALSE);
}


















static boolean
gsmsdp_check_answer_crypto_param (fsmdef_dcb_t *dcb_p, cc_sdp_t * cc_sdp_p,
                                  fsmdef_media_t *media, uint16_t *crypto_inst)
{
    const char     *fname = "gsmsdp_check_answer_crypto_param";
    uint16_t        num_attrs = 0; 
    uint16_t        attr;
    uint16_t        num_crypto_attr = 0;
    int32_t         dest_crypto_tag, offered_tag;
    sdp_attr_e      attr_type;
    sdp_result_e    rc;
    sdp_srtp_crypto_suite_t crypto_suite;
    vcm_crypto_algorithmID algorithmID;
    void           *dest_sdp = cc_sdp_p->dest_sdp;
    uint16_t        temp_inst, inst = 0;
    uint16_t        level;

    level        = media->level;
    
    *crypto_inst = 0;

    
    if (sdp_get_total_attrs(dest_sdp, level, 0, &num_attrs) != SDP_SUCCESS) {
        GSM_DEBUG(DEB_L_C_F_PREFIX
                  "Failed finding attributes for media level %d\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), level);
        return (FALSE);
    }

    



    for (attr = 1, num_crypto_attr = 0; attr <= num_attrs; attr++) {
        rc = sdp_get_attr_type(dest_sdp, level, 0, attr, &attr_type,
                               &temp_inst);
        if ((rc == SDP_SUCCESS) && (attr_type == SDP_ATTR_SDESCRIPTIONS)) {
            num_crypto_attr++;
            inst = temp_inst;
        }
    }
    if (num_crypto_attr != 1) {
        
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "Answer SDP contains invalid number of"
                  " crypto attributes %d for media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname,
				  num_crypto_attr, level);
        return (FALSE);
    }

    







    dest_crypto_tag = sdp_attr_get_sdescriptions_tag(dest_sdp, level, 0, inst);

    











    if (gsmsdp_local_offer_srtp(media)) {
        



        offered_tag = media->local_crypto.tag;
        algorithmID = media->local_crypto.algorithmID;
    } else {
        
        offered_tag = media->negotiated_crypto.tag;
        algorithmID = media->negotiated_crypto.algorithmID;
    }
    if (dest_crypto_tag != offered_tag) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "Answer SDP contains wrong tag %d vs %d"
                  " for the media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname,
				  dest_crypto_tag, offered_tag, level);
        return (FALSE);
    }

    
    crypto_suite = sdp_attr_get_sdescriptions_crypto_suite(dest_sdp, level,
                                                           0, inst);
    if (gsmsdp_crypto_suite_to_algorithmID(crypto_suite) != algorithmID) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "Answer SDP mismatch crypto suite %s at media level %d\n",
                  dcb_p->line, dcb_p->call_id, fname,
                  gsmsdp_crypto_suite_string(crypto_suite), level);
        return (FALSE);
    }

    
    if (!gsmsdp_check_common_crypto_param(dcb_p, dest_sdp, level,
                                          inst, FALSE)) {
        
        return (FALSE);
    }

    *crypto_inst = inst;
    return (TRUE);
}



















static sdp_transport_e
gsmsdp_negotiate_offer_crypto (fsmdef_dcb_t *dcb_p, cc_sdp_t *cc_sdp_p,
                               fsmdef_media_t *media, uint16_t *crypto_inst)
{
    sdp_transport_e remote_transport;
    sdp_transport_e negotiated_transport = SDP_TRANSPORT_INVALID;
    void           *sdp_p = cc_sdp_p->dest_sdp;
    uint16_t       level;
    int            sdpmode = 0;

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));
    level = media->level;
    *crypto_inst     = 0;
    remote_transport = sdp_get_media_transport(sdp_p, level);

    
    switch (remote_transport) {
    case SDP_TRANSPORT_RTPAVP:
        
        negotiated_transport = SDP_TRANSPORT_RTPAVP;
        break;

    case SDP_TRANSPORT_RTPSAVP:

        
        if (((sip_regmgr_get_sec_level(dcb_p->line) == ENCRYPTED) &&
            FSM_CHK_FLAGS(media->flags, FSM_MEDIA_F_SUPPORT_SECURITY)) || sdpmode) {
            
            if (gsmsdp_select_offer_crypto(dcb_p, sdp_p, level, crypto_inst)) {
                
                negotiated_transport = SDP_TRANSPORT_RTPSAVP;
            }
        }

        if (negotiated_transport == SDP_TRANSPORT_INVALID) {
            



            if (sip_regmgr_srtp_fallback_enabled(dcb_p->line)) {
                negotiated_transport = SDP_TRANSPORT_RTPAVP;
            }
        }
        break;

    case SDP_TRANSPORT_RTPSAVPF:
        
        negotiated_transport = SDP_TRANSPORT_RTPSAVPF;
        break;

    case SDP_TRANSPORT_SCTPDTLS:
        negotiated_transport = SDP_TRANSPORT_SCTPDTLS;
        break;

    default:
        
        break;
    }
    return (negotiated_transport);
}



















static sdp_transport_e
gsmsdp_negotiate_answer_crypto (fsmdef_dcb_t *dcb_p, cc_sdp_t *cc_sdp_p,
                                fsmdef_media_t *media, uint16_t *crypto_inst)
{
    const char     *fname = "gsmsdp_check_answer_crypto";
    sdp_transport_e remote_transport, local_transport;
    sdp_transport_e negotiated_transport = SDP_TRANSPORT_INVALID;
    uint16_t        level;

    level            = media->level;
    *crypto_inst     = 0;
    remote_transport = sdp_get_media_transport(cc_sdp_p->dest_sdp, level);
    local_transport  = sdp_get_media_transport(cc_sdp_p->src_sdp, level);
    GSM_DEBUG(GSM_F_PREFIX "remote transport %d\n", fname, remote_transport);
    GSM_DEBUG(GSM_F_PREFIX "local transport %d\n", fname, local_transport);

    
    switch (remote_transport) {
    case SDP_TRANSPORT_RTPAVP:
        
        if (local_transport == SDP_TRANSPORT_RTPSAVP) {
            if (sip_regmgr_srtp_fallback_enabled(dcb_p->line)) {
                
                negotiated_transport = SDP_TRANSPORT_RTPAVP;
            }
        } else {
            
            negotiated_transport = SDP_TRANSPORT_RTPAVP;
        }
        break;

    case SDP_TRANSPORT_RTPSAVP:
        GSM_DEBUG(GSM_F_PREFIX "remote SAVP case\n", fname);
        
        if (local_transport == SDP_TRANSPORT_RTPSAVP) {
        GSM_DEBUG(GSM_F_PREFIX "local SAVP case\n", fname);
            
            if (gsmsdp_check_answer_crypto_param(dcb_p, cc_sdp_p, media,
                                                 crypto_inst)) {
                
                negotiated_transport = SDP_TRANSPORT_RTPSAVP;
                GSM_DEBUG(GSM_F_PREFIX "crypto params verified\n", fname);
            }
        } else {
            
        }
        break;

    case SDP_TRANSPORT_RTPSAVPF:
        negotiated_transport = SDP_TRANSPORT_RTPSAVPF;
        break;

    case SDP_TRANSPORT_SCTPDTLS:
        negotiated_transport = SDP_TRANSPORT_SCTPDTLS;
        break;

    default:
        
        break;
    }
    GSM_DEBUG(GSM_F_PREFIX "negotiated transport %d\n", fname, negotiated_transport);
    return (negotiated_transport);
}





















sdp_transport_e
gsmsdp_negotiate_media_transport (fsmdef_dcb_t *dcb_p, cc_sdp_t *cc_sdp_p,
                                  boolean offer, fsmdef_media_t *media,
                                  uint16_t *crypto_inst)
{
    sdp_transport_e transport;

    
    if (offer) {
        transport = gsmsdp_negotiate_offer_crypto(dcb_p, cc_sdp_p, media,
                                                  crypto_inst);
    } else {
        transport = gsmsdp_negotiate_answer_crypto(dcb_p, cc_sdp_p, media,
                                                   crypto_inst);
    }
    return (transport);
}


















static sdp_result_e
gsmsdp_add_single_crypto_attr (void *sdp_p, uint16_t level, int32_t tag,
                               sdp_srtp_crypto_suite_t crypto_suite,
                               vcm_crypto_key_t * key, char *lifetime)
{
    sdp_result_e rc;
    uint16       inst_num;

    if (key == NULL) {
        return (SDP_INVALID_PARAMETER);
    }

    



    rc = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_SDESCRIPTIONS, &inst_num);
    if (rc != SDP_SUCCESS) {
        return (rc);
    }

    rc = sdp_attr_set_sdescriptions_tag(sdp_p, level, 0, inst_num, tag);
    if (rc != SDP_SUCCESS) {
        return (rc);
    }

    
    rc = sdp_attr_set_sdescriptions_crypto_suite(sdp_p, level, 0, inst_num,
                                                 crypto_suite);
    if (rc != SDP_SUCCESS) {
        return (rc);
    }

    
    rc = sdp_attr_set_sdescriptions_key(sdp_p, level, 0, inst_num,
                                        (char *) key->key);
    if (rc != SDP_SUCCESS) {
        return (rc);
    }

    rc = sdp_attr_set_sdescriptions_key_size(sdp_p, level, 0, inst_num,
                                             key->key_len);
    if (rc != SDP_SUCCESS) {
        return (rc);
    }

    
    rc = sdp_attr_set_sdescriptions_salt(sdp_p, level, 0, inst_num,
                                         (char *) key->salt);
    if (rc != SDP_SUCCESS) {
        return (rc);
    }

    rc = sdp_attr_set_sdescriptions_salt_size(sdp_p, level, 0, inst_num,
                                              key->salt_len);
    if (rc != SDP_SUCCESS) {
        return (rc);
    }

    if (lifetime != NULL) {
        rc = sdp_attr_set_sdescriptions_lifetime(sdp_p, level, 0, inst_num,
                                                 lifetime);
    }
    return (rc);
}

















static void
gsmsdp_add_all_crypto_lines (fsmdef_dcb_t *dcb_p, void *sdp_p,
                             fsmdef_media_t *media)
{
    const char *fname = "gsmsdp_add_all_crypto_lines";
    sdp_srtp_crypto_suite_t crypto_suite;

    



    media->local_crypto.tag = 1;
    media->local_crypto.algorithmID = GSMSDP_DEFAULT_ALGORITHM_ID;

    
    gsmsdp_generate_key(media->local_crypto.algorithmID,
                        &media->local_crypto.key);

    
    crypto_suite =
        gsmsdp_algorithmID_to_crypto_suite(media->local_crypto.algorithmID);
    if (gsmsdp_add_single_crypto_attr(sdp_p, media->level,
            media->local_crypto.tag, crypto_suite, &media->local_crypto.key,
            GSMSDP_DEFALT_KEY_LIFETIME) != SDP_SUCCESS) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "Failed to add crypto attributes\n",
                  dcb_p->line, dcb_p->call_id, fname);
    }
}













static void
gsmsdp_init_crypto_context (fsmdef_media_t *media)
{
    
    media->local_crypto.tag = SDP_INVALID_VALUE;
    media->local_crypto.algorithmID = VCM_NO_ENCRYPTION;
    media->local_crypto.key.key_len  = 0;
    media->local_crypto.key.salt_len = 0;

    
    media->negotiated_crypto.tag = SDP_INVALID_VALUE;
    media->negotiated_crypto.algorithmID = VCM_NO_ENCRYPTION;
    media->negotiated_crypto.tx_key.key_len  = 0;
    media->negotiated_crypto.tx_key.salt_len = 0;
    media->negotiated_crypto.rx_key.key_len  = 0;
    media->negotiated_crypto.rx_key.salt_len = 0;

    media->negotiated_crypto.flags = 0;
}















void
gsmsdp_init_sdp_media_transport (fsmdef_dcb_t *dcb_p, void *sdp_p,
                                 fsmdef_media_t *media)
{
    int  rtpsavpf = 0;
    int  sdpmode = 0;

    
    gsmsdp_init_crypto_context(media);

    config_get_value(CFGID_RTPSAVPF, &rtpsavpf, sizeof(rtpsavpf));
    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    if (SDP_MEDIA_APPLICATION == media->type) {
        media->transport = SDP_TRANSPORT_SCTPDTLS;
    } else if (rtpsavpf) {
        media->transport = SDP_TRANSPORT_RTPSAVPF;
    } else if (sdpmode) {
        media->transport = SDP_TRANSPORT_RTPSAVP;
    } else	if ((sip_regmgr_get_sec_level(dcb_p->line) != ENCRYPTED) ||
        (!FSM_CHK_FLAGS(media->flags, FSM_MEDIA_F_SUPPORT_SECURITY))) {
        



        media->transport = SDP_TRANSPORT_RTPAVP;
    } else {
        media->transport = SDP_TRANSPORT_RTPSAVP;
    }
}

















void
gsmsdp_reset_sdp_media_transport (fsmdef_dcb_t *dcb_p, void *sdp_p,
                                  fsmdef_media_t *media, boolean hold)
{
    if (hold) {
        
    } else {
        
        if ((sip_regmgr_get_cc_mode(dcb_p->line) == REG_MODE_CCM)) {
            









            gsmsdp_init_sdp_media_transport(dcb_p,
                                            dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL,
                                            media);
        } else {
            





        }
    }
}














void
gsmsdp_set_media_transport_for_option (void *sdp_p, uint16_t level)
{
    const char      *fname = "gsmsdp_set_media_transport_for_option";
    uint32_t         algorithmID;
    vcm_crypto_key_t key;
    sdp_srtp_crypto_suite_t crypto_suite;


    
    if (sip_regmgr_get_sec_level(1) != ENCRYPTED) {
        
        (void) sdp_set_media_transport(sdp_p, level, SDP_TRANSPORT_RTPAVP);
        return;
    }

    
    (void) sdp_set_media_transport(sdp_p, level, SDP_TRANSPORT_RTPSAVP);

    
    crypto_suite = SDP_SRTP_AES_CM_128_HMAC_SHA1_32;
    algorithmID = gsmsdp_crypto_suite_to_algorithmID(crypto_suite);
    gsmsdp_generate_key(algorithmID, &key);

    
    if (gsmsdp_add_single_crypto_attr(sdp_p, level, 1, crypto_suite, &key,
            GSMSDP_DEFALT_KEY_LIFETIME) != SDP_SUCCESS) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX
                  "Failed to add crypto attributes\n",
                  fname);
    }
}

















void
gsmsdp_update_local_sdp_media_transport (fsmdef_dcb_t *dcb_p, void *sdp_p,
                                         fsmdef_media_t *media,
                                         sdp_transport_e transport, boolean all)
{
    const char *fname = "gsmsdp_update_local_sdp_media_transport";
    sdp_srtp_crypto_suite_t crypto_suite;
    uint16_t level;

    level = media->level;
    
    if (transport == SDP_TRANSPORT_INVALID) {
        




        transport = media->transport;
    }

    


    if (sdp_get_media_transport(sdp_p, level) == SDP_TRANSPORT_INVALID) {
        (void) sdp_set_media_transport(sdp_p, level, transport);
    }

    if (transport != SDP_TRANSPORT_RTPSAVP) {
        







        return;
    }

    
    if (all || (media->negotiated_crypto.tag == SDP_INVALID_VALUE)) {
        



        if (media->negotiated_crypto.tag == SDP_INVALID_VALUE) {
            



            gsmsdp_add_all_crypto_lines(dcb_p, sdp_p, media);
            return;
        } else {
            
        }
    }

    


    crypto_suite =
        gsmsdp_algorithmID_to_crypto_suite(
            media->negotiated_crypto.algorithmID);
    if (gsmsdp_add_single_crypto_attr(sdp_p, level,
                                      media->negotiated_crypto.tag,
                                      crypto_suite,
                                      &media->negotiated_crypto.tx_key,
                                      GSMSDP_DEFALT_KEY_LIFETIME)
            != SDP_SUCCESS) {
        GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX
                  "Failed to add crypto attributes\n",
                  dcb_p->line, dcb_p->call_id, fname);
    }
}

















void
gsmsdp_update_crypto_transmit_key (fsmdef_dcb_t *dcb_p,
                                   fsmdef_media_t *media,
                                   boolean offer,
                                   boolean initial_offer,
                                   sdp_direction_e direction)
{
    const char *fname = "gsmsdp_update_crypto_transmit_key";
    boolean     generate_key = FALSE;

    if (media->transport != SDP_TRANSPORT_RTPSAVP) {
        return;
    }

    if (initial_offer || offer) {
        
        if (initial_offer) {
            
            generate_key = TRUE;
        } else if ((util_compare_ip(&(media->previous_sdp.dest_addr),
                                &(media->dest_addr)) == FALSE) &&
                   media->dest_addr.type != CPR_IP_ADDR_INVALID) {
            

            GSM_DEBUG(DEB_L_C_F_PREFIX
                      "Received offer with dest. address changes\n",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            















            if (gsmsdp_local_offer_srtp(media)) {
                



                media->negotiated_crypto.tx_key = media->local_crypto.key;
                GSM_DEBUG(DEB_L_C_F_PREFIX
                          "Local offered SDP has been sent, use offered key\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            } else {
                generate_key = TRUE;
            }
        } else if ((media->direction == SDP_DIRECTION_INACTIVE) &&
                   (direction != SDP_DIRECTION_INACTIVE)) {
            if (!gsmsdp_local_offer_srtp(media)) {
                









                generate_key = TRUE;
                GSM_DEBUG(DEB_L_C_F_PREFIX
                          "Received direction changes from inactive\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            }
        } else if (media->negotiated_crypto.tx_key.key_len == 0) {
            if (gsmsdp_local_offer_srtp(media)) {
                






                media->negotiated_crypto.tx_key = media->local_crypto.key;
                GSM_DEBUG(DEB_L_C_F_PREFIX
                          "Local offered SDP has been sent, use offered key\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            } else {
                





                generate_key = TRUE;
                GSM_DEBUG(DEB_L_C_F_PREFIX
                          "Received offer but no tx key, generate new key\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            }
        } else {
            
        }
        if (generate_key) {
            



            gsmsdp_generate_key(media->negotiated_crypto.algorithmID,
                                &media->negotiated_crypto.tx_key);
            GSM_DEBUG(DEB_L_C_F_PREFIX
                      "Generate tx key\n",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            media->negotiated_crypto.flags |= FSMDEF_CRYPTO_TX_CHANGE;
        }
    } else {
        






        if (gsmdsp_cmp_key(&media->local_crypto.key,
                           &media->negotiated_crypto.tx_key)) {
            media->negotiated_crypto.flags |= FSMDEF_CRYPTO_TX_CHANGE;
            GSM_DEBUG(DEB_L_C_F_PREFIX
                      "tx key changes in answered SDP\n",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        }
        media->negotiated_crypto.tx_key = media->local_crypto.key; 
    }
    
    gsmsdp_clear_local_offer_srtp(media);
}



















void
gsmsdp_update_negotiated_transport (fsmdef_dcb_t *dcb_p,
                                    cc_sdp_t *cc_sdp_p,
                                    fsmdef_media_t *media,
                                    uint16_t crypto_inst,
                                    sdp_transport_e transport)
{
    const char *fname = "gsmsdp_update_negotiated_transport";
    sdp_srtp_crypto_suite_t crypto_suite;
    void                   *dest_sdp = cc_sdp_p->dest_sdp;
    vcm_crypto_algorithmID  algorithmID;
    vcm_crypto_key_t        key;
    uint16_t                level;

    level = media->level;
    





    
    media->negotiated_crypto.flags &= ~(FSMDEF_CRYPTO_TX_CHANGE |
                                        FSMDEF_CRYPTO_RX_CHANGE);
    





    if ((media->transport != SDP_TRANSPORT_INVALID) &&
        (transport != media->transport)) {
        
        media->negotiated_crypto.flags |= (FSMDEF_CRYPTO_TX_CHANGE |
                                           FSMDEF_CRYPTO_RX_CHANGE);
        GSM_DEBUG(DEB_L_C_F_PREFIX
                  "SDP media transport changed to %d\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), transport);
    }
    
    media->transport = transport;

    if (media->transport != SDP_TRANSPORT_RTPSAVP) {
        



        GSM_DEBUG(DEB_L_C_F_PREFIX
                  "SDP media transport is RTP\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        return;
    }

    GSM_DEBUG(DEB_L_C_F_PREFIX "SDP media transport is SRTP\n",
              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));

    




    crypto_suite = sdp_attr_get_sdescriptions_crypto_suite(dest_sdp, level,
                                                           0, crypto_inst);

    
    algorithmID = gsmsdp_crypto_suite_to_algorithmID(crypto_suite);
    if (algorithmID != media->negotiated_crypto.algorithmID) {
        media->negotiated_crypto.flags |= (FSMDEF_CRYPTO_TX_CHANGE |
                                           FSMDEF_CRYPTO_RX_CHANGE);
        GSM_DEBUG(DEB_L_C_F_PREFIX "SDP algorithm ID change to %d\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), algorithmID);
    }
    media->negotiated_crypto.algorithmID = algorithmID;

    
    media->negotiated_crypto.tag = sdp_attr_get_sdescriptions_tag(dest_sdp,
                                                                  level, 0,
                                                                  crypto_inst);

    
    (void) gsmsdp_get_key_from_sdp(dcb_p, dest_sdp, level, crypto_inst, &key);
    if (gsmdsp_cmp_key(&key, &media->negotiated_crypto.rx_key)) {
        media->negotiated_crypto.flags |= FSMDEF_CRYPTO_RX_CHANGE;
        GSM_DEBUG(DEB_L_C_F_PREFIX "SDP rx key changes\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
    }
    media->negotiated_crypto.rx_key = key;
}
















boolean
gsmsdp_is_crypto_ready (fsmdef_media_t *media, boolean rx)
{
    




    if (media->transport == SDP_TRANSPORT_RTPAVP || media->transport == SDP_TRANSPORT_RTPSAVPF) {
        return (TRUE);
    }

    


    if (rx) {
        if (media->negotiated_crypto.rx_key.key_len == 0) {
            
            return (FALSE);
        }
    } else {
        if (media->negotiated_crypto.tx_key.key_len == 0) {
            
            return (FALSE);
        }
    }
    
    return (TRUE);
}















boolean
gsmsdp_is_media_encrypted (fsmdef_dcb_t *dcb_p)
{
    fsmdef_media_t *media;
    uint8_t num_encrypted;

    if (dcb_p == NULL) {
        return(FALSE);
    }
    num_encrypted = 0;
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }

        if (media->transport == SDP_TRANSPORT_RTPSAVP || media->transport == SDP_TRANSPORT_RTPSAVPF) {
            num_encrypted++;
        }
    }

    if ((num_encrypted == 0) ||
        (num_encrypted != GSMSDP_MEDIA_COUNT(dcb_p))) {
        




        return (FALSE);
    }
    return (TRUE);
}
















boolean
gsmsdp_crypto_params_change (boolean rcv_only, fsmdef_media_t *media)
{
    if (rcv_only) {
        if (media->negotiated_crypto.flags & FSMDEF_CRYPTO_RX_CHANGE) {
            return (TRUE);
        }
    } else {
        if (media->negotiated_crypto.flags & FSMDEF_CRYPTO_TX_CHANGE) {
            return (TRUE);
        }
    }
    return (FALSE);
}










void
gsmsdp_crypto_reset_params_change (fsmdef_media_t *media)
{
    media->negotiated_crypto.flags &= ~(FSMDEF_CRYPTO_RX_CHANGE |
                                        FSMDEF_CRYPTO_TX_CHANGE);
}
