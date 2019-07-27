





#include "cpr_in.h"
#include "cpr_rand.h"
#include "cpr_stdlib.h"
#include "lsm.h"
#include "fsm.h"
#include "ccapi.h"
#include "ccsip_sdp.h"
#include "sdp.h"
#include "gsm.h"
#include "gsm_sdp.h"
#include "util_string.h"
#include "rtp_defs.h"
#include "debug.h"
#include "dtmf.h"
#include "prot_configmgr.h"
#include "dns_utils.h"
#include "sip_interface_regmgr.h"
#include "platform_api.h"
#include "vcm.h"
#include "prlog.h"
#include "plstr.h"
#include "sdp_private.h"

static const char* logTag = "gsm_sdp";


#define MULTICAST_START_ADDRESS 0xe1000000
#define MULTICAST_END_ADDRESS   0xefffffff


#define GET_CODEC_TYPE(a)    ((uint8_t)((a) & 0XFF))

#define GSMSDP_SET_MEDIA_DIABLE(media) \
     (media->src_port = 0)

#define CAST_DEFAULT_BITRATE 320000







#define GSMSDP_MAX_MLINES_PER_CALL  (8)














#define GSMSDP_PERM_MEDIA_ELEMS   (LSM_MAX_CALLS)





static fsmdef_media_t gsmsdp_free_media_chunk[GSMSDP_PERM_MEDIA_ELEMS];
static sll_lite_list_t gsmsdp_free_media_list;

typedef enum {
    MEDIA_TABLE_GLOBAL,
    MEDIA_TABLE_SESSION
} media_table_e;


static cc_causes_t
gsmsdp_init_local_sdp (const char *peerconnection, cc_sdp_t **sdp_pp);

static void
gsmsdp_set_media_capability(fsmdef_media_t *media,
                            const cc_media_cap_t *media_cap);
static fsmdef_media_t *
gsmsdp_add_media_line(fsmdef_dcb_t *dcb_p, const cc_media_cap_t *media_cap,
                      uint8_t cap_index, uint16_t level,
                      cpr_ip_type addr_type, boolean offer);
static boolean
gsmsdp_add_remote_stream(uint16_t idx, int pc_stream_id,
                         fsmdef_dcb_t *dcb_p);

static boolean
gsmsdp_add_remote_track(uint16_t idx, uint16_t track,
                         fsmdef_dcb_t *dcb_p, fsmdef_media_t *media);



extern cc_media_cap_table_t g_media_table;

extern boolean g_disable_mass_reg_debug_print;












static const cc_media_cap_table_t *gsmsdp_get_media_capability (fsmdef_dcb_t *dcb_p)
{
    static const char *fname = "gsmsdp_get_media_capability";
    int                sdpmode = 0;

    if (g_disable_mass_reg_debug_print == FALSE) {
        GSM_DEBUG(DEB_F_PREFIX"dcb video pref %x",
                               DEB_F_PREFIX_ARGS(GSM, fname), dcb_p->video_pref);
    }

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    if ( dcb_p->media_cap_tbl == NULL ) {
         dcb_p->media_cap_tbl = (cc_media_cap_table_t*) cpr_calloc(1, sizeof(cc_media_cap_table_t));
         if ( dcb_p->media_cap_tbl == NULL ) {
             GSM_ERR_MSG(GSM_L_C_F_PREFIX"media table malloc failed.",
                    dcb_p->line, dcb_p->call_id, fname);
             return NULL;
         }
    }

    if (sdpmode) {
        


        dcb_p->media_cap_tbl->id = g_media_table.id;

        


        dcb_p->media_cap_tbl->cap[CC_AUDIO_1].name = CC_AUDIO_1;
        dcb_p->media_cap_tbl->cap[CC_VIDEO_1].name = CC_VIDEO_1;
        dcb_p->media_cap_tbl->cap[CC_DATACHANNEL_1].name = CC_DATACHANNEL_1;

        dcb_p->media_cap_tbl->cap[CC_AUDIO_1].type = SDP_MEDIA_AUDIO;
        dcb_p->media_cap_tbl->cap[CC_VIDEO_1].type = SDP_MEDIA_VIDEO;
        dcb_p->media_cap_tbl->cap[CC_DATACHANNEL_1].type = SDP_MEDIA_APPLICATION;

        dcb_p->media_cap_tbl->cap[CC_AUDIO_1].enabled = FALSE;
        dcb_p->media_cap_tbl->cap[CC_VIDEO_1].enabled = FALSE;
        dcb_p->media_cap_tbl->cap[CC_DATACHANNEL_1].enabled = FALSE;

        dcb_p->media_cap_tbl->cap[CC_AUDIO_1].support_security = TRUE;
        dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_security = TRUE;
        dcb_p->media_cap_tbl->cap[CC_DATACHANNEL_1].support_security = TRUE;

        
        dcb_p->media_cap_tbl->cap[CC_AUDIO_1].bundle_only = TRUE;
        dcb_p->media_cap_tbl->cap[CC_VIDEO_1].bundle_only = TRUE;
        dcb_p->media_cap_tbl->cap[CC_DATACHANNEL_1].bundle_only = TRUE;

        




        dcb_p->media_cap_tbl->cap[CC_AUDIO_1].support_direction =
          SDP_DIRECTION_RECVONLY;

        dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction =
          SDP_DIRECTION_RECVONLY;

        dcb_p->media_cap_tbl->cap[CC_DATACHANNEL_1].support_direction =
          SDP_DIRECTION_SENDRECV;
    } else {
        *(dcb_p->media_cap_tbl) = g_media_table;

        dcb_p->media_cap_tbl->cap[CC_DATACHANNEL_1].enabled = FALSE;

        if ( dcb_p->video_pref == SDP_DIRECTION_INACTIVE) {
            
            dcb_p->media_cap_tbl->cap[CC_VIDEO_1].enabled = FALSE;
        }

        if ( dcb_p->video_pref == SDP_DIRECTION_RECVONLY ) {
            if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_SENDRECV ) {
                dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = dcb_p->video_pref;
            }

            if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_SENDONLY ) {
                dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = SDP_DIRECTION_INACTIVE;
                DEF_DEBUG(GSM_L_C_F_PREFIX"video capability disabled to SDP_DIRECTION_INACTIVE from sendonly",
                dcb_p->line, dcb_p->call_id, fname);
            }
        } else if ( dcb_p->video_pref == SDP_DIRECTION_SENDONLY ) {
            if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_SENDRECV ) {
                dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = dcb_p->video_pref;
            }

            if ( dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction == SDP_DIRECTION_RECVONLY ) {
               dcb_p->media_cap_tbl->cap[CC_VIDEO_1].support_direction = SDP_DIRECTION_INACTIVE;
                DEF_DEBUG(GSM_L_C_F_PREFIX"video capability disabled to SDP_DIRECTION_INACTIVE from recvonly",
                    dcb_p->line, dcb_p->call_id, fname);
            }
        } 
    }

    return (dcb_p->media_cap_tbl);
}




void gsmsdp_process_cap_constraint(cc_media_cap_t *cap,
                                   cc_boolean constraint) {
  if (!constraint) {
    cap->support_direction &= ~SDP_DIRECTION_FLAG_RECV;
  } else {
    cap->support_direction |= SDP_DIRECTION_FLAG_RECV;
    cap->enabled = TRUE;
  }
}





void gsmsdp_process_cap_options(fsmdef_dcb_t *dcb,
                                    cc_media_options_t* options) {
  if (options->offer_to_receive_audio.was_passed) {
    gsmsdp_process_cap_constraint(&dcb->media_cap_tbl->cap[CC_AUDIO_1],
                                  options->offer_to_receive_audio.value);
  }
  if (options->offer_to_receive_video.was_passed) {
    gsmsdp_process_cap_constraint(&dcb->media_cap_tbl->cap[CC_VIDEO_1],
                                  options->offer_to_receive_video.value);
  }
  if (options->moz_dont_offer_datachannel.was_passed) {
    
    if (options->moz_dont_offer_datachannel.value) {
      dcb->media_cap_tbl->cap[CC_DATACHANNEL_1].enabled = FALSE;
    }
  }
}






void gsmsdp_copy_payloads_to_previous_sdp (fsmdef_media_t *media)
{
    static const char *fname = "gsmsdp_copy_payloads_to_previous_sdp";

    if ((!media->payloads) && (NULL != media->previous_sdp.payloads))
    {
      cpr_free(media->previous_sdp.payloads);
      media->previous_sdp.payloads = NULL;
      media->previous_sdp.num_payloads = 0;
    }

    
    if (media->num_payloads > media->previous_sdp.num_payloads)
    {
      media->previous_sdp.payloads =
        cpr_realloc(media->previous_sdp.payloads,
            media->num_payloads * sizeof(vcm_payload_info_t));
    }

    
    media->previous_sdp.num_payloads = media->num_payloads;
    memcpy(media->previous_sdp.payloads, media->payloads,
        media->num_payloads * sizeof(vcm_payload_info_t));
    media->previous_sdp.num_payloads = media->num_payloads;
}














vcm_payload_info_t *gsmsdp_find_info_for_codec(rtp_ptype codec,
                                         vcm_payload_info_t *payload_info,
                                         int num_payload_info,
                                         int instance) {
    int i;
    for (i = 0; i < num_payload_info; i++) {
        if (payload_info[i].codec_type == codec)
        {
            instance--;
            if (instance == 0) {
                return &(payload_info[i]);
            }
        }
    }
    return NULL;
}









static const cc_media_remote_stream_table_t *gsmsdp_get_media_stream_table (fsmdef_dcb_t *dcb_p)
{
    static const char *fname = "gsmsdp_get_media_stream_table";
    if ( dcb_p->remote_media_stream_tbl == NULL ) {
      dcb_p->remote_media_stream_tbl = (cc_media_remote_stream_table_t*) cpr_malloc(sizeof(cc_media_remote_stream_table_t));
      if ( dcb_p->remote_media_stream_tbl == NULL ) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"media track table malloc failed.",
                    dcb_p->line, dcb_p->call_id, fname);
        return NULL;
      }
    }

    memset(dcb_p->remote_media_stream_tbl, 0, sizeof(cc_media_remote_stream_table_t));

    return (dcb_p->remote_media_stream_tbl);
}













boolean
gsmsdp_create_free_media_list (void)
{
    uint32_t i;
    fsmdef_media_t *media;

    
    (void)sll_lite_init(&gsmsdp_free_media_list);

    




    media = &gsmsdp_free_media_chunk[0];    
    for (i = 0; i < GSMSDP_PERM_MEDIA_ELEMS; i++) {
        (void)sll_lite_link_head(&gsmsdp_free_media_list,
                                 (sll_lite_node_t *)media);
        media = media + 1; 
    }

    
    return (TRUE);
}









void
gsmsdp_destroy_free_media_list (void)
{
    




    (void)sll_lite_init(&gsmsdp_free_media_list);
}













static fsmdef_media_t *
gsmsdp_alloc_media (void)
{
    static const char fname[] = "gsmsdp_alloc_media";
    fsmdef_media_t *media = NULL;

    
    media = (fsmdef_media_t *)sll_lite_unlink_head(&gsmsdp_free_media_list);
    if (media == NULL) {
        
        media = cpr_malloc(sizeof(fsmdef_media_t));
        GSM_DEBUG(DEB_F_PREFIX"get from dynamic pool, media %p",
                               DEB_F_PREFIX_ARGS(GSM, fname), media);
    }
    return (media);
}














static void
gsmsdp_free_media (fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_free_media";

    if (media == NULL) {
        return;
    }

    if (media-> video != NULL ) {
      vcmFreeMediaPtr(media->video);
    }

    if(media->payloads != NULL) {
        cpr_free(media->payloads);
        media->payloads = NULL;
        media->num_payloads = 0;
    }
    



    if ((media >= &gsmsdp_free_media_chunk[0]) &&
        (media <= &gsmsdp_free_media_chunk[GSMSDP_PERM_MEDIA_ELEMS-1])) {
        
        (void)sll_lite_link_head(&gsmsdp_free_media_list,
                                 (sll_lite_node_t *)media);
    } else {
        
        cpr_free(media);
        GSM_DEBUG(DEB_F_PREFIX"free media %p to dynamic pool",
                  DEB_F_PREFIX_ARGS(GSM, fname), media);
    }
}












static void
gsmsdp_init_media (fsmdef_media_t *media)
{
    media->refid = CC_NO_MEDIA_REF_ID;
    media->type = SDP_MEDIA_INVALID; 
    media->packetization_period = ATTR_PTIME;
    media->max_packetization_period = ATTR_MAXPTIME;
    media->mode = (uint16_t)vcmGetILBCMode();
    media->vad = VCM_VAD_OFF;
    
    media->level = 0;
    media->dest_port = 0;
    media->dest_addr = ip_addr_invalid;
    media->is_multicast = FALSE;
    media->multicast_port = 0;
    media->avt_payload_type = RTP_NONE;
    media->src_port = 0;
    media->src_addr = ip_addr_invalid;
    media->rcv_chan = FALSE;
    media->xmit_chan = FALSE;

    media->direction = SDP_DIRECTION_INACTIVE;
    media->direction_set = FALSE;
    media->transport = SDP_TRANSPORT_INVALID;
    media->tias_bw = SDP_INVALID_VALUE;
    media->profile_level = 0;

    media->previous_sdp.avt_payload_type = RTP_NONE;
    media->previous_sdp.dest_addr = ip_addr_invalid;
    media->previous_sdp.dest_port = 0;
    media->previous_sdp.direction = SDP_DIRECTION_INACTIVE;
    media->previous_sdp.packetization_period = media->packetization_period;
    media->previous_sdp.max_packetization_period = media->max_packetization_period;
    media->previous_sdp.payloads = NULL;
    media->previous_sdp.num_payloads = 0;
    media->previous_sdp.tias_bw = SDP_INVALID_VALUE;
    media->previous_sdp.profile_level = 0;
    media->hold  = FSM_HOLD_NONE;
    media->flags = 0;                    
    media->cap_index = CC_MAX_MEDIA_CAP; 
    media->video = NULL;
    media->candidate_ct = 0;
    media->rtcp_mux = FALSE;
    media->audio_level = TRUE;
    media->audio_level_id = 1;
    
    media->setup = SDP_SETUP_ACTPASS;
    media->local_datachannel_port = 0;
    media->remote_datachannel_port = 0;
    media->datachannel_streams = WEBRTC_DATACHANNEL_STREAMS_DEFAULT;
    sstrncpy(media->datachannel_protocol, WEBRTC_DATA_CHANNEL_PROT, SDP_MAX_STRING_LEN);

    media->payloads = NULL;
    media->num_payloads = 0;
}
















static fsmdef_media_t *
gsmsdp_get_new_media (fsmdef_dcb_t *dcb_p, sdp_media_e media_type,
                      uint16_t level)
{
    static const char fname[] = "gsmsdp_get_new_media";
    fsmdef_media_t *media;
    static media_refid_t media_refid = CC_NO_MEDIA_REF_ID;
    sll_lite_return_e sll_lite_ret;

    
    if (GSMSDP_MEDIA_COUNT(dcb_p) >= GSMSDP_MAX_MLINES_PER_CALL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"exceeding media lines per call",
                    dcb_p->line, dcb_p->call_id, fname);
        return (NULL);
    }

    
    media = gsmsdp_alloc_media();
    if (media != NULL) {
        
        gsmsdp_init_media(media);

        
        if (++media_refid == CC_NO_MEDIA_REF_ID) {
            media_refid = 1;
        }
        media->refid = media_refid;
        media->type  = media_type;
        media->level = level;

        
        sll_lite_ret = sll_lite_link_tail(&dcb_p->media_list,
                           (sll_lite_node_t *)media);
        if (sll_lite_ret != SLL_LITE_RET_SUCCESS) {
            
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"error %d when add media to list",
                        dcb_p->line, dcb_p->call_id, fname, sll_lite_ret);
            gsmsdp_free_media(media);
            media = NULL;
        }
    }
    return (media);
}














static void gsmsdp_remove_media (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_remove_media";
    cc_action_data_t data;

    if (media == NULL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"removing NULL media",
                    dcb_p->line, dcb_p->call_id, fname);
        return;
    }

    if (media->rcv_chan || media->xmit_chan) {
        
        data.stop_media.media_refid = media->refid;
        (void)cc_call_action(dcb_p->call_id, dcb_p->line, CC_ACTION_STOP_MEDIA,
                             &data);
    }
    
    (void)sll_lite_remove(&dcb_p->media_list, (sll_lite_node_t *)media);

    
    vcmRxReleasePort(media->cap_index, dcb_p->group_id, media->refid,
                 lsm_get_ms_ui_call_handle(dcb_p->line, dcb_p->call_id, CC_NO_CALL_ID), media->src_port);

    
    gsmsdp_free_media(media);
}












void gsmsdp_clean_media_list (fsmdef_dcb_t *dcb_p)
{
    fsmdef_media_t *media = NULL;

    while (TRUE) {
        
        media = (fsmdef_media_t *)sll_lite_unlink_head(&dcb_p->media_list);
        if (media != NULL) {
            gsmsdp_free_media(media);
        } else {
            break;
        }
    }
}













void gsmsdp_init_media_list (fsmdef_dcb_t *dcb_p)
{
    const cc_media_cap_table_t *media_cap_tbl;
    const cc_media_remote_stream_table_t *media_track_tbl;
    const char                 fname[] = "gsmsdp_init_media_list";

    
    (void)sll_lite_init(&dcb_p->media_list);

    media_cap_tbl = gsmsdp_get_media_capability(dcb_p);

    if (media_cap_tbl == NULL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capbility available",
                    dcb_p->line, dcb_p->call_id, fname);
    }

    media_track_tbl = gsmsdp_get_media_stream_table(dcb_p);

    if (media_track_tbl == NULL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media tracks available",
                    dcb_p->line, dcb_p->call_id, fname);
    }
}













static fsmdef_media_t *
gsmsdp_find_media_by_level (fsmdef_dcb_t *dcb_p, uint16_t level)
{
    fsmdef_media_t *media = NULL;

    



    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->level == level) {
            
            return (media);
        }
    }
    return (NULL);
}













fsmdef_media_t *
gsmsdp_find_media_by_refid (fsmdef_dcb_t *dcb_p, media_refid_t refid)
{
    fsmdef_media_t *media = NULL;

    



    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->refid == refid) {
            
            return (media);
        }
    }
    return (NULL);
}













static fsmdef_media_t *
gsmsdp_find_media_by_cap_index (fsmdef_dcb_t *dcb_p, uint8_t cap_index)
{
    fsmdef_media_t *media = NULL;

    



    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->cap_index == cap_index) {
            
            return (media);
        }
    }
    return (NULL);

}












fsmdef_media_t *gsmsdp_find_audio_media (fsmdef_dcb_t *dcb_p)
{
    fsmdef_media_t *media = NULL;

    



    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->type == SDP_MEDIA_AUDIO) {
            
            return (media);
        }
    }
    return (NULL);
}











static uint16_t
gsmsdp_find_unused_media_line_with_type (void *sdp, sdp_media_e media_type)
{
    uint16_t num_m_lines, level;
    int32_t  port;

    num_m_lines  = sdp_get_num_media_lines(sdp);
    for (level = 1; level <= num_m_lines; level++) {
        port = sdp_get_media_portnum(sdp, level);
        if (port == 0) {
            
            if (sdp_get_media_type(sdp, level) == media_type) {
                
                return (level);
            }
        }
    }
    
    return (0);
}












static const cc_media_cap_t *
gsmsdp_get_media_cap_entry_by_index (uint8_t cap_index, fsmdef_dcb_t *dcb_p)
{
    const cc_media_cap_table_t *media_cap_tbl;

    media_cap_tbl = dcb_p->media_cap_tbl;

    if (media_cap_tbl == NULL) {
        return (NULL);
    }

    if (cap_index >= CC_MAX_MEDIA_CAP) {
        return (NULL);
    }
    return (&media_cap_tbl->cap[cap_index]);
}














fsmdef_media_t *
gsmsdp_find_anat_pair (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    fsmdef_media_t *searched_media = NULL;

    




    GSMSDP_FOR_ALL_MEDIA(searched_media, dcb_p) {
        if ((searched_media->cap_index == media->cap_index) &&
            (searched_media->level != media->level)) {
            
            return (searched_media);
        }
    }
    return (NULL);
}
















static boolean
gsmsdp_platform_addition_mix (fsmdef_dcb_t *dcb_p, sdp_media_e media_type)
{
    return (FALSE);
}
















