



#include "mozilla/Assertions.h"
#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "phone.h"
#include "fim.h"
#include "lsm.h"
#include "fsm.h"
#include "sm.h"
#include "ccapi.h"
#include "debug.h"
#include "phone_debug.h"
#include "util_string.h"
#include "sdp.h"
#include "gsm_sdp.h"
#include "ccsip_sdp.h"
#include "platform_api.h"

extern void set_next_sess_video_pref(int pref);
extern sm_rcs_t dcsm_process_event(void *event, int event_id);

#define FIM_MAX_CHNS (LSM_MAX_CALLS)
#define FIM_MAX_SCBS (FSM_TYPE_MAX)
#define FIM_MAX_ICBS (FSM_TYPE_MAX * FIM_MAX_CHNS)

static fim_scb_t *fim_scbs;
static fim_icb_t *fim_icbs;

static const char* logTag = "fim.c";

static void
fim_mwi (cc_mwi_t *msg)
{
    cc_action_data_t data;

    data.mwi.on = msg->msgSummary.on;
    data.mwi.type = msg->msgSummary.type;
    data.mwi.newCount = msg->msgSummary.newCount;
    data.mwi.oldCount = msg->msgSummary.oldCount;
    data.mwi.hpNewCount = msg->msgSummary.hpNewCount;
    data.mwi.hpOldCount = msg->msgSummary.hpOldCount;

    (void)cc_call_action(msg->call_id, msg->line, CC_ACTION_MWI, &data);
}


static void
fim_init_call_chns (void)
{
    int          chn;
    fsm_types_t  type;
    fim_icb_t   *icb = NULL;
    static const char fname[] = "fim_init_call_chns";

    fim_scbs = (fim_scb_t *) cpr_calloc(FIM_MAX_SCBS, sizeof(fim_scb_t));
    if (fim_scbs == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Failed to allocate FIM SCBs.", fname);
        return;
    }

    fim_icbs = (fim_icb_t *) cpr_calloc(FIM_MAX_ICBS, sizeof(fim_icb_t));
    if (fim_icbs == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Failed to allocate FIM ICBs.", fname);
        cpr_free(fim_scbs);
        fim_scbs = NULL;
        return;
    }

    


    icb = fim_icbs;
    for (chn = 0; chn < FIM_MAX_CHNS; chn++) {
        for (type = FSM_TYPE_HEAD; type < FSM_TYPE_MAX; type++, icb++) {
            icb->call_id = CC_NO_CALL_ID;
            icb->scb     = &(fim_scbs[type]);
            icb->cb      = NULL;

            



            if ((type == FSM_TYPE_HEAD) && (chn < (FIM_MAX_CHNS - 1))) {
                icb->next_chn = icb + FSM_TYPE_MAX;
            } else {
                icb->next_chn = NULL;
            }

            



            if (type < (FSM_TYPE_MAX - 1)) {
                icb->next_icb = icb + 1;
            } else {
                icb->next_icb = NULL;
            }
        }                       
    }                           

    


    icb = fim_icbs;
    for (type = FSM_TYPE_HEAD; type < FSM_TYPE_MAX; type++, icb++) {
        icb->scb->type = type;
        fsm_init_scb(icb, CC_NO_CALL_ID);
    }
}

static void
fim_free_call_chn (fim_icb_t *call_chn, line_t line, boolean update_call_cnt)
{
    static const char fname[] = "fim_free_call_chn";
    fim_icb_t *icb = call_chn;

    FIM_DEBUG(get_debug_string(GSM_DBG_PTR), "FIM", call_chn->call_id, fname,
              "call_chn", call_chn);

    


    for (icb = call_chn; icb != NULL; icb = icb->next_icb) {
        if (icb->scb->free_cb != NULL) {
            icb->scb->free_cb(icb, icb->call_id);
        }
        icb->call_id = CC_NO_CALL_ID;
        icb->cb = NULL;
    }
    if (update_call_cnt == TRUE) {
        lsm_decrement_call_chn_cnt(line);
    }
    else {
        FIM_DEBUG(get_debug_string(GSM_DBG_PTR), "lsm not decremented", call_chn->call_id, fname,
              "call_chn", call_chn);
    }
}


