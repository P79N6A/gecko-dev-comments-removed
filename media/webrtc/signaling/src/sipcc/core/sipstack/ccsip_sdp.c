

















































#include "cpr_types.h"
#include "cpr_memory.h"
#include "sdp.h"
#include "text_strings.h"
#include "ccsip_sdp.h"
#include "ccsip_core.h"
#include "phone_debug.h"



static void *ccsip_sdp_config = NULL; 













boolean
sip_sdp_init (void)
{
    ccsip_sdp_config = sdp_init_config();
    if (!ccsip_sdp_config) {
        CCSIP_ERR_MSG("sdp_init_config() failure");
        return FALSE;
    }

    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_AUDIO, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_VIDEO, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_APPLICATION, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_DATA, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_CONTROL, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_NAS_RADIUS, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_NAS_TACACS, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_NAS_DIAMETER, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_NAS_L2TP, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_NAS_LOGIN, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_NAS_NONE, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_IMAGE, TRUE);
    sdp_media_supported(ccsip_sdp_config, SDP_MEDIA_TEXT, TRUE);
    sdp_nettype_supported(ccsip_sdp_config, SDP_NT_INTERNET, TRUE);
    sdp_addrtype_supported(ccsip_sdp_config, SDP_AT_IP4, TRUE);
    sdp_addrtype_supported(ccsip_sdp_config, SDP_AT_IP6, TRUE);
    sdp_transport_supported(ccsip_sdp_config, SDP_TRANSPORT_RTPAVP, TRUE);
    sdp_transport_supported(ccsip_sdp_config, SDP_TRANSPORT_UDPTL, TRUE);
    sdp_require_session_name(ccsip_sdp_config, FALSE);

    return (TRUE);
}







void *
sipsdp_create (void)
{
    const char *fname = "sipsdp_create :";
    void *sdp;


    sdp = sdp_init_description(ccsip_sdp_config);
    if (!sdp) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"SDP allocation failure\n", fname);
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
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing src_sdp\n",
                          fname, sdp_ret);
        }
    }
    if ((*sip_sdp)->dest_sdp) {
        sdp_ret = sdp_free_description((*sip_sdp)->dest_sdp);
        if (sdp_ret != SDP_SUCCESS) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing dest_sdp\n",
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
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing src_sdp\n",
                              fname, sdp_ret);
            }
            (*sdp_info)->src_sdp = NULL;
        }
    }

    if (flags & CCSIP_DEST_SDP_BIT) {
        if ((*sdp_info)->dest_sdp) {
            sdp_ret = sdp_free_description((*sdp_info)->dest_sdp);
            if (sdp_ret != SDP_SUCCESS) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%d while freeing dest_sdp\n",
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
sipsdp_src_dest_create (uint16_t flags, cc_sdp_t **sdp_info)
{

    if (!(*sdp_info)) {
        *sdp_info = sipsdp_info_create();
        if (!(*sdp_info)) {
            return;
        }
    }

    
    if (flags & CCSIP_SRC_SDP_BIT) {
        (*sdp_info)->src_sdp = sipsdp_create();
        if (!((*sdp_info)->src_sdp)) {
            sipsdp_src_dest_free(flags, sdp_info);
            return;
        }
    }

    if (flags & CCSIP_DEST_SDP_BIT) {
        (*sdp_info)->dest_sdp = sipsdp_create();
        if (!((*sdp_info)->dest_sdp)) {
            sipsdp_src_dest_free(flags, sdp_info);
            return;
        }
    }
}









cc_sdp_t *
sipsdp_create_from_buf (char *buf, uint32_t nbytes, cc_sdp_t *sdp)
{
    const char *fname = "sipsdp_create_from_buf";
    cc_sdp_t *sip_info = NULL;

    if (!buf) {
        return (NULL);
    }

    if (sdp) {
        sip_info = sdp;
    } else {
        sipsdp_src_dest_create(CCSIP_DEST_SDP_BIT | CCSIP_SRC_SDP_BIT,
                               &sip_info);
    }

    if (!sip_info) {
        



        return (NULL);
    }


    if (sdp_parse(sip_info->dest_sdp, &buf, (uint16_t)nbytes) != SDP_SUCCESS) {
        sipsdp_src_dest_free(CCSIP_DEST_SDP_BIT | CCSIP_SRC_SDP_BIT,
                             &sip_info);
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error parsing SDP\n", fname);
        return (NULL);
    }

    return (sip_info);
}













char *
sipsdp_write_to_buf (cc_sdp_t *sdp_info, uint32_t *retbytes)
{
    flex_string fs;
    uint32_t sdp_len;
    sdp_result_e rc;

    flex_string_init(&fs);

    if (!sdp_info || !sdp_info->src_sdp) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"NULL sdp_info or src_sdp\n", __FUNCTION__);
        return (NULL);
    }

    if ((rc = sdp_build(sdp_info->src_sdp, &fs))
        != SDP_SUCCESS) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"sdp_build rc=%s\n", DEB_F_PREFIX_ARGS(SIP_SDP, __FUNCTION__),
                         sdp_get_result_name(rc));

        flex_string_free(&fs);
        *retbytes = 0;
        return (NULL);
    }

    *retbytes = fs.string_length;

    


    return fs.buffer;
}