static void
gsmsdp_update_local_time_stamp (fsmdef_dcb_t *dcb_p, boolean offer,
                                boolean initial_offer)
{
    const char                 fname[] = "gsmsdp_update_local_time_stamp";
    void           *local_sdp_p;
    void           *remote_sdp_p;

    local_sdp_p  = dcb_p->sdp->src_sdp;
    remote_sdp_p = dcb_p->sdp->dest_sdp;

    



    if (initial_offer) {
        



        (void) sdp_set_time_start(local_sdp_p,
                                  sdp_get_time_start(remote_sdp_p));
        (void) sdp_set_time_stop(local_sdp_p, sdp_get_time_stop(remote_sdp_p));
    } else if (offer) {
        


        if (sdp_timespec_valid(remote_sdp_p) != TRUE) {
            GSM_DEBUG(DEB_L_C_F_PREFIX"\nTimespec is invalid.",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            (void) sdp_set_time_start(local_sdp_p, "0");
            (void) sdp_set_time_stop(local_sdp_p, "0");
        } else {
            if (sdp_get_time_start(local_sdp_p) !=
                sdp_get_time_start(remote_sdp_p)) {
                (void) sdp_set_time_start(local_sdp_p,
                                          sdp_get_time_start(remote_sdp_p));
            }
            if (sdp_get_time_stop(local_sdp_p) !=
                sdp_get_time_stop(remote_sdp_p)) {
                (void) sdp_set_time_stop(local_sdp_p,
                                         sdp_get_time_stop(remote_sdp_p));
            }
        }
    }
}












static void
gsmsdp_get_local_source_v4_address (fsmdef_media_t *media)
{
    int              nat_enable = 0;
    char             curr_media_ip[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t    addr;
    const char       fname[] = "gsmsdp_get_local_source_v4_address";

    

;
    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        init_empty_str(curr_media_ip);
        config_get_value(CFGID_MEDIA_IP_ADDR, curr_media_ip,
                        MAX_IPADDR_STR_LEN);
        if (is_empty_str(curr_media_ip) == FALSE) {

        	 str2ip(curr_media_ip, &addr);
             util_ntohl(&addr, &addr);
             if (util_check_if_ip_valid(&media->src_addr) == FALSE)  {
                 
                 media->src_addr = addr;
                 GSM_ERR_MSG("%s:  Update IP %s", fname, curr_media_ip);
             }
        } else {
            sip_config_get_net_device_ipaddr(&media->src_addr);
        }
    } else {
        sip_config_get_nat_ipaddr(&media->src_addr);
    }
}












static void
gsmsdp_get_local_source_v6_address (fsmdef_media_t *media)
{
    int      nat_enable = 0;

    


    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        sip_config_get_net_ipv6_device_ipaddr(&media->src_addr);
    } else {
        sip_config_get_nat_ipaddr(&media->src_addr);
    }
}













static void
gsmsdp_set_connection_address (void *sdp_p, uint16_t level, char *addr)
{
    



    (void) sdp_set_conn_nettype(sdp_p, level, SDP_NT_INTERNET);

    if (addr && (strlen(addr) > strlen("123.123.123.123")))
    {
      
      (void) sdp_set_conn_addrtype(sdp_p, level, SDP_AT_IP6);
    }
    else
    {
      (void) sdp_set_conn_addrtype(sdp_p, level, SDP_AT_IP4);
    }

    (void) sdp_set_conn_address(sdp_p, level, addr);
}














static void
gsmsdp_set_2543_hold_sdp (fsmdef_dcb_t *dcb_p, uint16 level)
{
    (void) sdp_set_conn_nettype(dcb_p->sdp->src_sdp, level, SDP_NT_INTERNET);
    (void) sdp_set_conn_addrtype(dcb_p->sdp->src_sdp, level, SDP_AT_IP4);
    (void) sdp_set_conn_address(dcb_p->sdp->src_sdp, level, "0.0.0.0");
}


















static void
gsmsdp_set_video_media_attributes (uint32_t media_type, void *cc_sdp_p, uint16_t level,
                             uint16_t payload_number)
{
    uint16_t a_inst;
    int added_fmtp = 0;
    void *sdp_p = ((cc_sdp_t*)cc_sdp_p)->src_sdp;
    int max_fs = 0;
    int max_fr = 0;
    int max_br = 0;
    int max_mbps = 0;

    switch (media_type) {
        case RTP_H263:
        case RTP_H264_P0:
        case RTP_H264_P1:
        case RTP_VP8:
        


        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTPMAP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }

        (void) sdp_attr_set_rtpmap_payload_type(sdp_p, level, 0, a_inst,
                                                payload_number);

        switch (media_type) {
        case RTP_H263:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_H263v2);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                             RTPMAP_VIDEO_CLOCKRATE);
            break;
        case RTP_H264_P0:
        case RTP_H264_P1:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_H264);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                             RTPMAP_VIDEO_CLOCKRATE);
            
            if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst)
                != SDP_SUCCESS) {
                GSM_ERR_MSG("Failed to add attribute");
                return;
            }
            added_fmtp = 1;
            (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst,
                                                  payload_number);
            {
                char buffer[32];
                uint32_t profile_level_id = vcmGetVideoH264ProfileLevelID();
                snprintf(buffer, sizeof(buffer), "%06x", profile_level_id);
                (void) sdp_attr_set_fmtp_profile_level_id(sdp_p, level, 0, a_inst,
                                                          buffer);
            }
            if (media_type == RTP_H264_P1) {
                (void) sdp_attr_set_fmtp_pack_mode(sdp_p, level, 0, a_inst,
                                                   1);
            }
            
        
        
        
        
        
        
        
            break;
        case RTP_VP8:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_VP8);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                             RTPMAP_VIDEO_CLOCKRATE);
            break;
        }

        switch (media_type) {
        case RTP_H264_P0:
        case RTP_H264_P1:
            max_br = config_get_video_max_br((rtp_ptype) media_type); 
            max_mbps = config_get_video_max_mbps((rtp_ptype) media_type); 
            
        case RTP_VP8:
            max_fs = config_get_video_max_fs((rtp_ptype) media_type);
            max_fr = config_get_video_max_fr((rtp_ptype) media_type);

            if (max_fs || max_fr || max_br || max_mbps) {
                if (!added_fmtp) {
                    if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst)
                        != SDP_SUCCESS) {
                        GSM_ERR_MSG("Failed to add attribute");
                        return;
                    }
                    added_fmtp = 1;

                    (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst,
                                                          payload_number);
                }

                if (max_fs) {
                    (void) sdp_attr_set_fmtp_max_fs(sdp_p, level, 0, a_inst,
                                                    max_fs);
                }
                if (max_fr) {
                    (void) sdp_attr_set_fmtp_max_fr(sdp_p, level, 0, a_inst,
                                                    max_fr);
                }
                if (max_br) {
                    (void) sdp_attr_set_fmtp_max_br(sdp_p, level, 0, a_inst,
                                                    max_br);
                }
                if (max_mbps) {
                    (void) sdp_attr_set_fmtp_max_mbps(sdp_p, level, 0, a_inst,
                                                      max_mbps);
                }
            }
            break;
        }
        break;

        default:
            break;
    }
}

















static void
gsmsdp_set_media_attributes (uint32_t media_type, void *sdp_p, uint16_t level,
                             uint16_t payload_number)
{
    uint16_t a_inst, a_inst2, a_inst3, a_inst4;
    int      maxavbitrate = 0;
    int      maxcodedaudiobw = 0;
    int      usedtx = 0;
    int      stereo = 0;
    int      useinbandfec = 0;
    int      cbr = 0;
    int      maxptime = 0;


    config_get_value(CFGID_MAXAVBITRATE, &maxavbitrate, sizeof(maxavbitrate));
    config_get_value(CFGID_MAXCODEDAUDIOBW, &maxcodedaudiobw, sizeof(maxcodedaudiobw));
    config_get_value(CFGID_USEDTX, &usedtx, sizeof(usedtx));
    config_get_value(CFGID_STEREO, &stereo, sizeof(stereo));
    config_get_value(CFGID_USEINBANDFEC, &useinbandfec, sizeof(useinbandfec));
    config_get_value(CFGID_CBR, &cbr, sizeof(cbr));
    config_get_value(CFGID_MAXPTIME, &maxptime, sizeof(maxptime));



    switch (media_type) {
    case RTP_PCMU:             
    case RTP_PCMA:             
    case RTP_G729:             
    case RTP_G722:             
    case RTP_ILBC:
    case RTP_L16:
    case RTP_ISAC:
    case RTP_OPUS:
        


        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTPMAP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }

        (void) sdp_attr_set_rtpmap_payload_type(sdp_p, level, 0, a_inst,
                                                payload_number);

        switch (media_type) {
        case RTP_PCMU:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_PCMU);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);
            break;
        case RTP_PCMA:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_PCMA);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);
            break;
        case RTP_G729:
            {

            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_G729);
            if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst2)
                    != SDP_SUCCESS) {
                return;
            }
            (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst2,
                                                  payload_number);
            (void) sdp_attr_set_fmtp_annexb(sdp_p, level, 0, a_inst2, FALSE);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);
            }
            break;

        case RTP_G722:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_G722);
            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_CLOCKRATE);
            break;

        case RTP_L16:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_L16_256K);

            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_L16_CLOCKRATE);
            break;

        case RTP_ILBC:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_ILBC);
            if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst2)
                    != SDP_SUCCESS) {
                return;
            }
            (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst2,
                                                  payload_number);
            (void) sdp_attr_set_fmtp_mode(sdp_p, level, 0, a_inst2, vcmGetILBCMode());

            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                RTPMAP_CLOCKRATE);
            break;

        case RTP_ISAC:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_ISAC);

            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                                 RTPMAP_ISAC_CLOCKRATE);
            break;

        case RTP_OPUS:
            (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                               SIPSDP_ATTR_ENCNAME_OPUS);

            (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
            		RTPMAP_OPUS_CLOCKRATE);
            (void) sdp_attr_set_rtpmap_num_chan (sdp_p, level, 0, a_inst, 2);

            
            if (maxavbitrate || maxcodedaudiobw || usedtx || stereo || useinbandfec || cbr) {
                if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst2)
                    != SDP_SUCCESS) {
                    return;
                }

                (void) sdp_attr_set_fmtp_payload_type (sdp_p, level, 0, a_inst2, payload_number);

                if (maxavbitrate)
                    sdp_attr_set_fmtp_max_average_bitrate (sdp_p, level, 0, a_inst2, FMTP_MAX_AVERAGE_BIT_RATE);

                if(usedtx)
                    sdp_attr_set_fmtp_usedtx (sdp_p, level, 0, a_inst2, FALSE);

                if(stereo)
                    sdp_attr_set_fmtp_stereo (sdp_p, level, 0, a_inst2, FALSE);

                if(useinbandfec)
                    sdp_attr_set_fmtp_useinbandfec (sdp_p, level, 0, a_inst2, FALSE);

                if(maxcodedaudiobw) {
                    sdp_attr_set_fmtp_maxcodedaudiobandwidth (sdp_p, level, 0, a_inst2,
            		     max_coded_audio_bandwidth_table[opus_fb].name);
                }

                if(cbr)
                    sdp_attr_set_fmtp_cbr (sdp_p, level, 0, a_inst2, FALSE);
            }

            
            if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_PTIME, &a_inst3)
                    != SDP_SUCCESS) {
                return;
            }

            sdp_attr_set_simple_u32(sdp_p, SDP_ATTR_PTIME, level, 0, a_inst3, ATTR_PTIME);

            if(maxptime) {
                
                if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_MAXPTIME, &a_inst4)
                        != SDP_SUCCESS) {
                    return;
                }

                sdp_attr_set_simple_u32(sdp_p, SDP_ATTR_MAXPTIME, level, 0, a_inst4, ATTR_MAXPTIME);
            }

            break;
        }
        break;

    case RTP_AVT:
        


        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTPMAP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }
        (void) sdp_attr_set_rtpmap_encname(sdp_p, level, 0, a_inst,
                                           SIPSDP_ATTR_ENCNAME_TEL_EVENT);
        (void) sdp_attr_set_rtpmap_payload_type(sdp_p, level, 0, a_inst,
                                                payload_number);
        (void) sdp_attr_set_rtpmap_clockrate(sdp_p, level, 0, a_inst,
                                             RTPMAP_CLOCKRATE);

        


        if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_FMTP, &a_inst)
                != SDP_SUCCESS) {
            return;
        }
        (void) sdp_attr_set_fmtp_payload_type(sdp_p, level, 0, a_inst,
                                              payload_number);
        (void) sdp_attr_set_fmtp_range(sdp_p, level, 0, a_inst,
                                       SIPSDP_NTE_DTMF_MIN,
                                       SIPSDP_NTE_DTMF_MAX);


        break;

    default:
        















        break;
    }
}













static void
gsmsdp_set_sctp_attributes (void *sdp_p, uint16_t level, fsmdef_media_t *media)
{
    uint16_t a_inst;

    if (sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_SCTPMAP, &a_inst)
        != SDP_SUCCESS) {
            return;
    }

    sdp_attr_set_sctpmap_port(sdp_p, level, 0, a_inst,
        media->local_datachannel_port);

    sdp_attr_set_sctpmap_protocol (sdp_p, level, 0, a_inst,
        media->datachannel_protocol);

    sdp_attr_set_sctpmap_streams (sdp_p, level, 0, a_inst,
        media->datachannel_streams);
}













static void
gsmsdp_set_remote_sdp (fsmdef_dcb_t *dcb_p, cc_sdp_t *sdp_p)
{
    dcb_p->remote_sdp_present = TRUE;
}














static sdp_attr_e
gsmsdp_get_sdp_direction_attr (sdp_direction_e direction)
{
    sdp_attr_e sdp_attr = SDP_ATTR_SENDRECV;

    switch (direction) {
    case SDP_DIRECTION_INACTIVE:
        sdp_attr = SDP_ATTR_INACTIVE;
        break;
    case SDP_DIRECTION_SENDONLY:
        sdp_attr = SDP_ATTR_SENDONLY;
        break;
    case SDP_DIRECTION_RECVONLY:
        sdp_attr = SDP_ATTR_RECVONLY;
        break;
    case SDP_DIRECTION_SENDRECV:
        sdp_attr = SDP_ATTR_SENDRECV;
        break;
    default:
        GSM_ERR_MSG("\nFSMDEF ERROR: replace with formal error text");
    }

    return sdp_attr;
}















static void
gsmsdp_set_sdp_direction (fsmdef_media_t *media,
                          sdp_direction_e direction, void *sdp_p)
{
    sdp_attr_e    sdp_attr = SDP_ATTR_SENDRECV;
    uint16_t      a_instance = 0;

    


    sdp_attr = gsmsdp_get_sdp_direction_attr(direction);
    if (media->level) {
       (void) sdp_add_new_attr(sdp_p, media->level, 0, sdp_attr, &a_instance);
    } else {
       
       (void) sdp_add_new_attr(sdp_p, SDP_SESSION_LEVEL, 0, sdp_attr,
                               &a_instance);
    }
}

















static cc_causes_t
gsmsdp_get_ice_attributes (sdp_attr_e sdp_attr, uint16_t level, void *sdp_p, char ***ice_attribs, int *attributes_ctp)
{
    uint16_t        num_a_lines = 0;
    uint16_t        i;
    sdp_result_e    result;
    char*           ice_attrib;

    result = sdp_attr_num_instances(sdp_p, level, 0, sdp_attr, &num_a_lines);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("enumerating ICE attributes failed");
        return result;
    }

    if (num_a_lines < 1) {
        GSM_DEBUG("enumerating ICE attributes returned 0 attributes");
        return CC_CAUSE_OK;
    }

    *ice_attribs = (char **)cpr_malloc(num_a_lines * sizeof(char *));

    if (!(*ice_attribs))
      return CC_CAUSE_OUT_OF_MEM;

    *attributes_ctp = 0;

    for (i = 0; i < num_a_lines; i++) {
        result = sdp_attr_get_ice_attribute (sdp_p, level, 0, sdp_attr, (uint16_t) (i + 1),
          &ice_attrib);
        if (result != SDP_SUCCESS) {
            GSM_ERR_MSG("Failed to retrieve ICE attribute");
            cpr_free(*ice_attribs);
            return result == SDP_INVALID_SDP_PTR ?
                             CC_CAUSE_INVALID_SDP_POINTER :
                   result == SDP_INVALID_PARAMETER ?
                             CC_CAUSE_BAD_ICE_ATTRIBUTE :
                   
                             CC_CAUSE_ERROR;
        }
        (*ice_attribs)[i] = (char *) cpr_calloc(1, strlen(ice_attrib) + 1);
        if(!(*ice_attribs)[i])
            return CC_CAUSE_OUT_OF_MEM;

        sstrncpy((*ice_attribs)[i], ice_attrib, strlen(ice_attrib) + 1);
        (*attributes_ctp)++;
    }

    return CC_CAUSE_OK;
}















void
gsmsdp_set_ice_attribute (sdp_attr_e sdp_attr, uint16_t level, void *sdp_p, char *ice_attrib)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, sdp_attr, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_ice_attribute(sdp_p, level, 0, sdp_attr, a_instance, ice_attrib);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}














void
gsmsdp_set_rtcp_fb_ack_attribute (uint16_t level,
                                  void *sdp_p,
                                  u16 payload_type,
                                  sdp_rtcp_fb_ack_type_e ack_type)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }
    result = sdp_attr_set_rtcp_fb_ack(sdp_p, level, payload_type,
                                      a_instance, ack_type);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}













void
gsmsdp_set_extmap_attribute (uint16_t level,
                             void *sdp_p,
                             u16 id,
                             const char* uri)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_EXTMAP, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_extmap(sdp_p, level, id, uri, a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}














void
gsmsdp_set_rtcp_fb_nack_attribute (uint16_t level,
                                   void *sdp_p,
                                   u16 payload_type,
                                   sdp_rtcp_fb_nack_type_e nack_type)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_rtcp_fb_nack(sdp_p, level, payload_type,
                                       a_instance, nack_type);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}














void
gsmsdp_set_rtcp_fb_trr_int_attribute (uint16_t level,
                                      void *sdp_p,
                                      u16 payload_type,
                                      u32 trr_interval)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_rtcp_fb_trr_int(sdp_p, level, payload_type,
                                          a_instance, trr_interval);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}














void
gsmsdp_set_rtcp_fb_ccm_attribute (uint16_t level,
                                  void *sdp_p,
                                  u16 payload_type,
                                  sdp_rtcp_fb_ccm_type_e ccm_type)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_RTCP_FB, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_rtcp_fb_ccm(sdp_p, level, payload_type,
                                      a_instance, ccm_type);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}















static void
gsmsdp_set_rtcp_mux_attribute (sdp_attr_e sdp_attr, uint16_t level, void *sdp_p, boolean rtcp_mux)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, sdp_attr, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_rtcp_mux_attribute(sdp_p, level, 0, sdp_attr, a_instance, rtcp_mux);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}














static void
gsmsdp_set_setup_attribute(uint16_t level,
  void *sdp_p, sdp_setup_type_e setup_type) {
    uint16_t a_instance = 0;
    sdp_result_e result;

    result = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_SETUP, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_setup_attribute(sdp_p, level, 0,
      a_instance, setup_type);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set attribute");
    }
}
















static void
gsmsdp_set_dtls_fingerprint_attribute (sdp_attr_e sdp_attr, uint16_t level, void *sdp_p,
  char *hash_func, char *fingerprint)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;
    char hash_and_fingerprint[FSMDEF_MAX_DIGEST_ALG_LEN + FSMDEF_MAX_DIGEST_LEN + 2];

    snprintf(hash_and_fingerprint, sizeof(hash_and_fingerprint),
         "%s %s", hash_func, fingerprint);

    result = sdp_add_new_attr(sdp_p, level, 0, sdp_attr, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_dtls_fingerprint_attribute(sdp_p, level, 0, sdp_attr,
        a_instance, hash_and_fingerprint);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set dtls fingerprint attribute");
    }
}















static void
gsmsdp_set_identity_attribute (uint16_t level, void *sdp_p,
  char *identity)
{
    uint16_t      a_instance = 0;
    sdp_result_e  result;

    result = sdp_add_new_attr(sdp_p, level, 0, SDP_ATTR_IDENTITY, &a_instance);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to add attribute");
        return;
    }

    result = sdp_attr_set_simple_string(sdp_p, level, 0, SDP_ATTR_IDENTITY,
        a_instance, identity);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG("Failed to set identity attribute");
    }
}

















static void
gsmsdp_remove_sdp_direction (fsmdef_media_t *media,
                             sdp_direction_e direction, void *sdp_p)
{
    sdp_attr_e    sdp_attr = SDP_ATTR_SENDRECV;

    sdp_attr = gsmsdp_get_sdp_direction_attr(direction);
    (void) sdp_delete_attr(sdp_p, media->level, 0, sdp_attr, 1);
}














void
gsmsdp_set_local_sdp_direction (fsmdef_dcb_t *dcb_p,
                                fsmdef_media_t *media,
                                sdp_direction_e direction)
{
    




    if (media->type != SDP_MEDIA_APPLICATION) {
      if (media->direction_set) {
          media->previous_sdp.direction = media->direction;
          gsmsdp_remove_sdp_direction(media, media->direction,
                                      dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL );
          media->direction_set = FALSE;
      }
      gsmsdp_set_sdp_direction(media, direction, dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL);
    }

    




    media->direction = direction;
    media->direction_set = TRUE;
}

















static sdp_direction_e
gsmsdp_get_remote_sdp_direction (fsmdef_dcb_t *dcb_p, uint16_t level,
                                 cpr_ip_addr_t *dest_addr)
{
    sdp_direction_e direction = SDP_DIRECTION_SENDRECV;
    cc_sdp_t       *sdp_p = dcb_p->sdp;
    uint16_t       media_attr;
    uint16_t       i;
    uint32         port;
    int            sdpmode = 0;
    static const sdp_attr_e  dir_attr_array[] = {
        SDP_ATTR_INACTIVE,
        SDP_ATTR_RECVONLY,
        SDP_ATTR_SENDONLY,
        SDP_ATTR_SENDRECV,
        SDP_MAX_ATTR_TYPES
    };

    if (!sdp_p->dest_sdp) {
        return direction;
    }

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    media_attr = 0; 
    

















    for (i = 0; (dir_attr_array[i] != SDP_MAX_ATTR_TYPES); i++) {
        if (sdp_attr_num_instances(sdp_p->dest_sdp, level, 0,
                                   dir_attr_array[i], &media_attr) ==
            SDP_SUCCESS) {
            if (media_attr) {
                
                direction = sdp_get_media_direction(sdp_p->dest_sdp,
                                                    level, 0);
                break;
            }
        }
    }

    





    if (media_attr == 0) {
        
        direction = sdp_get_media_direction(sdp_p->dest_sdp,
                                            SDP_SESSION_LEVEL, 0);
    }

    



    if (dest_addr->type == CPR_IP_ADDR_IPV4 &&
        dest_addr->u.ip4 == 0) {

        





        port = sdp_get_media_portnum(sdp_p->dest_sdp, level);
        if (sdpmode && port != 0) {
            return direction;
        }

        direction = SDP_DIRECTION_INACTIVE;
    } else {

        
    }
    return direction;
}