fim_icb_t *
fim_get_call_chn_by_call_id (callid_t call_id)
{
    static const char fname[] = "fim_get_call_chn_by_call_id";
    fim_icb_t      *call_chn = NULL;
    fim_icb_t      *icb = NULL;

    for (icb = fim_icbs; icb != NULL; icb = icb->next_chn) {
        if (icb->call_id == call_id) {
            call_chn = icb;
            break;
        }
    }

    FIM_DEBUG(get_debug_string(GSM_DBG_PTR), "FIM", call_id, fname, "chn",
              call_chn);

    return call_chn;
}

fim_icb_t *
fim_get_new_call_chn (callid_t call_id)
{
    static const char fname[] = "fim_get_new_call_chn";
    fim_icb_t      *call_chn = NULL;
    fim_icb_t      *icb = NULL;

    


    call_chn = fim_get_call_chn_by_call_id(call_id);
    if (call_chn != NULL) {
        FIM_DEBUG(get_debug_string(GSM_DBG1), "FIM", call_id, fname,
                  "call_id in use");

        return NULL;
    }

    


    call_chn = fim_get_call_chn_by_call_id(CC_NO_CALL_ID);
    if (call_chn == NULL) {
        


        FIM_DEBUG(get_debug_string(GSM_DBG1), "FIM", call_id, fname,
                  "no free call_chns");

        return NULL;
    }

    call_chn->call_id = call_id;
    call_chn->ui_locked = FALSE;

    


    for (icb = call_chn; icb != NULL; icb = icb->next_icb) {
        FIM_DEBUG(get_debug_string(GSM_DBG1), "FIM", call_id, fname,
                  fsm_type_name(icb->scb->type));

        if (icb->scb->get_cb) {
            icb->scb->get_cb(icb, call_id);
            if (!icb->cb) {
              CSFLogError(logTag, "%s - unable to get control block for call %d", __FUNCTION__, call_id);
              fim_free_call_chn(call_chn, 0, FALSE);
              return NULL;
            }
            icb->call_id = call_id;
            icb->ui_locked = FALSE;
        }
    }

    FIM_DEBUG(get_debug_string(GSM_DBG_PTR), "FIM", call_chn->call_id, fname,
              "call_chn", call_chn);

    return call_chn;
}

void
fim_free_event (void *data)
{
    cc_msg_t *msg = (cc_msg_t *) data;

    
    cc_free_msg_data(msg);
}

static void
fim_process_options_msg (void *data)
{
    static const char fname[] = "fim_process_options_msg";
    cc_sdp_t             *local_sdp_p = NULL;
    cc_causes_t           cause = CC_CAUSE_MIN;
    cc_msgbody_info_t     msg_body;
    cc_options_sdp_req_t *options_msg = (cc_options_sdp_req_t *) data;

    gsmsdp_create_options_sdp(&local_sdp_p);

    cause = gsmsdp_encode_sdp(local_sdp_p, &msg_body);
    if (cause != CC_CAUSE_OK) {
        FIM_DEBUG(get_debug_string(GSM_DBG1), "FIM", options_msg->call_id,
                  fname, "Unable to build SDP\n");
    } else {
        cc_int_options_sdp_ack(CC_SRC_GSM, CC_SRC_SIP,
                               options_msg->call_id,
                               options_msg->line,
                               options_msg->pMessage, &msg_body);
    }
    sipsdp_src_dest_free(CCSIP_SRC_SDP_BIT, &local_sdp_p);
}

void
fim_lock_ui (callid_t call_id)
{
    fim_icb_t *call_chn = fim_get_call_chn_by_call_id(call_id);

    if (call_chn == NULL) {
        FIM_DEBUG(DEB_F_PREFIX"unknown call id", DEB_F_PREFIX_ARGS(FIM, "fim_lock_ui"));
        return;
    }
    call_chn->ui_locked = TRUE;
}

void
fim_unlock_ui (callid_t call_id)
{
    fim_icb_t *call_chn = fim_get_call_chn_by_call_id(call_id);

    if (call_chn == NULL) {
        FIM_DEBUG(DEB_F_PREFIX"unknown call id", DEB_F_PREFIX_ARGS(FIM, "fim_unlock_ui"));
        return;
    }
    call_chn->ui_locked = FALSE;
}

