



#include <time.h>
#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_locks.h"
#include "cpr_stdio.h"
#include "cpr_timers.h"
#include "cpr_in.h"
#include "cpr_errno.h"
#include "phone_types.h"
#include "phone.h"
#include "sdp.h"
#include "lsm.h"
#include "phone_debug.h"
#include "fsm.h"
#include "gsm_sdp.h"
#include "vcm.h"
#include "ccsip_pmh.h"
#include "dtmf.h"
#include "debug.h"
#include "rtp_defs.h"
#include "lsm_private.h"
#include "dialplanint.h"
#include "kpmlmap.h"
#include "prot_configmgr.h"
#include "dialplan.h"
#include "sip_interface_regmgr.h"
#include "gsm.h"
#include "phntask.h"
#include "fim.h"
#include "util_string.h"
#include "platform_api.h"

static const char* logTag = "lsm";

#ifndef NO
#define NO  (0)
#endif

#ifndef YES
#define YES (1)
#endif

#define CALL_INFO_NONE  (string_t)""

#define FROM_NOTIFY_PRI   1 // Same as SCCP phone behavior
#define LSM_DISPLAY_STR_LEN 256

static cc_rcs_t lsm_stop_tone (lsm_lcb_t *lcb, cc_action_data_tone_t *data);

static lsm_lcb_t *lsm_lcbs;
static uint32_t lsm_call_perline[MAX_REG_LINES];




static boolean cfwdall_state_in_ccm_mode[MAX_REG_LINES+1] ;

static const char *lsm_state_names[LSM_S_MAX] = {
    "IDLE",
    "PENDING",
    "OFFHOOK",
    "ONHOOK",
    "PROCEED",
    "RINGOUT",
    "RINGIN",
    "CONNECTED",
    "BUSY",
    "CONGESTION",
    "HOLDING",
    "CWT",
    "XFER",
    "ATTN_XFER",
    "CONF",
    "INVALID_NUMBER"
};

static const char *cc_state_names[] = {
    "OFFHOOK",
    "DIALING",
    "DIALING_COMPLETED",
    "CALL_SENT",
    "FAR_END_PROCEEDING",
    "FAR_END_ALERTING",
    "CALL_RECEIVED",
    "ALERTING",
    "ANSWERED",
    "CONNECTED",
    "HOLD",
    "RESUME",
    "ONHOOK",
    "CALL_FAILED",
    "HOLD_REVERT",
    "STATE_UNKNOWN"
};


static const char *cc_action_names[] = {
    "SPEAKER",
    "DIAL_MODE",
    "MWI",
    "MWI_LAMP",
    "OPEN_RCV",
    "UPDATE_UI",
    "MEDIA",
    "RINGER",
    "LINE_RINGER",
    "PLAY_TONE",
    "STOP_TONE",
    "STOP_MEDIA",
    "START_RCV",
    "ANSWER_PENDING",
    "PLAY_BLF_ALERT_TONE"
};


static const char *vm_alert_names[] = {
    "NONE",

    "VCM_RING_OFF",
    "VCM_INSIDE_RING",
    "VCM_OUTSIDE_RING",
    "VCM_FEATURE_RING",
    "VCM_BELLCORE_DR1",
    "VCM_RING_OFFSET",
    "VCM_BELLCORE_DR2",
    "VCM_BELLCORE_DR3",
    "VCM_BELLCORE_DR4",
    "VCM_BELLCORE_DR5",
    "VCM_BELLCORE_MAX",
    "VCM_FLASHONLY_RING",
    "VCM_STATION_PRECEDENCE_RING",
    "VCM_MAX_RING"
};



typedef enum {
    DISABLE = 1,
    FLASH_ONLY = 2,
    RING_ONCE = 3,
    RING = 4,
    BEEP_ONLY = 5
} config_value_type_t;

cprTimer_t lsm_tmr_tones;
cprTimer_t lsm_continuous_tmr_tones;
cprTimer_t lsm_tone_duration_tmr;
static uint32_t lsm_tmr_tones_ticks;
static int callWaitingDelay;
static int ringSettingIdle;
static int ringSettingActive;


static cc_rcc_ring_mode_e cc_line_ringer_mode[MAX_REG_LINES+1] =
    {CC_RING_DEFAULT};


static char cfwdall_url[MAX_URL_LENGTH];

static void lsm_update_inalert_status(line_t line, callid_t call_id,
                                      cc_state_data_alerting_t * data,
                                      boolean notify);
static void lsm_util_start_tone(vcm_tones_t tone, short alert_info,
        cc_call_handle_t call_handle, groupid_t group_id,
        streamid_t stream_id, uint16_t direction);

const char *
lsm_state_name (lsm_states_t id)
{
    if ((id <= LSM_S_MIN) || (id >= LSM_S_MAX)) {
        return get_debug_string(GSM_UNDEFINED);
    }

    return (lsm_state_names[id]);
}


static const char *
cc_state_name (cc_states_t id)
{
    if ((id <= CC_STATE_MIN) || (id >= CC_STATE_MAX)) {
        return (get_debug_string(GSM_UNDEFINED));
    }

    return (cc_state_names[id]);
}


static const char *
cc_action_name (cc_actions_t id)
{
    if ((id <= CC_ACTION_MIN) || (id >= CC_ACTION_MAX)) {
        return (get_debug_string(GSM_UNDEFINED));
    }

    return (cc_action_names[id]);
}


char
lsm_digit2ch (int digit)
{
    switch (digit) {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
        return (char)(digit + '0');
    case 0x0e:
        return ('*');
    case 0x0f:
        return ('#');
    default:
        return ('x');
    }
}


void
lsm_debug_entry (callid_t call_id, line_t line, const char *fname)
{
    LSM_DEBUG(get_debug_string(LSM_DBG_ENTRY), call_id, line, fname);
}

static void
lsm_ui_call_state (call_events event, line_t line, lsm_lcb_t *lcb, cc_causes_t cause)
{
    if (lcb->previous_call_event != event) {
        lcb->previous_call_event = event;

        




        ui_call_state(event, line, lcb->ui_id, cause);
    }
    else if(event == evConnected) {
	
	
	
        ui_call_state(event, line, lcb->ui_id, cause);
    }
}












void lsm_display_control_ringin_call (callid_t call_id, line_t line, boolean hide)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        ui_call_state(evRingIn, line, lcb->ui_id, CC_CAUSE_NORMAL);
    }
}









void lsm_set_lcb_dusting_call (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        FSM_SET_FLAGS(lcb->flags, LSM_FLAGS_DUSTING);
    }
}










void lsm_set_lcb_call_priority (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        FSM_SET_FLAGS(lcb->flags, LSM_FLAGS_CALL_PRIORITY_URGENT);
    }
}











void lsm_set_lcb_dialed_str_flag (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        FSM_SET_FLAGS(lcb->flags, LSM_FLAGS_DIALED_STRING);
    }
}











void lsm_update_gcid (callid_t call_id, char * gcid)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        if (lcb->gcid == NULL) {
            lcb->gcid = (char *)cpr_malloc(CC_GCID_LEN);
            sstrncpy(lcb->gcid, gcid, CC_GCID_LEN);
        }
    }

}











void lsm_set_lcb_prevent_ringing (callid_t call_id)
{
    lsm_lcb_t *lcb;
    char *gcid;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb == NULL) {
        return;
    }

    gcid = lcb->gcid;
    if (gcid == NULL) {
        return;
    }

    LSM_DEBUG(DEB_L_C_F_PREFIX"gcid=%s",
              DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, call_id, "lsm_set_lcb_prevent_ringing"), gcid);

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if (lcb->state == LSM_S_RINGIN) {
            if ((lcb->gcid != NULL) && (strncmp(gcid, lcb->gcid, CC_GCID_LEN) == 0)) {
                LSM_DEBUG(DEB_L_C_F_PREFIX"found ringing call, gcid %s",
                          DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, lcb->call_id, "lsm_set_lcb_prevent_ringing"), gcid);
                FSM_SET_FLAGS(lcb->flags, LSM_FLAGS_PREVENT_RINGING);
            }
            break;
        }
    }
}

void lsm_remove_lcb_prevent_ringing (callid_t call_id)
{
    lsm_lcb_t *lcb;
    char *gcid;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb == NULL) {
        return;
    }

    gcid = lcb->gcid;
    if (gcid == NULL) {
        return;
    }

    LSM_DEBUG(DEB_L_C_F_PREFIX"gcid=%s",
              DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, call_id, "lsm_remove_lcb_prevent_ringing"), gcid);

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if (lcb->state == LSM_S_RINGIN) {
            if ((lcb->gcid != NULL) && (strncmp(gcid, lcb->gcid, CC_GCID_LEN) == 0)) {
                
                lcb->flags = 0;
                LSM_DEBUG(DEB_L_C_F_PREFIX"found ringing call, gcid=%s, lcb->flags=%d.",
                          DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, lcb->call_id, "lsm_remove_lcb_prevent_ringing"), gcid, lcb->flags);
            }
            break;
        }
    }
}










boolean lsm_is_it_priority_call (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb == NULL) {
        return FALSE;
    }
    if (FSM_CHK_FLAGS(lcb->flags, LSM_FLAGS_CALL_PRIORITY_URGENT)) {
        return TRUE;
    }
    return FALSE;
}




















static void
lsm_internal_update_call_info (lsm_lcb_t *lcb, fsmdef_dcb_t *dcb)
{
    boolean inbound;
    calltype_t call_type;
    fsmcnf_ccb_t   *ccb;

    if ((lcb == NULL) || (dcb == NULL)) {
        return;
    }

    if (!dcb->ui_update_required) {
        return;
    }

    





    ccb = fsmcnf_get_ccb_by_call_id(lcb->call_id);
    if (ccb && (ccb->flags & LCL_CNF) && (ccb->active)
             && (ccb->cnf_call_id == lcb->call_id)) {
        return;
    }
    dcb->ui_update_required = FALSE;

    
    switch (dcb->orientation) {
    case CC_ORIENTATION_FROM:
        inbound = TRUE;
        break;

    case CC_ORIENTATION_TO:
        inbound = FALSE;
        break;

    default:
        


        inbound = dcb->inbound;
        break;
    }

    if ((dcb->call_type == FSMDEF_CALL_TYPE_FORWARD)
        && fsmdef_check_retain_fwd_info_state()) {
        call_type = (inbound) ? (calltype_t)dcb->call_type:FSMDEF_CALL_TYPE_OUTGOING;
    } else {
        if (inbound) {
            call_type = FSMDEF_CALL_TYPE_INCOMING;
        } else {
            call_type = FSMDEF_CALL_TYPE_OUTGOING;
        }
    }

    ui_call_info(dcb->caller_id.calling_name,
                 dcb->caller_id.calling_number,
                 dcb->caller_id.alt_calling_number,
                 dcb->caller_id.display_calling_number,
                 dcb->caller_id.called_name,
                 dcb->caller_id.called_number,
                 dcb->caller_id.display_called_number,
                 dcb->caller_id.orig_called_name,
                 dcb->caller_id.orig_called_number,
                 dcb->caller_id.last_redirect_name,
                 dcb->caller_id.last_redirect_number,
                 call_type,
                 lcb->line, lcb->ui_id,
                 dcb->caller_id.call_instance_id,
                 FSM_GET_SECURITY_STATUS(dcb),
                 FSM_GET_POLICY(dcb));

}



















static cc_rcs_t
lsm_open_rx (lsm_lcb_t *lcb, cc_action_data_open_rcv_t *data,
             fsmdef_media_t *media)
{
    static const char fname[] = "lsm_open_rx";
    int           port_allocated = 0;
    cc_rcs_t      rc = CC_RC_ERROR;
    fsmdef_dcb_t *dcb;
    int           sdpmode = 0;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (rc);
    }

    





    if (media == NULL) {
        
        if (data->media_refid != CC_NO_MEDIA_REF_ID) {
            media = gsmsdp_find_media_by_refid(dcb,
                                               data->media_refid);
        }
        if (media == NULL) {
            LSM_DEBUG(get_debug_string(LSM_DBG_INT1), lcb->call_id,
                      lcb->line, fname, "no media refID %d found",
                      data->media_refid);
            return (rc);
        }
    }

    LSM_DEBUG(get_debug_string(LSM_DBG_INT1), lcb->call_id, lcb->line, fname,
              "requested port", data->port);


    sdpmode = 0;
    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    if (data->keep == TRUE) {
      if (sdpmode && strlen(dcb->peerconnection)) {
        
        port_allocated = data->port;
      }
      else {
        
        (void) vcmRxOpen(media->cap_index, dcb->group_id, media->refid,
          lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id), data->port,
          media->is_multicast ? &media->dest_addr:&media->src_addr, data->is_multicast,
          &port_allocated);
      }
      if (port_allocated != -1) {
        data->port = (uint16_t)port_allocated;
        rc = CC_RC_SUCCESS;
      }
    } else {

      if (sdpmode) {
        if (!strlen(dcb->peerconnection)) {
          vcmRxAllocPort(media->cap_index, dcb->group_id, media->refid,
            lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id),
            data->port,
            &port_allocated);
          if (port_allocated != -1) {
            data->port = (uint16_t)port_allocated;
            rc = CC_RC_SUCCESS;
          }
        } else {
          char **candidates;
          int candidate_ct;
          char *default_addr;
          short status;

          status = vcmRxAllocICE(media->cap_index, dcb->group_id, media->refid,
            lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id),
            dcb->peerconnection,
            media->level,
            &default_addr, &port_allocated,
            &candidates, &candidate_ct);

          if (!status) {
            sstrncpy(dcb->ice_default_candidate_addr, default_addr, sizeof(dcb->ice_default_candidate_addr));
            cpr_free(default_addr);

            data->port = (uint16_t)port_allocated;
            media->candidate_ct = candidate_ct;
            media->candidatesp = candidates;
            rc = CC_RC_SUCCESS;
          }
        }
      }
    }

    if (rc == CC_RC_SUCCESS) {
      LSM_DEBUG(get_debug_string(LSM_DBG_INT1), lcb->call_id, lcb->line, fname,
                "allocated port", port_allocated);
    }

    return (rc);
}






void lsm_update_dscp_value(fsmdef_dcb_t   *dcb)
{
    static const char fname[] = "lsm_update_dscp_value";
    int dscp = 184;   
    
    if (dcb != NULL && dcb->cur_video_avail != SDP_DIRECTION_INACTIVE ) {
        config_get_value(CFGID_DSCP_VIDEO, (int *)&dscp, sizeof(dscp));
    } else {
        config_get_value(CFGID_DSCP_AUDIO, (int *)&dscp, sizeof(dscp));
    }
    
    if (dcb != NULL) {
        LSM_DEBUG(DEB_L_C_F_PREFIX"Setting dscp=%d for Rx group_id=%d",
            DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname), dscp,  dcb->group_id);
        vcmSetRtcpDscp(dcb->group_id, dscp);
    }
}





















static void
lsm_close_rx (lsm_lcb_t *lcb, boolean refresh, fsmdef_media_t *media)
{
    static const char fname[] = "lsm_close_rx";
    fsmdef_media_t *start_media, *end_media;
    fsmdef_dcb_t   *dcb;
    int             sdpmode = 0;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname);
        return;
    }

    LSM_DEBUG(DEB_L_C_F_PREFIX"Called with refresh set to %d",
              DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname), refresh);

    if (media == NULL) {
        
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb);
        end_media   = NULL; 
    } else {
        
        start_media = media;
        end_media   = media;
    }

    
    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb) {
        if (media->rcv_chan) {
            




            if (!refresh ||
                (refresh &&
                 gsmsdp_sdp_differs_from_previous_sdp(TRUE, media))) {
                LSM_DEBUG(get_debug_string(LSM_DBG_INT1), dcb->call_id,
                          dcb->line, fname, "port closed",
                          media->src_port);

                config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));
                if (!sdpmode) {

                    vcmRxClose(media->cap_index, dcb->group_id, media->refid,
                             lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id));
                }
                media->rcv_chan = FALSE;
            }
        }
    }
}





















static void
lsm_close_tx (lsm_lcb_t *lcb, boolean refresh, fsmdef_media_t *media)
{
    fsmdef_media_t *start_media, *end_media;
    fsmdef_dcb_t   *dcb;
    static const char fname[] = "lsm_close_tx";
    int             sdpmode = 0;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname);
        return;
    }
    LSM_DEBUG(DEB_L_C_F_PREFIX"called with refresh set to %d",
              DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname), refresh);

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    if (media == NULL) {
        
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb);
        end_media   = NULL; 
    } else {
        
        start_media = media;
        end_media   = media;
    }

    



    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb) {
        if (media->xmit_chan == TRUE) {

            if (!refresh ||
                (refresh &&
                 gsmsdp_sdp_differs_from_previous_sdp(FALSE, media))) {

                if (!sdpmode) {
                    vcmTxClose(media->cap_index, dcb->group_id, media->refid,
                        lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id));
                }

                if (dcb->active_tone == VCM_MONITORWARNING_TONE || dcb->active_tone == VCM_RECORDERWARNING_TONE) {
                    LSM_DEBUG(DEB_L_C_F_PREFIX"%s: Found active_tone: %d being played, current monrec_tone_action: %d. Need stop tone.",
                              DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname), fname,
                              dcb->active_tone, dcb->monrec_tone_action);
                    (void) lsm_stop_tone(lcb, NULL);
                }
                media->xmit_chan = FALSE;
                LSM_DEBUG(DEB_L_C_F_PREFIX"closed",
                          DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname));
            }
        }
    }
}


