static void
gsmsdp_feature_overide_direction (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    


    if ( CC_IS_VIDEO(media->cap_index) &&
                   dcb_p->join_call_id != CC_NO_CALL_ID ){
        media->support_direction = SDP_DIRECTION_INACTIVE;
    }

    if (CC_IS_VIDEO(media->cap_index) && media->support_direction == SDP_DIRECTION_INACTIVE) {
        DEF_DEBUG(GSM_F_PREFIX"video capability disabled to SDP_DIRECTION_INACTIVE", "gsmsdp_feature_overide_direction");
    }
}







































static sdp_direction_e
gsmsdp_negotiate_local_sdp_direction (fsmdef_dcb_t *dcb_p,
                                      fsmdef_media_t *media,
                                      boolean local_hold)
{
    sdp_direction_e direction = SDP_DIRECTION_SENDRECV;
    sdp_direction_e remote_direction = gsmsdp_get_remote_sdp_direction(dcb_p,
                                           media->level, &media->dest_addr);

    if (remote_direction == SDP_DIRECTION_SENDRECV) {
        if (local_hold) {
            if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
                (media->support_direction == SDP_DIRECTION_SENDONLY)) {
                direction = SDP_DIRECTION_SENDONLY;
            } else {
                direction = SDP_DIRECTION_INACTIVE;
            }
        } else {
            direction = media->support_direction;
        }
    } else if (remote_direction == SDP_DIRECTION_SENDONLY) {
        if (local_hold) {
            direction = SDP_DIRECTION_INACTIVE;
        } else {
            if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
                (media->support_direction == SDP_DIRECTION_RECVONLY)) {
                direction = SDP_DIRECTION_RECVONLY;
            } else {
                direction = SDP_DIRECTION_INACTIVE;
            }
        }
    } else if (remote_direction == SDP_DIRECTION_INACTIVE) {
        direction = SDP_DIRECTION_INACTIVE;
    } else if (remote_direction == SDP_DIRECTION_RECVONLY) {
        if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
            (media->support_direction == SDP_DIRECTION_SENDONLY)) {
            direction = SDP_DIRECTION_SENDONLY;
        } else {
            direction = SDP_DIRECTION_INACTIVE;
        }
    }

    return direction;
}



















static void
gsmsdp_add_default_audio_formats_to_local_sdp (fsmdef_dcb_t *dcb_p,
                                               cc_sdp_t * sdp_p,
                                               fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_add_default_audio_formats_to_local_sdp";
    int             local_media_types[CC_MAX_MEDIA_TYPES];
    int16_t         local_avt_payload_type = RTP_NONE;
    DtmfOutOfBandTransport_t transport = DTMF_OUTOFBAND_NONE;
    int             type_cnt;
    void           *local_sdp_p = NULL;
    uint16_t        media_format_count;
    uint16_t        level;
    int i;

    if (media) {
        level = media->level;
    } else {
        level = 1;
    }
    local_sdp_p = (void *) sdp_p->src_sdp;

    



    media_format_count = sip_config_local_supported_codecs_get(
                                (rtp_ptype *) local_media_types,
                                CC_MAX_MEDIA_TYPES);
    








    if (dcb_p && media && media->num_payloads == 0) {

        if (media->payloads &&
            (media->num_payloads < media_format_count)) {
            cpr_free(media->payloads);
            media->payloads = NULL;
        }

        if (!media->payloads) {
            media->payloads = cpr_calloc(media_format_count,
                                         sizeof(vcm_payload_info_t));
        }

        media->num_payloads = 0;
        for (i = 0; i < media_format_count; i++) {
            if (local_media_types[i] > RTP_NONE) {
                media->payloads[i].codec_type = local_media_types[i];
                media->payloads[i].local_rtp_pt = local_media_types[i];
                media->payloads[i].remote_rtp_pt = local_media_types[i];
                media->num_payloads++;
            }
        }
        gsmsdp_copy_payloads_to_previous_sdp(media);
    }

    


    config_get_value(CFGID_DTMF_OUTOFBAND, &transport, sizeof(transport));

    if ((transport == DTMF_OUTOFBAND_AVT) ||
        (transport == DTMF_OUTOFBAND_AVT_ALWAYS)) {
        int temp_payload_type = RTP_NONE;

        config_get_value(CFGID_DTMF_AVT_PAYLOAD,
                         &(temp_payload_type),
                         sizeof(temp_payload_type));
        local_avt_payload_type = (uint16_t) temp_payload_type;
    }

    


    for (type_cnt = 0;
         (type_cnt < media_format_count) &&
         (local_media_types[type_cnt] > RTP_NONE);
         type_cnt++) {

        if (sdp_add_media_payload_type(local_sdp_p, level,
                                       (uint16_t)local_media_types[type_cnt],
                                       SDP_PAYLOAD_NUMERIC) != SDP_SUCCESS) {
            GSM_ERR_MSG(DEB_L_C_F_PREFIX"Adding media payload type failed",
                        DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));

        }

        if (media->support_direction != SDP_DIRECTION_INACTIVE) {
            gsmsdp_set_media_attributes(local_media_types[type_cnt], local_sdp_p,
                                    level, (uint16_t)local_media_types[type_cnt]);
        }
    }

    


    if (local_avt_payload_type > RTP_NONE) {
        if (sdp_add_media_payload_type(local_sdp_p, level,
                                       local_avt_payload_type,
                                       SDP_PAYLOAD_NUMERIC) != SDP_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"Adding AVT payload type failed",
                        dcb_p->line, dcb_p->call_id, fname);
        }

        if (media->support_direction != SDP_DIRECTION_INACTIVE) {
            gsmsdp_set_media_attributes(RTP_AVT, local_sdp_p, level,
                                        local_avt_payload_type);
            if (media) {
                media->avt_payload_type = local_avt_payload_type;
            }
        }
    }
}



















static void
gsmsdp_add_default_video_formats_to_local_sdp (fsmdef_dcb_t *dcb_p,
                                               cc_sdp_t * sdp_p,
                                               fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_add_default_video_formats_to_local_sdp";
    int             video_media_types[CC_MAX_MEDIA_TYPES];
    int             type_cnt;
    void           *local_sdp_p = NULL;
    uint16_t        video_format_count;
    uint16_t        level;
    line_t          line = 0;
    callid_t        call_id = 0;
    int             i;

    if (dcb_p && media) {
        line = dcb_p->line;
        call_id = dcb_p->call_id;
    }
    GSM_DEBUG(DEB_L_C_F_PREFIX"", DEB_L_C_F_PREFIX_ARGS(GSM, line, call_id, fname));

    if (media) {
        level = media->level;
    } else {
        level = 2;
    }
    local_sdp_p = (void *) sdp_p->src_sdp;

    



    video_format_count = sip_config_video_supported_codecs_get( (rtp_ptype *) video_media_types,
                                                 CC_MAX_MEDIA_TYPES, TRUE );

    GSM_DEBUG(DEB_L_C_F_PREFIX"video_count=%d", DEB_L_C_F_PREFIX_ARGS(GSM, line, call_id, fname), video_format_count);
    







    if (dcb_p && media && media->num_payloads == 0) {
        if (media->payloads &&
            (media->num_payloads < video_format_count)) {
            cpr_free(media->payloads);
            media->payloads = NULL;
        }

        if (!media->payloads) {
            media->payloads = cpr_calloc(video_format_count,
                                         sizeof(vcm_payload_info_t));
        }

        media->num_payloads = 0;
        for (i = 0; i < video_format_count; i++) {
            if (video_media_types[i] > RTP_NONE) {
                media->payloads[i].codec_type = video_media_types[i];
                media->payloads[i].local_rtp_pt = video_media_types[i];
                media->payloads[i].remote_rtp_pt = video_media_types[i];
                media->num_payloads++;
            }
        }
        gsmsdp_copy_payloads_to_previous_sdp(media);
    }


    


    for (type_cnt = 0;
         (type_cnt < video_format_count) &&
         (video_media_types[type_cnt] > RTP_NONE);
         type_cnt++) {

        if (sdp_add_media_payload_type(local_sdp_p, level,
                                       (uint16_t)video_media_types[type_cnt],
                                       SDP_PAYLOAD_NUMERIC) != SDP_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"SDP ERROR(1)",
                        line, call_id, fname);
        }

        if (media->support_direction != SDP_DIRECTION_INACTIVE) {
            gsmsdp_set_video_media_attributes(video_media_types[type_cnt], sdp_p,
                               level, (uint16_t)video_media_types[type_cnt]);
        }
    }
}












static void gsmsdp_set_mid_attr (void *src_sdp_p, uint16_t level)
{
    uint16         inst_num;

    if (platform_get_ip_address_mode() == CPR_IP_MODE_DUAL) {
        


        (void) sdp_add_new_attr(src_sdp_p, level, 0, SDP_ATTR_MID, &inst_num);

        (void) sdp_attr_set_simple_u32(src_sdp_p, SDP_ATTR_MID, level, 0,
                                       inst_num, level);
    }
}











static void gsmsdp_set_anat_attr (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    void           *src_sdp_p = (void *) dcb_p->sdp->src_sdp;
    void           *dest_sdp_p = (void *) dcb_p->sdp->dest_sdp;
    uint16         inst_num;
    uint16_t       num_group_lines= 0;
    uint16_t       num_anat_lines = 0;
    u32            group_id_1, group_id_2;
    uint16_t       i;
    fsmdef_media_t *group_media;


    if (dest_sdp_p == NULL) {
        
        if (media->addr_type == SDP_AT_IP4) {
            group_media = gsmsdp_find_anat_pair(dcb_p, media);
            if (group_media != NULL) {
                


                 (void) sdp_add_new_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP, &inst_num);

                 (void) sdp_set_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, SDP_GROUP_ATTR_ANAT);

                 (void) sdp_set_group_num_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, 2);
                 (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, group_media->level);
                 (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, media->level);
            }
        }
    } else {
        
        (void) sdp_attr_num_instances(dest_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);

        for (i = 1; i <= num_group_lines; i++) {
             if (sdp_get_group_attr(dest_sdp_p, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {
                 num_anat_lines++;
             }
        }

        for (i = 1; i <= num_anat_lines; i++) {
             group_id_1 = sdp_get_group_id(dest_sdp_p, SDP_SESSION_LEVEL, 0, i, 1);
             group_id_2 = sdp_get_group_id(dest_sdp_p, SDP_SESSION_LEVEL, 0, i, 2);

             if ((media->level == group_id_1)  || (media->level == group_id_2)) {

                 group_media = gsmsdp_find_anat_pair(dcb_p, media);
                 if (group_media != NULL) {
                     if (sdp_get_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, i) != SDP_GROUP_ATTR_ANAT) {
                         


                         (void) sdp_add_new_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP, &inst_num);
                         (void) sdp_set_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, SDP_GROUP_ATTR_ANAT);

                     }
                     (void) sdp_set_group_num_id(src_sdp_p, SDP_SESSION_LEVEL, 0, i, 2);
                     (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, i, group_media->level);
                     (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, i, media->level);

                 } else {
                     


                     (void) sdp_add_new_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP, &inst_num);
                     (void) sdp_set_group_attr(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, SDP_GROUP_ATTR_ANAT);

                     (void) sdp_set_group_num_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, 1);
                     (void) sdp_set_group_id(src_sdp_p, SDP_SESSION_LEVEL, 0, inst_num, media->level);
                 }

             }
        }
    }
    gsmsdp_set_mid_attr (src_sdp_p, media->level);
}
























static void
gsmsdp_update_local_sdp_media (fsmdef_dcb_t *dcb_p, cc_sdp_t *cc_sdp_p,
                              boolean all_formats, fsmdef_media_t *media,
                              sdp_transport_e transport)
{
    static const char fname[] = "gsmsdp_update_local_sdp_media";
    uint16_t        port;
    sdp_result_e    result;
    uint16_t        level;
    void           *sdp_p;
    int             sdpmode = 0;
    int             i = 0;

    if (!dcb_p || !media)  {
        GSM_ERR_MSG(get_debug_string(FSMDEF_DBG_INVALID_DCB), fname);
        return;
    }
    level = media->level;
    port  = media->src_port;

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    sdp_p = cc_sdp_p ? (void *) cc_sdp_p->src_sdp : NULL;

    if (sdp_p == NULL) {

        gsmsdp_init_local_sdp(dcb_p->peerconnection, &(dcb_p->sdp));

        cc_sdp_p = dcb_p->sdp;
        if ((cc_sdp_p == NULL) || (cc_sdp_p->src_sdp == NULL)) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"sdp is NULL and init failed",
                    dcb_p->line, dcb_p->call_id, fname);
            return;
        }
        sdp_p = (void *) cc_sdp_p->src_sdp;
    } else {

    



    sdp_delete_media_line(sdp_p, level);
    media->direction_set = FALSE;
    }

    result = sdp_insert_media_line(sdp_p, level);
    if (result != SDP_SUCCESS) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"Inserting media line to Sdp failed",
                    dcb_p->line, dcb_p->call_id, fname);
        return;
    }

    gsmsdp_set_connection_address(sdp_p, media->level, dcb_p->ice_default_candidate_addr);

    (void) sdp_set_media_type(sdp_p, level, media->type);


    (void) sdp_set_media_portnum(sdp_p, level, port, media->local_datachannel_port);

    
    gsmsdp_update_local_sdp_media_transport(dcb_p, sdp_p, media, transport,
                                            all_formats);

    if (all_formats) {
        


        switch (media->type) {
        case SDP_MEDIA_AUDIO:
            gsmsdp_add_default_audio_formats_to_local_sdp(dcb_p, cc_sdp_p,
                                                          media);
            break;
        case SDP_MEDIA_VIDEO:
            gsmsdp_add_default_video_formats_to_local_sdp(dcb_p, cc_sdp_p,
                                                          media);
            break;
        case SDP_MEDIA_APPLICATION:
            gsmsdp_set_sctp_attributes (sdp_p, level, media);
            break;
        default:
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"SDP ERROR media %d for level %d is not"
                        " supported\n",
                        dcb_p->line, dcb_p->call_id, fname, media->type,
                        media->level);
            break;
        }
    } else {
        


        for(i = 0; i < media->num_payloads; i++) {
          result =
            sdp_add_media_payload_type(sdp_p, level,
                (uint16_t)(media->payloads[i].local_rtp_pt),
                SDP_PAYLOAD_NUMERIC);

          if (result != SDP_SUCCESS) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"Adding dynamic payload type failed",
                        dcb_p->line, dcb_p->call_id, fname);
          }

          switch (media->type) {
            case SDP_MEDIA_AUDIO:
              gsmsdp_set_media_attributes(media->payloads[i].codec_type,
                  sdp_p, level,
                  (uint16_t)(media->payloads[i].local_rtp_pt));
              break;
            case SDP_MEDIA_VIDEO:
              gsmsdp_set_video_media_attributes(media->payloads[i].codec_type,
                  cc_sdp_p, level,
                  (uint16_t)(media->payloads[i].local_rtp_pt));
              break;
            case SDP_MEDIA_APPLICATION:
              gsmsdp_set_sctp_attributes (sdp_p, level, media);
              break;
            default:
              GSM_ERR_MSG(GSM_L_C_F_PREFIX"SDP ERROR media %d for level %d is"
                        " not supported\n",
                        dcb_p->line, dcb_p->call_id, fname, media->type,
                        media->level);
              break;
          }

        }

        


        if (media->avt_payload_type > RTP_NONE) {
            result = sdp_add_media_payload_type(sdp_p, level,
                         (uint16_t)media->avt_payload_type,
                         SDP_PAYLOAD_NUMERIC);
            if (result != SDP_SUCCESS) {
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"Adding AVT payload type failed",
                            dcb_p->line, dcb_p->call_id, fname);
            }
            gsmsdp_set_media_attributes(RTP_AVT, sdp_p, level,
                (uint16_t) media->avt_payload_type);
        }
    }

    if (!sdpmode)
        gsmsdp_set_anat_attr(dcb_p, media);
}




















static boolean
gsmsdp_update_local_sdp (fsmdef_dcb_t *dcb_p, boolean offer,
                         boolean initial_offer,
                         fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_update_local_sdp";
    cc_action_data_t data;
    sdp_direction_e direction;
    boolean         local_hold = (boolean)FSM_CHK_FLAGS(media->hold, FSM_HOLD_LCL);
    sdp_result_e    sdp_res;

    if (media->src_port == 0) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"allocate receive port for media line",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        




        data.open_rcv.is_multicast = FALSE;
        data.open_rcv.listen_ip = ip_addr_invalid;
        data.open_rcv.port = 0;
        data.open_rcv.keep = FALSE;
        



        data.open_rcv.media_type = media->type;
        data.open_rcv.media_refid = media->refid;
        if (cc_call_action(dcb_p->call_id, dcb_p->line, CC_ACTION_OPEN_RCV,
                           &data) == CC_RC_SUCCESS) {
            
            media->src_port = data.open_rcv.port;
            media->rcv_chan = FALSE;  
        } else {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"allocate rx port failed",
                        dcb_p->line, dcb_p->call_id, fname);
            return (FALSE);
        }
    }

    


    direction = gsmsdp_negotiate_local_sdp_direction(dcb_p, media, local_hold);

    


    if (media->transport == SDP_TRANSPORT_RTPSAVP) {
        gsmsdp_update_crypto_transmit_key(dcb_p, media, offer,
                                       initial_offer, direction);
    }

    if (offer == TRUE) {
        gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, FALSE, media,
                                      media->transport);
    }

    


    if (media->direction_set) {
        if (media->direction != direction) {
            gsmsdp_set_local_sdp_direction(dcb_p, media, direction);
        }
    } else {
        gsmsdp_set_local_sdp_direction(dcb_p, media, direction);
    }
    return (TRUE);
}




















static boolean
gsmsdp_update_local_sdp_for_multicast (fsmdef_dcb_t *dcb_p,
                                      uint16_t portnum,
                                      fsmdef_media_t *media,
                                      boolean offer,
                                      boolean initial_offer)
{
   static const char fname[] = "gsmsdp_update_local_sdp_for_multicast";
    sdp_direction_e direction;
    char            addr_str[MAX_IPADDR_STR_LEN];
    uint16_t        level;
    char            *p_addr_str;
    char            *strtok_state;

    level = media->level;

    GSM_DEBUG(DEB_L_C_F_PREFIX"%d %d %d",
			  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
			  portnum, level, initial_offer);

    direction = gsmsdp_get_remote_sdp_direction(dcb_p, media->level,
                                                &media->dest_addr);
    GSM_DEBUG(DEB_L_C_F_PREFIX"sdp direction: %d",
              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), direction);
    



    gsmsdp_update_crypto_transmit_key(dcb_p, media, offer, initial_offer,
                                      direction);

    gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, FALSE,
                                  media, media->transport);

    


    if ((direction == SDP_DIRECTION_RECVONLY) || (direction == SDP_DIRECTION_INACTIVE)) {
        if ((media->support_direction == SDP_DIRECTION_SENDRECV) ||
            (media->support_direction == SDP_DIRECTION_RECVONLY)) {
            




        } else {
            direction = SDP_DIRECTION_INACTIVE;
            GSM_DEBUG(DEB_L_C_F_PREFIX"media line"
                      " does not support receive stream\n",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        }
        gsmsdp_set_local_sdp_direction(dcb_p, media, direction);
        media->direction_set = TRUE;
    } else {
        


        return (FALSE);
    }

    


    ipaddr2dotted(addr_str, &media->dest_addr);
    p_addr_str = PL_strtok_r(addr_str, "[ ]", &strtok_state);

    


    (void) sdp_set_media_portnum(dcb_p->sdp->src_sdp, level, portnum, 0);

    


    (void) sdp_set_conn_nettype(dcb_p->sdp->src_sdp, level, SDP_NT_INTERNET);
    (void) sdp_set_conn_addrtype(dcb_p->sdp->src_sdp, level, media->addr_type);
    (void) sdp_set_conn_address(dcb_p->sdp->src_sdp, level, p_addr_str);

    return (TRUE);
}














static int
gsmsdp_get_remote_avt_payload_type (uint16_t level, void *sdp_p)
{
    uint16_t        i;
    uint16_t        ptype;
    int             remote_avt_payload_type = RTP_NONE;
    uint16_t        num_a_lines = 0;
    const char     *encname = NULL;

    


    (void) sdp_attr_num_instances(sdp_p, level, 0, SDP_ATTR_RTPMAP,
                                  &num_a_lines);

    



    for (i = 0; i < num_a_lines; i++) {
        ptype = sdp_attr_get_rtpmap_payload_type(sdp_p, level, 0,
                                                 (uint16_t) (i + 1));
        if (sdp_media_dynamic_payload_valid(sdp_p, ptype, level)) {
            encname = sdp_attr_get_rtpmap_encname(sdp_p, level, 0,
                                                  (uint16_t) (i + 1));
            if (encname) {
                if (cpr_strcasecmp(encname, SIPSDP_ATTR_ENCNAME_TEL_EVENT) == 0) {
                    remote_avt_payload_type = ptype;
                }
            }
        }
    }
    return (remote_avt_payload_type);
}


#define MIX_NEAREND_STRING  "X-mix-nearend"





