static boolean
fsm_event_filtered_by_ui_lock (int event_id, cc_features_t feature_id)
{
    





    if (event_id == CC_MSG_ONHOOK) {
        return FALSE;
    }

    if ((event_id == CC_MSG_FEATURE) &&
        (feature_id == CC_FEATURE_END_CALL ||
         feature_id == CC_FEATURE_REQ_PEND_TIMER_EXP ||
         feature_id == CC_FEATURE_RESUME ||
         feature_id == CC_FEATURE_HOLD)) {
        return FALSE;
    }

    return TRUE;
}











boolean fim_is_app_generic_features (cc_features_t feat_id)
{
    return(FALSE);
}












static boolean
fim_check_feature_event (cc_setup_t *msg)
{

    if ((((cc_feature_t *)msg)->feature_id == CC_FEATURE_SELECT) ||
        (((cc_feature_t *)msg)->feature_id == CC_FEATURE_DIRTRXFR) ||
        (((cc_feature_t *)msg)->feature_id == CC_FEATURE_B2BCONF) ||
        (((cc_feature_t *)msg)->feature_id == CC_FEATURE_CFWD_ALL)) {
        return (TRUE);
    }
    return (FALSE);
}
















static boolean
fim_feature_need_outgoing_call_context (cc_setup_t *msg)
{
    if (((cc_feature_t *)msg)->feature_id == CC_FEATURE_NOTIFY) {
        


        return (FALSE);
    }
    return (TRUE);
}

