static void
lsm_rx_start (lsm_lcb_t *lcb, const char *fname, fsmdef_media_t *media)
{
    static const char fname1[] = "lsm_rx_start";
    cc_action_data_open_rcv_t open_rcv;
    uint16_t port;
    groupid_t         group_id = CC_NO_GROUP_ID;
    callid_t          call_id  = lcb->call_id;
    vcm_mixing_mode_t mix_mode = VCM_NO_MIX;
    vcm_mixing_party_t mix_party = VCM_PARTY_NONE;
    int ret_val;
    fsmdef_media_t *start_media, *end_media;
    boolean has_checked_conference = FALSE;
    fsmdef_dcb_t   *dcb, *grp_id_dcb;
    vcm_mediaAttrs_t attrs;
    int              sdpmode = 0;
    int pc_stream_id = 0;
    int pc_track_id = 0;
    attrs.video.opaque = NULL;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname1);
        return;
    }
    group_id = dcb->group_id;
    if (media == NULL) {
        
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb);
        end_media   = NULL; 
    } else {
        
        start_media = media;
        end_media   = media;
    }

    
    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            
            continue;
        }

        




        if (media->type != SDP_MEDIA_APPLICATION &&
            !gsmsdp_is_crypto_ready(media, TRUE)) {
            LSM_DEBUG(DEB_L_C_F_PREFIX"%s: Not ready to open receive port (%d)",
                      DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname1), fname, media->src_port);
            continue;
        }

        




        
        

        pc_stream_id = 0;
        
        pc_track_id = media->level;

        


        LSM_DEBUG(get_debug_string(LSM_DBG_INT1), dcb->call_id, dcb->line,
                  fname1, "rcv chan", media->rcv_chan);
        if (media->rcv_chan == FALSE) {

            memset(&open_rcv, 0, sizeof(open_rcv));
            port = media->src_port;

            if (media->is_multicast &&
                (media->direction == SDP_DIRECTION_RECVONLY)) {
                open_rcv.is_multicast = media->is_multicast;
                open_rcv.listen_ip    = media->dest_addr;
                port                  = media->multicast_port;
            }
            open_rcv.port = port;
            open_rcv.keep = TRUE;
            open_rcv.media_type = media->type;

            if (!has_checked_conference) {
                switch(dcb->session)
                {
                    case WHISPER_COACHING:
                        mix_mode  = VCM_MIX;
                        mix_party = VCM_PARTY_TxBOTH_RxNONE;
                        grp_id_dcb = fsmdef_get_dcb_by_call_id(dcb->join_call_id);
                        if (grp_id_dcb == NULL) {
                            LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname1);
                        } else {
                            group_id = grp_id_dcb->group_id;
                        }
                        break;

                    case MONITOR:
                    case LOCAL_CONF:
                    	
                    	
                        mix_mode  = VCM_MIX;
                        mix_party = VCM_PARTY_BOTH;
                        break;
                    case PRIMARY:
                    default:
                        mix_mode  = VCM_NO_MIX;
                        mix_party = VCM_PARTY_NONE;
                        break;
                }
                has_checked_conference = TRUE;
            }

            if (lsm_open_rx(lcb, &open_rcv, media) != CC_RC_SUCCESS) {
                LSM_ERR_MSG(LSM_L_C_F_PREFIX"%s: open receive port (%d) failed.",
                            dcb->line, dcb->call_id, fname1,
							fname, media->src_port);
            } else {
                
                media->rcv_chan = TRUE; 
                
                if (media->is_multicast) {
                    media->multicast_port = open_rcv.port;
                } else {
                    media->src_port = open_rcv.port;
                }

                attrs.rtcp_mux = media->rtcp_mux;

                attrs.is_video = FALSE;
                attrs.bundle_level = 0;
                attrs.bundle_stream_correlator = 0;
                attrs.num_ssrcs = 0;
                attrs.num_unique_payload_types = 0;
                




                if ( media->cap_index == CC_VIDEO_1 ) {
                    attrs.is_video = TRUE;
                    attrs.video.opaque = media->video;
                } else {
                    attrs.audio.packetization_period = media->packetization_period;
                    attrs.audio.max_packetization_period = media->max_packetization_period;
                    attrs.audio.avt_payload_type = media->avt_payload_type;
                    attrs.audio.mixing_mode = mix_mode;
                    attrs.audio.mixing_party = mix_party;
                }
                dcb->cur_video_avail &= ~CC_ATTRIB_CAST;

                config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));
                if (dcb->peerconnection) {
                    ret_val = vcmRxStartICE(media->cap_index, group_id, media->refid,
                    media->level,
                    pc_stream_id,
                    pc_track_id,
                    lsm_get_ms_ui_call_handle(dcb->line, call_id, CC_NO_CALL_ID),
                    dcb->peerconnection,
                    media->num_payloads,
                    media->payloads,
                    media->setup,
                    FSM_NEGOTIATED_CRYPTO_DIGEST_ALGORITHM(media),
                    FSM_NEGOTIATED_CRYPTO_DIGEST(media),
                    &attrs);
                } else if (!sdpmode) {
                    if (media->payloads == NULL) {
                        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname1);
                        return;
                    }
                    ret_val =  vcmRxStart(media->cap_index, group_id, media->refid,
                                          lsm_get_ms_ui_call_handle(dcb->line, call_id, CC_NO_CALL_ID),
                                          media->payloads,
                                          media->is_multicast ? &media->dest_addr:&media->src_addr,
                                          port,
                                          FSM_NEGOTIATED_CRYPTO_ALGORITHM_ID(media),
                                          FSM_NEGOTIATED_CRYPTO_RX_KEY(media),
                                          &attrs);
                    if (ret_val == -1) {
                        dcb->dsp_out_of_resources = TRUE;
                        return;
                    }
                } else {
                    ret_val = CC_RC_ERROR;
                }

                lsm_update_dscp_value(dcb);

                if (dcb->play_tone_action == FSMDEF_PLAYTONE_ZIP)
                {
                    vcm_tones_t tone = VCM_ZIP;
                    uint16_t    direction = dcb->tone_direction;

                    LSM_DEBUG(DEB_L_C_F_PREFIX"%s: Found play_tone_action: %d. Need to play tone.",
                              DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname), fname, dcb->play_tone_action);

                    
                    dcb->play_tone_action = FSMDEF_PLAYTONE_NO_ACTION;
                    dcb->tone_direction = VCM_PLAY_TONE_TO_EAR;

                    lsm_util_tone_start_with_speaker_as_backup(tone, VCM_ALERT_INFO_OFF,
                                                               lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID),
                                                               dcb->group_id,
                                                               ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                                               direction);
                }
            }
        }

        if (media->type == SDP_MEDIA_APPLICATION) {
          


          lsm_initialize_datachannel(dcb, media, pc_track_id);
        }
    }
}



















#define LSM_TMP_VAD_LEN 64

static void
lsm_tx_start (lsm_lcb_t *lcb, const char *fname, fsmdef_media_t *media)
{
    static const char fname1[] = "lsm_tx_start";
    int           dscp = 184;   
    char          tmp[LSM_TMP_VAD_LEN];
    fsmcnf_ccb_t *ccb = NULL;
    groupid_t         group_id;
    callid_t          call_id  = lcb->call_id;
    vcm_mixing_mode_t mix_mode = VCM_NO_MIX;
    vcm_mixing_party_t mix_party = VCM_PARTY_NONE;
    fsmdef_media_t *start_media, *end_media;
    boolean        has_checked_conference = FALSE;
    fsmdef_dcb_t   *dcb;
    vcm_mediaAttrs_t attrs;
    int              sdpmode;
    long strtol_result;
    char *strtol_end;

    attrs.video.opaque = NULL;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname1);
        return;
    }
    
    if ( media != NULL ){
        
        
        if ( dcb->cur_video_avail != SDP_DIRECTION_INACTIVE ) {
            config_get_value(CFGID_DSCP_VIDEO, (int *)&dscp, sizeof(dscp));
        } else if ( CC_IS_AUDIO(media->cap_index)){
            
            
            config_get_value(CFGID_DSCP_AUDIO, (int *)&dscp, sizeof(dscp));
        }
    }
    group_id = dcb->group_id;
    LSM_DEBUG(DEB_L_C_F_PREFIX"invoked", DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname1));

    if (media == NULL) {
        
        start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb);
        end_media   = NULL; 
    } else {
        
        start_media = media;
        end_media   = media;
    }

    
    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            
            continue;
        }
        




        if (!gsmsdp_is_crypto_ready(media, FALSE)) {
            LSM_DEBUG(DEB_L_C_F_PREFIX"%s: Not ready to open transmit port",
                      DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname1), fname);
            continue;
        }
        if (media->xmit_chan == FALSE && dcb->remote_sdp_present &&
            media->dest_addr.type != CPR_IP_ADDR_INVALID && media->dest_port) {

            
            if (!has_checked_conference) {
                switch(dcb->session)
                {
                    case WHISPER_COACHING:
                        mix_mode  = VCM_MIX;
                        mix_party = VCM_PARTY_TxBOTH_RxNONE;
                        group_id = fsmdef_get_dcb_by_call_id(dcb->join_call_id)->group_id;
                        break;

                    case MONITOR:
                    case LOCAL_CONF:
                    	
                    	
                        mix_mode  = VCM_MIX;
                        mix_party = VCM_PARTY_BOTH;
                        break;
                    case PRIMARY:
                    default:
                        mix_mode  = VCM_NO_MIX;
                        mix_party = VCM_PARTY_NONE;
                        break;
                }
                has_checked_conference = TRUE;
            }

            


            
            ccb = fsmcnf_get_ccb_by_call_id(lcb->call_id);
            if (ccb != NULL) {
                media->vad = VCM_VAD_OFF;
            } else {
                config_get_string(CFGID_ENABLE_VAD, tmp, sizeof(tmp));

                errno = 0;

                strtol_result = strtol(tmp, &strtol_end, 10);

                if (errno || tmp == strtol_end ||
                    strtol_result < VCM_VAD_OFF || strtol_result > VCM_VAD_ON) {
                    LSM_ERR_MSG("%s parse error of vad: %s", __FUNCTION__, tmp);
                    return;
                }

                media->vad = (vcm_vad_t) strtol_result;
            }

            




            sdpmode = 0;
        	config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));
        	if (!sdpmode) {

        		if (vcmTxOpen(media->cap_index, dcb->group_id, media->refid,
                            lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id)) != 0) {
        			LSM_DEBUG(DEB_L_C_F_PREFIX"%s: vcmTxOpen failed",
                          DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname1), fname);
        			continue;
            	}
        	}
            media->xmit_chan = TRUE;
            attrs.mute = FALSE;
            attrs.rtcp_mux = media->rtcp_mux;
            attrs.audio_level = media->audio_level;
            attrs.audio_level_id = (uint8_t)media->audio_level_id;
            attrs.is_video = FALSE;
            attrs.bundle_level = 0;
            attrs.bundle_stream_correlator = 0;
            attrs.num_ssrcs = 0;
            attrs.num_unique_payload_types = 0;
            




            if ( CC_IS_VIDEO(media->cap_index)) {
                attrs.is_video = TRUE;
                attrs.video.opaque = media->video;
                if (lcb->vid_mute) {
                    attrs.mute = TRUE;
                }

            } else if ( CC_IS_AUDIO(media->cap_index)){
                attrs.audio.packetization_period = media->packetization_period;
                attrs.audio.max_packetization_period = media->max_packetization_period;
                attrs.audio.avt_payload_type = media->avt_payload_type;
                attrs.audio.vad = media->vad;
                attrs.audio.mixing_mode = mix_mode;
                attrs.audio.mixing_party = mix_party;
            }

            dcb->cur_video_avail &= ~CC_ATTRIB_CAST;

            if (media->payloads == NULL) {
                LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname1);
                return;
            }
            if (!strlen(dcb->peerconnection)){
              if (vcmTxStart(media->cap_index, group_id,
                  media->refid,
                  lsm_get_ms_ui_call_handle(dcb->line, call_id, CC_NO_CALL_ID),
                  media->payloads,
                  (short)dscp,
                  &media->src_addr,
                  media->src_port,
                  &media->dest_addr,
                  media->dest_port,
                  FSM_NEGOTIATED_CRYPTO_ALGORITHM_ID(media),
                  FSM_NEGOTIATED_CRYPTO_TX_KEY(media),
                  &attrs) == -1)
              {
                LSM_DEBUG(DEB_L_C_F_PREFIX"%s: vcmTxStart failed",
                  DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname1), fname);
                dcb->dsp_out_of_resources = TRUE;
                return;
              }
            }
            else {
              if (vcmTxStartICE(media->cap_index, group_id,
                  media->refid,
                  media->level,
                  

                  dcb->media_cap_tbl->cap[media->cap_index].pc_stream,
                  dcb->media_cap_tbl->cap[media->cap_index].pc_track,
                  lsm_get_ms_ui_call_handle(dcb->line, call_id, CC_NO_CALL_ID),
                  dcb->peerconnection,
                  media->payloads,
                  (short)dscp,
                  media->setup,
                  FSM_NEGOTIATED_CRYPTO_DIGEST_ALGORITHM(media),
                  FSM_NEGOTIATED_CRYPTO_DIGEST(media),
                  &attrs) == -1)
              {
                LSM_DEBUG(DEB_L_C_F_PREFIX"%s: vcmTxStartICE failed",
                  DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname1), fname);
                dcb->dsp_out_of_resources = TRUE;
                return;
              }
            }

            lsm_update_dscp_value(dcb);

            LSM_DEBUG(DEB_L_C_F_PREFIX"%s: vcmTxStart started",
                  DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname1), fname);

            if ( dcb->monrec_tone_action != FSMDEF_MRTONE_NO_ACTION)
            {
                vcm_tones_t tone = VCM_NO_TONE;
                uint16_t    direction = VCM_PLAY_TONE_TO_EAR;
                boolean     play_both_tones = FALSE;

                LSM_DEBUG(DEB_L_C_F_PREFIX"%s: Found monrec_tone_action: %d. Need to restart playing tone.",
                          DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname), fname, dcb->monrec_tone_action);

                switch (dcb->monrec_tone_action) {
                    case FSMDEF_MRTONE_RESUME_MONITOR_TONE:
                        tone = VCM_MONITORWARNING_TONE;
                        direction = dcb->monitor_tone_direction;
                        break;

                    case FSMDEF_MRTONE_RESUME_RECORDER_TONE:
                        tone = VCM_RECORDERWARNING_TONE;
                        direction = dcb->recorder_tone_direction;
                        break;

                    case FSMDEF_MRTONE_RESUME_BOTH_TONES:
                        play_both_tones = TRUE;
                        tone = VCM_MONITORWARNING_TONE;
                        direction = dcb->monitor_tone_direction;
                        break;

                    default:
                        break;
                }

                if (play_both_tones == TRUE) {
                    lsm_util_tone_start_with_speaker_as_backup(VCM_RECORDERWARNING_TONE, VCM_ALERT_INFO_OFF,
                                                          lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID),
                                                          dcb->group_id,
                                                          ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                                          dcb->recorder_tone_direction);
                }

                lsm_util_tone_start_with_speaker_as_backup(tone, VCM_ALERT_INFO_OFF, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID),
                                                    dcb->group_id,
                                                    ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                                    direction);
            }
        }
    }
}


static cc_rcs_t
lsm_start_tone (lsm_lcb_t *lcb, cc_action_data_tone_t *data)
{
    callid_t call_id = lcb->call_id;
    fsmdef_media_t *media;

    if (lcb->dcb == NULL) {
        
        return (CC_RC_ERROR);
    }
    media = gsmsdp_find_audio_media(lcb->dcb);

    lsm_util_start_tone(data->tone, VCM_ALERT_INFO_OFF, lsm_get_ms_ui_call_handle(lcb->line, call_id, CC_NO_CALL_ID), lcb->dcb->group_id,
                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                   VCM_PLAY_TONE_TO_EAR);

    return (CC_RC_SUCCESS);
}

static cc_rcs_t
lsm_stop_tone (lsm_lcb_t *lcb, cc_action_data_tone_t *data)
{
    



    static const char fname[] = "lsm_stop_tone";
    callid_t      call_id;
    fsmdef_dcb_t *dcb;

    if (lcb == NULL) {
        LSM_DEBUG(DEB_F_PREFIX"NULL lcb passed", DEB_F_PREFIX_ARGS(LSM, fname));
        return (CC_RC_ERROR);
    }
    call_id = lcb->call_id;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_DEBUG(DEB_F_PREFIX" NULL dcb passed for call_id = %d", DEB_F_PREFIX_ARGS(LSM, fname), call_id);
        return (CC_RC_ERROR);
    }

    
    if (dcb->active_tone != VCM_NO_TONE) {
            fsmdef_media_t *media = gsmsdp_find_audio_media(lcb->dcb);
            vcmToneStop(dcb->active_tone, dcb->group_id,
                          ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                  lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id));
        







        if (dcb->active_tone == VCM_RECORDERWARNING_TONE ||
        dcb->active_tone == VCM_MONITORWARNING_TONE)
        {
            vcmToneStop(dcb->active_tone == VCM_RECORDERWARNING_TONE ?
                VCM_MONITORWARNING_TONE : VCM_RECORDERWARNING_TONE,
                dcb->group_id,
                ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id));

            
            switch (dcb->monrec_tone_action) {
                case FSMDEF_MRTONE_PLAYED_MONITOR_TONE:
                    dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_MONITOR_TONE;
                    break;

                case FSMDEF_MRTONE_PLAYED_RECORDER_TONE:
                    dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_RECORDER_TONE;
                    break;

                case FSMDEF_MRTONE_PLAYED_BOTH_TONES:
                    dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_BOTH_TONES;
                    break;

                default:
                    break;
            }

            LSM_DEBUG(DEB_L_C_F_PREFIX"%s: Setting monrec_tone_action: %d so resume to play correct tone.",
                              DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname), fname,
			                  dcb->monrec_tone_action);
        }
        dcb->active_tone = VCM_NO_TONE;
    } else {
        LSM_DEBUG(DEB_L_C_F_PREFIX"Ignoring tone stop request",
                  DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, call_id, fname));
    }

    return (CC_RC_SUCCESS);
}












void
lsm_tone_start_with_duration (vcm_tones_t tone, short alert_info,
                              cc_call_handle_t call_handle, groupid_t group_id,
                              streamid_t stream_id, uint16_t direction,
				    		  uint32_t duration)
{

    static const char *fname = "lsm_tone_start_with_duration";

    DEF_DEBUG(DEB_L_C_F_PREFIX"tone=%-2d: direction=%-2d duration=%-2d",
              DEB_L_C_F_PREFIX_ARGS(LSM, GET_LINE_ID(call_handle), GET_CALL_ID(call_handle), fname),
              tone, direction, duration);

    


    vcmToneStart (tone, alert_info, call_handle, group_id, stream_id, direction);

    lsm_update_active_tone (tone, GET_CALL_ID(call_handle));

    lsm_start_tone_duration_timer (tone, duration, call_handle);
}











int lsm_get_used_instances_cnt (line_t line)
{
    static const char fname[] = "lsm_get_used_instances_cnt";
    int             used_instances = 0;
    lsm_lcb_t      *lcb;

    if (!sip_config_check_line(line)) {
        LSM_ERR_MSG(LSM_F_PREFIX"invalid line (%d)", fname, line);

        return (-1);
    }

    


    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb->call_id != CC_NO_CALL_ID) &&
            (lcb->line == line) &&
            (lcb->state != LSM_S_IDLE)) {
            used_instances++;
        }
    }

    return (used_instances);
}









int lsm_get_all_used_instances_cnt ()
{
    int             used_instances = 0;
    lsm_lcb_t      *lcb;

    


    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb->call_id != CC_NO_CALL_ID) &&
            (lcb->state != LSM_S_IDLE)) {
            used_instances++;
        }
    }

    return (used_instances);
}











void lsm_increment_call_chn_cnt (line_t line)
{
    if ( line <=0 || line > MAX_REG_LINES ) {
        LSM_ERR_MSG(LSM_F_PREFIX"invalid line (%d)", __FUNCTION__, line);
        return;
    }
    lsm_call_perline[line-1]++;

    LSM_DEBUG(DEB_F_PREFIX"number of calls on line[%d]=%d",
        DEB_F_PREFIX_ARGS(LSM, __FUNCTION__),
        line, lsm_call_perline[line-1]);
}











void lsm_decrement_call_chn_cnt (line_t line)
{
    if ( line <=0 || line > MAX_REG_LINES ) {
        LSM_ERR_MSG(LSM_F_PREFIX"invalid line (%d)", __FUNCTION__, line);
        return;
    }

    lsm_call_perline[line-1]--;

    LSM_DEBUG(DEB_F_PREFIX"number of calls on line[%d]=%d",
        DEB_F_PREFIX_ARGS(LSM, __FUNCTION__),
        line, lsm_call_perline[line-1]);
}