static int
gsmsdp_negotiate_codec (fsmdef_dcb_t *dcb_p, cc_sdp_t *sdp_p,
                        fsmdef_media_t *media, boolean offer,
                        boolean initial_offer, uint16 media_level)
{
    static const char fname[] = "gsmsdp_negotiate_codec";
    rtp_ptype       pref_codec = RTP_NONE;
    uint16_t        i;
    uint16_t        j;
    int            *master_list_p = NULL;
    int            *slave_list_p = NULL;
    DtmfOutOfBandTransport_t transport = DTMF_OUTOFBAND_NONE;
    int             avt_payload_type;
    uint16_t        num_remote_types;
    uint16_t        num_local_types;
    uint16_t        num_master_types;
    uint16_t        num_slave_types;
    int             remote_codecs[CC_MAX_MEDIA_TYPES];
    int             remote_payload_types[CC_MAX_MEDIA_TYPES];
    int             local_codecs[CC_MAX_MEDIA_TYPES];
    sdp_payload_ind_e pt_indicator;
    uint32          ptime = 0;
    uint32          maxptime = 0;
    const char*     attr_label;
    uint16_t        level;
    boolean         explicit_reject = FALSE;
    boolean         found_codec = FALSE;
    int32_t         num_match_payloads = 0;
    int             codec = RTP_NONE;
    int             remote_pt = RTP_NONE;
    int32_t         payload_types_count = 0; 

    int             temp;
    u16             a_inst;
    vcm_payload_info_t *payload_info = NULL;
    vcm_payload_info_t *previous_payload_info;

    if (!dcb_p || !sdp_p || !media) {
        return (RTP_NONE);
    }

    level = media_level;
    attr_label = sdp_attr_get_simple_string(sdp_p->dest_sdp,
                                            SDP_ATTR_LABEL, level, 0, 1);

    if (attr_label != NULL) {
        if (strcmp(attr_label, MIX_NEAREND_STRING) == 0) {
            dcb_p->session = WHISPER_COACHING;
        }
    }
    


    num_remote_types = sdp_get_media_num_payload_types(sdp_p->dest_sdp, level);

    if (num_remote_types > CC_MAX_MEDIA_TYPES) {
        num_remote_types = CC_MAX_MEDIA_TYPES;
    }

    for (i = 0; i < num_remote_types; i++) {
        temp = sdp_get_media_payload_type(sdp_p->dest_sdp, level,
                                          (uint16_t) (i + 1), &pt_indicator);
        remote_codecs[i] = GET_CODEC_TYPE(temp);
        remote_payload_types[i] = GET_DYN_PAYLOAD_TYPE_VALUE(temp);
    }

    


    if (media->type == SDP_MEDIA_AUDIO) {
        num_local_types = sip_config_local_supported_codecs_get(
                                    (rtp_ptype *)local_codecs,
                                    CC_MAX_MEDIA_TYPES);
    } else if (media->type == SDP_MEDIA_VIDEO) {
        num_local_types = sip_config_video_supported_codecs_get(
            (rtp_ptype *)local_codecs, CC_MAX_MEDIA_TYPES, offer);
    } else {
        GSM_DEBUG(DEB_L_C_F_PREFIX"unsupported media type %d",
            DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
            media->type);
        return (RTP_NONE);
    }

    




    config_get_value(CFGID_DTMF_OUTOFBAND, &transport, sizeof(transport));

    


    media->previous_sdp.avt_payload_type = media->avt_payload_type;

    switch (transport) {
        case DTMF_OUTOFBAND_AVT:
            avt_payload_type = gsmsdp_get_remote_avt_payload_type(
                                   media->level, sdp_p->dest_sdp);
            if (avt_payload_type > RTP_NONE) {
                media->avt_payload_type = avt_payload_type;
            } else {
                media->avt_payload_type = RTP_NONE;
            }
            break;

        case DTMF_OUTOFBAND_AVT_ALWAYS:
            avt_payload_type = gsmsdp_get_remote_avt_payload_type(
                                   media->level, sdp_p->dest_sdp);
            if (avt_payload_type > RTP_NONE) {
                media->avt_payload_type = avt_payload_type;
            } else {
                




                config_get_value(CFGID_DTMF_AVT_PAYLOAD,
                                 &media->avt_payload_type,
                                 sizeof(media->avt_payload_type));

                GSM_DEBUG(DEB_L_C_F_PREFIX"AVT_ALWAYS forcing out-of-band DTMF,"
                          " payload_type = %d\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line,
                                                dcb_p->call_id, fname),
                          media->avt_payload_type);
            }
            break;

        case DTMF_OUTOFBAND_NONE:
        default:
            media->avt_payload_type = RTP_NONE;
            break;
    }

    





    pref_codec = sip_config_preferred_codec();
    if (pref_codec != RTP_NONE) {
        





        if (local_codecs[0] != pref_codec) {
            



            pref_codec = RTP_NONE;
        }
    }

    if (pref_codec == RTP_NONE) {
        master_list_p = remote_codecs;
        slave_list_p = local_codecs;
        num_master_types = num_remote_types;
        num_slave_types = num_local_types;
        GSM_DEBUG(DEB_L_C_F_PREFIX"Remote Codec list is Master",
            DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
    } else {
        master_list_p = local_codecs;
        slave_list_p = remote_codecs;
        num_master_types = num_local_types;
        num_slave_types = num_remote_types;
        GSM_DEBUG(DEB_L_C_F_PREFIX"Local Codec list is Master",
           DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
    }

    


    gsmsdp_copy_payloads_to_previous_sdp(media);

    



    media->num_payloads = 0;
    if(num_master_types <= num_slave_types ) {
      payload_types_count = num_master_types;
    } else {
      payload_types_count = num_slave_types;
    }

    
    if (media->payloads) {
        cpr_free(media->payloads);
    }

    
    media->payloads = cpr_calloc(payload_types_count,
                                 sizeof(vcm_payload_info_t));

    
    if (media->type == SDP_MEDIA_AUDIO) {
        ptime = sdp_attr_get_simple_u32(sdp_p->dest_sdp,
                                    SDP_ATTR_PTIME, level, 0, 1);
        if (ptime != 0) {
            media->packetization_period = (uint16_t) ptime;
        }
        maxptime = sdp_attr_get_simple_u32(sdp_p->dest_sdp,
                      SDP_ATTR_MAXPTIME, level, 0, 1);
        if (maxptime != 0) {
            media->max_packetization_period = (uint16_t) maxptime;
        }
    }

    for (i = 0; i < num_master_types; i++) {
        for (j = 0; j < num_slave_types; j++) {
            if (master_list_p[i] == slave_list_p[j]) {

                

                codec = slave_list_p[j];
                payload_info = &(media->payloads[media->num_payloads]);

                if (master_list_p == remote_codecs) {
                    remote_pt = remote_payload_types[i];
                } else {
                    remote_pt = remote_payload_types[j];
                }

                payload_info->codec_type = codec;
                payload_info->local_rtp_pt = remote_pt;
                payload_info->remote_rtp_pt = remote_pt;

                


                if (!offer) {
                    previous_payload_info =
                        gsmsdp_find_info_for_codec(codec,
                           media->previous_sdp.payloads,
                           media->previous_sdp.num_payloads, 0);
                    if ((previous_payload_info == NULL) ||
                        (previous_payload_info->local_rtp_pt
                            != payload_info->local_rtp_pt)) {
                        payload_info->local_rtp_pt = codec;
                    }
                }

                if (media->type == SDP_MEDIA_AUDIO) {

                    if (sdp_attr_rtpmap_payload_valid(sdp_p->dest_sdp, level, 0,
                        &a_inst, remote_pt) ) {
                        

                        payload_info->audio.channels =
                            sdp_attr_get_rtpmap_num_chan(sdp_p->dest_sdp,
                                                         level, 0, a_inst);
                        if (payload_info->audio.channels == 0) {
                            payload_info->audio.channels = 1;
                        }

                        


                        payload_info->audio.frequency =
                            sdp_attr_get_rtpmap_clockrate(sdp_p->dest_sdp,
                                      level, 0, a_inst);
                    } else {
                        GSM_DEBUG(DEB_L_C_F_PREFIX"Could not find rtpmap "
                            "entry for payload %d -- setting defaults\n",
                            DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line,
                            dcb_p->call_id, fname), codec);
                        payload_info->audio.channels = 1;
                        

                        switch (codec) {
                            case STATIC_RTP_AVP_DVI4_16000_1:
                                codec = RTP_DVI4;
                                payload_info->audio.frequency = 16000;
                                break;
                            case STATIC_RTP_AVP_L16_44100_2:
                                codec = RTP_L16;
                                payload_info->audio.frequency = 44100;
                                payload_info->audio.channels = 2;
                                break;
                            case STATIC_RTP_AVP_L16_44100_1:
                                codec = RTP_L16;
                                payload_info->audio.frequency = 44100;
                                break;
                            case STATIC_RTP_AVP_DVI4_11025_1:
                                codec = RTP_DVI4;
                                payload_info->audio.frequency = 11025;
                                break;
                            case STATIC_RTP_AVP_DVI4_22050_1:
                                codec = RTP_DVI4;
                                payload_info->audio.frequency = 22050;
                                break;
                            default:
                                payload_info->audio.frequency = 8000;
                        }
                    }


                    switch (codec) {
                        case RTP_PCMA:
                        case RTP_PCMU:
                            
                            payload_info->audio.packet_size =
                                payload_info->audio.frequency / 50;

                            payload_info->audio.bitrate = 8 *
                                payload_info->audio.frequency *
                                payload_info->audio.channels;
                            break;


                        case RTP_OPUS:
                            if (!sdp_attr_rtpmap_payload_valid(sdp_p->dest_sdp,
                                  level, 0, &a_inst, remote_pt) ||
                                (payload_info->audio.frequency
                                  != RTPMAP_OPUS_CLOCKRATE) ||
                                (payload_info->audio.channels != 2)) {

                                


                                explicit_reject = TRUE;
                                continue; 
                            }

                            
                            








                            payload_info->audio.channels = 1;
                            

                            
                            sdp_attr_get_fmtp_max_average_bitrate (
                                sdp_p->dest_sdp, level, 0, 1,
                                &payload_info->opus.max_average_bitrate);

                            payload_info->opus.maxcodedaudiobandwidth =
                                sdp_attr_get_fmtp_maxcodedaudiobandwidth(
                                    sdp_p->dest_sdp, level, 0, 1);

                            sdp_attr_get_fmtp_usedtx (sdp_p->dest_sdp, level, 0,
                                1, &payload_info->opus.usedtx);

                            sdp_attr_get_fmtp_stereo (sdp_p->dest_sdp, level, 0,
                                1, &payload_info->opus.stereo);

                            sdp_attr_get_fmtp_useinbandfec (sdp_p->dest_sdp,
                                level, 0, 1, &payload_info->opus.useinbandfec);

                            sdp_attr_get_fmtp_cbr (sdp_p->dest_sdp, level, 0, 1,
                                &payload_info->opus.cbr);

                            

                            payload_info->audio.frequency = 48000;
                            payload_info->audio.packet_size = 960;
                            payload_info->audio.bitrate = 16000; 
                            break;

                        case RTP_ISAC:
                            
                            payload_info->audio.frequency = 16000;
                            payload_info->audio.packet_size = 480;
                            payload_info->audio.bitrate = 32000;
                            break;

                        case RTP_G722:
                            














                            payload_info->audio.frequency = 16000;
                            payload_info->audio.packet_size = 320;
                            payload_info->audio.bitrate = 64000;
                            break;

                        case RTP_ILBC:
                            payload_info->ilbc.mode =
                              (uint16_t)sdp_attr_get_fmtp_mode_for_payload_type(
                                  sdp_p->dest_sdp, level, 0, remote_pt);

                            

                            if (payload_info->ilbc.mode == SIPSDP_ILBC_MODE20)
                            {
                                payload_info->audio.packet_size = 160;
                                payload_info->audio.bitrate = 15200;
                            }
                            else 
                            {
                                payload_info->audio.packet_size = 240;
                                payload_info->audio.bitrate = 13300;
                            }
                            break;

                          default:
                              GSM_DEBUG(DEB_L_C_F_PREFIX"codec=%d not setting "
                                  "codec parameters (not implemented)\n",
                                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line,
                                  dcb_p->call_id, fname), codec);
                            payload_info->audio.packet_size = -1;
                            payload_info->audio.bitrate = -1;
                            MOZ_ASSERT(0);
                        } 


                } else if (media->type == SDP_MEDIA_VIDEO) {
                    if ( media->video != NULL ) {
                       vcmFreeMediaPtr(media->video);
                       media->video = NULL;
                    }

                    if (!vcmCheckAttribs(codec, sdp_p, level, remote_pt,
                                         &media->video)) {
                          GSM_DEBUG(DEB_L_C_F_PREFIX"codec= %d ignored - "
                               "attribs not accepted\n",
                               DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line,
                               dcb_p->call_id, fname), codec);
                          explicit_reject = TRUE;
                          continue; 
                    }

                    
                    media->previous_sdp.tias_bw = media->tias_bw;
                    media->tias_bw =  ccsdpGetBandwidthValue(sdp_p,level, 1);
                    if ( (attr_label =
                        ccsdpAttrGetFmtpProfileLevelId(sdp_p,level,0,1))
                            != NULL ) {
                        media->previous_sdp.profile_level =
                            media->profile_level;
                        sscanf(attr_label,"%x", &media->profile_level);
                    }

                    


                    payload_info->video.width = 0;
                    payload_info->video.height = 0;

                    
                    payload_info->video.max_fs = 0;
                    sdp_attr_get_fmtp_max_fs(sdp_p->dest_sdp, level, 0, 1,
                                             &payload_info->video.max_fs);

                    
                    payload_info->video.max_fr = 0;
                    sdp_attr_get_fmtp_max_fr(sdp_p->dest_sdp, level, 0, 1,
                                             &payload_info->video.max_fr);
                } 

                GSM_DEBUG(DEB_L_C_F_PREFIX"codec= %d",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line,
                                            dcb_p->call_id, fname), codec);


                found_codec = TRUE;

                


                media->num_payloads++;
                if(media->num_payloads >= payload_types_count) {
                    
                    return codec;
                }

                if(offer) {
                    


                    return codec;
                }
            }
        }
    }

    
    if(found_codec) {
        return (media->payloads[0].codec_type);
    }

    








    if (!initial_offer && !explicit_reject) {
        for (i = 0; i < num_remote_types; i++) {
            if (media->num_payloads != 0 && media->payloads[0].codec_type ==
                remote_payload_types[i]) {
                GSM_DEBUG(DEB_L_C_F_PREFIX"local codec list was empty codec= %d"
                          " local=%d remote =%d\n", DEB_L_C_F_PREFIX_ARGS(GSM,
                          dcb_p->line, dcb_p->call_id, fname),
                          media->payloads[0].codec_type,
                          media->payloads[0].local_rtp_pt,
                          media->payloads[0].remote_rtp_pt);
                return (media->payloads[0].codec_type);
            }
        }
    }

    return (RTP_NONE);
}









static void
gsmsdp_negotiate_datachannel_attribs(fsmdef_dcb_t* dcb_p, cc_sdp_t* sdp_p, uint16_t level,
    fsmdef_media_t* media, boolean offer)
{
    uint32          num_streams;
    char           *protocol;

    sdp_attr_get_sctpmap_streams (sdp_p->dest_sdp, level, 0, 1, &num_streams);

    media->datachannel_streams = num_streams;

    sdp_attr_get_sctpmap_protocol(sdp_p->dest_sdp, level, 0, 1, media->datachannel_protocol);

    media->remote_datachannel_port = sdp_get_media_sctp_port(sdp_p->dest_sdp, level);
}

















static void
gsmsdp_add_unsupported_stream_to_local_sdp (cc_sdp_t *sdp_p,
                                            uint16_t level)
{
    static const char fname[] = "gsmsdp_add_unsupported_stream_to_local_sdp";
    uint32_t          remote_pt;
    sdp_payload_ind_e remote_pt_indicator;
    cpr_ip_addr_t     addr;

    if (sdp_p == NULL) {
        GSM_ERR_MSG(GSM_F_PREFIX"sdp is null.", fname);
        return;
    }

    if (sdp_get_media_type(sdp_p->src_sdp, level) != SDP_MEDIA_INVALID) {
        sdp_delete_media_line(sdp_p->src_sdp, level);
    }

    if (sdp_p->dest_sdp == NULL) {
        GSM_ERR_MSG(GSM_F_PREFIX"no remote SDP available", fname);
        return;
    }

    


    if (sdp_insert_media_line(sdp_p->src_sdp, level) != SDP_SUCCESS) {
        GSM_ERR_MSG(GSM_F_PREFIX"failed to insert a media line", fname);
        return;
    }

    



    (void) sdp_set_media_type(sdp_p->src_sdp, level,
                              sdp_get_media_type(sdp_p->dest_sdp, level));
    (void) sdp_set_media_portnum(sdp_p->src_sdp, level, 0, 0);
    (void) sdp_set_media_transport(sdp_p->src_sdp, level,
                    sdp_get_media_transport(sdp_p->dest_sdp, level));

    remote_pt = sdp_get_media_payload_type(sdp_p->dest_sdp, level, 1,
                                           &remote_pt_indicator);
    




    (void) sdp_add_media_payload_type(sdp_p->src_sdp, level,
                                      (uint16_t) remote_pt,
                                      remote_pt_indicator);
    
















    gsmsdp_set_connection_address(sdp_p->src_sdp, level, "0.0.0.0");
}






















static boolean
gsmsdp_get_remote_media_address (fsmdef_dcb_t *dcb_p,
                                 cc_sdp_t * sdp_p, uint16_t level,
                                 cpr_ip_addr_t *dest_addr)
{
   const char fname[] = "gsmsdp_get_remote_media_address";
    const char     *addr_str = NULL;
    int             dns_err_code;
    boolean         stat;

    *dest_addr = ip_addr_invalid;

    stat = sdp_connection_valid(sdp_p->dest_sdp, level);
    if (stat) {
        addr_str = sdp_get_conn_address(sdp_p->dest_sdp, level);
    } else {
        
        stat = sdp_connection_valid(sdp_p->dest_sdp, SDP_SESSION_LEVEL);
        if (stat) {
            addr_str = sdp_get_conn_address(sdp_p->dest_sdp, SDP_SESSION_LEVEL);
        }
    }

    if (stat && addr_str) {
        
        if (str2ip(addr_str, dest_addr) != 0) {
            
            dns_err_code = dnsGetHostByName(addr_str, dest_addr, 100, 1);
            if (dns_err_code) {
                *dest_addr = ip_addr_invalid;
                stat = FALSE;
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"DNS remote address error %d"
                            " with media at %d\n", dcb_p->line, dcb_p->call_id,
                            fname, dns_err_code, level);
            }
        }
    } else {
        


        GSM_ERR_MSG(GSM_L_C_F_PREFIX"No remote address from SDP with at %d",
                    dcb_p->line, dcb_p->call_id, fname, level);
    }
    





    util_ntohl(dest_addr, dest_addr);
    return (stat);
}



















int
gsmsdp_is_multicast_address (cpr_ip_addr_t theIpAddress)
{
    if  (theIpAddress.type == CPR_IP_ADDR_IPV4) {
    


        if ((theIpAddress.u.ip4 >= MULTICAST_START_ADDRESS) &&
            (theIpAddress.u.ip4 <= MULTICAST_END_ADDRESS)) {
        return (TRUE);
    }
    } else {
        

    }
    return (FALSE);
}

















static fsmdef_media_t*
gsmsdp_find_anat_media_line (fsmdef_dcb_t *dcb_p, cc_sdp_t *sdp_p, uint16_t level)
{
    fsmdef_media_t *anat_media = NULL;
    u32            group_id_1, group_id_2;
    u32            dst_mid, group_mid;
    uint16_t       num_group_lines= 0;
    uint16_t       num_anat_lines = 0;
    uint16_t       i;

    


    (void) sdp_attr_num_instances(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);

    for (i = 1; i <= num_group_lines; i++) {
         if (sdp_get_group_attr(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {
             num_anat_lines++;
         }
    }

    for (i = 1; i <= num_anat_lines; i++) {

        dst_mid = sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, level, 0, 1);
        group_id_1 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 1);
        group_id_2 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 2);

        if (dst_mid == group_id_1) {
            GSMSDP_FOR_ALL_MEDIA(anat_media, dcb_p) {
                group_mid = sdp_attr_get_simple_u32(sdp_p->src_sdp,
                                                    SDP_ATTR_MID, (uint16_t) group_id_2, 0, 1);
                if (group_mid == group_id_2) {
                    
                    return (anat_media);
                }
            }
        } else if (dst_mid == group_id_2) {
            GSMSDP_FOR_ALL_MEDIA(anat_media, dcb_p) {
                group_mid = sdp_attr_get_simple_u32(sdp_p->src_sdp,
                                                    SDP_ATTR_MID, (uint16_t) group_id_1, 0, 1);
                if (group_mid == group_id_1) {
                    
                    return (anat_media);
                }
            }
        }
    }
    return (anat_media);
}















static boolean
gsmsdp_validate_anat (cc_sdp_t *sdp_p)
{
    u16          i, num_group_id;
    u32          group_id_1, group_id_2;
    sdp_media_e  media_type_gid1, media_type_gid2;
    uint16_t     num_group_lines= 0;
    uint16_t     num_anat_lines = 0;

    


    (void) sdp_attr_num_instances(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);

    for (i = 1; i <= num_group_lines; i++) {
         if (sdp_get_group_attr(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {
             num_anat_lines++;
         }
    }

    for (i = 1; i <= num_anat_lines; i++) {
         num_group_id = sdp_get_group_num_id (sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i);
         if ((num_group_id <=0) || (num_group_id > 2)) {
             
             return (FALSE);
         } else if (num_group_id == 2) {
            
            group_id_1 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 1);
            group_id_2 = sdp_get_group_id(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i, 2);
            media_type_gid1 = sdp_get_media_type(sdp_p->dest_sdp, (u16) group_id_1);
            media_type_gid2 = sdp_get_media_type(sdp_p->dest_sdp, (u16) group_id_2);
            if (media_type_gid1 != media_type_gid2) {
                
                return (FALSE);
            }
            if (group_id_1 != sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, (u16) group_id_1, 0, 1)) {
                
                return (FALSE);
            }
            if (group_id_2 != sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, (u16) group_id_2, 0, 1)) {
                return (FALSE);
            }
         }
    }

    return (TRUE);
}
















