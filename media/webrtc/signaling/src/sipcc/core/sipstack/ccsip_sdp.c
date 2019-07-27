

















































#include "cpr_types.h"
#include "cpr_memory.h"
#include "sdp.h"
#include "text_strings.h"
#include "ccsip_sdp.h"
#include "ccsip_core.h"
#include "phone_debug.h"













static void *
sip_sdp_init (void)
{
    void *sdp_config;

    sdp_config = sdp_init_config();

    if (sdp_config) {
        sdp_media_supported(sdp_config, SDP_MEDIA_AUDIO, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_VIDEO, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_APPLICATION, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_DATA, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_CONTROL, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_NAS_RADIUS, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_NAS_TACACS, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_NAS_DIAMETER, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_NAS_L2TP, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_NAS_LOGIN, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_NAS_NONE, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_IMAGE, TRUE);
        sdp_media_supported(sdp_config, SDP_MEDIA_TEXT, TRUE);
        sdp_nettype_supported(sdp_config, SDP_NT_INTERNET, TRUE);
        sdp_addrtype_supported(sdp_config, SDP_AT_IP4, TRUE);
        sdp_addrtype_supported(sdp_config, SDP_AT_IP6, TRUE);
        sdp_transport_supported(sdp_config, SDP_TRANSPORT_RTPAVP, TRUE);
        sdp_transport_supported(sdp_config, SDP_TRANSPORT_UDPTL, TRUE);
        sdp_require_session_name(sdp_config, FALSE);
    }

    return sdp_config;
}







sdp_t *
sipsdp_create (const char *peerconnection)
{
    sdp_t *sdp;
    void *sdp_config;

    sdp_config = sip_sdp_init();
    if (!sdp_config) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"SDP config init failure", __FUNCTION__);
        return (NULL);
    }

    sdp = sdp_init_description(peerconnection, sdp_config);
    if (!sdp) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"SDP allocation failure", __FUNCTION__);
        return (NULL);
    }

    



    CCSIP_INFO_DEBUG {
        sdp_debug(sdp, SDP_DEBUG_WARNINGS, TRUE);
    }

    CCSIP_ERR_DEBUG {
        sdp_debug(sdp, SDP_DEBUG_ERRORS, TRUE);
    }


#ifdef DEBUG_SDP_LIB
    





    CCSIP_MESSAGES_DEBUG {
        sdp_debug(sdp, SDP_DEBUG_TRACE, TRUE);
    }
#endif

    return (sdp);
}









void
sipsdp_free (cc_sdp_t **sip_sdp)
{
    const char *fname = "sipsdp_free: ";
    sdp_result_e sdp_ret;

    if (!*sip_sdp) {
        return;
    }

    if ((*sip_sdp)->src_sdp) {
        sdp_ret = sdp_free_description((*sip_sdp)->src_sdp);
        if (sdp_ret != SDP_SUCCESS) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing src_sdp",
                          fname, sdp_ret);
        }
    }
    if ((*sip_sdp)->dest_sdp) {
        sdp_ret = sdp_free_description((*sip_sdp)->dest_sdp);
        if (sdp_ret != SDP_SUCCESS) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing dest_sdp",
                          fname, sdp_ret);
        }
    }

    SDP_FREE(*sip_sdp);
}










cc_sdp_t *
sipsdp_info_create (void)
{
    cc_sdp_t *sdp_info = (cc_sdp_t *) cpr_malloc(sizeof(cc_sdp_t));

    if (sdp_info) {
        sdp_info->src_sdp = NULL;
        sdp_info->dest_sdp = NULL;
    }

    return (sdp_info);
}















void
sipsdp_src_dest_free (uint16_t flags, cc_sdp_t **sdp_info)
{
    const char *fname = "sipsdp_src_dest_free: ";
    sdp_result_e sdp_ret;

    if ((sdp_info == NULL) || (*sdp_info == NULL)) {
        return;
    }

    
    if (flags & CCSIP_SRC_SDP_BIT) {
        if ((*sdp_info)->src_sdp) {
            sdp_ret = sdp_free_description((*sdp_info)->src_sdp);
            if (sdp_ret != SDP_SUCCESS) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing src_sdp",
                              fname, sdp_ret);
            }
            (*sdp_info)->src_sdp = NULL;
        }
    }

    if (flags & CCSIP_DEST_SDP_BIT) {
        if ((*sdp_info)->dest_sdp) {
            sdp_ret = sdp_free_description((*sdp_info)->dest_sdp);
            if (sdp_ret != SDP_SUCCESS) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing dest_sdp",
                              fname, sdp_ret);
            }
            (*sdp_info)->dest_sdp = NULL;
        }

    }

    



    if (((*sdp_info)->src_sdp == NULL) && ((*sdp_info)->dest_sdp == NULL)) {
        sipsdp_free(sdp_info);
        *sdp_info = NULL;
    }
}















void
sipsdp_src_dest_create (const char *peerconnection,
    uint16_t flags, cc_sdp_t **sdp_info)
{

    if (!(*sdp_info)) {
        *sdp_info = sipsdp_info_create();
        if (!(*sdp_info)) {
            return;
        }
    }

    
    if (flags & CCSIP_SRC_SDP_BIT) {
        (*sdp_info)->src_sdp = sipsdp_create(peerconnection);
        if (!((*sdp_info)->src_sdp)) {
            sipsdp_src_dest_free(flags, sdp_info);
            return;
        }
    }

    if (flags & CCSIP_DEST_SDP_BIT) {
        (*sdp_info)->dest_sdp = sipsdp_create(peerconnection);
        if (!((*sdp_info)->dest_sdp)) {
            sipsdp_src_dest_free(flags, sdp_info);
            return;
        }
    }
}













char *
sipsdp_write_to_buf (sdp_t *sdp_info, uint32_t *retbytes)
{
    flex_string fs;
    uint32_t sdp_len;
    sdp_result_e rc;

    flex_string_init(&fs);

    if (!sdp_info) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"NULL sdp_info or src_sdp", __FUNCTION__);
        flex_string_free(&fs);
        return (NULL);
    }

    if ((rc = sdp_build(sdp_info, &fs))
        != SDP_SUCCESS) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"sdp_build rc=%s", DEB_F_PREFIX_ARGS(SIP_SDP, __FUNCTION__),
                         sdp_get_result_name(rc));

        flex_string_free(&fs);
        *retbytes = 0;
        return (NULL);
    }

    *retbytes = fs.string_length;

    


    return fs.buffer;
}