#define NO_ROLLOVER 0
#define ROLLOVER_ACROSS_SAME_DN 1
#define ROLLOVER_NEXT_AVAILABLE_LINE 2














line_t lsm_find_next_available_line (line_t line, boolean same_dn, boolean incoming)
{
    char current_line_dn_name[MAX_LINE_NAME_SIZE];
    char dn_name[MAX_LINE_NAME_SIZE];
    uint32_t line_feature;
    line_t  i, j;

    config_get_line_string(CFGID_LINE_NAME, current_line_dn_name, line, sizeof(current_line_dn_name));
    
    
    for (i=line+1; i <= MAX_REG_LINES; i++) {
        config_get_line_value(CFGID_LINE_FEATURE, &line_feature, sizeof(line_feature), i);

        
        if (line_feature != cfgLineFeatureDN) {
            continue;
        }

        if (same_dn == TRUE) {
            config_get_line_string(CFGID_LINE_NAME, dn_name, i, sizeof(dn_name));
            
            if (cpr_strcasecmp(dn_name, current_line_dn_name) == 0) {
                return (i);
            }
        } else {
            return (i);
        }
    }
    





    for (j=1; j <= line; j++) {
        config_get_line_value(CFGID_LINE_FEATURE, &line_feature, sizeof(line_feature), j);

        
        if (line_feature != cfgLineFeatureDN) {
            continue;
        }

        if (same_dn == TRUE) {
            config_get_line_string(CFGID_LINE_NAME, dn_name, j, sizeof(dn_name));
            
            if (cpr_strcasecmp(dn_name, current_line_dn_name) == 0) {
                return (j);
            }
        } else {
            return (j);
        }
    }

    return (NO_LINES_AVAILABLE);
}











line_t lsm_get_newcall_line (line_t line)
{
    return line;
}












line_t lsm_get_available_line (boolean incoming)
{
    return 1;
}












boolean lsm_is_line_available (line_t line, boolean incoming)
{
    return TRUE;
}









int
lsm_get_instances_available_cnt (line_t line, boolean expline)
{
    static const char fname[] = "lsm_get_instances_available_cnt";
    int             max_instances;
    int             used_instances = 0;
    int             free_instances;

    if (!sip_config_check_line(line)) {
        LSM_ERR_MSG(LSM_F_PREFIX"invalid line (%d)", fname, line);

        return (-1);
    }

    used_instances = lsm_get_used_instances_cnt(line);

    max_instances = (expline) ? (LSM_MAX_EXP_INSTANCES) : (LSM_MAX_INSTANCES);

    free_instances = max_instances - used_instances;

    if(free_instances > 0){
         int all_used_instances = lsm_get_all_used_instances_cnt();
         int all_max_instances = (expline) ? (LSM_MAX_CALLS) : (LSM_MAX_CALLS - 1);
         int all_free_instances = all_max_instances - all_used_instances;
         free_instances = ((free_instances < all_free_instances) ? free_instances : all_free_instances);
         LSM_DEBUG("lsm_get_instances_available_cnt: line=%d, expline=%d, free=%d, all_used=%d, all_max=%d, all_free=%d",
         	line, expline, free_instances, all_used_instances, all_max_instances, all_free_instances);

    }
    LSM_DEBUG("lsm_get_instances_available_cnt: line=%d, expline=%d, free_instances=%d",
         	line, expline, free_instances);
    return (free_instances);
}


static void
lsm_init_lcb (lsm_lcb_t *lcb)
{
    lcb->call_id  = CC_NO_CALL_ID;
    lcb->line     = LSM_NO_LINE;
    lcb->previous_call_event = evMaxEvent;
    lcb->state    = LSM_S_IDLE;
    lcb->mru      = 0;
    lcb->enable_ringback = TRUE;
    lcb->flags    = 0;
    lcb->dcb      = NULL;
    lcb->gcid     = NULL;
    lcb->vid_flags = 0; 
    lcb->ui_id    = CC_NO_CALL_ID;
}






static void lsm_release_port (lsm_lcb_t *lcb)
{
    static const char fname[] = "lsm_release_port";
    fsmdef_media_t *start_media, *end_media;
    fsmdef_dcb_t   *dcb;
    fsmdef_media_t *media;
    int            sdpmode = 0;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname);
        return;
    }

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    LSM_DEBUG(DEB_L_C_F_PREFIX,
              DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname));

    start_media = GSMSDP_FIRST_MEDIA_ENTRY(dcb);
    end_media   = NULL; 

    GSMSDP_FOR_MEDIA_LIST(media, start_media, end_media, dcb) {
        if (!sdpmode) {
            vcmRxReleasePort(media->cap_index, dcb->group_id, media->refid,
                            lsm_get_ms_ui_call_handle(lcb->line, lcb->call_id, lcb->ui_id), media->src_port);
        }
    }
}

static void
lsm_free_lcb (lsm_lcb_t *lcb)
{
    lsm_release_port(lcb);
    cpr_free(lcb->gcid);
    lsm_init_lcb(lcb);
}














static lsm_lcb_t *
lsm_get_free_lcb (callid_t call_id, line_t line, fsmdef_dcb_t *dcb)
{
    static const char fname[] = "lsm_get_free_lcb";
    static int      mru = 0;
    lsm_lcb_t      *lcb;
    lsm_lcb_t      *lcb_found = NULL;

    if (!sip_config_check_line(line)) {
        LSM_ERR_MSG(LSM_F_PREFIX"invalid line (%d)", fname, line);

        return (NULL);
    }


    



    if (++mru < 0) {
        mru = 1;
    }

    


    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb->call_id == CC_NO_CALL_ID) && (lcb->state == LSM_S_IDLE)) {
            lcb_found    = lcb;
            lcb->call_id = call_id;
            lcb->line    = line;
            lcb->state   = LSM_S_PENDING;
            lcb->mru     = mru;
            lcb->dcb     = dcb;
            
            lcb->vid_mute = cc_media_getVideoAutoTxPref() ? FALSE : TRUE;

            lcb->ui_id = call_id;   
            break;
        }
    }

    return (lcb_found);
}


lsm_lcb_t *
lsm_get_lcb_by_call_id (callid_t call_id)
{
    lsm_lcb_t *lcb;
    lsm_lcb_t *lcb_found = NULL;
    LSM_DEBUG(DEB_L_C_F_PREFIX"call_id=%d.",
              DEB_L_C_F_PREFIX_ARGS(LSM, 0, call_id, "lsm_get_lcb_by_call_id"), call_id);

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if (lcb->call_id == call_id) {
            lcb_found = lcb;
            break;
        }
    }

    return (lcb_found);
}










lsm_states_t
lsm_get_state (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);

    if (lcb == NULL) {
        
        return (LSM_S_NONE);
    }
    return (lcb->state);
}

static void
lsm_change_state (lsm_lcb_t *lcb, int line_num, lsm_states_t new_state)
{
    static const char fname1[] = "lsm_change_state";
    LSM_DEBUG(DEB_L_C_F_PREFIX"%d: %s -> %s",
			  DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, lcb->call_id, fname1),
              line_num, lsm_state_name(lcb->state), lsm_state_name(new_state));

    lcb->state = new_state;
}

boolean
lsm_is_phone_idle (void)
{
	static const char fname[] = "lsm_is_phone_idle";
    boolean         idle = TRUE;
    lsm_lcb_t      *lcb;

	if(!lsm_lcbs){
		LSM_DEBUG(DEB_F_PREFIX"No lsm line cb", DEB_F_PREFIX_ARGS(LSM, fname));
		return (idle);
	}

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb->call_id != CC_NO_CALL_ID) && (lcb->state != LSM_S_IDLE)) {
            idle = FALSE;
            break;
        }
    }

    return (idle);
}


















boolean
lsm_is_phone_inactive (void)
{
    boolean         inactive = TRUE;
    lsm_lcb_t      *lcb;

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb->call_id != CC_NO_CALL_ID) &&
            ((lcb->state == LSM_S_OFFHOOK) ||
             (lcb->state == LSM_S_PENDING) ||
             (lcb->state == LSM_S_PROCEED) ||
             (lcb->state == LSM_S_RINGOUT) ||
             (lcb->state == LSM_S_RINGIN)  ||
             (lcb->state == LSM_S_CONNECTED))) {
            inactive = FALSE;
            break;
        }
    }

    return (inactive);
}















boolean
lsm_callwaiting (void)
{
    lsm_lcb_t *lcb;

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if (lcb->call_id != CC_NO_CALL_ID) {
            switch (lcb->state) {
            case LSM_S_OFFHOOK:
            case LSM_S_PROCEED:
            case LSM_S_RINGOUT:
            case LSM_S_CONNECTED:
                return (TRUE);

            default:
                break;
            }
        }
    }

    return (FALSE);
}

static callid_t
lsm_find_state (lsm_states_t state)
{
    callid_t        found_callid = CC_NO_CALL_ID;
    lsm_lcb_t      *lcb;

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb->call_id != CC_NO_CALL_ID) && (lcb->state == state)) {
            found_callid = lcb->call_id;
            break;
        }
    }

    return (found_callid);
}




























cc_causes_t
lsm_get_facility_by_called_number (callid_t call_id,
                                   const char *called_number,
                                   line_t *free_line, boolean expline,
                                   void *dcb)
{
    static const char fname[] = "lsm_get_facility_by_called_number";
    line_t     line;
    lsm_lcb_t *lcb;
    int        free_instances;
    line_t     madn_line;

    lsm_debug_entry(call_id, 0, fname);
    LSM_DEBUG(DEB_F_PREFIX"called_number= %s", DEB_F_PREFIX_ARGS(LSM, fname), called_number);

    
    line = 1;
    if (line == 0) {
        return (CC_CAUSE_UNASSIGNED_NUM);
    }
    *free_line = line;

    
    madn_line = sip_config_get_line_by_called_number((line_t)(line + 1),
                                              called_number);

    


    free_instances = lsm_get_instances_available_cnt(line, expline);

    


    if ((madn_line) && (free_instances < 2)) {
        while (madn_line) {
            free_instances = lsm_get_instances_available_cnt(madn_line, expline);
            if (free_instances == 2) {
                *free_line = line = madn_line;
                break;
            }
            madn_line = sip_config_get_line_by_called_number((line_t)(madn_line + 1),
                                                      called_number);
        }
        if (madn_line == 0) {
            return (CC_CAUSE_BUSY);
        }
    }

    if (free_instances <= 0) {
        return (CC_CAUSE_BUSY);
    }

    lcb = lsm_get_free_lcb(call_id, line, (fsmdef_dcb_t *)dcb);
    if (lcb == NULL) {
        return (CC_CAUSE_NO_RESOURCE);
    }

    return (CC_CAUSE_OK);
}

















cc_causes_t lsm_allocate_call_bandwidth (callid_t call_id, int sessions)
{
    
    line_t line = lsm_get_line_by_call_id(call_id);
    

    
    vcmActivateWlan(TRUE);

    if (vcmAllocateBandwidth(lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID), sessions)) {
        return(CC_CAUSE_OK);
    }

    return(CC_CAUSE_CONGESTION);
}






















cc_causes_t
lsm_get_facility_by_line (callid_t call_id, line_t line, boolean expline,
                          void *dcb)
{
    static const char fname[] = "lsm_get_facility_by_line";
    lsm_lcb_t      *lcb;
    int             free_instances;

    LSM_DEBUG(get_debug_string(LSM_DBG_INT1), call_id, line, fname,
              "exp", expline);

    


    free_instances = lsm_get_instances_available_cnt(line, expline);
    if (free_instances <= 0) {
        return (CC_CAUSE_BUSY);
    }

    lcb = lsm_get_free_lcb(call_id, line, (fsmdef_dcb_t *)dcb);
    if (lcb == NULL) {
        return (CC_CAUSE_NO_RESOURCE);
    }

    return (CC_CAUSE_OK);
}


#ifdef _WIN32






void
terminate_active_calls (void)
{
    callid_t        call_id = CC_NO_CALL_ID;
    lsm_lcb_t      *lcb;
    line_t          line;

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if (lcb->call_id != CC_NO_CALL_ID) {
            line = lsm_get_line_by_call_id(lcb->call_id);
            


            
            cc_feature(CC_SRC_UI, call_id, line, CC_FEATURE_END_CALL, NULL);
            call_id = lcb->call_id;
        }
    }
}


#endif

line_t
lsm_get_line_by_call_id (callid_t call_id)
{
    fsmdef_dcb_t   *dcb;
    line_t          line;

    dcb = fsmdef_get_dcb_by_call_id(call_id);

    if (dcb != NULL) {
        line = dcb->line;
    } else {
        line = LSM_DEFAULT_LINE;
    }

    return (line);
}











void
lsm_tmr_tones_callback (void *data)
{
    static const char fname[] = "lsm_tmr_tones_callback";
    callid_t     call_id;
    fsmdef_dcb_t *dcb = NULL;
    fsmdef_media_t *media;

    LSM_DEBUG(DEB_F_PREFIX"invoked", DEB_F_PREFIX_ARGS(LSM, fname));

    call_id = (callid_t)(long)data;
    if (call_id == CC_NO_CALL_ID) {
        
        LSM_DEBUG(DEB_F_PREFIX"invalid call id", DEB_F_PREFIX_ARGS(LSM, fname));
        return;
    }

    




    
    dcb = fsmdef_get_dcb_by_call_id(call_id);
    if (dcb == NULL) {
        LSM_DEBUG(DEB_F_PREFIX"no dcb found for call_id %d", DEB_F_PREFIX_ARGS(LSM, fname), call_id);
        return;
    }

    media = gsmsdp_find_audio_media(dcb);

    if ((lsm_find_state(LSM_S_RINGIN) > CC_NO_CALL_ID) && (lsm_callwaiting())) {

            
            switch (dcb->alert_info) {

            case ALERTING_RING:

                
                switch (dcb->alerting_ring) {
                case VCM_BELLCORE_DR2:
                    lsm_util_start_tone(VCM_CALL_WAITING_2_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   dcb->tone_direction);
                    break;
                case VCM_BELLCORE_DR3:
                    lsm_util_start_tone(VCM_CALL_WAITING_3_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   dcb->tone_direction);
                    break;
                case VCM_BELLCORE_DR4:
                    lsm_util_start_tone(VCM_CALL_WAITING_4_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   dcb->tone_direction);
                    break;
                default:
                    lsm_util_start_tone(VCM_CALL_WAITING_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   dcb->tone_direction);
                }
                break;

            case ALERTING_TONE:

                



                switch (dcb->alerting_tone) {
                case VCM_BUSY_VERIFY_TONE:
                    lsm_util_start_tone(VCM_CALL_WAITING_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   dcb->tone_direction);
                    if (cprStartTimer(lsm_tmr_tones, BUSY_VERIFICATION_DELAY,
                        (void *)(long)dcb->call_id) == CPR_FAILURE) {
                        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                                  fname, "cprStartTimer", cpr_errno);
                    }
                    break;

                case VCM_CALL_WAITING_TONE:
                case VCM_CALL_WAITING_2_TONE:
                case VCM_CALL_WAITING_3_TONE:
                case VCM_CALL_WAITING_4_TONE:
                    lsm_util_start_tone(dcb->alerting_tone, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   dcb->tone_direction);
                    break;

                case VCM_MSG_WAITING_TONE:
                case VCM_STUTTER_TONE:
                    lsm_util_start_tone(VCM_INSIDE_DIAL_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   dcb->tone_direction);
                    lsm_tmr_tones_ticks = 0;
                    break;
                default:
                    break;
                }
                break;

            default:
                lsm_util_start_tone(VCM_CALL_WAITING_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                               ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                               dcb->tone_direction);

                break;
            }

    } else if (dcb->dialplan_tone) {
        dcb->dialplan_tone = FALSE;
        switch (dcb->alert_info) {

        case ALERTING_TONE:
            




            switch (dcb->alerting_tone) {
            case VCM_MSG_WAITING_TONE:
            case VCM_STUTTER_TONE:
                lsm_util_start_tone(VCM_INSIDE_DIAL_TONE, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                               ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                               dcb->tone_direction);
                break;

            case VCM_HOLD_TONE:
                lsm_util_start_tone(dcb->alerting_tone, NO, lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID), dcb->group_id,
                               ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                               dcb->tone_direction);
                break;

            default:
                break;
            }

            break;

        default:
            break;
        }
    }
}










void
lsm_start_multipart_tone_timer (vcm_tones_t tone,
                                uint32_t delay,
                                callid_t callId)
{
    static const char fname[] = "lsm_start_multipart_tone_timer";
    fsmdef_dcb_t *dcb;

    
    dcb = fsmdef_get_dcb_by_call_id(callId);
    dcb->alert_info = ALERTING_TONE;
    dcb->alerting_tone = tone;
    dcb->dialplan_tone = TRUE;

    if (cprCancelTimer(lsm_tmr_tones) == CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprCancelTimer", cpr_errno);
    }
    if (cprStartTimer(lsm_tmr_tones, delay, (void *)(long)dcb->call_id) ==
            CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprStartTimer", cpr_errno);
    }
}









void
lsm_stop_multipart_tone_timer (void)
{
    static const char fname[] = "lsm_stop_multipart_tone_timer";

    if (cprCancelTimer(lsm_tmr_tones) == CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprCancelTimer", cpr_errno);
    }
}









void
lsm_start_continuous_tone_timer (vcm_tones_t tone,
                                 uint32_t delay,
                                 callid_t callId)
{
    static const char fname[] = "lsm_start_continuous_tone_timer";
    fsmdef_dcb_t *dcb;

    
    dcb = fsmdef_get_dcb_by_call_id(callId);
    dcb->alert_info = ALERTING_TONE;
    dcb->alerting_tone = tone;
    dcb->dialplan_tone = TRUE;

    if (cprCancelTimer(lsm_continuous_tmr_tones) == CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprCancelTimer", cpr_errno);
    }
    if (cprStartTimer(lsm_continuous_tmr_tones, delay, (void *)(long)dcb->call_id)
            == CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprStartTimer", cpr_errno);
    }
}









void
lsm_stop_continuous_tone_timer (void)
{
    static const char fname[] = "lsm_stop_continuous_tone_timer";

    if (cprCancelTimer(lsm_continuous_tmr_tones) == CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprCancelTimer", cpr_errno);
    }
}









void
lsm_start_tone_duration_timer (vcm_tones_t tone,
                                uint32_t duration,
                                cc_call_handle_t call_handle)
{
    static const char fname[] = "lsm_start_tone_duration_timer";
    fsmdef_dcb_t *dcb;

    
    dcb = fsmdef_get_dcb_by_call_id(GET_CALL_ID(call_handle));

    if (cprCancelTimer(lsm_tone_duration_tmr) == CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprCancelTimer", cpr_errno);
    }
    if (cprStartTimer(lsm_tone_duration_tmr, duration*1000, (void *)(long)dcb->call_id) ==
            CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprStartTimer", cpr_errno);
    }
}