static boolean
gsmsdp_validate_mid (cc_sdp_t *sdp_p, uint16_t level)
{
    int32     src_mid, dst_mid;
    u16       i;
    uint16_t  num_group_lines= 0;
    uint16_t  num_anat_lines = 0;

    


    (void) sdp_attr_num_instances(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, SDP_ATTR_GROUP,
                                  &num_group_lines);

    for (i = 1; i <= num_group_lines; i++) {
         if (sdp_get_group_attr(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0, i) == SDP_GROUP_ATTR_ANAT) {
             num_anat_lines++;
         }
    }


    if (num_anat_lines > 0) {
        dst_mid = sdp_attr_get_simple_u32(sdp_p->dest_sdp, SDP_ATTR_MID, level, 0, 1);
        if (dst_mid == 0) {
            return (FALSE);
        }
        if (sdp_get_group_attr(sdp_p->src_sdp, SDP_SESSION_LEVEL, 0, 1) == SDP_GROUP_ATTR_ANAT) {
            src_mid = sdp_attr_get_simple_u32(sdp_p->src_sdp, SDP_ATTR_MID, level, 0, 1);
            if (dst_mid != src_mid) {
                return (FALSE);
             }
        }

    }
    return (TRUE);
}















static boolean
gsmsdp_negotiate_addr_type (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_negotiate_addr_type";
    cpr_ip_type     media_addr_type;
    cpr_ip_mode_e   ip_mode;
    fsmdef_media_t  *group_media;

    media_addr_type = media->dest_addr.type;
    if ((media_addr_type != CPR_IP_ADDR_IPV4) &&
        (media_addr_type != CPR_IP_ADDR_IPV6)) {
        
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"address type is not IPv4 or IPv6",
                    dcb_p->line, dcb_p->call_id, fname);
        return (FALSE);
    }
    ip_mode = platform_get_ip_address_mode();
    


    group_media = gsmsdp_find_anat_pair(dcb_p, media);

    








    if (!FSM_CHK_FLAGS(media->flags, FSM_MEDIA_F_SUPPORT_SECURITY)) {
        if (media_addr_type != CPR_IP_ADDR_IPV4) {
            
            GSM_DEBUG(DEB_L_C_F_PREFIX"offboard device does not support IPV6",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
            return (FALSE);
        }

        




        if ((ip_mode == CPR_IP_MODE_DUAL) || (ip_mode == CPR_IP_MODE_IPV4)) {
            if (group_media != NULL) {
                



                return (FALSE);
            }
            gsmsdp_get_local_source_v4_address(media);
            return (TRUE);
        }
        
        return (FALSE);
    }

    if (ip_mode == CPR_IP_MODE_DUAL) {
         



         if (group_media == NULL) {
             



             if (media_addr_type == CPR_IP_ADDR_IPV4) {
                 gsmsdp_get_local_source_v4_address(media);
             } else {
                 gsmsdp_get_local_source_v6_address(media);
             }
             return (TRUE);
         }

         



         if (media_addr_type == CPR_IP_ADDR_IPV4) {
             




             return (FALSE);
         }

         
         if (group_media->src_addr.type == CPR_IP_ADDR_IPV4) {
             




              gsmsdp_add_unsupported_stream_to_local_sdp(dcb_p->sdp,
                                                         group_media->level);
              gsmsdp_remove_media(dcb_p, group_media);
              
              gsmsdp_get_local_source_v6_address(media);
              return (TRUE);
         }
         




         return (FALSE);
    }

    



    if ((ip_mode == CPR_IP_MODE_IPV6) &&
        (media_addr_type == CPR_IP_ADDR_IPV4)) {
        
        return (FALSE);
    }
    if ((ip_mode == CPR_IP_MODE_IPV4) &&
        (media_addr_type == CPR_IP_ADDR_IPV6)) {
        
        return (FALSE);
    }

    if (group_media != NULL) {
        



        return (FALSE);
    }

    



    if (media_addr_type == CPR_IP_ADDR_IPV4) {
        gsmsdp_get_local_source_v4_address(media);
    } else {
        gsmsdp_get_local_source_v6_address(media);
    }
    
    return (TRUE);
}

















static uint8_t
gsmdsp_find_best_match_media_cap_index (fsmdef_dcb_t    *dcb_p,
                                        cc_sdp_t        *sdp_p,
                                        fsmdef_media_t  *media,
                                        media_table_e   media_table)
{
    const cc_media_cap_t *media_cap;
    uint8_t              cap_index, candidate_cap_index;
    boolean              srtp_fallback;
    sdp_direction_e      remote_direction, support_direction;
    sdp_transport_e      remote_transport;
    sdp_media_e          media_type;

    remote_transport = sdp_get_media_transport(sdp_p->dest_sdp, media->level);
    remote_direction = gsmsdp_get_remote_sdp_direction(dcb_p, media->level,
                                                       &media->dest_addr);
    srtp_fallback    = sip_regmgr_srtp_fallback_enabled(dcb_p->line);
    media_type       = media->type;


    















    candidate_cap_index = CC_MAX_MEDIA_CAP;
    for (cap_index = 0; cap_index < CC_MAX_MEDIA_CAP; cap_index++) {
        
        if (media_table == MEDIA_TABLE_GLOBAL) {
            media_cap = &g_media_table.cap[cap_index];
        } else {
            media_cap = gsmsdp_get_media_cap_entry_by_index(cap_index,dcb_p);
        }
        if ((media_cap == NULL) || !media_cap->enabled ||
            (media_cap->type != media_type)) {
            
            continue;
        }

        
        if (gsmsdp_find_media_by_cap_index(dcb_p, cap_index) != NULL) {
            
            continue;
        }

        






        if (remote_transport == SDP_TRANSPORT_RTPSAVP) {
            if (!media_cap->support_security && !srtp_fallback) {
                



                continue;
            }
            if (!media_cap->support_security) {
                



                candidate_cap_index = cap_index;
            }
        }

        






        support_direction = media_cap->support_direction;
        if (remote_direction == SDP_DIRECTION_INACTIVE) {
            if (support_direction != SDP_DIRECTION_SENDRECV) {
                
                candidate_cap_index = cap_index;
            }
        } else if (remote_direction == SDP_DIRECTION_RECVONLY) {
            if ((support_direction != SDP_DIRECTION_SENDRECV) &&
                (support_direction != SDP_DIRECTION_SENDONLY)) {
                
                continue;
            } else if (support_direction != SDP_DIRECTION_SENDONLY) {
                candidate_cap_index = cap_index;
            }
        } else if (remote_direction == SDP_DIRECTION_SENDONLY) {
            if ((support_direction != SDP_DIRECTION_SENDRECV) &&
                (support_direction != SDP_DIRECTION_RECVONLY)) {
                
                continue;
            } else if (support_direction != SDP_DIRECTION_RECVONLY) {
                candidate_cap_index = cap_index;
            }
        } else if (remote_direction == SDP_DIRECTION_SENDRECV) {
            if (support_direction != SDP_DIRECTION_SENDRECV) {
                candidate_cap_index = cap_index;
            }
        }

        if (candidate_cap_index == cap_index) {
            
            continue;
        }
        
        break;
    }

    if (cap_index == CC_MAX_MEDIA_CAP) {
        if (candidate_cap_index != CC_MAX_MEDIA_CAP) {
            
            cap_index = candidate_cap_index;
        }
    }

    return cap_index;
}



















static boolean
gsmsdp_assign_cap_entry_to_incoming_media (fsmdef_dcb_t    *dcb_p,
                                           cc_sdp_t        *sdp_p,
                                           fsmdef_media_t  *media)
{
    static const char fname[] = "gsmsdp_assign_cap_entry_to_incoming_media";
    const cc_media_cap_t *media_cap;
    uint8_t              cap_index;
    fsmdef_media_t       *anat_media;

    



    anat_media = gsmsdp_find_anat_media_line(dcb_p, sdp_p, media->level);
    if (anat_media != NULL) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(anat_media->cap_index, dcb_p);
        if (media_cap == NULL) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capability",
                        dcb_p->line, dcb_p->call_id, fname);
            return (FALSE);
        }
        gsmsdp_set_media_capability(media, media_cap);
        
        media->cap_index = anat_media->cap_index;
        return (TRUE);
    }


    cap_index  = gsmdsp_find_best_match_media_cap_index(dcb_p,
                                                        sdp_p,
                                                        media,
                                                        MEDIA_TABLE_SESSION);

    if (cap_index == CC_MAX_MEDIA_CAP) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"reached max streams supported or"
                      " no suitable media capability\n",
                      dcb_p->line, dcb_p->call_id, fname);
            return (FALSE);
        }

    
    media_cap = gsmsdp_get_media_cap_entry_by_index(cap_index,dcb_p);
    if (media_cap == NULL) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media cap",
                    dcb_p->line, dcb_p->call_id, fname);
        return (FALSE);
    }
    gsmsdp_set_media_capability(media, media_cap);

    
    gsmsdp_feature_overide_direction(dcb_p, media);
    if (media->support_direction == SDP_DIRECTION_INACTIVE) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"feature overrides direction to inactive,"
                  " no capability assigned\n",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        return (FALSE);
    }

    media->cap_index = cap_index;
    GSM_DEBUG(DEB_L_C_F_PREFIX"assign media cap index %d",
              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), cap_index);
    return (TRUE);
}


















static fsmdef_media_t *
gsmsdp_negotiate_add_media_line (fsmdef_dcb_t  *dcb_p,
                                 sdp_media_e   media_type,
                                 uint16_t      level,
                                 uint16_t      remote_port,
                                 boolean       offer)
{
    static const char fname[] = "gsmsdp_negotiate_add_media_line";
    fsmdef_media_t       *media;

    if (remote_port == 0) {
        


        return (NULL);
    }

    if (!offer) {
        



        GSM_ERR_MSG(GSM_L_C_F_PREFIX"remote trying add media in answer SDP",
                    dcb_p->line, dcb_p->call_id, fname);
        return (NULL);
    }

    


    media = gsmsdp_get_new_media(dcb_p, media_type, level);
    if (media == NULL) {
        
        return (NULL);
    }

    



    if ((dcb_p->fcb->state == FSMDEF_S_HOLDING) ||
        (dcb_p->fcb->state == FSMDEF_S_HOLD_PENDING)) {
        
        FSM_SET_FLAGS(media->hold, FSM_HOLD_LCL);
    }
    return (media);
}


















static boolean
gsmsdp_negotiate_remove_media_line (fsmdef_dcb_t *dcb_p,
                                    fsmdef_media_t *media,
                                    uint16_t remote_port,
                                    boolean offer)
{
    static const char fname[] = "gsmsdp_negotiate_remove_media_line";

    if (offer) {
        
        if (remote_port != 0) {
            
            return (FALSE);
        }
        



    } else {
        
        if ((media->src_port != 0) && (remote_port != 0)) {
            
            return (FALSE);
        }
        







        if ((media->src_port == 0) && (remote_port != 0)) {
            
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"remote insists on keeping media line",
                        dcb_p->line, dcb_p->call_id, fname);
        }
    }

    


    return (TRUE);
}




fsmdef_media_t* gsmsdp_find_media_by_media_type(fsmdef_dcb_t *dcb_p, sdp_media_e media_type) {

    fsmdef_media_t *media = NULL;

    


    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->type == media_type) {
            
            return (media);
        }
    }
    return (NULL);
}


















cc_causes_t
gsmsdp_add_rtcp_fb (int level, sdp_t *sdp_p,
                    rtp_ptype codec, unsigned int types)
{
    int num_pts;
    int pt_codec;
    sdp_payload_ind_e indicator;

    int pt_index;
    unsigned int j;
    num_pts = sdp_get_media_num_payload_types(sdp_p, level);
    for (pt_index = 1; pt_index <= num_pts; pt_index++) {
        pt_codec = sdp_get_media_payload_type (sdp_p, level, pt_index,
                                               &indicator);
        if (codec == RTP_NONE || (pt_codec & 0xFF) == codec) {
            int pt = GET_DYN_PAYLOAD_TYPE_VALUE(pt_codec);

            
            for (j = 0; j < SDP_MAX_RTCP_FB_NACK; j++) {
                if (types & sdp_rtcp_fb_nack_to_bitmap(j)) {
                    gsmsdp_set_rtcp_fb_nack_attribute(level, sdp_p, pt, j);
                }
            }

            
            for (j = 0; j < SDP_MAX_RTCP_FB_ACK; j++) {
                if (types & sdp_rtcp_fb_ack_to_bitmap(j)) {
                    gsmsdp_set_rtcp_fb_nack_attribute(level, sdp_p, pt, j);
                }
            }

            
            for (j = 0; j < SDP_MAX_RTCP_FB_CCM; j++) {
                if (types & sdp_rtcp_fb_ccm_to_bitmap(j)) {
                    gsmsdp_set_rtcp_fb_ccm_attribute(level, sdp_p, pt, j);
                }
            }

        }
    }
    return CC_CAUSE_OK;
}

















cc_causes_t
gsmsdp_negotiate_rtcp_fb (cc_sdp_t *cc_sdp_p,
                          fsmdef_media_t *media,
                          boolean offer)
{
    int level = media->level;
    int pt_codec;
    int remote_pt;
    sdp_payload_ind_e indicator;
    int pt_index, i;
    sdp_rtcp_fb_nack_type_e nack_type;
    sdp_rtcp_fb_ack_type_e ack_type;
    sdp_rtcp_fb_ccm_type_e ccm_type;
    uint32_t fb_types = 0;

    int num_pts = sdp_get_media_num_payload_types(cc_sdp_p->dest_sdp, level);

    



    sdp_result_e result = SDP_SUCCESS;
    while (result == SDP_SUCCESS) {
        result = sdp_delete_attr (cc_sdp_p->src_sdp, level, 0,
                                  SDP_ATTR_RTCP_FB, 1);
    }

    



    for (pt_index = 1; pt_index <= num_pts; pt_index++) {
        int pt_codec = sdp_get_media_payload_type (cc_sdp_p->dest_sdp,
                                                   level, pt_index, &indicator);
        int codec = pt_codec & 0xFF;
        remote_pt = GET_DYN_PAYLOAD_TYPE_VALUE(pt_codec);
        fb_types = 0;

        
        i = 1;
        do {
            nack_type = sdp_attr_get_rtcp_fb_nack(cc_sdp_p->dest_sdp,
                                                  level, remote_pt, i);
            if (nack_type >= 0 && nack_type < SDP_MAX_RTCP_FB_NACK) {
                fb_types |= sdp_rtcp_fb_nack_to_bitmap(nack_type);
            }
            i++;
        } while (nack_type != SDP_RTCP_FB_NACK_NOT_FOUND);

        
        i = 1;
        do {
            ack_type = sdp_attr_get_rtcp_fb_ack(cc_sdp_p->dest_sdp,
                                                level, remote_pt, i);
            if (ack_type >= 0 && ack_type < SDP_MAX_RTCP_FB_ACK) {
                fb_types |= sdp_rtcp_fb_ack_to_bitmap(ack_type);
            }
            i++;
        } while (ack_type != SDP_RTCP_FB_ACK_NOT_FOUND);

        
        i = 1;
        do {
            ccm_type = sdp_attr_get_rtcp_fb_ccm(cc_sdp_p->dest_sdp,
                                                level, remote_pt, i);
            if (ccm_type >= 0 && ccm_type < SDP_MAX_RTCP_FB_CCM) {
                fb_types |= sdp_rtcp_fb_ccm_to_bitmap(ccm_type);
            }
            i++;
        } while (ccm_type != SDP_RTCP_FB_CCM_NOT_FOUND);

        


        switch (codec) {
            
            case RTP_VP8:
            case RTP_H263:
            case RTP_H264_P0:
            case RTP_H264_P1:
            case RTP_I420:
                fb_types &=
                  sdp_rtcp_fb_nack_to_bitmap(SDP_RTCP_FB_NACK_BASIC) |
                  sdp_rtcp_fb_nack_to_bitmap(SDP_RTCP_FB_NACK_PLI) |
                  sdp_rtcp_fb_ccm_to_bitmap(SDP_RTCP_FB_CCM_FIR);
                break;
            default:
                fb_types = 0;
                break;
        }

        



        if (fb_types) {
            gsmsdp_add_rtcp_fb (level, cc_sdp_p->src_sdp, codec, fb_types);
        }

        



        for (i = 0; i < media->num_payloads; i++) {
            if (media->payloads[i].remote_rtp_pt == remote_pt) {
                media->payloads[i].video.rtcp_fb_types = fb_types;
            }
        }
    }
    return CC_CAUSE_OK;
}
















cc_causes_t
gsmsdp_negotiate_extmap (cc_sdp_t *cc_sdp_p,
                          fsmdef_media_t *media,
                          boolean offer)
{
    boolean audio_level = FALSE;
    u16 audio_level_id = 0xFFFF;
    int level = media->level;
    int i;
    const char* uri;

    



    sdp_result_e result = SDP_SUCCESS;
    while (result == SDP_SUCCESS) {
        result = sdp_delete_attr (cc_sdp_p->src_sdp, level, 0,
                                  SDP_ATTR_EXTMAP, 1);
    }

    i = 1;
    do {
        uri = sdp_attr_get_extmap_uri(cc_sdp_p->dest_sdp, level, i);

        if (uri != NULL && strcmp(uri, SDP_EXTMAP_AUDIO_LEVEL) == 0) {
          audio_level = TRUE;
          audio_level_id = sdp_attr_get_extmap_id(cc_sdp_p->dest_sdp, level, i);
        }
        i++;
    } while (uri != NULL);

    media->audio_level = audio_level;
    media->audio_level_id = audio_level_id;

    



    if (media->audio_level) {
        gsmsdp_set_extmap_attribute (level, cc_sdp_p->src_sdp, audio_level_id, SDP_EXTMAP_AUDIO_LEVEL);
    }

    return CC_CAUSE_OK;
}







