boolean
fim_process_event (void *data, boolean cac_passed)
{
    static const char fname[] = "fim_process_event";
    cc_setup_t     *msg = (cc_setup_t *) data;
    cc_msgs_t       msg_id   = msg->msg_id;
    callid_t        call_id  = msg->call_id;
    line_t          line     = msg->line;
    int             event_id = msg_id;
    sm_event_t      event;
    fim_icb_t      *call_chn = NULL;
    fim_icb_t      *icb      = NULL;
    boolean         done     = FALSE;
    sm_rcs_t        rc       = SM_RC_ERROR;
    fim_cb_hdr_t   *cb_hdr   = NULL;
    fsm_fcb_t      *fcb      = NULL;
    cc_feature_t   *feat_msg = (cc_feature_t *) data;
    cc_feature_data_t * feat_data = &(feat_msg->data);
    boolean        update_call_cnt = TRUE;
    uint32_t       no_of_session = 1;
    callid_t       bw_call_id;

    FIM_DEBUG(DEB_L_C_F_PREFIX"Msg name = %s", DEB_L_C_F_PREFIX_ARGS(FIM, line, call_id, fname),
        cc_msg_name(msg_id));

    


    if ((event_id <= CC_MSG_MIN) || (event_id >= CC_MSG_MAX)) {
        cc_call_state(call_id, line, CC_STATE_UNKNOWN, NULL);
        return(TRUE);
    }

    


    if (dcsm_process_event(data, event_id) == SM_RC_END) {
        
        return(FALSE);
    }

    



    if (msg_id == CC_MSG_MWI) {
        fim_mwi((cc_mwi_t *) msg);
    }

    if (event_id == CC_MSG_OPTIONS) {
        fim_process_options_msg(data);
        return(TRUE);
    }


    if (platWlanISActive() && cac_passed == FALSE) {
        




        if ((msg->src_id != CC_SRC_GSM) &&
        ((event_id == CC_MSG_SETUP)      ||
        (event_id == CC_MSG_OFFHOOK)    ||
        (event_id == CC_MSG_DIALSTRING) ||
        (event_id == CC_MSG_LINE)       ||
        ((event_id == CC_MSG_FEATURE) &&
        ((((cc_feature_t *) msg)->feature_id == CC_FEATURE_NEW_CALL))))) {

                bw_call_id = call_id;

                if ((event_id == CC_MSG_SETUP) &&
                        ((((cc_setup_t *)msg)->call_info.type == CC_FEAT_MONITOR))) {
                    no_of_session = 2;
                    bw_call_id = msg->call_info.data.join.join_call_id;
                }

                if (fsm_cac_call_bandwidth_req (bw_call_id, no_of_session, msg) != CC_CAUSE_OK) {
                return(TRUE);
            }
        

        return(FALSE);
        }
    }

    if ((event_id == CC_MSG_FEATURE) && ((call_id == CC_NO_CALL_ID) ||
        fim_is_app_generic_features(((cc_feature_t *) msg)->feature_id))) {
        if (((cc_feature_t *)msg)->feature_id == CC_FEATURE_BLF_ALERT_TONE) {
            (void)cc_call_action(0, 0, CC_ACTION_PLAY_BLF_ALERTING_TONE, NULL);
        } else if (((cc_feature_t *)msg)->feature_id == CC_FEATURE_UPD_MEDIA_CAP) {
            fsmdef_update_media_cap_feature_event(feat_msg);
        }
        return(TRUE);
    }

    







    if (call_id < 1) {
        return(TRUE);
    }


    


    call_chn = fim_get_call_chn_by_call_id(call_id);

    if (call_chn == NULL) {
        



        if ((event_id == CC_MSG_SETUP)      ||
            (event_id == CC_MSG_OFFHOOK)    ||
            (event_id == CC_MSG_DIALSTRING) ||
            (event_id == CC_MSG_LINE)       ||
            ((event_id == CC_MSG_FEATURE) &&
             ((((cc_feature_t *) msg)->feature_id == CC_FEATURE_NEW_CALL)))) {
            call_chn = fim_get_new_call_chn(call_id);

            


            if (call_chn == NULL) {
                FIM_DEBUG(get_debug_string(GSM_DBG1), "FIM", call_id, fname,
                          "no call_chn");

                fsm_display_no_free_lines();

                cc_call_state(call_id, line, CC_STATE_UNKNOWN, NULL);
                return(TRUE);
            }

        } else if ((event_id == CC_MSG_FEATURE) &&
                   (fim_check_feature_event(msg))) {

            call_chn = fim_get_new_call_chn(call_id);

            


            if (call_chn == NULL) {
                FIM_DEBUG(get_debug_string(GSM_DBG1), "FIM", call_id, fname,
                          "no call_chn");

                fsm_display_no_free_lines();

                cc_call_state(call_id, line, CC_STATE_UNKNOWN, NULL);
                return(TRUE);
            }

            if (fim_feature_need_outgoing_call_context(msg)) {
                
                if (fsm_get_new_outgoing_call_context(call_id, line,
                        fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF),
                        FALSE) != CC_CAUSE_OK) {
                    cc_call_state(call_id, line, CC_STATE_UNKNOWN, NULL);
                    return(TRUE);
                }
            }

        } else {
            FIM_DEBUG(get_debug_string(GSM_DBG1), "FIM", call_id, fname,
                      "not a call establishment event\n");
            if( feat_msg->feature_id == CC_FEATURE_UPD_SESSION_MEDIA_CAP) {
                FIM_DEBUG(DEB_L_C_F_PREFIX"set_next_sess_video_pref = %d",
                        DEB_L_C_F_PREFIX_ARGS(FIM, line, call_id, fname),
                        feat_data->caps.support_direction);
                set_next_sess_video_pref(feat_data->caps.support_direction);
            }
            return(TRUE);
        }
        if (event_id != CC_MSG_SETUP) {
            





           lsm_increment_call_chn_cnt(line);
        }
    } 


    


    if (call_chn->ui_locked && ((cc_feature_t *) msg)->src_id == CC_SRC_UI) {
        if (fsm_event_filtered_by_ui_lock(event_id,
                ((cc_feature_t *)msg)->feature_id)) {
            FIM_DEBUG(DEB_L_C_F_PREFIX" %s filtered by UI lock",
                      DEB_L_C_F_PREFIX_ARGS(FIM, line, call_id, fname),
                      cc_feature_name(((cc_feature_t *)msg)->feature_id));
            return(TRUE);
        }
    }

    


    icb = call_chn->next_icb;
    MOZ_ASSERT(icb);
    while (icb && !done) {
        


        cb_hdr = (fim_cb_hdr_t *) (icb->cb);

        MOZ_ASSERT(cb_hdr);
        if (!cb_hdr) {
            done = TRUE;
            break;
        }

        event.data  = cb_hdr;
        event.state = cb_hdr->state;
        event.event = event_id;
        event.msg   = data;

        fcb = (fsm_fcb_t *) icb->cb;

        








        if ((fcb->fsm_type == FSM_TYPE_DEF) && (event_id == CC_MSG_RELEASE)) {
            if (fcb->dcb != NULL) {
                line = fcb->dcb->line;
            }
        }

        


        update_call_cnt = TRUE; 
        if (fcb->dcb != NULL) {
            update_call_cnt = (fcb->dcb->call_not_counted_in_mnc_bt) ? FALSE: TRUE;
        }

        FIM_DEBUG(DEB_L_C_F_PREFIX" %s(%s:%s)",
                  DEB_L_C_F_PREFIX_ARGS(FIM, line, call_id, fname), fsm_type_name(icb->scb->type),
                  fsm_state_name(fcb->fsm_type, event.state),
                  cc_msg_name((cc_msgs_t) (event.event)));

        rc = sm_process_event(icb->scb->sm, &event);


        switch (rc) {
        case SM_RC_CONT:
        case SM_RC_DEF_CONT:
            icb = icb->next_icb;
            break;

        case SM_RC_END:
            done = TRUE;
            break;

        case SM_RC_ERROR:
            FIM_DEBUG(DEB_L_C_F_PREFIX" fsm sm error(%d:%d)",
                      DEB_L_C_F_PREFIX_ARGS(FIM, line, call_id, fname), event.state, event.event);
            done = TRUE;
            cc_call_state(call_id, line, CC_STATE_UNKNOWN, NULL);
            break;

        case SM_RC_CLEANUP:
            done = TRUE;
            cc_call_state(call_id, line, CC_STATE_UNKNOWN, NULL);
            break;

        default:
            done = TRUE;
            cc_call_state(call_id, line, CC_STATE_UNKNOWN, NULL);
            break;
        }                       

        if ((rc == SM_RC_END) && (fcb->fsm_type == FSM_TYPE_DEF) &&
            (event_id == CC_MSG_FEATURE))
        {
            if ( ((cc_feature_t *) msg)->feature_id == CC_FEATURE_CFWD_ALL){
                lsm_decrement_call_chn_cnt(line);
            }
        }

        if (icb == NULL) {
            done = TRUE;
            FIM_DEBUG("icb is null and get here!");
        }

    }                           

    if (rc == SM_RC_CLEANUP) {
        fim_free_call_chn(call_chn, line, update_call_cnt);

    }

    return(TRUE);
}