void
lsm_stop_tone_duration_timer (void)
{
    static const char fname[] = "lsm_stop_tone_duration_timer";

    if (cprCancelTimer(lsm_tone_duration_tmr) == CPR_FAILURE) {
        LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                  fname, "cprCancelTimer", cpr_errno);
    }
}











void
lsm_tone_duration_tmr_callback (void *data)
{
    static const char fname[] = "lsm_tone_duration_tmr_callback";
    callid_t     call_id;
    fsmdef_dcb_t *dcb = NULL;
    fsmdef_media_t *media;

    LSM_DEBUG(DEB_F_PREFIX"invoked", DEB_F_PREFIX_ARGS(LSM, fname));

    call_id = (callid_t)(long)data;
    if (call_id == CC_NO_CALL_ID) {
        
        LSM_DEBUG(DEB_F_PREFIX"invalid call id", DEB_F_PREFIX_ARGS(LSM, fname));
        return;
    }

    
    dcb = fsmdef_get_dcb_by_call_id(call_id);
    if (dcb == NULL) {
        LSM_DEBUG(DEB_F_PREFIX"no dcb found for call_id %d", DEB_F_PREFIX_ARGS(LSM, fname), call_id);
        return;
    }

    media = gsmsdp_find_audio_media(dcb);

    vcmToneStop(dcb->active_tone, dcb->group_id,
              ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
              lsm_get_ms_ui_call_handle(dcb->line, dcb->call_id, CC_NO_CALL_ID));

    
    
    

    cc_int_release(CC_SRC_GSM, CC_SRC_GSM, call_id, dcb->line, CC_CAUSE_NORMAL, NULL, NULL);
}








static callid_t
lsm_answer_pending (void)
{
    callid_t  found_callid = CC_NO_CALL_ID;
    lsm_lcb_t *lcb;

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb->call_id != CC_NO_CALL_ID) &&
            (FSM_CHK_FLAGS(lcb->flags, LSM_FLAGS_ANSWER_PENDING))) {

            found_callid = lcb->call_id;
            break;
        }
    }

    return (found_callid);
}













static void
lsm_reversion_ringer (lsm_lcb_t *lcb, callid_t call_id, line_t line)
{
    vcm_ring_mode_t ringerMode = VCM_INSIDE_RING;
    vcm_tones_t     toneMode = VCM_CALL_WAITING_TONE;

    if (!lsm_callwaiting()) {
        config_get_line_value(CFGID_LINE_RING_SETTING_IDLE,
                              &ringSettingIdle, sizeof(ringSettingIdle),
                              line);
        if (cc_line_ringer_mode[line] == CC_RING_DISABLE) {
            ringerMode = VCM_FLASHONLY_RING;
        } else if (ringSettingIdle == DISABLE) {
            ringerMode = VCM_RING_OFF;
        } else if (ringSettingIdle == FLASH_ONLY) {
            ringerMode = VCM_FLASHONLY_RING;
        }

        vcmControlRinger(ringerMode, YES, NO, line, call_id);

    } else {
        lsm_tmr_tones_ticks = 0;

        config_get_line_value(CFGID_LINE_RING_SETTING_ACTIVE,
                              &ringSettingActive, sizeof(ringSettingActive),
                              line);
        if (ringSettingActive == DISABLE) {
            ringerMode = VCM_RING_OFF;
        } else if (ringSettingActive == FLASH_ONLY) {
            ringerMode = VCM_FLASHONLY_RING;
        }

        if (ringSettingActive == BEEP_ONLY) {
            fsmdef_media_t *media = gsmsdp_find_audio_media(lcb->dcb);

            lsm_util_start_tone(toneMode, NO, lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID), lcb->dcb->group_id,
                           ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                           VCM_PLAY_TONE_TO_EAR);
        } else {
            vcmControlRinger(ringerMode, YES, NO, line, call_id);
        }
    }
}









static void
lsm_set_beep_only_settings (fsmdef_dcb_t *dcb, vcm_tones_t *toneMode_p)
{
    switch (dcb->alert_info) {
    







    case ALERTING_RING:
        lsm_tmr_tones_ticks = callWaitingDelay;
        switch (dcb->alerting_ring) {
        case VCM_BELLCORE_DR2:
            *toneMode_p = VCM_CALL_WAITING_2_TONE;
            break;

        case VCM_BELLCORE_DR3:
            *toneMode_p = VCM_CALL_WAITING_3_TONE;
            break;

        case VCM_BELLCORE_DR4:
            *toneMode_p = VCM_CALL_WAITING_4_TONE;
            break;

        default:
            break;
        }
        break;

    
    case ALERTING_TONE:
        








        if (sip_regmgr_get_cc_mode(dcb->line) == REG_MODE_CCM) {
            dcb->alerting_tone = VCM_CALL_WAITING_TONE;
            LSM_DEBUG(DEB_F_PREFIX" - Overriding value in Alert-Info header as line %d is \
                      connected to a Call Manager.",
                      DEB_F_PREFIX_ARGS(LSM, __FUNCTION__), dcb->line);
        }
        *toneMode_p = dcb->alerting_tone;
        switch (dcb->alerting_tone) {
        case VCM_MSG_WAITING_TONE:
            lsm_tmr_tones_ticks = MSG_WAITING_DELAY;
            break;

        case VCM_STUTTER_TONE:
            lsm_tmr_tones_ticks = STUTTER_DELAY;
            break;

        case VCM_BUSY_VERIFY_TONE:
            lsm_tmr_tones_ticks = BUSY_VERIFY_DELAY;
            break;

        case VCM_CALL_WAITING_TONE:
        case VCM_CALL_WAITING_2_TONE:
        case VCM_CALL_WAITING_3_TONE:
        case VCM_CALL_WAITING_4_TONE:
            lsm_tmr_tones_ticks = callWaitingDelay;
            break;

        default:
            break;
        }
        break;

    default:
        lsm_tmr_tones_ticks = callWaitingDelay;
    }
}














static void
lsm_set_ringer (lsm_lcb_t *lcb, callid_t call_id, line_t line, int alerting)
{
    static const char fname[] = "lsm_set_ringer";
    fsmdef_dcb_t   *dcb;
    boolean         ringer_set = FALSE;
    callid_t        other_call_id = CC_NO_CALL_ID;
    callid_t        priority_call_id = CC_NO_CALL_ID;
    int             callHoldRingback = 0;
    int             dcb_cnt = 0;
    int             i = 0;
    fsmxfr_xcb_t   *xcb;
    fsmcnf_ccb_t   *ccb;
    lsm_lcb_t *lcb2;
    fsmdef_dcb_t   *dcbs[LSM_MAX_CALLS];
    vcm_ring_mode_t ringerMode = VCM_INSIDE_RING;
    short           ringOnce = NO;
    boolean         alertInfo = NO;
    vcm_tones_t     toneMode = VCM_CALL_WAITING_TONE;
    fsmdef_media_t *media;
    boolean         isDusting = FSM_CHK_FLAGS(lcb->flags, LSM_FLAGS_DUSTING) ? TRUE : FALSE;
    int            sdpmode = 0;


    LSM_DEBUG(DEB_L_C_F_PREFIX"Entered, state=%d.",
              DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname), lcb->state);

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    












    if (priority_call_id == CC_NO_CALL_ID) {
        





        if (lcb->state == LSM_S_RINGIN) {
            other_call_id = call_id;
        } else {
            other_call_id = lsm_find_state(LSM_S_RINGIN);
        }
    }

    if (((priority_call_id != CC_NO_CALL_ID) || (other_call_id != CC_NO_CALL_ID)) &&
        (lsm_answer_pending() == CC_NO_CALL_ID)) {
        



        dcb = fsmdef_get_dcb_by_call_id((priority_call_id != CC_NO_CALL_ID) ?
                                        priority_call_id : other_call_id);
        lcb = lsm_get_lcb_by_call_id((priority_call_id != CC_NO_CALL_ID) ?
                                        priority_call_id : other_call_id);
        isDusting = ((lcb != NULL) && FSM_CHK_FLAGS(lcb->flags, LSM_FLAGS_DUSTING)) ? TRUE : FALSE;

        













        line = dcb->line;

        if (!lsm_callwaiting()) {

            LSM_DEBUG(DEB_L_C_F_PREFIX"No call waiting, lcb->line=%d, lcb->flag=%d.",
                      DEB_L_C_F_PREFIX_ARGS(LSM, line, lcb->call_id, fname),
                      lcb->line,
                      lcb->flags);

            ringer_set = TRUE;
            lsm_tmr_tones_ticks = 0;

            






            if (isDusting) {
                ringSettingIdle = FLASH_ONLY;
            }
            else if (FSM_CHK_FLAGS(lcb->flags, LSM_FLAGS_PREVENT_RINGING)) {
                


                ringSettingIdle = DISABLE;
            } else if (cc_line_ringer_mode[line] == CC_RING_DISABLE) {
                


                ringSettingIdle = FLASH_ONLY;
            } else if (cc_line_ringer_mode[line] == CC_RING_ENABLE) {
                ringSettingIdle = RING;
            } else {
                config_get_line_value(CFGID_LINE_RING_SETTING_IDLE,
                                      &ringSettingIdle, sizeof(ringSettingIdle),
                                      line);
            }
            LSM_DEBUG(DEB_L_C_F_PREFIX"Ring set mode=%d.",
                      DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname), ringSettingIdle);

            


            if (ringSettingIdle == DISABLE) {
                ringerMode = VCM_RING_OFF;

                


            } else if (ringSettingIdle == FLASH_ONLY) {
                ringerMode = VCM_FLASHONLY_RING;

                


            } else if (ringSettingIdle == RING_ONCE) {
                ringOnce = YES;

                



            } else if (ringSettingIdle == RING) {

                
                switch (dcb->alert_info) {
                case ALERTING_NONE:
                    
                    break;

                case ALERTING_RING:
                    ringerMode = dcb->alerting_ring;
                    break;

                case ALERTING_OLD:
                default:
                    alertInfo = YES;
                }
            } else if (ringSettingIdle == BEEP_ONLY) {
                lsm_set_beep_only_settings (dcb, &toneMode);

            }
            LSM_DEBUG(DEB_L_C_F_PREFIX"Alert info=%d, ringSettingIdle=%d, ringerMode=%d",
                                  DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname),
                                  dcb->alert_info,
                                  ringSettingIdle,
                                  ringerMode);

            




            if (alerting) {
                




                if (sip_regmgr_get_cc_mode(line) == REG_MODE_CCM) {
                    if (ringerMode == VCM_BELLCORE_DR1) {
                        ringerMode = VCM_INSIDE_RING;
                    } else if (ringerMode == VCM_BELLCORE_DR2) {
                        ringerMode = VCM_OUTSIDE_RING;
                    }
                }
                if (ringSettingIdle == BEEP_ONLY) {

                    LSM_DEBUG(DEB_L_C_F_PREFIX"Idle phone RING SETTING: Beep_only",
                              DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname));

                    media = gsmsdp_find_audio_media(lcb->dcb);
                    lsm_util_tone_start_with_speaker_as_backup(toneMode, NO, lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID),
                                                          lcb->dcb->group_id,
                          ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                                          VCM_PLAY_TONE_TO_EAR);
                } else {
                    LSM_DEBUG(DEB_L_C_F_PREFIX"Idle phone RING SETTING: ringer Mode = %s,"
                              " Ring once = %d, alertInfo = %d\n",
                              DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname),
                              vm_alert_names[ringerMode], ringOnce, alertInfo);

                    vcmControlRinger(ringerMode, ringOnce, alertInfo, line, lcb->ui_id);
                }
            }

            if ( lcb->state != LSM_S_HOLDING &&
                 lcb->state != LSM_S_RINGIN ) {
                ui_set_call_status(platform_get_phrase_index_str(CALL_ALERTING),
                                   line, lcb->ui_id);
            }
        } else {

            
	    FSM_FOR_ALL_CBS(lcb2, lsm_lcbs, LSM_MAX_LCBS) {
		 if ((lcb2->call_id != CC_NO_CALL_ID) &&
		     (lcb2->state == LSM_S_RINGIN) )
		 {
            LSM_DEBUG(DEB_L_C_F_PREFIX"Call waiting RING SETTING: "
                      "ringer Mode = RING_OFF, Ring once = NO, alertInfo = NO\n",
                      DEB_L_C_F_PREFIX_ARGS(LSM, lcb2->line, lcb2->call_id, fname));
                      vcmControlRinger(VCM_RING_OFF, NO, NO, lcb2->line, call_id);
                 }
            }
            ringer_set = TRUE;
            lsm_tmr_tones_ticks = 0;

            




            if (isDusting) {
                ringSettingActive = FLASH_ONLY;
            } else {
                config_get_line_value(CFGID_LINE_RING_SETTING_ACTIVE,
                                      &ringSettingActive, sizeof(ringSettingActive),
                                      line);
            }

            


            if (ringSettingActive == DISABLE) {
                ringerMode = VCM_RING_OFF;

                


            } else if (ringSettingActive == FLASH_ONLY) {
                ringerMode = VCM_FLASHONLY_RING;

                


            } else if (ringSettingActive == RING_ONCE) {
                ringOnce = YES;

                











            } else if (ringSettingActive == RING) {

                
                switch (dcb->alert_info) {
                case ALERTING_NONE:
                    
                    break;

                case ALERTING_RING:
                    ringerMode = dcb->alerting_ring;
                    break;

                case ALERTING_OLD:
                default:
                    alertInfo = YES;
                }

                


            } else if (ringSettingActive == BEEP_ONLY) {
                lsm_set_beep_only_settings (dcb, &toneMode);

            }

            




            if (alerting) {
                




                if (ringSettingActive == BEEP_ONLY) {
                    media = gsmsdp_find_audio_media(dcb);

                    lsm_util_start_tone(toneMode, NO, lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID), dcb->group_id,
                                   ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                                   VCM_PLAY_TONE_TO_EAR);
                } else {

                    LSM_DEBUG(DEB_L_C_F_PREFIX"Active call RING SETTING: "
                              "ringer Mode = %s, Ring once = %d, alertInfo = %d\n",
                              DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname),
                              vm_alert_names[ringerMode], ringOnce, alertInfo);

                    vcmControlRinger(ringerMode, ringOnce, alertInfo, line, lcb->ui_id);
                }
            }

            


            if (lsm_tmr_tones_ticks > 0) {
                if (cprCancelTimer(lsm_tmr_tones) == CPR_FAILURE) {
                    LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                              fname, "cprCancelTimer", cpr_errno);
                }
                if (cprStartTimer(lsm_tmr_tones, lsm_tmr_tones_ticks,
                                  (void *)(long)dcb->call_id) == CPR_FAILURE) {
                    LSM_DEBUG(get_debug_string(DEBUG_GENERAL_SYSTEMCALL_FAILED),
                              fname, "cprStartTimer", cpr_errno);
                }
            }
        }
    } else if (lcb->state == LSM_S_IDLE) {
        




        config_get_value(CFGID_CALL_HOLD_RINGBACK, &callHoldRingback,
                         sizeof(callHoldRingback));
        if (callHoldRingback & 0x1) {
            callid_t        ui_id;

            dcb_cnt = fsmdef_get_dcbs_in_held_state(dcbs, call_id);
            for (i = 0, dcb = dcbs[i]; i < dcb_cnt; i++, dcb = dcbs[i]) {
              	ccb = fsmcnf_get_ccb_by_call_id(call_id);
               	xcb = fsmxfr_get_xcb_by_call_id(call_id);
               	if ((lsm_is_phone_inactive() == TRUE) &&
	               	(ccb == NULL) && (xcb == NULL) &&
                   	(lcb->enable_ringback == TRUE)) {
                    LSM_DEBUG(DEB_L_C_F_PREFIX"Applying ringback",
                              	DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, lcb->call_id, fname));
                    ringer_set = TRUE;

                    LSM_DEBUG(DEB_L_C_F_PREFIX"Hold RINGBACK SETTING: ringer Mode = "
                        	      "VCM_INSIDE_RING, Ring once = YES, alertInfo = YES\n",
                            	  DEB_L_C_F_PREFIX_ARGS(LSM, line, dcb->call_id, fname));
                    vcmControlRinger(VCM_INSIDE_RING, YES, YES, line, call_id);

                    
                    ui_id = lsm_get_ui_id(dcb->call_id);
                    ui_set_call_status(platform_get_phrase_index_str(CALL_INITIATE_HOLD),
                        	                  dcb->line, ui_id);
                }
            }
        }
    }

    if (ringer_set == FALSE) {

        LSM_DEBUG(DEB_L_C_F_PREFIX"Ringer_set = False : "
                  "ringer Mode = VCM_RING_OFF, Ring once = NO, alertInfo = NO\n",
                  DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname));


        if (!sdpmode) {
            vcmControlRinger(VCM_RING_OFF, NO, NO, line, call_id);
        }

    }
}