cc_causes_t
gsmsdp_negotiate_media_lines (fsm_fcb_t *fcb_p, cc_sdp_t *sdp_p, boolean initial_offer,
                              boolean offer, boolean notify_stream_added, boolean create_answer)
{
    static const char fname[] = "gsmsdp_negotiate_media_lines";
    cc_causes_t     cause = CC_CAUSE_OK;
    uint16_t        num_m_lines = 0;
    uint16_t        num_local_m_lines = 0;
    uint16_t        i = 0;
    sdp_media_e     media_type;
    fsmdef_dcb_t   *dcb_p = fcb_p->dcb;
    uint16_t        port;
    boolean         update_local_ret_value = TRUE;
    sdp_transport_e transport;
    uint16_t        crypto_inst;
    boolean         media_found = FALSE;
    cpr_ip_addr_t   remote_addr;
    boolean         new_media;
    sdp_direction_e video_avail = SDP_DIRECTION_INACTIVE;
    boolean        unsupported_line;
    fsmdef_media_t *media;
    uint8_t         cap_index;
    sdp_direction_e remote_direction;
    boolean         result;
    int             sdpmode = 0;
    char           *session_pwd;
    cc_action_data_t  data;
    int             j=0;
    int             rtcpmux = 0;
    tinybool        rtcp_mux = FALSE;
    sdp_result_e    sdp_res;
    boolean         created_media_stream = FALSE;
    int             lsm_rc;
    int             sctp_port;
    u32             datachannel_streams;

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    num_m_lines = sdp_get_num_media_lines(sdp_p->dest_sdp);
    if (num_m_lines == 0) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"no media lines found.",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        return CC_CAUSE_NO_MEDIA;
    }

    


    if (!gsmsdp_validate_anat(sdp_p)) {
        
        GSM_DEBUG(DEB_L_C_F_PREFIX"failed anat validation",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
        return (CC_CAUSE_NO_MEDIA);
    }

    


    for (i = 1; i <= num_m_lines; i++) {
        unsupported_line = FALSE; 
        new_media        = FALSE;
        media            = NULL;
        media_type = sdp_get_media_type(sdp_p->dest_sdp, i);

        




        if (create_answer) {

            




            media = gsmsdp_find_media_by_media_type(dcb_p, media_type);

            if (media_type == SDP_MEDIA_AUDIO && !media) {
                
                continue;
            }

            if (media_type == SDP_MEDIA_VIDEO && !media) {
                continue;
            }

            if (media_type == SDP_MEDIA_APPLICATION && !media) {
                continue;
            }
        }

        port = (uint16_t) sdp_get_media_portnum(sdp_p->dest_sdp, i);
        GSM_DEBUG(DEB_L_C_F_PREFIX"Port is %d at %d %d",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
                  port, i, initial_offer);

        switch (media_type) {
        case SDP_MEDIA_AUDIO:
        case SDP_MEDIA_VIDEO:
        case SDP_MEDIA_APPLICATION:
            




            if (!gsmsdp_get_remote_media_address(dcb_p, sdp_p, i,
                                             &remote_addr)) {
                
                GSM_DEBUG(DEB_L_C_F_PREFIX"unable to get remote addr at %d",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                unsupported_line = TRUE;
                break;
            }

            




            if(!create_answer)
              media = gsmsdp_find_media_by_level(dcb_p, i);

            if (media == NULL) {
                
                media = gsmsdp_negotiate_add_media_line(dcb_p, media_type, i,
                                                        port, offer);
                if (media == NULL) {
                    
                    unsupported_line = TRUE;
                    break;
                }
                



                new_media = TRUE;
                GSM_DEBUG(DEB_L_C_F_PREFIX"new media entry at %d",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
            } else if (media->type == media_type) {
                



                if (gsmsdp_negotiate_remove_media_line(dcb_p, media, port,
                                                       offer)) {
                    
                    unsupported_line = TRUE;
                    GSM_DEBUG(DEB_L_C_F_PREFIX"media at %d is removed",
                              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                   break;
                }
            } else {
                
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"mismatch media type at %d",
                            dcb_p->line, dcb_p->call_id, fname, i);
                unsupported_line = TRUE;
                break;
            }

            
            if (SDP_DIRECTION_INACTIVE == media->direction) {
                break;
            }

            
            media->is_multicast = FALSE;
            media->multicast_port = 0;

            
            media->previous_sdp.dest_addr = media->dest_addr;
            media->dest_addr = remote_addr;

            



            if (media->cap_index == CC_MAX_MEDIA_CAP) {
                if (!gsmsdp_assign_cap_entry_to_incoming_media(dcb_p, sdp_p,
                                                               media)) {
                    unsupported_line = TRUE;
                    GSM_DEBUG(DEB_L_C_F_PREFIX"unable to assign capability entry at %d",
                              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                    
                    if ( offer && media_type == SDP_MEDIA_VIDEO &&
                          ( ( g_media_table.cap[CC_VIDEO_1].support_direction !=
                                   SDP_DIRECTION_INACTIVE) )  ) {
                        
                        remote_direction = gsmsdp_get_remote_sdp_direction(dcb_p,
                                                                           media->level,
                                                                           &media->dest_addr);
                        cap_index        = gsmdsp_find_best_match_media_cap_index(dcb_p,
                                                                                  sdp_p,
                                                                                  media,
                                                                                  MEDIA_TABLE_GLOBAL);

                        GSM_DEBUG(DEB_L_C_F_PREFIX"remote_direction: %d global match %sfound",
                            DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
                            remote_direction, (cap_index != CC_MAX_MEDIA_CAP) ? "" : "not ");
                        if ( cap_index != CC_MAX_MEDIA_CAP &&
                               remote_direction != SDP_DIRECTION_INACTIVE ) {
                           
                           GSM_DEBUG(DEB_L_C_F_PREFIX"\n\n\n\nUpdate video Offered Called %d",
                                    DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), remote_direction);
                           lsm_update_video_offered(dcb_p->line, dcb_p->call_id, remote_direction);
                        }
                    }
                    break;
                }
            }

            



            if (!gsmsdp_negotiate_addr_type(dcb_p, media)) {
                unsupported_line = TRUE;
                break;
            }

            



            transport = gsmsdp_negotiate_media_transport(dcb_p, sdp_p,
                                                         offer, media,
                                                         &crypto_inst, i);
            if (transport == SDP_TRANSPORT_INVALID) {
                
                unsupported_line = TRUE;
                GSM_DEBUG(DEB_L_C_F_PREFIX"transport mismatch at %d",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                break;
            }

            
            if (SDP_MEDIA_APPLICATION != media_type) {

                


                if (gsmsdp_negotiate_codec(dcb_p, sdp_p, media, offer, initial_offer, i) ==
                    RTP_NONE) {
                    
                    unsupported_line = TRUE;
                    
                    cause = CC_CAUSE_PAYLOAD_MISMATCH;
                    GSM_DEBUG(DEB_L_C_F_PREFIX"codec mismatch at %d",
                              DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
                    break;
                }
            } else {
                gsmsdp_negotiate_datachannel_attribs(dcb_p, sdp_p, i, media, offer);
            }

            




            gsmsdp_update_negotiated_transport(dcb_p, sdp_p, media,
                                               crypto_inst, transport, i);
            GSM_DEBUG(DEB_F_PREFIX"local transport after updating negotiated: %d",DEB_F_PREFIX_ARGS(GSM, fname), sdp_get_media_transport(dcb_p->sdp->src_sdp, 1));
            


            if (gsmsdp_is_multicast_address(media->dest_addr)) {
                




                GSM_DEBUG(DEB_L_C_F_PREFIX"Got multicast offer",
                         DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
                media->is_multicast = TRUE;
                media->multicast_port = port;
                update_local_ret_value =
                     gsmsdp_update_local_sdp_for_multicast(dcb_p, port,
                                                           media, offer,
                                                           new_media);
            } else {
                update_local_ret_value = gsmsdp_update_local_sdp(dcb_p,
                                                                 offer,
                                                                 new_media,
                                                                 media);
            }
            GSM_DEBUG(DEB_F_PREFIX"local transport after updateing local SDP: %d",DEB_F_PREFIX_ARGS(GSM, fname), sdp_get_media_transport(dcb_p->sdp->src_sdp, 1));

            


            if (media_type == SDP_MEDIA_VIDEO ) {
                video_avail = media->direction;
            }

            if (update_local_ret_value == TRUE) {
                media->previous_sdp.dest_port = media->dest_port;
                media->dest_port = port;
                if (media_type == SDP_MEDIA_AUDIO || sdpmode) {
                    
                    media_found = TRUE;
                }
            } else {
                


                unsupported_line = TRUE;
                update_local_ret_value = TRUE;
            }
            
            if (media && media_type == SDP_MEDIA_VIDEO) {
                gsmsdp_negotiate_rtcp_fb (dcb_p->sdp, media, offer);
            }
            
            if (media && media_type == SDP_MEDIA_AUDIO) {
                gsmsdp_negotiate_extmap (dcb_p->sdp, media, offer);
            }

            


            if(SDP_MEDIA_APPLICATION != media_type) {
              sdp_res = sdp_attr_get_rtcp_mux_attribute(sdp_p->dest_sdp, i,
                                                        0, SDP_ATTR_RTCP_MUX,
                                                        1, &rtcp_mux);
              if (SDP_SUCCESS == sdp_res) {
                media->rtcp_mux = TRUE;
              }
            }

            




            if (media_type == SDP_MEDIA_APPLICATION) {
              config_get_value(CFGID_SCTP_PORT, &sctp_port, sizeof(sctp_port));
              media->local_datachannel_port = sctp_port;

              sdp_res = sdp_attr_get_sctpmap_streams(sdp_p->dest_sdp,
                media->level, 0, 1, &datachannel_streams);

              

              if (sdp_res == SDP_SUCCESS) {
                  media->datachannel_streams = datachannel_streams;
              }

              gsmsdp_set_sctp_attributes(sdp_p->src_sdp, i, media);
            }

            if (!unsupported_line) {

              if (sdpmode) {
                  sdp_setup_type_e remote_setup_type;
                  int j;

                  sdp_res = sdp_attr_get_setup_attribute(
                      sdp_p->dest_sdp, i, 0, 1, &remote_setup_type);

                  







                  if (sdp_res != SDP_SUCCESS) {
                      remote_setup_type =
                          offer ? SDP_SETUP_PASSIVE : SDP_SETUP_ACTIVE;
                  }

                  

                  switch (remote_setup_type) {
                    case SDP_SETUP_ACTIVE:
                        media->setup = SDP_SETUP_PASSIVE;
                        break;
                    case SDP_SETUP_PASSIVE:
                        media->setup = SDP_SETUP_ACTIVE;
                        break;
                    case SDP_SETUP_ACTPASS:
                        






                        media->setup = SDP_SETUP_ACTIVE;
                        break;
                    case SDP_SETUP_HOLDCONN:
                        media->setup = SDP_SETUP_HOLDCONN;
                        media->direction = SDP_DIRECTION_INACTIVE;
                        break;
                    default:
                        




                        media->setup =
                            offer ? SDP_SETUP_ACTIVE : SDP_SETUP_PASSIVE;
                  }

                  if (create_answer) {
                      gsmsdp_set_setup_attribute(media->level,
                                                 dcb_p->sdp->src_sdp,
                                                 media->setup);
                  }

                  
                  for (j=0; j<media->candidate_ct; j++) {
                    gsmsdp_set_ice_attribute (SDP_ATTR_ICE_CANDIDATE, media->level,
                                              sdp_p->src_sdp, media->candidatesp[j]);
                  }

                  

                  config_get_value(CFGID_RTCPMUX, &rtcpmux, sizeof(rtcpmux));
                  if (rtcpmux && media->rtcp_mux) {
                    gsmsdp_set_rtcp_mux_attribute (SDP_ATTR_RTCP_MUX, media->level,
                                                   sdp_p->src_sdp, TRUE);
                  }

                  if (notify_stream_added) {
                      


                      if (SDP_MEDIA_APPLICATION != media_type &&
                          
                          (media->direction == SDP_DIRECTION_SENDRECV ||
                           media->direction == SDP_DIRECTION_RECVONLY)) {
                          int pc_stream_id = -1;

                          



                          if (!created_media_stream){
                              lsm_rc = lsm_add_remote_stream (dcb_p->line,
                                                              dcb_p->call_id,
                                                              media,
                                                              &pc_stream_id);
                              if (lsm_rc) {
                                return (CC_CAUSE_NO_MEDIA);
                              } else {
                                MOZ_ASSERT(pc_stream_id == 0);
                                
                                result = gsmsdp_add_remote_stream(0,
                                                                  pc_stream_id,
                                                                  dcb_p);
                                MOZ_ASSERT(result);  


                                created_media_stream = TRUE;
                              }
                          }

                          if (created_media_stream) {
                                

                                result = gsmsdp_add_remote_track(0, i, dcb_p, media);
                                MOZ_ASSERT(result);  


                          }
                      }
                  }
              }
            }

            break;

        default:
            
            unsupported_line = TRUE;
            break;
        }

        if (unsupported_line) {
            
            gsmsdp_add_unsupported_stream_to_local_sdp(sdp_p, i);
            gsmsdp_set_mid_attr(sdp_p->src_sdp, i);
            
            if (media != NULL) {
                
                gsmsdp_remove_media(dcb_p, media);
            }
        }
        if (!gsmsdp_validate_mid(sdp_p, i)) {
             
            cause = CC_CAUSE_NO_MEDIA;
            GSM_DEBUG(DEB_L_C_F_PREFIX"failed mid validation at %d",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), i);
        }
    }

    



    if (!media_found) {
        if (cause != CC_CAUSE_PAYLOAD_MISMATCH) {
            cause = CC_CAUSE_NO_MEDIA;
        }
    } else {
        if (cause == CC_CAUSE_PAYLOAD_MISMATCH) {
            



            cause = CC_CAUSE_OK;
        }

        



        gsmsdp_update_local_time_stamp(dcb_p, offer, initial_offer);

        




        num_local_m_lines = sdp_get_num_media_lines(sdp_p->src_sdp);
        if (num_local_m_lines > num_m_lines) {
            for (i = num_m_lines + 1; i <= num_local_m_lines; i++) {
                (void) sdp_set_media_portnum(sdp_p->src_sdp, i, 0, 0);
            }
        }

        


        if (sdpmode) {

            
            cause = gsmsdp_configure_dtls_data_attributes(fcb_p);
            if (cause != CC_CAUSE_OK) {
                GSM_DEBUG("gsmsdp_negotiate_media_lines- DTLS negotiation failed");
                return cause;
            }

            



            


            if (notify_stream_added) {
                for (j=0; j < CC_MAX_STREAMS; j++ ) {
                    if (dcb_p->remote_media_stream_tbl->streams[j].
                        num_tracks &&
                        (!dcb_p->remote_media_stream_tbl->streams[j].
                         num_tracks_notified)) {
                        



                        ui_on_remote_stream_added(evOnRemoteStreamAdd,
                            fcb_p->state, dcb_p->line, dcb_p->call_id,
                            dcb_p->caller_id.call_instance_id,
                            dcb_p->remote_media_stream_tbl->streams[j]);

                        dcb_p->remote_media_stream_tbl->streams[j].num_tracks_notified =
                            dcb_p->remote_media_stream_tbl->streams[j].num_tracks;
                    }
                }
            }
        }
    }
    



    dcb_p->remote_sdp_in_ack = FALSE;

    


    GSM_DEBUG(DEB_L_C_F_PREFIX"Update video Avail Called %d",
               DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),video_avail);

    
    dcb_p->cur_video_avail &= CC_ATTRIB_CAST;
    dcb_p->cur_video_avail |= (uint8_t)video_avail;

    lsm_update_video_avail(dcb_p->line, dcb_p->call_id, dcb_p->cur_video_avail);

    return cause;
}





cc_causes_t
gsmsdp_get_offered_media_types (fsm_fcb_t *fcb_p, cc_sdp_t *sdp_p, boolean *has_audio,
                                boolean *has_video, boolean *has_data)
{
    cc_causes_t     cause = CC_CAUSE_OK;
    uint16_t        num_m_lines = 0;
    uint16_t        i = 0;
    sdp_media_e     media_type;
    fsmdef_dcb_t   *dcb_p = fcb_p->dcb;
    boolean         result;

    num_m_lines = sdp_get_num_media_lines(sdp_p->dest_sdp);
    if (num_m_lines == 0) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"no media lines found.",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, __FUNCTION__));
        return CC_CAUSE_NO_MEDIA;
    }

    *has_audio = FALSE;
    *has_video = FALSE;
    *has_data = FALSE;

    


    for (i = 1; i <= num_m_lines; i++) {
        media_type = sdp_get_media_type(sdp_p->dest_sdp, i);

        if(SDP_MEDIA_AUDIO == media_type)
            *has_audio = TRUE;
        else if(SDP_MEDIA_VIDEO == media_type)
            *has_video = TRUE;
        else if(SDP_MEDIA_APPLICATION == media_type)
            *has_data = TRUE;
    }

    return cause;
}























static cc_causes_t
gsmsdp_init_local_sdp (const char *peerconnection, cc_sdp_t **sdp_pp)
{
    char            addr_str[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t   ipaddr;
    unsigned long   session_id = 0;
    char            session_version_str[GSMSDP_VERSION_STR_LEN];
    void           *local_sdp_p = NULL;
    cc_sdp_t       *sdp_p = NULL;
    int             nat_enable = 0;
    char           *p_addr_str;
    cpr_ip_mode_e   ip_mode;
    char           *strtok_state;

    if (!peerconnection) {
        return CC_CAUSE_NO_PEERCONNECTION;
    }
    if (!sdp_pp) {
        return CC_CAUSE_NULL_POINTER;
    }

    ip_mode = platform_get_ip_address_mode();
    


    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        if ((ip_mode == CPR_IP_MODE_DUAL) || (ip_mode == CPR_IP_MODE_IPV6)) {
            sip_config_get_net_ipv6_device_ipaddr(&ipaddr);
        } else if (ip_mode == CPR_IP_MODE_IPV4) {
            sip_config_get_net_device_ipaddr(&ipaddr);
        }
    } else {
        sip_config_get_nat_ipaddr(&ipaddr);
    }


	ipaddr2dotted(addr_str, &ipaddr);

    p_addr_str = PL_strtok_r(addr_str, "[ ]", &strtok_state);

    


    if (*sdp_pp == NULL) {
        sipsdp_src_dest_create(peerconnection,
            CCSIP_SRC_SDP_BIT, sdp_pp);
    } else {
        sdp_p = *sdp_pp;
        if (sdp_p->src_sdp != NULL) {
            sipsdp_src_dest_free(CCSIP_SRC_SDP_BIT, sdp_pp);
        }
        sipsdp_src_dest_create(peerconnection,
            CCSIP_SRC_SDP_BIT, sdp_pp);
    }
    sdp_p = *sdp_pp;

    if ( sdp_p == NULL )
       return CC_CAUSE_NO_SDP;

    local_sdp_p = sdp_p->src_sdp;

    


    (void) sdp_set_version(local_sdp_p, SIPSDP_VERSION);

    



    (void) sdp_set_owner_username(local_sdp_p, SIPSDP_ORIGIN_USERNAME);

    session_id = abs(cpr_rand() % 28457);
    snprintf(session_version_str, sizeof(session_version_str), "%d",
             (int) session_id);
    (void) sdp_set_owner_sessionid(local_sdp_p, session_version_str);

    snprintf(session_version_str, sizeof(session_version_str), "%d", 0);
    (void) sdp_set_owner_version(local_sdp_p, session_version_str);

    (void) sdp_set_owner_network_type(local_sdp_p, SDP_NT_INTERNET);

    if ((ip_mode == CPR_IP_MODE_DUAL) || (ip_mode == CPR_IP_MODE_IPV6)) {
        (void) sdp_set_owner_address_type(local_sdp_p, SDP_AT_IP6);
    } else if (ip_mode == CPR_IP_MODE_IPV4) {
       (void) sdp_set_owner_address_type(local_sdp_p, SDP_AT_IP4);
    }
    (void) sdp_set_owner_address(local_sdp_p, p_addr_str);

    


    (void) sdp_set_session_name(local_sdp_p, SIPSDP_SESSION_NAME);

    




    (void) sdp_set_time_start(local_sdp_p, "0");
    (void) sdp_set_time_stop(local_sdp_p, "0");

    return CC_CAUSE_OK;
}















static void
gsmsdp_set_media_capability (fsmdef_media_t *media,
                             const cc_media_cap_t *media_cap)
{
    
    media->direction = media_cap->support_direction;
    media->support_direction = media_cap->support_direction;
    if (media_cap->support_security) {
        
        FSM_SET_FLAGS(media->flags, FSM_MEDIA_F_SUPPORT_SECURITY);
    }
}


















static fsmdef_media_t *
gsmsdp_add_media_line (fsmdef_dcb_t *dcb_p, const cc_media_cap_t *media_cap,
                       uint8_t cap_index, uint16_t level,
                       cpr_ip_type addr_type, boolean offer)
{
    static const char fname[] = "gsmsdp_add_media_line";
    cc_action_data_t  data;
    fsmdef_media_t   *media = NULL;
    int               i=0;
    int               rtcpmux = 0;
    int               sctp_port = 0;

    switch (media_cap->type) {
    case SDP_MEDIA_AUDIO:
    case SDP_MEDIA_VIDEO:
    case SDP_MEDIA_APPLICATION:
        media = gsmsdp_get_new_media(dcb_p, media_cap->type, level);
        if (media == NULL) {
            
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media entry available",
                        dcb_p->line, dcb_p->call_id, fname);
            return (NULL);
        }

        
        gsmsdp_set_media_capability(media, media_cap);

        
        media->cap_index = cap_index; 

        
        gsmsdp_feature_overide_direction(dcb_p, media);
        if (media->support_direction == SDP_DIRECTION_INACTIVE) {
            GSM_DEBUG(DEB_L_C_F_PREFIX"feature overrides direction to inactive"
                      " no media added\n",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));

            



            if (!offer) {
                media->src_port = 0;
            } else {
              gsmsdp_remove_media(dcb_p, media);
              return (NULL);
            }
        }

        if (media->support_direction != SDP_DIRECTION_INACTIVE) {
          



          data.open_rcv.is_multicast = FALSE;
          data.open_rcv.listen_ip = ip_addr_invalid;
          data.open_rcv.port = 0;
          data.open_rcv.keep = FALSE;
          




          data.open_rcv.media_type = media->type;
          data.open_rcv.media_refid = media->refid;
          if (cc_call_action(dcb_p->call_id, dcb_p->line,
                             CC_ACTION_OPEN_RCV,
                             &data) != CC_RC_SUCCESS) {
              GSM_ERR_MSG(GSM_L_C_F_PREFIX"allocate rx port failed",
                          dcb_p->line, dcb_p->call_id, fname);
              gsmsdp_remove_media(dcb_p, media);
              return (NULL);
          }

          

          media->src_port = data.open_rcv.port;

          if(media_cap->type == SDP_MEDIA_APPLICATION) {
            config_get_value(CFGID_SCTP_PORT, &sctp_port, sizeof(sctp_port));
            media->local_datachannel_port = sctp_port;
          }

          


          if (addr_type == CPR_IP_ADDR_IPV6) {
              gsmsdp_get_local_source_v6_address(media);
          } else if (addr_type == CPR_IP_ADDR_IPV4) {
              gsmsdp_get_local_source_v4_address(media);
          } else {
              GSM_ERR_MSG(GSM_L_C_F_PREFIX"invalid IP address mode",
                          dcb_p->line, dcb_p->call_id, fname);
              gsmsdp_remove_media(dcb_p, media);
              return (NULL);
          }

        }

        



        gsmsdp_init_sdp_media_transport(dcb_p, dcb_p->sdp->src_sdp, media);


        gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, TRUE, media,
                                          media->transport);

        if (media->support_direction != SDP_DIRECTION_INACTIVE) {

          gsmsdp_set_local_sdp_direction(dcb_p, media, media->direction);

          
          if (media_cap->type == SDP_MEDIA_VIDEO) {
              gsmsdp_add_rtcp_fb (level, dcb_p->sdp->src_sdp, RTP_NONE, 
                  sdp_rtcp_fb_nack_to_bitmap(SDP_RTCP_FB_NACK_BASIC) |
                  sdp_rtcp_fb_nack_to_bitmap(SDP_RTCP_FB_NACK_PLI) |
                  sdp_rtcp_fb_ccm_to_bitmap(SDP_RTCP_FB_CCM_FIR));
          }
          
          if (media_cap->type == SDP_MEDIA_AUDIO) {
              gsmsdp_set_extmap_attribute(level, dcb_p->sdp->src_sdp, 1,
                  SDP_EXTMAP_AUDIO_LEVEL);
          }

          
          gsmsdp_set_setup_attribute(level, dcb_p->sdp->src_sdp, media->setup);
          


          for (i=0; i<media->candidate_ct; i++) {
            gsmsdp_set_ice_attribute (SDP_ATTR_ICE_CANDIDATE, level, dcb_p->sdp->src_sdp, media->candidatesp[i]);
          }

          config_get_value(CFGID_RTCPMUX, &rtcpmux, sizeof(rtcpmux));
          if (SDP_MEDIA_APPLICATION != media_cap->type && rtcpmux) {
            gsmsdp_set_rtcp_mux_attribute (SDP_ATTR_RTCP_MUX, level, dcb_p->sdp->src_sdp, TRUE);
          }


          



          media->previous_sdp.avt_payload_type = media->avt_payload_type;
          media->previous_sdp.direction = media->direction;
          media->previous_sdp.packetization_period = media->packetization_period;
          gsmsdp_copy_payloads_to_previous_sdp(media);
          break;
        }

    default:
        
        GSM_DEBUG(DEB_L_C_F_PREFIX"media type %d is not supported",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), media_cap->type);
        break;
    }
    return (media);
}
