cc_int32_t
fim_show_cmd (cc_int32_t argc, const char *argv[])
{
    fim_icb_t      *icb = NULL;
    fim_scb_t      *scb = NULL;
    int             i = 0;

    


    if ((argc == 2) && (argv[1][0] == '?')) {
        debugif_printf("show fim\n");
    }

    


    debugif_printf("\n---------------------------------- FIM icbs -----------------------------------");
    debugif_printf("\ni   call_id  type    icb         next_chn    next_icb    cb          scb");
    debugif_printf("\n-------------------------------------------------------------------------------\n");

    FSM_FOR_ALL_CBS(icb, fim_icbs, FIM_MAX_ICBS) {
        debugif_printf("%-3d  %-7d  %-6s  0x%8p  0x%8p  0x%8p  0x%8p  0x%8p\n",
                       i++, icb->call_id, fsm_type_name(icb->scb->type),
                       icb, icb->next_chn, icb->next_icb, icb->cb, icb->scb);
    }

    


    i = 0;
    debugif_printf
        ("\n------------------------ FIM scbs ------------------------");
    debugif_printf("\ni   type    scb         sm          get_cb      free_cb");
    debugif_printf
        ("\n----------------------------------------------------------\n");

    FSM_FOR_ALL_CBS(scb, fim_scbs, FIM_MAX_SCBS) {
        debugif_printf("%-2d  %-6s  0x%8p  0x%8p  0x%8p  0x%8p\n",
                       i++, fsm_type_name(scb->type), scb,
                       scb->sm, scb->get_cb, scb->free_cb);
    }

    return (0);
}


void
fim_init (void)
{

    fim_init_call_chns();
}

void
fim_shutdown (void)
{
    cpr_free(fim_scbs);
    cpr_free(fim_icbs);
    fim_scbs = NULL;
    fim_icbs = NULL;
}