static cc_rcs_t
lsm_offhook (lsm_lcb_t *lcb, cc_state_data_offhook_t *data)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    fsmxfr_xcb_t   *xcb;
    lsm_lcb_t      *lcb2;
    callid_t        call_id2;
    int             attr;
    fsmdef_dcb_t   *dcb;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    lsm_change_state(lcb, __LINE__, LSM_S_OFFHOOK);

    



    FSM_FOR_ALL_CBS(lcb2, lsm_lcbs, LSM_MAX_LCBS) {
        if ((lcb2->call_id != CC_NO_CALL_ID) &&
            (lcb2->state == LSM_S_RINGIN)) {

            vcmControlRinger(VCM_RING_OFF, NO, NO, lcb2->line, lcb2->call_id);
        }
    }

    dp_offhook(line, call_id);

    attr = fsmutil_get_call_attr(dcb, line, call_id);

    ui_new_call(evOffHook, line, lcb->ui_id, attr,
                dcb->caller_id.call_instance_id,
                (boolean)FSM_CHK_FLAGS(lcb->flags, LSM_FLAGS_DIALED_STRING));

    xcb = fsmxfr_get_xcb_by_call_id(call_id);
    if (xcb != NULL) {
        call_id2 = ((lcb->call_id == xcb->xfr_call_id) ?
                    (xcb->cns_call_id) : (xcb->xfr_call_id));
        lcb2 = lsm_get_lcb_by_call_id(call_id2);
    }

    

    vcmEnableSidetone(YES);

    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_dialing (lsm_lcb_t *lcb, cc_state_data_dialing_t *data)
{
    fsmxfr_xcb_t   *xcb = NULL;
    int             stutterMsgWaiting = 0;
    fsmdef_dcb_t   *dcb = lcb->dcb;
    fsmdef_media_t *media = gsmsdp_find_audio_media(dcb);


    if ( dcb == NULL) {
        return (CC_RC_ERROR);
    }

    
    xcb = fsmxfr_get_xcb_by_call_id(lcb->call_id);
    if ((xcb != NULL) && (xcb->mode != FSMXFR_MODE_TRANSFEROR)) {
        return (CC_RC_SUCCESS);
    }

    


    if ((data->play_dt == TRUE)
        && (dp_check_for_plar_line(lcb->line) == FALSE)
        ) {

        
        config_get_value(CFGID_LINE_MESSAGE_WAITING_AMWI + lcb->line - 1, &stutterMsgWaiting,
                         sizeof(stutterMsgWaiting));
        if ( stutterMsgWaiting != 1 && stutterMsgWaiting != 0) {
          
          config_get_value(CFGID_STUTTER_MSG_WAITING, &stutterMsgWaiting,
                         sizeof(stutterMsgWaiting));
          stutterMsgWaiting &= 0x1; 
        }

        if ( (data->suppress_stutter == FALSE) &&
             (ui_line_has_mwi_active(lcb->line)) && 
		      stutterMsgWaiting ) {
            lsm_util_start_tone(VCM_STUTTER_TONE, FALSE, lsm_get_ms_ui_call_handle(lcb->line, CC_NO_CALL_ID, lcb->ui_id),
                    dcb->group_id,
                           ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                           VCM_PLAY_TONE_TO_EAR);
        } else {
            lsm_util_start_tone(VCM_INSIDE_DIAL_TONE, FALSE, lsm_get_ms_ui_call_handle(lcb->line, CC_NO_CALL_ID, lcb->ui_id),
                    dcb->group_id,
                           ((media != NULL) ? media->refid : CC_NO_MEDIA_REF_ID),
                           VCM_PLAY_TONE_TO_EAR);
        }
    }

    




    ui_call_state(evWaitingForDigits, lcb->line, lcb->ui_id, CC_CAUSE_NORMAL);


    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_dialing_completed (lsm_lcb_t *lcb, cc_state_data_dialing_completed_t *data)
{
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb = lcb->dcb;

    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    lsm_change_state(lcb, __LINE__, LSM_S_PROCEED);

    


    if (dp_get_kpml_state()) {
        return (CC_RC_SUCCESS);
    }

    ui_call_info(data->caller_id.calling_name,
                 data->caller_id.calling_number,
                 data->caller_id.alt_calling_number,
                 data->caller_id.display_calling_number,
                 data->caller_id.called_name,
                 data->caller_id.called_number,
                 data->caller_id.display_called_number,
                 data->caller_id.orig_called_name,
                 data->caller_id.orig_called_number,
                 data->caller_id.last_redirect_name,
                 data->caller_id.last_redirect_number,
                 (calltype_t)dcb->call_type,
                 line, lcb->ui_id,
                 dcb->caller_id.call_instance_id,
                 FSM_GET_SECURITY_STATUS(dcb),
                 FSM_GET_POLICY(dcb));

    lsm_ui_call_state(evProceed, line, lcb, CC_CAUSE_NORMAL);

    (void) lsm_stop_tone(lcb, NULL);

    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_call_sent (lsm_lcb_t *lcb, cc_state_data_call_sent_t *data)
{
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb;
    char            tmp_str[STATUS_LINE_MAX_LEN];
    fsmdef_media_t *media;
    static const char fname[] = "lsm_call_sent";

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    lsm_change_state(lcb, __LINE__, LSM_S_PROCEED);

    (void) lsm_stop_tone(lcb, NULL);

    







    GSMSDP_FOR_ALL_MEDIA(media, dcb) {
         if (!GSMSDP_MEDIA_ENABLED(media)) {
             continue;
         }
         LSM_DEBUG(DEB_F_PREFIX"%d %d %d", DEB_F_PREFIX_ARGS(LSM, fname), media->direction_set,
                   media->direction, media->is_multicast);
         if ((media->direction_set) &&
             ((media->direction == SDP_DIRECTION_SENDRECV) ||
              (media->direction == SDP_DIRECTION_RECVONLY))) {

             lsm_rx_start(lcb, cc_state_name(CC_STATE_FAR_END_ALERTING),
                          media);
         }
    }

    if (!dp_get_kpml_state()) {
        if ((platGetPhraseText(STR_INDEX_CALLING,
                                     (char *) tmp_str,
                                     STATUS_LINE_MAX_LEN - 1)) == CPR_SUCCESS) {
            ui_set_call_status(tmp_str, line, lcb->ui_id);
        }
    }

    


    dp_int_cancel_offhook_timer(line, lcb->call_id);

    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_far_end_proceeding (lsm_lcb_t *lcb,
                       cc_state_data_far_end_proceeding_t * data)
{
    line_t        line = lcb->line;
    fsmdef_dcb_t *dcb;

    lsm_change_state(lcb, __LINE__, LSM_S_PROCEED);

    if (!dp_get_kpml_state()) {
        ui_set_call_status(platform_get_phrase_index_str(CALL_PROCEEDING_IN),
                           line, lcb->ui_id);
        


        dcb = lcb->dcb;
        if (dcb != NULL && dcb->placed_call_update_required) {
            lsm_update_placed_callinfo(dcb);
            dcb->placed_call_update_required = FALSE;
        }
    }


    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_far_end_alerting (lsm_lcb_t *lcb, cc_state_data_far_end_alerting_t *data)
{
    static const char fname[] = "lsm_far_end_alerting";
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb;
    const char     *status = NULL;
    fsmcnf_ccb_t   *ccb;
    boolean         rcv_port_started = FALSE;
    char            tmp_str[STATUS_LINE_MAX_LEN];
    boolean         spoof_ringout;
    fsmdef_media_t  *media;
    call_events     call_state;
    fsmdef_media_t *audio_media;
    boolean         is_session_progress = FALSE;


    











    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }
    audio_media =  gsmsdp_find_audio_media(dcb);

    if (dcb->inband) {
        
        lsm_close_rx(lcb, TRUE, NULL);
        lsm_close_tx(lcb, TRUE, NULL);
    }

    







    if (dcb->spoof_ringout_requested &&
        ((lcb->state == LSM_S_CONNECTED) || (lcb->state == LSM_S_HOLDING))) {
        
        spoof_ringout = TRUE;
    } else {
        spoof_ringout = FALSE;
    }
    lsm_change_state(lcb, __LINE__, LSM_S_RINGOUT);

    


    if (dcb->active_feature != CC_FEATURE_CFWD_ALL) {
        dp_int_update(line, call_id, data->caller_id.called_number);
    }


    






    if (dcb->inband != TRUE || spoof_ringout) {
        status = platform_get_phrase_index_str(CALL_ALERTING_LOCAL);

        if (spoof_ringout) {

            if (audio_media) {

            



                lsm_util_start_tone(VCM_ALERTING_TONE, FALSE, lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID),
                           dcb->group_id,audio_media->refid,
                           VCM_PLAY_TONE_TO_EAR);
            }

        }
    } else {
    	is_session_progress = TRUE;
        (void) lsm_stop_tone(lcb, NULL);

        if ((platGetPhraseText(STR_INDEX_SESSION_PROGRESS,
                                     (char *) tmp_str,
                                     STATUS_LINE_MAX_LEN - 1)) == CPR_SUCCESS) {
            status = tmp_str;
        }

        
        GSMSDP_FOR_ALL_MEDIA(media, dcb) {
            if (!GSMSDP_MEDIA_ENABLED(media)) {
                
                continue;
            }
            LSM_DEBUG(DEB_L_C_F_PREFIX"direction_set:%d direction:%d"
                      " dest_addr:%p is_multicast:%d",
                      DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname),
                      media->direction_set,
                      media->direction, &media->dest_addr,
                      media->is_multicast);

            if (media->direction_set) {
                if (media->direction == SDP_DIRECTION_SENDRECV ||
                    media->direction == SDP_DIRECTION_RECVONLY) {
                    lsm_rx_start(lcb,
                                 cc_state_name(CC_STATE_FAR_END_ALERTING),
                                 media);
                    rcv_port_started = TRUE;
                }

                if (media->direction == SDP_DIRECTION_SENDRECV ||
                    media->direction == SDP_DIRECTION_SENDONLY) {
                    lsm_tx_start(lcb,
                             cc_state_name(CC_STATE_FAR_END_ALERTING),
                             media);
                }
            }
        }

        if (!rcv_port_started) {
            







            status = platform_get_phrase_index_str(CALL_ALERTING_LOCAL);
            lsm_util_start_tone(VCM_ALERTING_TONE, FALSE, lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID), dcb->group_id,
                           ((audio_media != NULL) ? audio_media->refid :
                                                   CC_NO_MEDIA_REF_ID),
                           VCM_PLAY_TONE_TO_EAR);
        } else {
            lsm_set_ringer(lcb, call_id, line, YES);
        }
    }

    ccb = fsmcnf_get_ccb_by_call_id(call_id);

    
    lsm_internal_update_call_info(lcb, dcb);

    



    ccb = fsmcnf_get_ccb_by_call_id(lcb->call_id);

    if ((ccb != NULL) && (ccb->active == TRUE) &&
        (ccb->flags & LCL_CNF)) {
        call_state = evConference;
    } else {
        call_state = evRingOut;
    }

    








    if (dcb->active_feature != CC_FEATURE_CFWD_ALL) {
    	if(!is_session_progress) {
    		


    		if (dcb->placed_call_update_required) {
    		    lsm_update_placed_callinfo(dcb);
    		    dcb->placed_call_update_required = FALSE;
    		}

    		if (status) {
    		    ui_set_call_status(status, line, lcb->ui_id);
    		}
    	}

        lsm_ui_call_state(call_state, line, lcb, CC_CAUSE_NORMAL);

    }
    




    if (dcb->active_feature == CC_FEATURE_CFWD_ALL) {
        lsm_ui_call_state(evReorder, line, lcb, CC_CAUSE_NORMAL);
    }

    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_call_received (lsm_lcb_t *lcb, cc_state_data_call_received_t *data)
{
    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_alerting (lsm_lcb_t *lcb, cc_state_data_alerting_t *data)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    lsm_change_state(lcb, __LINE__, LSM_S_RINGIN);

    dcb->ui_update_required = TRUE;
    lsm_internal_update_call_info(lcb, dcb);

    ui_new_call(evRingIn, line, lcb->ui_id, NORMAL_CALL,
                dcb->caller_id.call_instance_id, FALSE);

    fsmutil_set_shown_calls_ci_element(dcb->caller_id.call_instance_id, line);
    lsm_ui_call_state(evRingIn, line, lcb, CC_CAUSE_NORMAL);
    lsm_update_inalert_status(line, lcb->ui_id, data, TRUE);


    lsm_set_ringer(lcb, call_id, line, YES);

    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_answered (lsm_lcb_t *lcb, cc_state_data_answered_t *data)
{
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    lsm_change_state(lcb, __LINE__, LSM_S_OFFHOOK);


    lsm_internal_update_call_info(lcb, dcb);

    vcmControlRinger(VCM_RING_OFF, NO, NO, line, dcb->call_id);

    lsm_ui_call_state(evOffHook, line, lcb, CC_CAUSE_NORMAL);

    

    (void) lsm_stop_tone(lcb, NULL);

    return (CC_RC_SUCCESS);
}













static void
lsm_update_media (lsm_lcb_t *lcb, const char *caller_fname)
{
    static const char fname[] = "lsm_update_media";
    fsmdef_dcb_t   *dcb;
    fsmdef_media_t *media;
    boolean        rx_refresh;
    boolean        tx_refresh;
    char           addr_str[MAX_IPADDR_STR_LEN];
    int            i;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL),
                    fname);
        return;
    }

    addr_str[0] = '\0';

    









    GSMSDP_FOR_ALL_MEDIA(media, dcb) {
        if (!GSMSDP_MEDIA_ENABLED(media) ||
            FSM_CHK_FLAGS(media->hold, FSM_HOLD_LCL)) {
            
            continue;
        }

        rx_refresh = FALSE;
        tx_refresh = FALSE;

        if ((media->direction_set) && (media->is_multicast == FALSE)) {
            if (media->direction == SDP_DIRECTION_SENDRECV ||
                media->direction == SDP_DIRECTION_RECVONLY) {
                rx_refresh = TRUE;
            }
            if (media->direction == SDP_DIRECTION_SENDRECV ||
                media->direction == SDP_DIRECTION_SENDONLY) {
                tx_refresh = TRUE;
            }
        }

        lsm_close_rx(lcb, rx_refresh, media);
        lsm_close_tx(lcb, tx_refresh, media);

        if (LSMDebug) {
            
            ipaddr2dotted(addr_str, &media->dest_addr);
            for (i = 0; i < media->num_payloads; i++)
            {
                LSM_DEBUG(DEB_L_C_F_PREFIX"%d rx, tx refresh's are %d %d"
                          ", dir=%d, payload=%p addr=%s, multicast=%d\n",
                          DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line,
                          dcb->call_id, fname), media->refid, rx_refresh,
                          tx_refresh, media->direction,
                          &media->payloads[i], addr_str, media->is_multicast );
            }
        }
        if (rx_refresh ||
            (media->is_multicast &&
             media->direction_set &&
             media->direction == SDP_DIRECTION_RECVONLY)) {
            lsm_rx_start(lcb, caller_fname, media);
        }
        if (tx_refresh) {
            lsm_tx_start(lcb, caller_fname, media);
        }
        if ( rx_refresh &&
              (media->cap_index == CC_VIDEO_1)) {
             
             ui_update_video_avail(dcb->line, lcb->ui_id, dcb->cur_video_avail);
             LSM_DEBUG(DEB_L_C_F_PREFIX"Video Avail Called %d",
                  DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, lcb->ui_id, fname), dcb->cur_video_avail);
         }
    }
}














static void
lsm_call_state_media (lsm_lcb_t *lcb, line_t line, const char *fname)
{
    fsmcnf_ccb_t   *ccb;
    fsmdef_dcb_t   *dcb;
    call_events     call_state;
    callid_t        call_id = lcb->call_id;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL),
                    "lsm_call_state_media");
        return;
    }

    ccb = fsmcnf_get_ccb_by_call_id(call_id);

    
    lsm_update_media(lcb, fname);

    if ((ccb != NULL) && (ccb->active == TRUE)) {
        
        if ((ccb->flags & JOINED) ||
            (fname == cc_state_name(CC_STATE_RESUME))) {
            call_state = evConnected;
        } else {
            call_state = evConference;
        }
    } else {
        call_state = evConnected;
    }

    




    
    

    lsm_ui_call_state(call_state, line, lcb, CC_CAUSE_NORMAL);
    
    lsm_internal_update_call_info(lcb, dcb);
}


static cc_rcs_t
lsm_connected (lsm_lcb_t *lcb, cc_state_data_connected_t *data)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb;
    int             alerting = YES;
    call_events     original_call_event;
    int ringSettingBusyStationPolicy;
    boolean tone_stop_bool = TRUE;
    int             sdpmode = 0;
    boolean         start_ice = FALSE;

    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    original_call_event = lcb->previous_call_event;
    



    if (lcb->state == LSM_S_HOLDING) {
        config_get_value(CFGID_RING_SETTING_BUSY_POLICY,
                     &ringSettingBusyStationPolicy,
                     sizeof(ringSettingBusyStationPolicy));
        if (0 == ringSettingBusyStationPolicy) {
            alerting = NO;
        }

		



		if(lcb->dcb->active_tone == VCM_MONITORWARNING_TONE || lcb->dcb->active_tone == VCM_RECORDERWARNING_TONE)
			tone_stop_bool = FALSE;
    }

    


    if (strlen(dcb->peerconnection) && lcb->state != LSM_S_CONNECTED)
      start_ice = TRUE;

    lsm_change_state(lcb, __LINE__, LSM_S_CONNECTED);

    if (!sdpmode) {
        if (tone_stop_bool == TRUE)
            (void) lsm_stop_tone(lcb, NULL);
    }

    
    if (start_ice) {
      short res = vcmStartIceChecks(dcb->peerconnection,
                                    !dcb->inbound || dcb->peer_ice_lite);

      
      if (res)
        return CC_RC_SUCCESS;
    }

    


    lsm_call_state_media(lcb, line, cc_state_name(CC_STATE_CONNECTED));


    if (!sdpmode) {
        vcmEnableSidetone(YES);

        lsm_set_ringer(lcb, call_id, line, alerting);
    }

    FSM_RESET_FLAGS(lcb->flags, LSM_FLAGS_ANSWER_PENDING);
    FSM_RESET_FLAGS(lcb->flags, LSM_FLAGS_DUSTING);

    


    if (dcb->placed_call_update_required) {
        lsm_update_placed_callinfo(dcb);
        dcb->placed_call_update_required = FALSE;
    }

    


    if (lcb->previous_call_event != original_call_event) {
        if (lcb->previous_call_event == evConference) {
        } else {

             ui_set_call_status(platform_get_phrase_index_str(CALL_CONNECTED),
                               line, lcb->ui_id);
        }
    }
    ui_update_video_avail(line, lcb->ui_id, dcb->cur_video_avail);
    return (CC_RC_SUCCESS);
}













static cc_rcs_t
lsm_hold_reversion (lsm_lcb_t *lcb)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;

    
    lsm_ui_call_state(evHoldRevert, line, lcb, CC_CAUSE_NORMAL);

    if (lsm_find_state(LSM_S_RINGIN) > CC_NO_CALL_ID) {
        
        return CC_RC_SUCCESS;
    }
    ui_set_notification(line, call_id,
                        (char *)INDEX_STR_HOLD_REVERSION, CALL_ALERT_TIMEOUT,
                        FALSE, HR_NOTIFY_PRI);
    lsm_reversion_ringer(lcb, call_id, line);

    return (CC_RC_SUCCESS);
}








static cc_rcs_t
lsm_hold_local (lsm_lcb_t *lcb, cc_state_data_hold_t *data)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb;
    cc_causes_t    cause;
    int ringSettingBusyStationPolicy;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    


    if (dcb->spoof_ringout_applied) {
        (void) lsm_stop_tone(lcb, NULL);
    }

    
    lsm_close_rx(lcb, FALSE, NULL);
    lsm_close_tx(lcb, FALSE, NULL);
    





    lsm_change_state(lcb, __LINE__, LSM_S_HOLDING);
    



    cause = CC_CAUSE_NORMAL;
    if (data->reason == CC_REASON_XFER) {
            cause = CC_CAUSE_XFER_LOCAL;
    } else if (data->reason == CC_REASON_CONF) {
        cause = CC_CAUSE_CONF;
    }

    lsm_ui_call_state(evHold, line, lcb, cause);

    ui_set_call_status(platform_get_phrase_index_str(CALL_INITIATE_HOLD),
                       line, lcb->ui_id);

    config_get_value(CFGID_RING_SETTING_BUSY_POLICY,
                     &ringSettingBusyStationPolicy,
                     sizeof(ringSettingBusyStationPolicy));
    if (ringSettingBusyStationPolicy) {
        lsm_set_ringer(lcb, call_id, line, YES);
    } else {
        









        if (data->reason == CC_REASON_INTERNAL) {
            lsm_set_ringer(lcb, call_id, line, NO);
        } else {
            lsm_set_ringer(lcb, call_id, line, YES);
        }
    }

    vcmActivateWlan(FALSE);

    return (CC_RC_SUCCESS);
}