cc_causes_t
gsmsdp_create_local_sdp (fsmdef_dcb_t *dcb_p, boolean force_streams_enabled,
                         boolean audio, boolean video, boolean data, boolean offer)
{
    static const char fname[] = "gsmsdp_create_local_sdp";
    uint16_t        level;
    const cc_media_cap_table_t *media_cap_tbl;
    const cc_media_cap_t       *media_cap;
    cpr_ip_mode_e   ip_mode;
    uint8_t         cap_index;
    fsmdef_media_t  *media;
    boolean         has_audio;
    int             sdpmode = 0;
    boolean         media_enabled;
    cc_causes_t     cause;

    cause = gsmsdp_init_local_sdp(dcb_p->peerconnection, &(dcb_p->sdp));
    if ( cause != CC_CAUSE_OK) {
      return cause;
    }

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    dcb_p->src_sdp_version = 0;

    media_cap_tbl = dcb_p->media_cap_tbl;

    if (media_cap_tbl == NULL) {
        
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capbility available",
                    dcb_p->line, dcb_p->call_id, fname);
        return (CC_CAUSE_NO_MEDIA_CAPABILITY);
    }

    media_cap = &media_cap_tbl->cap[0];
    level = 0;
    for (cap_index = 0; cap_index < CC_MAX_MEDIA_CAP-1; cap_index++) {

        
        media_enabled = TRUE;
        if (FALSE == audio && SDP_MEDIA_AUDIO == media_cap->type) {
            media_enabled = FALSE;
        } else if (FALSE == video && SDP_MEDIA_VIDEO == media_cap->type) {
            media_enabled = FALSE;
        } else if (FALSE == data && SDP_MEDIA_APPLICATION == media_cap->type) {
            media_enabled = FALSE;
        }

        


        if (media_enabled && ( media_cap->enabled || force_streams_enabled)) {
            level = level + 1;  

            
            if (media_cap->type != SDP_MEDIA_AUDIO &&
                media_cap->type != SDP_MEDIA_VIDEO) {
                vcmDisableRtcpComponent(dcb_p->peerconnection, level);
            }

            ip_mode = platform_get_ip_address_mode();
            if (ip_mode >= CPR_IP_MODE_IPV6) {
                if (gsmsdp_add_media_line(dcb_p, media_cap, cap_index,
                                          level, CPR_IP_ADDR_IPV6, offer)
                    == NULL) {
                    
                    level = level - 1;
                }

                if (ip_mode == CPR_IP_MODE_DUAL) {
                    level = level + 1;  
                    if (gsmsdp_add_media_line(dcb_p, media_cap, cap_index,
                                              level, CPR_IP_ADDR_IPV4, offer) ==
                        NULL) {
                        
                        level = level - 1;
                    }
                }
            } else {
                if (gsmsdp_add_media_line(dcb_p, media_cap, cap_index, level,
                                          CPR_IP_ADDR_IPV4, offer) == NULL) {
                    
                    level = level - 1;
                }
            }
        }
        
        media_cap++;
    }

    if (level == 0) {
        



        GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media line for SDP",
                    dcb_p->line, dcb_p->call_id, fname);
        return (CC_CAUSE_NO_M_LINE);
    }

    




    if (dcb_p->ice_ufrag)
        gsmsdp_set_ice_attribute (SDP_ATTR_ICE_UFRAG, SDP_SESSION_LEVEL, dcb_p->sdp->src_sdp, dcb_p->ice_ufrag);
    if (dcb_p->ice_pwd)
        gsmsdp_set_ice_attribute (SDP_ATTR_ICE_PWD, SDP_SESSION_LEVEL, dcb_p->sdp->src_sdp, dcb_p->ice_pwd);

    if(strlen(dcb_p->digest_alg)  > 0)
        gsmsdp_set_dtls_fingerprint_attribute (SDP_ATTR_DTLS_FINGERPRINT, SDP_SESSION_LEVEL,
            dcb_p->sdp->src_sdp, dcb_p->digest_alg, dcb_p->digest);

    if (!sdpmode) {

       


        has_audio = FALSE;
        GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
            if (media->type == SDP_MEDIA_AUDIO) {
                has_audio = TRUE; 
                break;
            }
        }
        if (!has_audio) {
            
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"no audio media line for SDP",
                    dcb_p->line, dcb_p->call_id, fname);
            return (CC_CAUSE_NO_AUDIO);
        }
    }

    return CC_CAUSE_OK;
}










void
gsmsdp_create_options_sdp (cc_sdp_t ** sdp_pp)
{
    cc_sdp_t *sdp_p;

    
    if (gsmsdp_init_local_sdp("", sdp_pp) != CC_CAUSE_OK) {
        return;
    }

    sdp_p = *sdp_pp;

    


    if (sdp_insert_media_line(sdp_p->src_sdp, 1) != SDP_SUCCESS) {
        
        return;
    }

    (void) sdp_set_media_type(sdp_p->src_sdp, 1, SDP_MEDIA_AUDIO);
    (void) sdp_set_media_portnum(sdp_p->src_sdp, 1, 0, 0);
    gsmsdp_set_media_transport_for_option(sdp_p->src_sdp, 1);

    


    gsmsdp_add_default_audio_formats_to_local_sdp(NULL, sdp_p, NULL);

    
    if ( g_media_table.cap[CC_VIDEO_1].enabled == TRUE ) {
        if (sdp_insert_media_line(sdp_p->src_sdp, 2) != SDP_SUCCESS) {
            
            return;
        }

        (void) sdp_set_media_type(sdp_p->src_sdp, 2, SDP_MEDIA_VIDEO);
        (void) sdp_set_media_portnum(sdp_p->src_sdp, 2, 0, 0);
        gsmsdp_set_media_transport_for_option(sdp_p->src_sdp, 2);

        gsmsdp_add_default_video_formats_to_local_sdp(NULL, sdp_p, NULL);
    }
}












static boolean
gsmsdp_check_remove_local_sdp_media (fsmdef_dcb_t *dcb_p)
{
    static const char fname[] = "gsmsdp_check_remove_local_sdp_media";
    fsmdef_media_t             *media, *media_to_remove;
    const cc_media_cap_t       *media_cap;
    boolean                    removed = FALSE;

    media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
    while (media) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(media->cap_index,dcb_p);
        if (media_cap != NULL) {
            
            if (!media_cap->enabled) {
                GSM_DEBUG(DEB_L_C_F_PREFIX"remove media at level %d",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), media->level);
                
                gsmsdp_add_unsupported_stream_to_local_sdp(dcb_p->sdp,
                                                           media->level);
                



                media_to_remove = media;
                media = GSMSDP_NEXT_MEDIA_ENTRY(media);

                
                gsmsdp_remove_media(dcb_p, media_to_remove);
                removed = TRUE;
                continue;
            }
        }
        media = GSMSDP_NEXT_MEDIA_ENTRY(media);
    }
    return (removed);
}














static boolean
gsmsdp_check_add_local_sdp_media (fsmdef_dcb_t *dcb_p, boolean hold)
{
    static const char fname[] = "gsmsdp_check_add_local_sdp_media";
    fsmdef_media_t             *media;
    const cc_media_cap_t       *media_cap;
    uint8_t                    cap_index;
    uint16_t                   num_m_lines, level_to_use;
    void                       *src_sdp;
    boolean                    need_mix = FALSE;
    boolean                    added = FALSE;
    cpr_ip_mode_e              ip_mode;
    cpr_ip_type                ip_addr_type[2]; 
    uint16_t                   i, num_ip_addrs;

    if (fsmcnf_get_ccb_by_call_id(dcb_p->call_id) != NULL) {
        







        need_mix = TRUE;
    }

    


    src_sdp = dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL;
    for (cap_index = 0; cap_index < CC_MAX_MEDIA_CAP; cap_index++) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(cap_index, dcb_p);
        if (media_cap == NULL) {
            GSM_ERR_MSG(GSM_L_C_F_PREFIX"no media capbility available",
                        dcb_p->line, dcb_p->call_id, fname);
            continue;
        }
        if (!media_cap->enabled) {
            
            continue;
        }
        media = gsmsdp_find_media_by_cap_index(dcb_p, cap_index);
        if (media != NULL) {
            
            continue;
        }

        


        if (CC_IS_AUDIO(cap_index) && need_mix) {
            if (!gsmsdp_platform_addition_mix(dcb_p, media_cap->type)) {
                
                GSM_DEBUG(DEB_L_C_F_PREFIX"no support addition mixing for %d "
                          "media type\n",
                          DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
						  media_cap->type);
                continue;
            }
        }

        



        ip_mode = platform_get_ip_address_mode();
        switch (ip_mode) {
        case CPR_IP_MODE_DUAL:
            
            num_ip_addrs = 2;
            ip_addr_type[0] = CPR_IP_ADDR_IPV6;
            ip_addr_type[1] = CPR_IP_ADDR_IPV4;
            break;
        case CPR_IP_MODE_IPV6:
            
            num_ip_addrs = 1;
            ip_addr_type[0] = CPR_IP_ADDR_IPV6;
            break;
        default:
            
            num_ip_addrs = 1;
            ip_addr_type[0] = CPR_IP_ADDR_IPV4;
            break;
        }
        
        for (i = 0; i < num_ip_addrs; i++) {
            







            level_to_use = gsmsdp_find_unused_media_line_with_type(src_sdp,
                               media_cap->type);
            if (level_to_use == 0) {
                
                num_m_lines  = sdp_get_num_media_lines(src_sdp);
                level_to_use = num_m_lines + 1;
            }
            GSM_DEBUG(DEB_L_C_F_PREFIX"add media at level %d",
                      DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), level_to_use);

            
            media = gsmsdp_add_media_line(dcb_p, media_cap, cap_index,
                                          level_to_use, ip_addr_type[i], FALSE);
            if (media != NULL) {
                
                if (hold) {
                    
                    gsmsdp_set_local_hold_sdp(dcb_p, media);
                }
                added = TRUE;
            } else {
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"Unable to add a new media",
                            dcb_p->line, dcb_p->call_id, fname);
            }
        }
    }
    return (added);
}















static boolean
gsmsdp_check_direction_change_local_sdp_media (fsmdef_dcb_t *dcb_p,
                                               boolean no_sdp_update)
{
    static const char fname[] = "gsmsdp_check_direction_change_local_sdp_media";
    fsmdef_media_t             *media;
    const cc_media_cap_t       *media_cap;
    boolean                    direction_change = FALSE;
    sdp_direction_e            save_supported_direction;

    media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
    while (media) {
        media_cap = gsmsdp_get_media_cap_entry_by_index(media->cap_index, dcb_p);
        if (media_cap != NULL) {
            if (media->support_direction !=
                media_cap->support_direction) {
                






                save_supported_direction = media->support_direction;
                media->support_direction = media_cap->support_direction;
                gsmsdp_feature_overide_direction(dcb_p, media);
                if (media->support_direction == save_supported_direction) {
                    
                } else {
                    
                    direction_change = TRUE;
                }
                if (direction_change) {
                    
                    GSM_DEBUG(DEB_L_C_F_PREFIX"change support direction at level %d"
                              " from %d to %d\n",
							  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname),
							  media->level, media->support_direction,
                              media_cap->support_direction);
                    if (no_sdp_update) {
                        


                        media->direction = media_cap->support_direction;
                    } else {
                        




                        gsmsdp_set_local_sdp_direction(dcb_p, media,
                                               media->support_direction);
                    }
                }
            }
        }
        media = GSMSDP_NEXT_MEDIA_ENTRY(media);
    }
    return (direction_change);
}








static void gsmsdp_reset_media(fsmdef_dcb_t *dcb_p, fsmdef_media_t *media, boolean hold){
    gsmsdp_reset_local_sdp_media(dcb_p, media, hold);
    if (hold) {
        gsmsdp_set_local_hold_sdp(dcb_p, media);
    } else {
        gsmsdp_set_local_resume_sdp(dcb_p, media);
    }
}





















boolean
gsmsdp_media_ip_changed (fsmdef_dcb_t *dcb_p)
{
    static const char     fname[] = "gsmsdp_media_ip_changed";
    boolean               ip_changed = FALSE;
    cpr_ip_addr_t         addr ;
    char                  curr_media_ip[MAX_IPADDR_STR_LEN];
    char                  addr_str[MAX_IPADDR_STR_LEN];
    fsmdef_media_t        *media;

    


    init_empty_str(curr_media_ip);
    config_get_value(CFGID_MEDIA_IP_ADDR, curr_media_ip,
                        MAX_IPADDR_STR_LEN);
    if (!is_empty_str(curr_media_ip)) {
        str2ip(curr_media_ip, &addr);
        util_ntohl(&addr, &addr);
        GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
            if ((util_check_if_ip_valid(&media->src_addr) == TRUE) &&
                (util_check_if_ip_valid(&addr) == TRUE) &&
                (util_compare_ip(&media->src_addr, &addr) == FALSE)) {
                ipaddr2dotted(curr_media_ip, &media->src_addr); 

                (void)cc_call_action(dcb_p->call_id, dcb_p->line,
                          CC_ACTION_STOP_MEDIA,
                          NULL);
                ip_changed = TRUE;
                media->src_addr = addr;
                if (dcb_p->sdp != NULL) {
                    gsmsdp_set_connection_address(dcb_p->sdp->src_sdp,
                            media->level,
                            dcb_p->ice_default_candidate_addr);
                }
                ipaddr2dotted(addr_str, &media->src_addr);  
                GSM_ERR_MSG("%s MEDIA IP_CHANGED: after Update IP %s"\
                            " before %s" ,fname, addr_str, curr_media_ip );
            }
        }
    }

    return (ip_changed);
}





boolean is_gsmsdp_media_ip_updated_to_latest( fsmdef_dcb_t * dcb ) {
    cpr_ip_addr_t         media_ip_in_host_order ;
    char                  curr_media_ip[MAX_IPADDR_STR_LEN];
    fsmdef_media_t        *media;

    init_empty_str(curr_media_ip);
    config_get_value(CFGID_MEDIA_IP_ADDR, curr_media_ip, MAX_IPADDR_STR_LEN);
    if (is_empty_str(curr_media_ip) == FALSE) {
        str2ip(curr_media_ip, &media_ip_in_host_order);
        util_ntohl(&media_ip_in_host_order, &media_ip_in_host_order);

        GSMSDP_FOR_ALL_MEDIA(media, dcb) {
            if (util_check_if_ip_valid(&media->src_addr) == TRUE) {
                if (util_compare_ip(&media->src_addr, &media_ip_in_host_order) == FALSE) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}




















boolean
gsmsdp_update_local_sdp_media_capability (fsmdef_dcb_t *dcb_p, boolean reset,
                                          boolean hold)
{
    static const char     fname[] = "gsmsdp_update_local_sdp_media_capability";
    fsmdef_media_t             *media;
    boolean                    change_found = FALSE;
    boolean                    check_for_change = FALSE;

    change_found = gsmsdp_media_ip_changed(dcb_p);

    



    if ((g_media_table.id != dcb_p->media_cap_tbl->id) || reset) {

            




            check_for_change = TRUE;
        }

    









    if (check_for_change && gsmsdp_check_remove_local_sdp_media(dcb_p)) {
        
        change_found = TRUE;
    }

    


    if ( check_for_change &&
         gsmsdp_check_direction_change_local_sdp_media(dcb_p, reset)) {
        
        change_found = TRUE;
    }

    



    if (reset) {
        GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
            gsmsdp_reset_media(dcb_p, media, hold);
        }
    }

    


    if ( check_for_change && gsmsdp_check_add_local_sdp_media(dcb_p, hold)) {
        change_found = TRUE;
    }

    if (change_found) {
        GSM_DEBUG(DEB_L_C_F_PREFIX"media capability change found",
                  DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname));
    }


    
    return (change_found);
}





















void
gsmsdp_reset_local_sdp_media (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media,
                              boolean hold)
{
    fsmdef_media_t *start_media, *end_media;

    if (media == NULL) {
        
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
        end_media   = NULL; 
    } else {
        
        start_media = media;
        end_media   = media;
    }

    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }

        





        gsmsdp_reset_sdp_media_transport(dcb_p, dcb_p->sdp ? dcb_p->sdp->src_sdp : NULL,
                                             media, hold);

        gsmsdp_update_local_sdp_media(dcb_p, dcb_p->sdp, TRUE, media,
                                          media->transport);


        




        if (!hold) {
            



            gsmsdp_set_local_sdp_direction(dcb_p, media,
                                           media->support_direction);
        }
    }
}
















void
gsmsdp_set_local_hold_sdp (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    int             old_style_hold = 0;
    fsmdef_media_t *start_media, *end_media;

    if (media == NULL) {
        
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
        end_media   = NULL; 
    } else {
        
        start_media = media;
        end_media   = media;
    }

    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }
        






        config_get_value(CFGID_2543_HOLD, &old_style_hold,
                         sizeof(old_style_hold));
        if (old_style_hold) {
            gsmsdp_set_2543_hold_sdp(dcb_p, media->level);
            gsmsdp_set_local_sdp_direction(dcb_p, media,
                                           SDP_DIRECTION_INACTIVE);
        } else {
            




            if (media->direction == SDP_DIRECTION_SENDRECV ||
                media->direction == SDP_DIRECTION_SENDONLY) {
                gsmsdp_set_local_sdp_direction(dcb_p, media,
                                               SDP_DIRECTION_SENDONLY);
            } else {
                gsmsdp_set_local_sdp_direction(dcb_p, media,
                                               SDP_DIRECTION_INACTIVE);
            }
        }
    }
}















void
gsmsdp_set_local_resume_sdp (fsmdef_dcb_t *dcb_p, fsmdef_media_t *media)
{
    fsmdef_media_t *start_media, *end_media;

    if (media == NULL) {
        
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb_p);
        end_media   = NULL; 
    } else {
        
        start_media = media;
        end_media   = media;
    }

    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }
        



        gsmsdp_set_local_sdp_direction(dcb_p, media, media->support_direction);
    }
}















cc_causes_t
gsmsdp_encode_sdp (cc_sdp_t *sdp_p, cc_msgbody_info_t *msg_body)
{
    char           *sdp_body;
    cc_msgbody_t   *part;
    uint32_t        body_length;

    if (!msg_body || !sdp_p) {
        return CC_CAUSE_NULL_POINTER;
    }

    
    sdp_body = sipsdp_write_to_buf(sdp_p->src_sdp, &body_length);

    if (sdp_body == NULL) {
        return CC_CAUSE_SDP_ENCODE_FAILED;
    } else if (body_length == 0) {
        cpr_free(sdp_body);
        return CC_CAUSE_SDP_ENCODE_FAILED;
    }

    
    cc_initialize_msg_body_parts_info(msg_body);

    
    msg_body->num_parts = 1;
    msg_body->content_type = cc_content_type_SDP;
    part = &msg_body->parts[0];
    part->body = sdp_body;
    part->body_length = body_length;
    part->content_type = cc_content_type_SDP;
    part->content_disposition.required_handling = FALSE;
    part->content_disposition.disposition = cc_disposition_session;
    part->content_id = NULL;
    return CC_CAUSE_OK;
}
















cc_causes_t
gsmsdp_encode_sdp_and_update_version (fsmdef_dcb_t *dcb_p, cc_msgbody_info_t *msg_body)
{
    char version_str[GSMSDP_VERSION_STR_LEN];
    cc_causes_t cause;

    snprintf(version_str, sizeof(version_str), "%d", dcb_p->src_sdp_version);

    if ( dcb_p->sdp == NULL || dcb_p->sdp->src_sdp == NULL ) {
        cause = gsmsdp_init_local_sdp(dcb_p->peerconnection, &(dcb_p->sdp));
        if ( cause != CC_CAUSE_OK) {
            return cause;
        }
    }
    (void) sdp_set_owner_version(dcb_p->sdp->src_sdp, version_str);

    if (gsmsdp_encode_sdp(dcb_p->sdp, msg_body) != CC_CAUSE_OK) {
        return CC_CAUSE_SDP_ENCODE_FAILED;
    }

    dcb_p->src_sdp_version++;
    return CC_CAUSE_OK;
}





















static uint32_t
gsmsdp_get_sdp_body (cc_msgbody_info_t *msg_body,
                     cc_msgbody_t **part_array,
                     uint32_t max_parts)
{
    uint32_t      i, count;
    cc_msgbody_t *part;

    if ((msg_body == NULL) || (msg_body->num_parts == 0)) {
        
        return (0);
    }
    







    count = 0;
    part = &msg_body->parts[msg_body->num_parts - 1];
    for (i = 0; (i < msg_body->num_parts) && (i < max_parts); i++) {
        if (part->content_type == cc_content_type_SDP) {
            
            *part_array = part; 
            part_array++;       
            count++;
        }
        
        part--;
    }
    
    return (count);
}
















static cc_causes_t
gsmsdp_realloc_dest_sdp (fsmdef_dcb_t *dcb_p)
{
    
    if (dcb_p->sdp == NULL) {
        
        sipsdp_src_dest_create(dcb_p->peerconnection,
            CCSIP_DEST_SDP_BIT, &dcb_p->sdp);
    } else {
        




        if (dcb_p->sdp->dest_sdp) {
            sipsdp_src_dest_free(CCSIP_DEST_SDP_BIT, &dcb_p->sdp);
        }
        sipsdp_src_dest_create(dcb_p->peerconnection,
            CCSIP_DEST_SDP_BIT, &dcb_p->sdp);
    }

    
    if ((dcb_p->sdp == NULL) || (dcb_p->sdp->dest_sdp == NULL)) {
        
        return CC_CAUSE_SDP_CREATE_FAILED;
    }
    return CC_CAUSE_OK;
}