static cc_rcs_t
lsm_hold_remote (lsm_lcb_t *lcb, cc_state_data_hold_t *data)
{
    static const char fname[] = "lsm_hold_remote";
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    const char     *prompt_status;
    fsmdef_dcb_t   *dcb;
    fsmdef_media_t *media;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    
    GSMSDP_FOR_ALL_MEDIA(media, dcb) {
        if (!GSMSDP_MEDIA_ENABLED(media)) {
            
            continue;
        }
        if (media->direction_set &&
            media->direction == SDP_DIRECTION_INACTIVE) {
            lsm_close_rx(lcb, FALSE, media);
        } else {
            lsm_close_rx(lcb, TRUE, media);
        }

        
        if (media->direction_set &&
            media->direction == SDP_DIRECTION_RECVONLY) {
            lsm_rx_start(lcb, fname, media);
        }
        
        if ((media->direction == SDP_DIRECTION_INACTIVE) ||
            (media->direction == SDP_DIRECTION_RECVONLY)) {
            lsm_close_tx(lcb, FALSE, media);
        }
    }

    lsm_internal_update_call_info(lcb, dcb);

    lsm_ui_call_state(evRemHold, line, lcb, CC_CAUSE_NORMAL);

    prompt_status = ((lcb->state == LSM_S_CONNECTED) ?
                     platform_get_phrase_index_str(CALL_CONNECTED) :
                     platform_get_phrase_index_str(CALL_INITIATE_HOLD));
    ui_set_call_status(prompt_status, line, lcb->ui_id);

    lsm_set_ringer(lcb, call_id, line, YES);


    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_hold (lsm_lcb_t *lcb, cc_state_data_hold_t *data)
{
    cc_rcs_t cc_rc;

    if (data == NULL) {
        return (CC_RC_ERROR);
    }

    LSM_DEBUG(get_debug_string(LSM_DBG_INT1), lcb->call_id, lcb->line,
              "lsm_hold", "local", data->local);

    switch (data->local) {
    case (TRUE):
        cc_rc = lsm_hold_local(lcb, data);
        break;

    case (FALSE):
        cc_rc = lsm_hold_remote(lcb, data);
        break;

    default:
        cc_rc = CC_RC_ERROR;
        break;
    }
    vcmEnableSidetone(NO);
    return (cc_rc);
}


static cc_rcs_t
lsm_resume_local (lsm_lcb_t *lcb, cc_state_data_resume_t *data)
{
    line_t        line = lcb->line;
    fsmdef_dcb_t *dcb;

    lsm_change_state(lcb, __LINE__, LSM_S_HOLDING);

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    ui_set_call_status(platform_get_phrase_index_str(CALL_CONNECTED),
                       line, lcb->ui_id);

    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_resume_remote (lsm_lcb_t *lcb, cc_state_data_resume_t *data)
{
    callid_t      call_id = lcb->call_id;
    line_t        line = lcb->line;
    const char   *prompt_status;

    if (lcb->dcb == NULL) {
        return (CC_RC_ERROR);
    }

    lsm_update_media(lcb, cc_state_name(CC_STATE_RESUME));

    prompt_status = ((lcb->state == LSM_S_CONNECTED) ?
                     platform_get_phrase_index_str(CALL_CONNECTED) :
                     platform_get_phrase_index_str(CALL_INITIATE_HOLD));
    ui_set_call_status(prompt_status, line, lcb->ui_id);

    lsm_set_ringer(lcb, call_id, line, YES);

    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_resume (lsm_lcb_t *lcb, cc_state_data_resume_t *data)
{
    cc_rcs_t cc_rc;

    if (data == NULL) {
        return (CC_RC_ERROR);
    }

    LSM_DEBUG(get_debug_string(LSM_DBG_INT1), lcb->call_id, lcb->line,
              "lsm_resume", "local", data->local);

    switch (data->local) {
    case (TRUE):
        cc_rc = lsm_resume_local(lcb, data);
        break;

    case (FALSE):
        cc_rc = lsm_resume_remote(lcb, data);
        break;

    default:
        cc_rc = CC_RC_ERROR;
        break;
    }

    vcmActivateWlan(TRUE);

    vcmEnableSidetone(YES);
    return (cc_rc);
}


static cc_rcs_t
lsm_onhook (lsm_lcb_t *lcb, cc_state_data_onhook_t *data)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    fsmdef_dcb_t   *dcb;
    cc_causes_t     cause;
    int             sdpmode = 0;

	config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));


    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    dp_int_onhook(line, call_id);

    
    lsm_close_rx(lcb, FALSE, NULL);
    lsm_close_tx(lcb, FALSE, NULL);

    lsm_change_state(lcb, __LINE__, LSM_S_IDLE);

    if (lsm_is_phone_inactive()) {
        vcmEnableSidetone(NO);
    }

    ui_set_call_status(ui_get_idle_prompt_string(), line, lcb->ui_id);


    (void) lsm_stop_tone(lcb, NULL);

    if (!sdpmode) {
        vcmControlRinger(VCM_RING_OFF, NO, NO, line, dcb->call_id);
    }

    lsm_set_ringer(lcb, call_id, line, YES);

    cause = data->cause;
    if (FSM_CHK_FLAGS(dcb->flags, FSMDEF_F_XFER_COMPLETE)) {
        DEF_DEBUG(DEB_F_PREFIX"Transfer complete.", DEB_F_PREFIX_ARGS(LSM, "lsm_onhook"));
        cause = CC_CAUSE_XFER_COMPLETE;
    }
    lsm_ui_call_state(evOnHook, line, lcb, cause);


    lsm_free_lcb(lcb);

    vcmActivateWlan(FALSE);

    vcmRemoveBandwidth(lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID));

    return (CC_RC_SUCCESS);
}

static cc_rcs_t
lsm_call_failed (lsm_lcb_t *lcb, cc_state_data_call_failed_t *data)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    vcm_tones_t     tone;
    lsm_states_t    line_state;
    const char     *status = NULL;
    call_events     state;
    boolean         send_call_info = TRUE;
	fsmdef_dcb_t   *dcb;
    boolean         must_log = FALSE;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    


   if (data->cause != CC_CAUSE_UI_STATE_BUSY) {
       
       lsm_close_rx(lcb, FALSE, NULL);
       lsm_close_tx(lcb, FALSE, NULL);
   }

    switch (data->cause) {
    case (CC_CAUSE_BUSY):
        line_state = LSM_S_BUSY;
        state = evBusy;
        tone = VCM_LINE_BUSY_TONE;
        status = platform_get_phrase_index_str(LINE_BUSY);
        dp_int_update(line, call_id, data->caller_id.called_number);
        send_call_info = FALSE;
        break;

    case (CC_CAUSE_UI_STATE_BUSY):
        line_state = LSM_S_BUSY;
        state = evBusy;
        tone = VCM_LINE_BUSY_TONE;
        dp_int_update(line, call_id, data->caller_id.called_number);
        break;

    case (CC_CAUSE_INVALID_NUMBER):
        line_state = LSM_S_INVALID_NUMBER;
        state = evReorder;
        tone = VCM_REORDER_TONE;
        send_call_info = FALSE;
        break;

    case (CC_CAUSE_CONGESTION):
    case (CC_CAUSE_PAYLOAD_MISMATCH):
        dp_int_update(line, call_id, data->caller_id.called_number);

        
        
    default:
        send_call_info = FALSE;
        line_state = LSM_S_CONGESTION;
        state = evReorder;
        tone = VCM_REORDER_TONE;
        if ( (data->cause == CC_CAUSE_NO_USER_ANS)||
             (data->cause == CC_TEMP_NOT_AVAILABLE) ) {
           must_log = TRUE;
        }
        break;
    }

    lsm_change_state(lcb, __LINE__, line_state);

    if (status) {
        ui_set_call_status(status, line, lcb->ui_id);
    }

    if (state == evReorder && !must_log) {
        ui_log_disposition(dcb->call_id, CC_CALL_LOG_DISP_IGNORE);
    }

    
    if (send_call_info == TRUE) {
        ui_call_info(data->caller_id.calling_name,
                 data->caller_id.calling_number,
                 data->caller_id.alt_calling_number,
                 data->caller_id.display_calling_number,
                 data->caller_id.called_name,
                 data->caller_id.called_number,
                 data->caller_id.display_called_number,
                 data->caller_id.orig_called_name,
                 data->caller_id.orig_called_number,
                 data->caller_id.last_redirect_name,
                 data->caller_id.last_redirect_number,
                 (calltype_t)dcb->call_type,
                 line, lcb->ui_id,
                 dcb->caller_id.call_instance_id,
                 FSM_GET_SECURITY_STATUS(dcb),
                 FSM_GET_POLICY(dcb));
    }

    lsm_ui_call_state(state, line, lcb, CC_CAUSE_NORMAL);

    

    if ((data->cause != CC_CAUSE_UI_STATE_BUSY) && (data->cause != CC_CAUSE_REMOTE_DISCONN_REQ_PLAYTONE)) {
        fsmdef_media_t *audio_media = gsmsdp_find_audio_media(dcb);

        lsm_util_start_tone(tone, FALSE, lsm_get_ms_ui_call_handle(line, call_id, CC_NO_CALL_ID), dcb->group_id,
                       ((audio_media != NULL) ? audio_media->refid :
                                                CC_NO_MEDIA_REF_ID),
                       VCM_PLAY_TONE_TO_EAR);
    }

    return (CC_RC_SUCCESS);
}

static void
lsm_ringer (lsm_lcb_t *lcb, cc_action_data_ringer_t *data)
{
    vcm_ring_mode_t ringer;
    line_t line = lcb->line;

    ringer = (data->on == FALSE) ? (VCM_RING_OFF) : (VCM_FEATURE_RING);

    LSM_DEBUG(DEB_F_PREFIX"CTI RING SETTING: line = %d, ringer Mode = %s,"
              "Ring once = NO, alertInfo = NO\n", DEB_F_PREFIX_ARGS(LSM, "lsm_ringer"),
              line, vm_alert_names[ringer]);

    vcmControlRinger(ringer, NO, NO, line, lcb->call_id);
}

static cc_rcs_t
lsm_dial_mode (lsm_lcb_t *lcb, cc_action_data_dial_mode_t *data)
{
    return (CC_RC_SUCCESS);
}


static cc_rcs_t
lsm_mwi (lsm_lcb_t *lcb, callid_t call_id, line_t line,
         cc_action_data_mwi_t *data)
{
    ui_set_mwi(line, data->on, data->type, data->newCount, data->oldCount, data->hpNewCount, data->hpOldCount);

    return (CC_RC_SUCCESS);
}

















cc_rcs_t
lsm_update_ui (lsm_lcb_t *lcb, cc_action_data_update_ui_t *data)
{
    callid_t        call_id = lcb->call_id;
    line_t          line = lcb->line;
    lsm_states_t    instance_state;
    call_events     call_state = evMaxEvent;
    fsmcnf_ccb_t   *ccb;
    fsmdef_dcb_t   *dcb;
    boolean         update = FALSE;
    boolean         inbound;
    cc_feature_data_call_info_t *call_info;
    call_events     original_call_event;
    lsm_lcb_t       *lcb_tmp;
    const char      *conf_str;

    instance_state = lcb->state;

    switch (data->action) {
    case CC_UPDATE_CONF_ACTIVE:

        switch (instance_state) {
        case LSM_S_RINGOUT:
            call_state = evRingOut;
            break;

        case LSM_S_CONNECTED:
        default:
            ccb = fsmcnf_get_ccb_by_call_id(call_id);
            if ((ccb != NULL) && (ccb->active == TRUE)) {

                conf_str = platform_get_phrase_index_str(UI_CONFERENCE);
                lcb_tmp = lsm_get_lcb_by_call_id(ccb->cnf_call_id);
                dcb = lcb_tmp->dcb;
                ui_call_info(CALL_INFO_NONE,
                              CALL_INFO_NONE,
                              CALL_INFO_NONE,
                              0,
                              conf_str,
                              CALL_INFO_NONE,
                              0,
                              CALL_INFO_NONE,
                              CALL_INFO_NONE,
                              CALL_INFO_NONE,
                              CALL_INFO_NONE,
                              FSMDEF_CALL_TYPE_OUTGOING,
                              dcb->line, lcb_tmp->ui_id,
                              dcb->caller_id.call_instance_id,
                              FSM_GET_SECURITY_STATUS(dcb),
                              FSM_GET_POLICY(dcb));

                call_state = evConference;

            } else if (instance_state == LSM_S_CONNECTED) {

                call_state = evConnected;

            } else {

                call_state = evRingOut;
            }
            break;
        }                       

        break;

    case CC_UPDATE_CALLER_INFO:

        





        ccb = fsmcnf_get_ccb_by_call_id(call_id);
        if (ccb && (ccb->flags & LCL_CNF) &&
            (ccb->cnf_call_id == call_id)) {
            break;
        }

        call_info = &data->data.caller_info;
        dcb = lcb->dcb;
        if (dcb == NULL || call_info == NULL) {
            return (CC_RC_ERROR);
        }

        inbound = dcb->inbound;
        if (call_info->feature_flag & CC_ORIENTATION) {
            inbound =
                (call_info->orientation == CC_ORIENTATION_FROM) ? TRUE : FALSE;
            update = TRUE;
        }

        if (call_info->feature_flag & CC_CALLER_ID) {
            update = TRUE;
            








            if ( (instance_state == LSM_S_RINGIN) && inbound ) {
                cc_state_data_alerting_t alerting_data;

                alerting_data.caller_id = dcb->caller_id;
                lsm_update_inalert_status(line, lcb->ui_id, &alerting_data, TRUE);
            }
        }

        if (call_info->feature_flag & CC_CALL_INSTANCE) {
            update = TRUE;
        }

        if (call_info->feature_flag & CC_SECURITY) {
            update = TRUE;
        }

	if (call_info->feature_flag & CC_POLICY) {
	    update = TRUE;
	}

        




        if (dcb->spoof_ringout_requested &&
            !dcb->spoof_ringout_applied &&
            lcb->state == LSM_S_CONNECTED) {
            cc_state_data_far_end_alerting_t alerting_data;

            alerting_data.caller_id = dcb->caller_id;
            (void) lsm_far_end_alerting(lcb, &alerting_data);
            dcb->spoof_ringout_applied = TRUE;
        } else if (update && dcb->ui_update_required) {

            calltype_t call_type;

            if (dcb->call_type == FSMDEF_CALL_TYPE_FORWARD) {
                call_type = (inbound) ? (calltype_t)dcb->call_type:FSMDEF_CALL_TYPE_OUTGOING;
            } else {
                if (inbound) {
                    call_type = FSMDEF_CALL_TYPE_INCOMING;
                } else {
                    call_type = FSMDEF_CALL_TYPE_OUTGOING;
                }
            }

             ui_call_info(dcb->caller_id.calling_name,
                          dcb->caller_id.calling_number,
                          dcb->caller_id.alt_calling_number,
                          dcb->caller_id.display_calling_number,
                          dcb->caller_id.called_name,
                          dcb->caller_id.called_number,
                          dcb->caller_id.display_called_number,
                          dcb->caller_id.orig_called_name,
                          dcb->caller_id.orig_called_number,
                          dcb->caller_id.last_redirect_name,
                          dcb->caller_id.last_redirect_number,
                          call_type,
                          line,
                          lcb->ui_id,
                          dcb->caller_id.call_instance_id,
                          FSM_GET_SECURITY_STATUS(dcb),
                          FSM_GET_POLICY(dcb));

            dcb->ui_update_required = FALSE;

            conf_str = platform_get_phrase_index_str(UI_CONFERENCE);
 	    if(cpr_strncasecmp(dcb->caller_id.called_name, conf_str, strlen(conf_str)) == 0){
 		    dcb->is_conf_call = TRUE;
 	    } else {
 		    dcb->is_conf_call = FALSE;
 	    }
        }

        break;

    case CC_UPDATE_SET_CALL_STATUS:
        {
            
            cc_set_call_status_data_t *call_status_p =
                &data->data.set_call_status_parms;
            ui_set_call_status(call_status_p->phrase_str_p, call_status_p->line,
                               lcb->ui_id);
            break;
        }
    case CC_UPDATE_SET_NOTIFICATION:
        {
            
            cc_set_notification_data_t *call_notification_p =
                &data->data.set_notification_parms;
            ui_set_notification(line, lcb->ui_id,
                                call_notification_p->phrase_str_p,
                                call_notification_p->timeout, FALSE,
                                (char)call_notification_p->priority);
            break;
        }
    case CC_UPDATE_CLEAR_NOTIFICATION:
        
        ui_clear_notification();
        break;

    case CC_UPDATE_SECURITY_STATUS:
        
        break;

    case CC_UPDATE_XFER_PRIMARY:
        call_state = evConnected;
        break;

    case CC_UPDATE_CALL_PRESERVATION:

        
        ui_call_in_preservation(line, lcb->ui_id);
        break;

    case CC_UPDATE_CALL_CONNECTED:
        if (instance_state == LSM_S_CONNECTED) {
            call_state = evConnected;
        }
        break;

    case CC_UPDATE_CONF_RELEASE:
        dcb = lcb->dcb;

        if (instance_state == LSM_S_CONNECTED) {
            call_state = evConnected;

        } else if (instance_state == LSM_S_RINGOUT) {
            call_state = evRingOut;
        }

        




        if (dcb->spoof_ringout_requested &&
            !dcb->spoof_ringout_applied &&
            lcb->state == LSM_S_CONNECTED) {
            cc_state_data_far_end_alerting_t alerting_data;

            alerting_data.caller_id = dcb->caller_id;
            (void) lsm_far_end_alerting(lcb, &alerting_data);
            dcb->spoof_ringout_applied = TRUE;

            call_state = evRingOut;

        } else {
            calltype_t call_type;
            if (dcb->orientation == CC_ORIENTATION_FROM) {
                call_type = FSMDEF_CALL_TYPE_INCOMING;
            } else if (dcb->orientation == CC_ORIENTATION_TO) {
                call_type = FSMDEF_CALL_TYPE_OUTGOING;
            } else {
                call_type = (calltype_t)(dcb->call_type);
            }
            ui_call_info(dcb->caller_id.calling_name,
                          dcb->caller_id.calling_number,
                          dcb->caller_id.alt_calling_number,
                          dcb->caller_id.display_calling_number,
                          dcb->caller_id.called_name,
                          dcb->caller_id.called_number,
                          dcb->caller_id.display_called_number,
                          dcb->caller_id.orig_called_name,
                          dcb->caller_id.orig_called_number,
                          dcb->caller_id.last_redirect_name,
                          dcb->caller_id.last_redirect_number,
                          call_type,
                          line,
                          lcb->ui_id,
                          dcb->caller_id.call_instance_id,
                          FSM_GET_SECURITY_STATUS(dcb),
                          FSM_GET_POLICY(dcb));
        }


        break;

    default:
        break;
    }

    if (call_state != evMaxEvent) {
        original_call_event = lcb->previous_call_event;

        lsm_ui_call_state(call_state, line, lcb, CC_CAUSE_NORMAL);
        if (original_call_event != call_state) {
            
            switch (call_state) {
            case evConference:
                break;

            case evConnected:
            case evWhisper:
                ui_set_call_status(
                           platform_get_phrase_index_str(CALL_CONNECTED),
                           line, lcb->ui_id);
                break;

            default:
                break;
            }
        }
    }

    return (CC_RC_SUCCESS);
}














#define CISCO_PLAR_STRING  "x-cisco-serviceuri-offhook"
void
lsm_update_placed_callinfo (void *data)
{
    const char     *tmp_called_number = NULL;
    const char     *called_name = NULL;
    fsmdef_dcb_t   *dcb = NULL;
    lsm_lcb_t      *lcb;
    static const char fname[] = "lsm_update_placed_callinfo";
    boolean has_called_number = FALSE;

    LSM_DEBUG(DEB_F_PREFIX"Entering ...", DEB_F_PREFIX_ARGS(LSM, fname));
    dcb = (fsmdef_dcb_t *) data;
    lcb = lsm_get_lcb_by_call_id(dcb->call_id);
    if (lcb == NULL) {
        LSM_DEBUG(DEB_F_PREFIX"Exiting: lcb not found", DEB_F_PREFIX_ARGS(LSM, fname));
        return;
    }

    if (dcb->caller_id.called_number != NULL &&
        dcb->caller_id.called_number[0] != NUL) {
        has_called_number = TRUE;
    }


    tmp_called_number = lsm_get_gdialed_digits();


    
    if (tmp_called_number == NULL || (*tmp_called_number) == NUL) {
        LSM_DEBUG(DEB_L_C_F_PREFIX"Exiting : dialed digits is empty",
                  DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, lcb->call_id, fname));
        return;
    }

    



    if (has_called_number) {
        if (strcmp(tmp_called_number, CISCO_PLAR_STRING) == 0) {
            tmp_called_number = dcb->caller_id.called_number;
        }
        
        if (strcmp(dcb->caller_id.called_number, tmp_called_number) == 0) {
            called_name = dcb->caller_id.called_name;
        } else {
        	char tmp_str[STATUS_LINE_MAX_LEN];
        	platGetPhraseText(STR_INDEX_ANONYMOUS_SPACE, (char *)tmp_str, STATUS_LINE_MAX_LEN - 1);
            if(strcmp(dcb->caller_id.called_number,tmp_str) == 0
               && strcmp(dcb->caller_id.orig_rpid_number, tmp_called_number) == 0
               && strcmp(dcb->caller_id.called_name, platform_get_phrase_index_str(UI_UNKNOWN)) != 0) {
        	  called_name = dcb->caller_id.called_name;
            }
        }
    }
    ui_update_placed_call_info(lcb->line, lcb->call_id, called_name,
                               tmp_called_number);
    LSM_DEBUG(DEB_L_C_F_PREFIX"Exiting: invoked ui_update_placed_call_info()",
              DEB_L_C_F_PREFIX_ARGS(LSM, lcb->line, lcb->call_id, fname));
}

cc_int32_t
lsm_show_cmd (cc_int32_t argc, const char *arv[])
{
    int             i = 0;
    lsm_lcb_t      *lcb;

    debugif_printf("\n------------------ LSM lcbs -------------------");
    debugif_printf("\ni   call_id  line  state             lcb");
    debugif_printf("\n-----------------------------------------------\n");

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        debugif_printf("%-2d  %-7d  %-4d  %-16s  0x%8p\n",
                       i++, lcb->call_id, lcb->line,
                       lsm_state_name(lcb->state), lcb);

    }

    return (0);
}

void
lsm_init_config (void)
{
    






    config_get_value(CFGID_CALL_WAITING_SILENT_PERIOD, &callWaitingDelay,
                     sizeof(callWaitingDelay));
    callWaitingDelay = callWaitingDelay * 1000;
}

void
lsm_init (void)
{
    static const char fname[] = "lsm_init";
    lsm_lcb_t *lcb;
    int i;

    


    lsm_lcbs = (lsm_lcb_t *) cpr_calloc(LSM_MAX_LCBS, sizeof(lsm_lcb_t));
    if (lsm_lcbs == NULL) {
        LSM_ERR_MSG(LSM_F_PREFIX"lsm_lcbs cpr_calloc returned NULL", fname);
        return;
    }

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        lsm_init_lcb(lcb);
    }

    



    lsm_tmr_tones = cprCreateTimer("lsm_tmr_tones",
                                   GSM_MULTIPART_TONES_TIMER,
                                   TIMER_EXPIRATION, gsm_msgq);
    lsm_continuous_tmr_tones = cprCreateTimer("lsm_continuous_tmr_tones",
                                              GSM_CONTINUOUS_TONES_TIMER,
                                              TIMER_EXPIRATION,
                                              gsm_msgq);
    lsm_tone_duration_tmr = cprCreateTimer("lsm_tone_duration_tmr",
                                   		   GSM_TONE_DURATION_TIMER,
                                   		   TIMER_EXPIRATION, gsm_msgq);
    lsm_init_config();

    for (i=0 ; i<MAX_REG_LINES; i++) {
        lsm_call_perline[i] = 0;
    }

    memset(cfwdall_state_in_ccm_mode, 0, sizeof(cfwdall_state_in_ccm_mode));
}

void
lsm_shutdown (void)
{
    (void) cprDestroyTimer(lsm_tmr_tones);

    (void) cprDestroyTimer(lsm_continuous_tmr_tones);

    cpr_free(lsm_lcbs);
}










void
lsm_reset (void)
{
    line_t line;
    int    i;
    lsm_lcb_t *lcb;

    lsm_init_config();

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        lsm_init_lcb(lcb);
    }

    for (i=0 ; i<MAX_REG_LINES; i++) {
        lsm_call_perline[i] = 0;
    }

    for (line=0; line < MAX_REG_LINES+1; line++) {

        cc_line_ringer_mode[line] = CC_RING_DEFAULT;
    }
}









void
cc_call_attribute (callid_t call_id, line_t line, call_attr_t attribute)
{
    static const char fname[] = "cc_call_attribute";


    LSM_DEBUG(DEB_L_C_F_PREFIX"attribute=%d",
			  DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname), attribute);

    ui_set_call_attr(line, call_id, attribute);
}







void
cc_call_state (callid_t call_id, line_t line, cc_states_t state,
               cc_state_data_t *data)
{
    static const char fname[] = "cc_call_state";
    cc_rcs_t   result = CC_RC_SUCCESS;
    lsm_lcb_t *lcb;

    LSM_DEBUG(get_debug_string(LSM_DBG_ENTRY), call_id, line,
              cc_state_name(state));

    lcb = lsm_get_lcb_by_call_id(call_id);

    if (lcb == NULL) {
        LSM_DEBUG(get_debug_string(DEBUG_INPUT_NULL), fname);
        return;
    }

    switch (state) {
    case CC_STATE_OFFHOOK:
        result = lsm_offhook(lcb, &(data->offhook));
#ifdef TEST
        test_dial_calls(line, call_id, 500, "10011234");
#endif
        break;

    case CC_STATE_DIALING:
        result = lsm_dialing(lcb, &(data->dialing));
        break;

    case CC_STATE_DIALING_COMPLETED:
        result = lsm_dialing_completed(lcb, &(data->dialing_completed));
        break;

    case CC_STATE_CALL_SENT:
        result = lsm_call_sent(lcb, &(data->call_sent));
        break;

    case CC_STATE_FAR_END_PROCEEDING:
        result = lsm_far_end_proceeding(lcb, &(data->far_end_proceeding));
        break;

    case CC_STATE_FAR_END_ALERTING:
        result = lsm_far_end_alerting(lcb, &(data->far_end_alerting));
        break;

    case CC_STATE_CALL_RECEIVED:
        result = lsm_call_received(lcb, &(data->call_received));
        break;

    case CC_STATE_ALERTING:
        result = lsm_alerting(lcb, &(data->alerting));
        break;

    case CC_STATE_ANSWERED:
        result = lsm_answered(lcb, &(data->answered));
        break;

    case CC_STATE_CONNECTED:
        result = lsm_connected(lcb, &(data->connected));
#ifdef TEST
        test_disc_call(line, call_id);
        test_line_offhook(line, cc_get_new_call_id());
#endif
        break;

    case CC_STATE_HOLD:
        result = lsm_hold(lcb, &(data->hold));
        break;

    case CC_STATE_HOLD_REVERT:
        result = lsm_hold_reversion(lcb);
        break;

    case CC_STATE_RESUME:
        result = lsm_resume(lcb, &(data->resume));
        break;

    case CC_STATE_ONHOOK:
        result = lsm_onhook(lcb, &(data->onhook));
        break;

    case CC_STATE_CALL_FAILED:
        result = lsm_call_failed(lcb, &(data->call_failed));
        break;

    default:
        break;
    }

    if (result == CC_RC_ERROR) {
        LSM_DEBUG(get_debug_string(LSM_DBG_CC_ERROR), call_id, line, fname,
                  state, data);
    }

    return;
}

static cc_rcs_t
lsm_media (lsm_lcb_t *lcb, callid_t call_id, line_t line)
{
    fsmdef_dcb_t *dcb;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        return (CC_RC_ERROR);
    }

    if (!dcb->spoof_ringout_requested) {
        lsm_update_media(lcb, "MEDIA");
        vcmEnableSidetone(YES);
    } else if (!dcb->spoof_ringout_applied &&
               (lcb->state == LSM_S_CONNECTED)) {
        cc_state_data_far_end_alerting_t alerting_data;

        alerting_data.caller_id = dcb->caller_id;
        (void) lsm_far_end_alerting(lcb, &alerting_data);
        dcb->spoof_ringout_applied = TRUE;
    }

    return (CC_RC_SUCCESS);
}
















static void
lsm_stop_media (lsm_lcb_t *lcb, callid_t call_id, line_t line,
                cc_action_data_t *data)
{
    static const char fname[] = "lsm_stop_media";
    fsmdef_dcb_t *dcb;
    fsmdef_media_t *media;

    dcb = lcb->dcb;
    if (dcb == NULL) {
        LSM_DEBUG(get_debug_string(DEBUG_INPUT_NULL), fname);
        return;
    }

    
    if ((data == NULL) ||
        (data->stop_media.media_refid == CC_NO_MEDIA_REF_ID)) {
        
        lsm_close_rx(lcb, FALSE, NULL);
        lsm_close_tx(lcb, FALSE, NULL);
    } else {
        
        media = gsmsdp_find_media_by_refid(dcb,
                                           data->stop_media.media_refid);
        if (media != NULL) {
            lsm_close_rx(lcb, FALSE, media);
            lsm_close_tx(lcb, FALSE, media);
        } else {
            
            LSM_DEBUG(DEB_L_C_F_PREFIX"no media with reference ID %d found",
                      DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, dcb->call_id, fname),
					  data->stop_media.media_refid);
            return;
        }
    }
    lsm_set_ringer(lcb, call_id, line, YES);
}

















cc_rcs_t
lsm_add_remote_stream (line_t line, callid_t call_id, fsmdef_media_t *media, int *pc_stream_id)
{
    lsm_lcb_t *lcb;
    fsmdef_dcb_t *dcb;
    int vcm_ret;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (!lcb) {
        CSFLogError(logTag, "%s: lcb is null", __FUNCTION__);
        return CC_RC_ERROR;
    }

    dcb = lcb->dcb;
    if (!dcb) {
        CSFLogError(logTag, "%s: dcb is null", __FUNCTION__);
        return CC_RC_ERROR;
    }

    vcm_ret = vcmCreateRemoteStream(media->cap_index, dcb->peerconnection,
            pc_stream_id);

    if (vcm_ret) {
        CSFLogError(logTag, "%s: vcmCreateRemoteStream returned error: %d",
            __FUNCTION__, vcm_ret);
        return CC_RC_ERROR;
    }

    return CC_RC_SUCCESS;
}














void lsm_initialize_datachannel (fsmdef_dcb_t *dcb, fsmdef_media_t *media,
                                 int track_id)
{
    if (!dcb) {
        CSFLogError(logTag, "%s DCB is NULL", __FUNCTION__);
        return;
    }

    if (!media) {
        CSFLogError(logTag, "%s media is NULL", __FUNCTION__);
        return;
    }

    



    vcmInitializeDataChannel(dcb->peerconnection,
        track_id, media->datachannel_streams,
        media->local_datachannel_port, media->remote_datachannel_port,
        media->datachannel_protocol);
}















static boolean
cc_call_non_call_action (callid_t call_id, line_t line,
                         cc_actions_t action, cc_action_data_t *data)
{
    



    switch (action) {

    case CC_ACTION_MWI_LAMP_ONLY:
        if (data != NULL) {
            ui_change_mwi_lamp(data->mwi.on);
            return(TRUE);
        }
        break;

    case CC_ACTION_SET_LINE_RINGER:

        if (data != NULL) {
            return(TRUE);
        }
        break;

    case CC_ACTION_PLAY_BLF_ALERTING_TONE:
        lsm_play_tone(CC_FEATURE_BLF_ALERT_TONE);
        return TRUE;

    default:
        break;
    }

    return(FALSE);
}






















cc_rcs_t
cc_call_action (callid_t call_id, line_t line, cc_actions_t action,
               cc_action_data_t *data)
{
    static const char fname[] = "cc_call_action";
    cc_rcs_t   result = CC_RC_SUCCESS;
    lsm_lcb_t *lcb;
    fsmdef_dcb_t *dcb;
    fsmdef_media_t *media;

    LSM_DEBUG(get_debug_string(LSM_DBG_ENTRY), call_id, line,
              cc_action_name(action));

    


    if (cc_call_non_call_action(call_id, line, action, data)) {
        return (result);
    }

    lcb = lsm_get_lcb_by_call_id(call_id);

    if ((lcb == NULL) && (action != CC_ACTION_MWI)) {
        LSM_DEBUG(get_debug_string(DEBUG_INPUT_NULL), fname);
        return (CC_RC_ERROR);
    }

    switch (action) {
    case CC_ACTION_PLAY_TONE:
        if (data != NULL) {
            result = lsm_start_tone(lcb, &(data->tone));
        } else {
            result = CC_RC_ERROR;
        }
        break;

    case CC_ACTION_STOP_TONE:
        if (data != NULL) {
            result = lsm_stop_tone(lcb, &(data->tone));
        } else {
            result = CC_RC_ERROR;
        }
        break;

    case CC_ACTION_SPEAKER:
        break;

    case CC_ACTION_DIAL_MODE:
        if (data != NULL) {
            result = lsm_dial_mode(lcb, &(data->dial_mode));
        } else {
            result = CC_RC_ERROR;
        }
        break;

    case CC_ACTION_MWI:
        if (data != NULL) {
            result = lsm_mwi(NULL, call_id, line, &(data->mwi));
        } else {
            result = CC_RC_ERROR;
        }
        break;

    case CC_ACTION_OPEN_RCV:
        if (data != NULL) {
            result = lsm_open_rx(lcb, &(data->open_rcv), NULL);
        } else {
            result = CC_RC_ERROR;
        }
        break;

    case CC_ACTION_UPDATE_UI:
        if (data != NULL) {
            result = lsm_update_ui(lcb, &(data->update_ui));
        } else {
            result = CC_RC_ERROR;
        }
        break;

    case CC_ACTION_MEDIA:
        result = lsm_media(lcb, call_id, line);
        break;

    case CC_ACTION_RINGER:
        if (data != NULL) {
            lsm_ringer(lcb, &(data->ringer));
        }
        break;

    case CC_ACTION_STOP_MEDIA:
        lsm_stop_media(lcb, call_id, line, data);
        break;

    case CC_ACTION_START_RCV:
        
        dcb = lcb->dcb;
        if (dcb == NULL) {
            
            result = CC_RC_ERROR;
            break;
        }

        GSMSDP_FOR_ALL_MEDIA(media, dcb) {
            if (!GSMSDP_MEDIA_ENABLED(media)) {
                
                continue;
            }

            
            lsm_rx_start(lcb, fname, media);
        }
        break;

    case CC_ACTION_ANSWER_PENDING:
        FSM_SET_FLAGS(lcb->flags, LSM_FLAGS_ANSWER_PENDING);
        break;

    default:
        break;
    }


    if (result == CC_RC_ERROR) {
        LSM_DEBUG(get_debug_string(LSM_DBG_CC_ERROR), call_id, line, fname,
                  action, data);
    }

    return (result);
}


void
lsm_ui_display_notify (const char *notify_str, unsigned long timeout)
{
    


    ui_set_notification(CC_NO_LINE, CC_NO_CALL_ID,
                        (char *)notify_str, (int)timeout, FALSE,
                        DEF_NOTIFY_PRI);
}

void
lsm_ui_display_status (const char *status_str, line_t line, callid_t call_id)
{
    lsm_lcb_t *lcb;

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }
    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb == NULL) {
        return;
    }

    ui_set_call_status((char *) status_str, line, lcb->ui_id);
}








void lsm_ui_display_notify_str_index (int str_index)
{
    char tmp_str[STATUS_LINE_MAX_LEN];

    if ((platGetPhraseText(str_index,
                                 (char *)tmp_str,
                                 (STATUS_LINE_MAX_LEN - 1))) == CPR_SUCCESS) {
        lsm_ui_display_notify(tmp_str, NO_FREE_LINES_TIMEOUT);
    }
}











string_t
lsm_parse_displaystr (string_t displaystr)
{
    return (sippmh_parse_displaystr(displaystr));
}

void
lsm_speaker_mode (short mode)
{
    ui_set_speaker_mode((boolean)mode);
}













void
lsm_update_active_tone (vcm_tones_t tone, callid_t call_id)
{
    static const char fname[] = "lsm_update_active_tone";
    fsmdef_dcb_t *dcb;

    




    switch (tone) {
    
    case VCM_INSIDE_DIAL_TONE:
    case VCM_LINE_BUSY_TONE:
    case VCM_ALERTING_TONE:
    case VCM_STUTTER_TONE:
    case VCM_REORDER_TONE:
    case VCM_OUTSIDE_DIAL_TONE:
    case VCM_PERMANENT_SIGNAL_TONE:
    case VCM_RECORDERWARNING_TONE:
    case VCM_MONITORWARNING_TONE:
        dcb = fsmdef_get_dcb_by_call_id(call_id);

        if (dcb == NULL) {
            


            dcb = fsmdef_get_dcb_by_call_id(lsm_get_callid_from_ui_id(call_id));
        }

        if (dcb != NULL) {
            




            if (dcb->active_tone != VCM_NO_TONE) {
                LSM_DEBUG(DEB_L_C_F_PREFIX"Active Tone current = %d  new = %d",
                          DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, call_id, fname),
						  dcb->active_tone, tone);
            }
            dcb->active_tone = tone;
        }
        break;

    default:
        
        break;
    }
}











boolean
lsm_is_tx_channel_opened(callid_t call_id)
{
    fsmdef_dcb_t *dcb_p = fsmdef_get_dcb_by_call_id(call_id);
    fsmdef_media_t *media = NULL;

    if (dcb_p == NULL) {
        return (FALSE);
    }

    



    GSMSDP_FOR_ALL_MEDIA(media, dcb_p) {
        if (media->type == SDP_MEDIA_AUDIO) {
            
            if (media->xmit_chan)
               return (TRUE);
        }
    }
    return (FALSE);
}