cc_causes_t
gsmsdp_negotiate_answer_sdp (fsm_fcb_t *fcb_p, cc_msgbody_info_t *msg_body)
{
    static const char fname[] = "gsmsdp_negotiate_answer_sdp";
    fsmdef_dcb_t *dcb_p = fcb_p->dcb;
    cc_msgbody_t *sdp_bodies[CC_MAX_BODY_PARTS];
    uint32_t      i, num_sdp_bodies;
    cc_causes_t   status;
    char         *sdp_body;

    
    num_sdp_bodies = gsmsdp_get_sdp_body(msg_body, &sdp_bodies[0],
                                         CC_MAX_BODY_PARTS);
    GSM_DEBUG(DEB_F_PREFIX"",DEB_F_PREFIX_ARGS(GSM, fname));
    if (num_sdp_bodies == 0) {
        


        return CC_CAUSE_NO_SDP;
    }

    
    status = gsmsdp_realloc_dest_sdp(dcb_p);
    if (status != CC_CAUSE_OK) {
        
        return status;
    }

    



    status = CC_CAUSE_SDP_PARSE_FAILED;
    for (i = 0; (i < num_sdp_bodies); i++) {
        if ((sdp_bodies[i]->body != NULL) && (sdp_bodies[i]->body_length > 0)) {
            
            sdp_body = sdp_bodies[i]->body;
            if (sdp_parse(dcb_p->sdp->dest_sdp, &sdp_body,
                          (uint16_t)sdp_bodies[i]->body_length)
                    == SDP_SUCCESS) {
                status = CC_CAUSE_OK;
                break;
            }
        }
    }
    if (status != CC_CAUSE_OK) {
        
        return status;
    }

    gsmsdp_set_remote_sdp(dcb_p, dcb_p->sdp);

    status = gsmsdp_negotiate_media_lines(fcb_p, dcb_p->sdp, FALSE, FALSE, TRUE, TRUE);
    GSM_DEBUG(DEB_F_PREFIX"returns with %d",DEB_F_PREFIX_ARGS(GSM, fname), status);
    return (status);
}



















cc_causes_t
gsmsdp_negotiate_offer_sdp (fsm_fcb_t *fcb_p,
  cc_msgbody_info_t *msg_body, boolean init)
{
    cc_causes_t status;
    fsmdef_dcb_t *dcb_p = fcb_p->dcb;

    status = gsmsdp_process_offer_sdp(fcb_p, msg_body, init);
    if (status != CC_CAUSE_OK)
       return status;

    



    status = gsmsdp_negotiate_media_lines(fcb_p, dcb_p->sdp, init, TRUE, FALSE, FALSE);
    return (status);
}


















cc_causes_t
gsmsdp_process_offer_sdp (fsm_fcb_t *fcb_p,
  cc_msgbody_info_t *msg_body, boolean init)
{
    static const char fname[] = "gsmsdp_process_offer_sdp";
    fsmdef_dcb_t *dcb_p = fcb_p->dcb;
    cc_causes_t   status;
    cc_msgbody_t *sdp_bodies[CC_MAX_BODY_PARTS];
    uint32_t      i, num_sdp_bodies;
    char         *sdp_body;

    
    num_sdp_bodies = gsmsdp_get_sdp_body(msg_body, &sdp_bodies[0],
                                         CC_MAX_BODY_PARTS);
    GSM_DEBUG(DEB_L_C_F_PREFIX"Init is %d",
        DEB_L_C_F_PREFIX_ARGS(GSM, dcb_p->line, dcb_p->call_id, fname), init);
    if (num_sdp_bodies == 0) {
        




        if (init) {
            status = gsmsdp_create_local_sdp(dcb_p, FALSE, TRUE,
                                             TRUE, TRUE, TRUE);
            if ( status != CC_CAUSE_OK) {
                return status;
            }
        } else {
            


           (void)gsmsdp_update_local_sdp_media_capability(dcb_p, TRUE, FALSE);
        }
        dcb_p->remote_sdp_in_ack = TRUE;
        return CC_CAUSE_OK;
    }

    
    status = gsmsdp_realloc_dest_sdp(dcb_p);
    if (status != CC_CAUSE_OK) {
        
        return status;
    }

    



    status = CC_CAUSE_SDP_PARSE_FAILED;
    for (i = 0; (i < num_sdp_bodies); i++) {
        if ((sdp_bodies[i]->body != NULL) && (sdp_bodies[i]->body_length > 0)) {
            
            sdp_body = sdp_bodies[i]->body;
            if (sdp_parse(dcb_p->sdp->dest_sdp, &sdp_body,
                          (uint16_t)sdp_bodies[i]->body_length)
                    == SDP_SUCCESS) {
                status = CC_CAUSE_OK;
                break;
            }
        }
    }
    if (status != CC_CAUSE_OK) {
        
        return status;
    }

    if (init) {
        (void)gsmsdp_init_local_sdp(dcb_p->peerconnection, &(dcb_p->sdp));
        
    }

    gsmsdp_set_remote_sdp(dcb_p, dcb_p->sdp);

    return (status);
}











cc_causes_t
gsmsdp_check_ice_attributes_exist(fsm_fcb_t *fcb_p) {
    fsmdef_dcb_t    *dcb_p = fcb_p->dcb;
    sdp_result_e     sdp_res;
    char            *ufrag;
    char            *pwd;
    fsmdef_media_t  *media;
    boolean          has_session_ufrag = FALSE;
    boolean          has_session_pwd = FALSE;

    
    sdp_res = sdp_attr_get_ice_attribute(dcb_p->sdp->dest_sdp,
        SDP_SESSION_LEVEL, 0, SDP_ATTR_ICE_UFRAG, 1, &ufrag);
    if (sdp_res == SDP_SUCCESS && ufrag) {
        has_session_ufrag = TRUE;
    }

    sdp_res = sdp_attr_get_ice_attribute(dcb_p->sdp->dest_sdp,
        SDP_SESSION_LEVEL, 0, SDP_ATTR_ICE_PWD, 1, &pwd);
    if (sdp_res == SDP_SUCCESS && pwd) {
        has_session_pwd = TRUE;
    }

    if (has_session_ufrag && has_session_pwd) {
        
        return CC_CAUSE_OK;
    }

    
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            continue;
        }

        if (!has_session_ufrag) {
            sdp_res = sdp_attr_get_ice_attribute(dcb_p->sdp->dest_sdp,
                media->level, 0, SDP_ATTR_ICE_UFRAG, 1, &ufrag);

            if (sdp_res != SDP_SUCCESS || !ufrag) {
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"missing ICE ufrag parameter.",
                            dcb_p->line, dcb_p->call_id, __FUNCTION__);
                return CC_CAUSE_MISSING_ICE_ATTRIBUTES;
            }
        }

        if (!has_session_pwd) {
            sdp_res = sdp_attr_get_ice_attribute(dcb_p->sdp->dest_sdp,
                media->level, 0, SDP_ATTR_ICE_PWD, 1, &pwd);

            if (sdp_res != SDP_SUCCESS || !pwd) {
                GSM_ERR_MSG(GSM_L_C_F_PREFIX"missing ICE pwd parameter.",
                            dcb_p->line, dcb_p->call_id, __FUNCTION__);
                return CC_CAUSE_MISSING_ICE_ATTRIBUTES;
            }
        }
    }

    return CC_CAUSE_OK;
}










cc_causes_t
gsmsdp_install_peer_ice_attributes(fsm_fcb_t *fcb_p)
{
    char            *ufrag;
    char            *pwd;
    char            **candidates;
    int             candidate_ct;
    sdp_result_e    sdp_res;
    short           vcm_res;
    fsmdef_dcb_t    *dcb_p = fcb_p->dcb;
    cc_sdp_t        *sdp_p = dcb_p->sdp;
    fsmdef_media_t  *media;
    int             level;
    cc_causes_t     result;

    

    sdp_res = sdp_attr_get_ice_attribute(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0,
      SDP_ATTR_ICE_UFRAG, 1, &ufrag);
    if (sdp_res != SDP_SUCCESS)
      ufrag = NULL;

    sdp_res = sdp_attr_get_ice_attribute(sdp_p->dest_sdp, SDP_SESSION_LEVEL, 0,
      SDP_ATTR_ICE_PWD, 1, &pwd);
    if (sdp_res != SDP_SUCCESS)
      pwd = NULL;

    
    dcb_p->peer_ice_lite = sdp_attr_is_present(sdp_p->dest_sdp,
      SDP_ATTR_ICE_LITE, SDP_SESSION_LEVEL, 0);

    if ((ufrag && pwd) || dcb_p->peer_ice_lite) {
        vcm_res = vcmSetIceSessionParams(dcb_p->peerconnection, ufrag, pwd,
                                         dcb_p->peer_ice_lite);
        if (vcm_res)
            return (CC_CAUSE_SETTING_ICE_SESSION_PARAMETERS_FAILED);
    }

    
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
      if (!GSMSDP_MEDIA_ENABLED(media))
        continue;

      

      if (media->rtcp_mux) {
        vcm_res = vcmDisableRtcpComponent(dcb_p->peerconnection,
          media->level);

        if (vcm_res) {
          return (CC_CAUSE_SETTING_ICE_SESSION_PARAMETERS_FAILED);
        }
      }

      sdp_res = sdp_attr_get_ice_attribute(sdp_p->dest_sdp, media->level, 0,
        SDP_ATTR_ICE_UFRAG, 1, &ufrag);
      if (sdp_res != SDP_SUCCESS)
        ufrag = NULL;

      sdp_res = sdp_attr_get_ice_attribute(sdp_p->dest_sdp, media->level, 0,
        SDP_ATTR_ICE_PWD, 1, &pwd);
      if (sdp_res != SDP_SUCCESS)
        pwd = NULL;

      candidate_ct = 0;
      candidates = NULL;
      result = gsmsdp_get_ice_attributes (SDP_ATTR_ICE_CANDIDATE, media->level, sdp_p->dest_sdp,
                                          &candidates, &candidate_ct);
      if(result != CC_CAUSE_OK)
        return (result);

      

      vcm_res = vcmSetIceMediaParams(dcb_p->peerconnection, media->level, ufrag, pwd,
                                    candidates, candidate_ct);

      
      if(candidates) {
        int i;

        for (i=0; i<candidate_ct; i++) {
          if (candidates[i])
            cpr_free(candidates[i]);
        }
        cpr_free(candidates);
      }

      if (vcm_res)
        return (CC_CAUSE_SETTING_ICE_SESSION_PARAMETERS_FAILED);

    }

    return CC_CAUSE_OK;
}











cc_causes_t
gsmsdp_configure_dtls_data_attributes(fsm_fcb_t *fcb_p)
{
    char            *fingerprint = NULL;
    char            *session_fingerprint = NULL;
    sdp_result_e    sdp_res;
    sdp_result_e    sdp_session_res;
    short           vcm_res;
    fsmdef_dcb_t    *dcb_p = fcb_p->dcb;
    cc_sdp_t        *sdp_p = dcb_p->sdp;
    fsmdef_media_t  *media;
    int             level = SDP_SESSION_LEVEL;
    short           result;
    char           *token;
    char            line_to_split[FSMDEF_MAX_DIGEST_ALG_LEN + FSMDEF_MAX_DIGEST_LEN + 2];
    char           *delim = " ";
    char            digest_alg[FSMDEF_MAX_DIGEST_ALG_LEN];
    char            digest[FSMDEF_MAX_DIGEST_LEN];
    char           *strtok_state;
    cc_causes_t     cause = CC_CAUSE_OK;

    
    sdp_session_res = sdp_attr_get_dtls_fingerprint_attribute (sdp_p->dest_sdp, SDP_SESSION_LEVEL,
                                      0, SDP_ATTR_DTLS_FINGERPRINT, 1, &session_fingerprint);

    
    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media))
            continue;

        
        sdp_res = sdp_attr_get_dtls_fingerprint_attribute (sdp_p->dest_sdp, media->level,
                                    0, SDP_ATTR_DTLS_FINGERPRINT, 1, &fingerprint);

        if (SDP_SUCCESS == sdp_res ) {
            if (strlen(fingerprint) >= sizeof(line_to_split))
                return CC_CAUSE_DTLS_FINGERPRINT_TOO_LONG;
            sstrncpy(line_to_split, fingerprint, sizeof(line_to_split));
        } else if (SDP_SUCCESS == sdp_session_res) {
            if (strlen(session_fingerprint) >= sizeof(line_to_split))
                return CC_CAUSE_DTLS_FINGERPRINT_TOO_LONG;
            sstrncpy(line_to_split, session_fingerprint, sizeof(line_to_split));
        } else {
            cause = CC_CAUSE_NO_DTLS_FINGERPRINT;
            continue;
        }

        if (SDP_SUCCESS == sdp_res || SDP_SUCCESS == sdp_session_res) {
            if(!(token = PL_strtok_r(line_to_split, delim, &strtok_state)))
                return CC_CAUSE_DTLS_FINGERPRINT_PARSE_ERROR;

            if (strlen(token) >= sizeof(digest_alg))
                return CC_CAUSE_DTLS_DIGEST_ALGORITHM_TOO_LONG;

            sstrncpy(digest_alg, token, sizeof(digest_alg));
            if(!(token = PL_strtok_r(NULL, delim, &strtok_state)))
                return CC_CAUSE_DTLS_FINGERPRINT_PARSE_ERROR;

            if (strlen(token) >= sizeof(digest))
                return CC_CAUSE_DTLS_DIGEST_TOO_LONG;

            sstrncpy(digest, token, sizeof(digest));

            if (strlen(digest_alg) >= sizeof(media->negotiated_crypto.algorithm))
                return CC_CAUSE_DTLS_DIGEST_ALGORITHM_TOO_LONG;

            sstrncpy(media->negotiated_crypto.algorithm, digest_alg, sizeof(media->negotiated_crypto.algorithm));
            if (strlen(media->negotiated_crypto.algorithm) == 0) {
                return CC_CAUSE_DTLS_DIGEST_ALGORITHM_EMPTY;
            }

            if (strlen(digest) >= sizeof(media->negotiated_crypto.digest))
                return CC_CAUSE_DTLS_DIGEST_TOO_LONG;

            sstrncpy(media->negotiated_crypto.digest, digest, sizeof(media->negotiated_crypto.digest));
            if (strlen(media->negotiated_crypto.digest) == 0) {
                return CC_CAUSE_DTLS_DIGEST_EMPTY;
            }

            
            cause = CC_CAUSE_OK;

        } else {
            GSM_DEBUG(DEB_F_PREFIX"DTLS attribute error",
                                   DEB_F_PREFIX_ARGS(GSM, __FUNCTION__));
            return CC_CAUSE_DTLS_ATTRIBUTE_ERROR;
        }
    }

    return cause;
}











void
gsmsdp_free (fsmdef_dcb_t *dcb_p)
{
    if ((dcb_p != NULL) && (dcb_p->sdp != NULL)) {
        sipsdp_free(&dcb_p->sdp);
        dcb_p->sdp = NULL;
    }
}















boolean
gsmsdp_sdp_differs_from_previous_sdp (boolean rcv_only, fsmdef_media_t *media)
{
    static const char fname[] = "gsmsdp_sdp_differs_from_previous_sdp";
    char    prev_addr_str[MAX_IPADDR_STR_LEN];
    char    dest_addr_str[MAX_IPADDR_STR_LEN];
    int     i;

    

    if ((0 == media->num_payloads) || (0 == media->previous_sdp.num_payloads) ||
        (media->num_payloads != media->previous_sdp.num_payloads)){
        GSM_DEBUG(DEB_F_PREFIX"previous # payloads: %d new # payloads: %d",
                  DEB_F_PREFIX_ARGS(GSM, fname),
                  media->previous_sdp.num_payloads, media->num_payloads);
    }

    if (media->previous_sdp.avt_payload_type != media->avt_payload_type){
        GSM_DEBUG(DEB_F_PREFIX"previous avt PT: %d new avt PT: %d",
                  DEB_F_PREFIX_ARGS(GSM, fname),
                  media->previous_sdp.avt_payload_type,
                  media->avt_payload_type);
        return TRUE;
    }

    for (i = 0; i < media->num_payloads; i++) {
      if ((media->previous_sdp.payloads[i].remote_rtp_pt !=
           media->payloads[i].remote_rtp_pt) ||
          (media->previous_sdp.payloads[i].codec_type !=
           media->payloads[i].codec_type)){
          GSM_DEBUG(DEB_F_PREFIX"previous dynamic payload (PT) #%d: "
                    "%d; new dynamic payload: %d\n",
                    DEB_F_PREFIX_ARGS(GSM, fname), i,
                    media->previous_sdp.payloads[i].remote_rtp_pt,
                    media->payloads[i].remote_rtp_pt);
          GSM_DEBUG(DEB_F_PREFIX"previous codec #%d: %d; new codec: %d",
                    DEB_F_PREFIX_ARGS(GSM, fname), i,
                    media->previous_sdp.payloads[i].codec_type,
                    media->payloads[i].codec_type);
          return TRUE;
      }
    }

    





    if ( (media->previous_sdp.dest_port != 0) && (rcv_only == FALSE)) {
        if ((util_compare_ip(&(media->previous_sdp.dest_addr),
                            &(media->dest_addr)) == FALSE) ||
            (media->previous_sdp.dest_port != media->dest_port)) {
        prev_addr_str[0] = '\0'; 
        dest_addr_str[0] = '\0';
        ipaddr2dotted(prev_addr_str, &media->previous_sdp.dest_addr);
        ipaddr2dotted(dest_addr_str, &media->dest_addr);
        GSM_DEBUG(DEB_F_PREFIX"previous address: %s new address: %s",
                  DEB_F_PREFIX_ARGS(GSM, fname), prev_addr_str, dest_addr_str);
        GSM_DEBUG(DEB_F_PREFIX"previous port: %d new port: %d",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.dest_port, media->dest_port);
            return TRUE;
        } else if ( media->tias_bw != media->previous_sdp.tias_bw) {
            GSM_DEBUG(DEB_F_PREFIX"previous bw: %d new bw: %d",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.tias_bw, media->tias_bw);
            return TRUE;
        } else if ( media->profile_level != media->previous_sdp.profile_level) {
            GSM_DEBUG(DEB_F_PREFIX"previous prof_level: %X new prof_level: %X",
                  DEB_F_PREFIX_ARGS(GSM, fname), media->previous_sdp.profile_level, media->profile_level);
            return TRUE;
        }
    }


    
    if (gsmsdp_crypto_params_change(rcv_only, media)) {
        return TRUE;
    }
    return FALSE;
}


















static boolean gsmsdp_add_remote_stream(uint16_t idx, int pc_stream_id, fsmdef_dcb_t *dcb_p) {
  PR_ASSERT(idx < CC_MAX_STREAMS);
  if (idx >= CC_MAX_STREAMS)
    return FALSE;

  PR_ASSERT(!dcb_p->remote_media_stream_tbl->streams[idx].created);
  if (dcb_p->remote_media_stream_tbl->streams[idx].created)
    return FALSE;

  dcb_p->remote_media_stream_tbl->streams[idx].media_stream_id = pc_stream_id;
  dcb_p->remote_media_stream_tbl->streams[idx].created = TRUE;

  return TRUE;
}


















static boolean gsmsdp_add_remote_track(uint16_t idx, uint16_t track,
                                       fsmdef_dcb_t *dcb_p,
                                       fsmdef_media_t *media) {
  cc_media_remote_track_table_t *stream;
  int vcm_ret;

  PR_ASSERT(idx < CC_MAX_STREAMS);
  if (idx >= CC_MAX_STREAMS)
    return FALSE;

  stream = &dcb_p->remote_media_stream_tbl->streams[idx];

  PR_ASSERT(stream->created);
  if (!stream->created)
    return FALSE;

  PR_ASSERT(stream->num_tracks < (CC_MAX_TRACKS - 1));
  if (stream->num_tracks > (CC_MAX_TRACKS - 1))
    return FALSE;

  stream->track[stream->num_tracks].media_stream_track_id = track;
  stream->track[stream->num_tracks].video =
      (media->type == SDP_MEDIA_VIDEO) ? TRUE : FALSE;

  ++stream->num_tracks;

  if (media->type == SDP_MEDIA_VIDEO) {
    vcm_ret = vcmAddRemoteStreamHint(dcb_p->peerconnection, idx, TRUE);
  } else if (media->type == SDP_MEDIA_AUDIO) {
    vcm_ret = vcmAddRemoteStreamHint(dcb_p->peerconnection, idx, FALSE);
  } else {
    
    MOZ_ASSERT(FALSE);
    
    
    vcm_ret = 0;
  }

  if (vcm_ret) {
      CSFLogError(logTag, "%s: vcmAddRemoteStreamHint returned error: %d",
          __FUNCTION__, vcm_ret);
      return FALSE;
  }

  return TRUE;
}

cc_causes_t
gsmsdp_find_level_from_mid(fsmdef_dcb_t * dcb_p, const char * mid, uint16_t *level) {

    fsmdef_media_t  *media;
    u32              mid_id;
    char             buf[5];

    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (!GSMSDP_MEDIA_ENABLED(media))
            continue;

        mid_id = sdp_attr_get_simple_u32(dcb_p->sdp->dest_sdp, SDP_ATTR_MID, media->level, 0, 1);
        snprintf(buf, sizeof(buf), "%u", mid_id);
        if (strcmp(mid, buf) == 0) {
        	*level = media->level;
        	return CC_CAUSE_OK;
        }
    }
    return CC_CAUSE_VALUE_NOT_FOUND;
}