void
lsm_update_monrec_tone_action (vcm_tones_t tone, callid_t call_id, uint16_t direction)
{
    static const char fname[] = "lsm_update_monrec_tone_action";
    fsmdef_dcb_t *dcb;
    boolean tx_opened = lsm_is_tx_channel_opened(call_id);

    dcb = fsmdef_get_dcb_by_call_id(call_id);

    if (dcb != NULL) {
            switch(tone) {
                case VCM_MONITORWARNING_TONE:
                    switch (dcb->monrec_tone_action) {
                        case FSMDEF_MRTONE_NO_ACTION:
                            if (!tx_opened) {
                                dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_MONITOR_TONE;
                            } else {
                                dcb->monrec_tone_action = FSMDEF_MRTONE_PLAYED_MONITOR_TONE;
                            }
                            break;

                        case FSMDEF_MRTONE_PLAYED_RECORDER_TONE:
                            dcb->monrec_tone_action = FSMDEF_MRTONE_PLAYED_BOTH_TONES;
                            break;

                         case FSMDEF_MRTONE_RESUME_RECORDER_TONE:
                            dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_BOTH_TONES;
                            break;

                        case FSMDEF_MRTONE_PLAYED_MONITOR_TONE:
                        case FSMDEF_MRTONE_PLAYED_BOTH_TONES:
                        case FSMDEF_MRTONE_RESUME_MONITOR_TONE:
                        case FSMDEF_MRTONE_RESUME_BOTH_TONES:
                        default:
                            DEF_DEBUG(DEB_F_PREFIX"Invalid action request... tone:%d monrec_tone_action:%d",
                                      DEB_F_PREFIX_ARGS("RCC", fname), tone, dcb->monrec_tone_action);
                            break;
                    }
                    dcb->monitor_tone_direction = direction;
                    break;

                case VCM_RECORDERWARNING_TONE:
                    switch (dcb->monrec_tone_action) {
                        case FSMDEF_MRTONE_NO_ACTION:
                            if (!tx_opened) {
                                dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_RECORDER_TONE;
                            } else {
                                dcb->monrec_tone_action = FSMDEF_MRTONE_PLAYED_RECORDER_TONE;
                            }
                            break;

                        case FSMDEF_MRTONE_PLAYED_MONITOR_TONE:
                            dcb->monrec_tone_action = FSMDEF_MRTONE_PLAYED_BOTH_TONES;
                            break;

                        case FSMDEF_MRTONE_RESUME_MONITOR_TONE:
                            dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_BOTH_TONES;
                            break;

                        case FSMDEF_MRTONE_PLAYED_RECORDER_TONE:
                        case FSMDEF_MRTONE_PLAYED_BOTH_TONES:
                        case FSMDEF_MRTONE_RESUME_RECORDER_TONE:
                        case FSMDEF_MRTONE_RESUME_BOTH_TONES:
                        default:
                            DEF_DEBUG(DEB_F_PREFIX"Invalid action request... tone:%d monrec_tone_action:%d",
                                      DEB_F_PREFIX_ARGS("RCC", fname), tone, dcb->monrec_tone_action);
                            break;
                    }
                    dcb->recorder_tone_direction = direction;
                    break;

                default:
                    break;
        } 

        LSM_DEBUG(DEB_L_C_F_PREFIX"Start request for tone: %d. Set monrec_tone_action: %d",
                  DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, call_id, fname),
			      tone, dcb->monrec_tone_action);

    } 
}













void
lsm_downgrade_monrec_tone_action (vcm_tones_t tone, callid_t call_id)
{
    static const char fname[] = "lsm_downgrade_monrec_tone_action";
    fsmdef_dcb_t *dcb;

    dcb = fsmdef_get_dcb_by_call_id(call_id);

    

    if (dcb != NULL) {
        switch (tone){
            case VCM_MONITORWARNING_TONE:
                switch (dcb->monrec_tone_action) {
                    case FSMDEF_MRTONE_PLAYED_MONITOR_TONE:
                    case FSMDEF_MRTONE_RESUME_MONITOR_TONE:
                        dcb->monrec_tone_action = FSMDEF_MRTONE_NO_ACTION;
                        break;

                    case FSMDEF_MRTONE_RESUME_BOTH_TONES:
                        dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_RECORDER_TONE;
                        break;

                    case FSMDEF_MRTONE_PLAYED_BOTH_TONES:
                        dcb->monrec_tone_action = FSMDEF_MRTONE_PLAYED_RECORDER_TONE;
                        break;

                    case FSMDEF_MRTONE_NO_ACTION:
                    case FSMDEF_MRTONE_PLAYED_RECORDER_TONE:
                    case FSMDEF_MRTONE_RESUME_RECORDER_TONE:
                    default:
                        DEF_DEBUG(DEB_F_PREFIX"Invalid action request... tone:%d monrec_tone_action:%d",
                                  DEB_F_PREFIX_ARGS("RCC", fname), tone, dcb->monrec_tone_action);
                        break;
                }
                dcb->monitor_tone_direction = VCM_PLAY_TONE_TO_EAR;
                break;

            case VCM_RECORDERWARNING_TONE:
                switch (dcb->monrec_tone_action) {
                    case FSMDEF_MRTONE_PLAYED_RECORDER_TONE:
                    case FSMDEF_MRTONE_RESUME_RECORDER_TONE:
                        dcb->monrec_tone_action = FSMDEF_MRTONE_NO_ACTION;
                        break;

                    case FSMDEF_MRTONE_RESUME_BOTH_TONES:
                        dcb->monrec_tone_action = FSMDEF_MRTONE_RESUME_MONITOR_TONE;
                        break;

                    case FSMDEF_MRTONE_PLAYED_BOTH_TONES:
                        dcb->monrec_tone_action = FSMDEF_MRTONE_PLAYED_MONITOR_TONE;
                        break;

                    case FSMDEF_MRTONE_NO_ACTION:
                    case FSMDEF_MRTONE_PLAYED_MONITOR_TONE:
                    case FSMDEF_MRTONE_RESUME_MONITOR_TONE:
                    default:
                        DEF_DEBUG(DEB_F_PREFIX"Invalid action request... tone:%d monrec_tone_action:%d",
                                  DEB_F_PREFIX_ARGS("RCC", fname), tone, dcb->monrec_tone_action);
                        break;
                }
                dcb->recorder_tone_direction = VCM_PLAY_TONE_TO_EAR;
                break;

            default:
                break;
        } 

        LSM_DEBUG(DEB_L_C_F_PREFIX"Stop request for tone: %d Downgrade monrec_tone_action: %d",
                  DEB_L_C_F_PREFIX_ARGS(LSM, dcb->line, call_id, fname),
			      tone, dcb->monrec_tone_action);
    } 
}













void
lsm_set_hold_ringback_status(callid_t call_id, boolean ringback_status)
{
    lsm_lcb_t      *lcb;

    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if (lcb->call_id == call_id) {
            LSM_DEBUG(DEB_F_PREFIX"Setting ringback to %d for lcb %d",
                      DEB_F_PREFIX_ARGS(LSM, "lsm_set_hold_ringback_status"),  ringback_status, call_id);
            lcb->enable_ringback = ringback_status;
            break;
        }
    }
}

void lsm_play_tone (cc_features_t feature_id)
{
    int play_tone;

    switch (feature_id) {
    case CC_FEATURE_BLF_ALERT_TONE:
        if (lsm_find_state(LSM_S_RINGIN) > CC_NO_CALL_ID) {
            
            return;
        }

        if (!lsm_callwaiting()) {
            config_get_value(CFGID_BLF_ALERT_TONE_IDLE, &play_tone, sizeof(play_tone));
            if (play_tone == 0) {
                return;
            }
            lsm_util_tone_start_with_speaker_as_backup(VCM_CALL_WAITING_TONE, VCM_ALERT_INFO_OFF,
                                                  CC_NO_CALL_ID, CC_NO_GROUP_ID,
                                                  CC_NO_MEDIA_REF_ID, VCM_PLAY_TONE_TO_EAR);
        } else {
            config_get_value(CFGID_BLF_ALERT_TONE_BUSY, &play_tone, sizeof(play_tone));
            if (play_tone == 0) {
                return;
            }
            lsm_util_tone_start_with_speaker_as_backup(VCM_CALL_WAITING_TONE, VCM_ALERT_INFO_OFF,
                                                  CC_NO_CALL_ID, CC_NO_GROUP_ID,
                                                  CC_NO_MEDIA_REF_ID, VCM_PLAY_TONE_TO_EAR);
        }
        break;

    default:
        break;
    }
}

















static void
lsm_update_inalert_status (line_t line, callid_t call_id,
                           cc_state_data_alerting_t * data,
                           boolean notify)
{
    static const char fname[] = "lsm_update_inalert_status";
    char disp_str[LSM_DISPLAY_STR_LEN];

    
    sstrncpy(disp_str, platform_get_phrase_index_str(UI_FROM),
             sizeof(disp_str));

    LSM_DEBUG(DEB_L_C_F_PREFIX"+++ calling number = %s",
			  DEB_L_C_F_PREFIX_ARGS(LSM, line, call_id, fname),
              data->caller_id.calling_number);

    
    
    if ((data->caller_id.calling_number) &&
        (data->caller_id.calling_number[0] != '\0') &&
        data->caller_id.display_calling_number) {

        sstrncat(disp_str, data->caller_id.calling_number,
                sizeof(disp_str) - strlen(disp_str));
    } else {
        sstrncat(disp_str, platform_get_phrase_index_str(UI_UNKNOWN),
                sizeof(disp_str) - strlen(disp_str));
    }

    
    
    
    
    
    if (notify == TRUE) {
        ui_set_notification(line, call_id,
                            (char *)disp_str, (unsigned long)CALL_ALERT_TIMEOUT,
                            FALSE, FROM_NOTIFY_PRI);
    }
    
    lsm_ui_display_status((char *)disp_str, line, call_id);

    return;
}












void
lsm_set_cfwd_all_nonccm (line_t line, char *callfwd_dialstring)
{
    
    ui_cfwd_status(line, TRUE, callfwd_dialstring, TRUE);
}












void
lsm_set_cfwd_all_ccm (line_t line, char *callfwd_dialstring)
{
    
    cfwdall_state_in_ccm_mode[line] = TRUE;

    
    ui_cfwd_status((line_t)line, TRUE, callfwd_dialstring, FALSE);
}









void
lsm_clear_cfwd_all_nonccm (line_t line)
{
    
    ui_cfwd_status(line, FALSE, "", TRUE);
}












void
lsm_clear_cfwd_all_ccm (line_t line)
{
    
    cfwdall_state_in_ccm_mode[line] = FALSE;

    
    ui_cfwd_status((line_t)line, FALSE, "", FALSE);
}











int
lsm_check_cfwd_all_nonccm (line_t line)
{
    char cfg_cfwd_url[MAX_URL_LENGTH];

    cfg_cfwd_url[0] = '\0';

    
    config_get_string(CFGID_LINE_CFWDALL+line-1, cfg_cfwd_url, MAX_URL_LENGTH);

    
    if (cfg_cfwd_url[0]) {
        return ((int) TRUE);
    } else {
        return ((int) FALSE);
    }
}











int
lsm_check_cfwd_all_ccm (line_t line)
{
    return ((int) cfwdall_state_in_ccm_mode[line]);
}


















char *
lsm_is_phone_forwarded (line_t line)
{
    static const char fname[] = "lsm_is_phone_forwarded";
    char     proxy_ipaddr_str[MAX_IPADDR_STR_LEN];
    int      port_number = 5060; 
    char    *domain = NULL;
    char    *port = NULL;
    cpr_ip_addr_t proxy_ipaddr;


    LSM_DEBUG(DEB_F_PREFIX"called", DEB_F_PREFIX_ARGS(LSM, fname));

    
    
    if (sip_regmgr_get_cc_mode(TEL_CCB_START) == REG_MODE_CCM) {
        return (NULL);
    }
    

    config_get_string(CFGID_LINE_CFWDALL+line-1, cfwdall_url, sizeof(cfwdall_url));

    if (cfwdall_url[0]) {
        
        domain = strchr(cfwdall_url, '@');
        if (!domain) {
            (void) sipTransportGetServerAddress(&proxy_ipaddr,
                                        1, TEL_CCB_START);
            if (proxy_ipaddr.type != CPR_IP_ADDR_INVALID) {
                ipaddr2dotted(proxy_ipaddr_str, &proxy_ipaddr);
                port_number = sipTransportGetServerPort(1, TEL_CCB_START);
            }
        } else {
            port = strchr(domain + 1, ':');
        }

        
        if (domain == NULL) {
            



            snprintf(cfwdall_url + strlen(cfwdall_url),
                     MAX_URL_LENGTH - strlen(cfwdall_url),
                     "@%s:%d", proxy_ipaddr_str, port_number);
        } else if (port == NULL) {
            



            if (!str2ip((const char *) domain + 1, &proxy_ipaddr)) {
                port_number = sipTransportGetServerPort(1, TEL_CCB_START);
                snprintf(cfwdall_url + strlen(cfwdall_url),
                         MAX_URL_LENGTH - strlen(cfwdall_url),
                         ":%d", port_number);
            }
        } else {
            



            memcpy(proxy_ipaddr_str, domain + 1, (port - domain - 1));
            *(proxy_ipaddr_str + (port - domain - 1)) = '\0';
            if (str2ip((const char *) proxy_ipaddr_str, &proxy_ipaddr) != 0) {
                *port = '\0';
            }
        }
        return ((char *)cfwdall_url);
    } else {
        return ((char *)NULL);
    }
}












callid_t
lsm_get_callid_from_ui_id (callid_t uid)
{
    lsm_lcb_t *lcb;
    FSM_FOR_ALL_CBS(lcb, lsm_lcbs, LSM_MAX_LCBS) {
        if (lcb->ui_id == uid) {
            return lcb->call_id;
        }
    }
    return (CC_NO_CALL_ID);
}













callid_t
lsm_get_ui_id (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        return (lcb->ui_id);
    }
    return (CC_NO_CALL_ID);
}














cc_call_handle_t
lsm_get_ms_ui_call_handle (line_t line, callid_t call_id, callid_t ui_id)
{
    callid_t lsm_ui_id;

    if (ui_id != CC_NO_CALL_ID) {
        return CREATE_CALL_HANDLE(line, ui_id);
    }

    
    lsm_ui_id = lsm_get_ui_id(call_id);

    if (lsm_ui_id != CC_NO_CALL_ID) {
        return CREATE_CALL_HANDLE(line, lsm_ui_id);
    }

    return CREATE_CALL_HANDLE(line, call_id);
}












void
lsm_set_ui_id (callid_t call_id, callid_t ui_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        lcb->ui_id = ui_id;
    }
}

char *
lsm_get_gdialed_digits (void)
{
    return (dp_get_gdialed_digits());
}














void lsm_update_video_avail (line_t line, callid_t call_id, int dir)
{
    static const char fname[] = "lsm_update_video_avail";
    fsmdef_dcb_t   *dcb;
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        dcb = lcb->dcb;
        if (dcb == NULL) {
            LSM_ERR_MSG(get_debug_string(DEBUG_INPUT_NULL), fname);
            return;
        }

        dir &= ~CC_ATTRIB_CAST;


        ui_update_video_avail (line, lcb->ui_id, dir);

        lsm_update_dscp_value(dcb);
    }
}














void lsm_update_video_offered (line_t line, callid_t call_id, int dir)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
       ui_update_video_offered (line, lcb->ui_id, dir);
    }
}














void lsm_set_video_mute (callid_t call_id, int mute)
{
    lsm_lcb_t *lcb;
    callid_t cid = lsm_get_callid_from_ui_id(call_id); 

    lcb = lsm_get_lcb_by_call_id(cid);
    if (lcb != NULL) {
       lcb->vid_mute = mute;
    }
}













int lsm_get_video_mute (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
       return lcb->vid_mute;
    }
    return (-1);
}


















void lsm_set_video_window (callid_t call_id, int flags, int x, int y, int h, int w)
{
    lsm_lcb_t *lcb;
    callid_t cid = lsm_get_callid_from_ui_id(call_id); 

    lcb = lsm_get_lcb_by_call_id(cid);
    if (lcb != NULL) {
       lcb->vid_flags = flags;
       lcb->vid_x = x;
       lcb->vid_y = y;
       lcb->vid_h = h;
       lcb->vid_w = w;
    }
}

















void lsm_get_video_window (callid_t call_id, int *flags, int *x, int *y, int *h, int *w)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb != NULL) {
        *flags = lcb->vid_flags;
        *x = lcb->vid_x;
        *y = lcb->vid_y;
        *h = lcb->vid_h;
        *w = lcb->vid_w;
    }
}












boolean lsm_is_kpml_subscribed (callid_t call_id)
{
    lsm_lcb_t *lcb;

    lcb = lsm_get_lcb_by_call_id(call_id);
    if (lcb == NULL) {
        return FALSE;
    }
    return kpml_is_subscribed(call_id, lcb->line);
}




static void lsm_util_start_tone(vcm_tones_t tone, short alert_info,
        cc_call_handle_t call_handle, groupid_t group_id,
        streamid_t stream_id, uint16_t direction) {

	int               sdpmode = 0;
    static const char fname[] = "lsm_util_start_tone";
    line_t line = GET_LINE_ID(call_handle);
    callid_t call_id = GET_CALL_ID(call_handle);
    DEF_DEBUG(DEB_F_PREFIX"Enter, line=%d, call_id=%d.",
              DEB_F_PREFIX_ARGS(MED_API, fname), line, call_id);

    sdpmode = 0;
    config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));
    if (!sdpmode) {
        vcmToneStart(tone, alert_info, call_handle, group_id, stream_id, direction);
	}
    







    switch (tone) {
    case VCM_MSG_WAITING_TONE:
        lsm_start_multipart_tone_timer(tone, MSG_WAITING_DELAY, call_id);
        break;

    case VCM_HOLD_TONE:
        lsm_start_continuous_tone_timer(tone, TOH_DELAY, call_id);
        break;

    default:
        break;
    }

    



    lsm_update_active_tone(tone, call_id);
}












void
lsm_util_tone_start_with_speaker_as_backup (vcm_tones_t tone, short alert_info,
                                    cc_call_handle_t call_handle, groupid_t group_id,
                                    streamid_t stream_id, uint16_t direction) {
    static const char *fname = "lsm_util_tone_start_with_speaker_as_backup";
    line_t line = GET_LINE_ID(call_handle);
    callid_t call_id = GET_CALL_ID(call_handle);
    DEF_DEBUG(DEB_L_C_F_PREFIX"tone=%-2d: direction=%-2d",
              DEB_L_C_F_PREFIX_ARGS(MED_API, line, call_id, fname),
              tone, direction);

    
    vcmToneStart(tone, alert_info, call_handle, group_id, stream_id, direction);

    







    switch (tone) {
    case VCM_MSG_WAITING_TONE:
        lsm_start_multipart_tone_timer(tone, MSG_WAITING_DELAY, call_id);
        break;

    case VCM_HOLD_TONE:
        lsm_start_continuous_tone_timer(tone, TOH_DELAY, call_id);
        break;

    default:
        break;
    }

    



    lsm_update_active_tone(tone, call_id);

}
