



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "cpr_stdio.h"
#include "fsm.h"
#include "fim.h"
#include "lsm.h"
#include "sm.h"
#include "ccapi.h"
#include "phone_debug.h"
#include "text_strings.h"
#include "config.h"
#include "debug.h"
#include "gsm_sdp.h"
#include "regmgrapi.h"
#include "platform_api.h"

static fsmcnf_ccb_t *fsmcnf_ccbs;

static int softkey_mask_list[MAX_SOFT_KEYS];

#define FSMCNF_CONNECTED_SET "CONNECTED"

typedef enum {
    FSMCNF_S_MIN = -1,
    FSMCNF_S_IDLE,
    FSMCNF_S_CNFING,
    FSMCNF_S_CNFED,
    FSMCNF_S_MAX
} fsmcnf_states_t;

static const char *fsmcnf_state_names[] = {
    "IDLE",
    "CNFING",
    "CNFED"
};


static sm_rcs_t fsmcnf_ev_idle_setup(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_idle_feature(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_cnfing_release(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_cnfing_feature(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_cnfing_onhook(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_cnfed_release(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_cnfed_feature(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_cnfed_feature_ack(sm_event_t *event);
static sm_rcs_t fsmcnf_ev_cnfed_onhook(sm_event_t *event);

static sm_function_t fsmcnf_function_table[FSMCNF_S_MAX][CC_MSG_MAX] =
{

    {
     fsmcnf_ev_idle_setup,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     fsmcnf_ev_idle_feature,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL
    },


    {
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     fsmcnf_ev_cnfing_release,
     NULL,
     fsmcnf_ev_cnfing_feature,
     NULL,
     NULL,
     fsmcnf_ev_cnfing_onhook,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL
    },


    {
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     fsmcnf_ev_cnfed_release,
     NULL,
     fsmcnf_ev_cnfed_feature,
     fsmcnf_ev_cnfed_feature_ack,
     NULL,
     fsmcnf_ev_cnfed_onhook,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL
    }
};

static sm_table_t fsmcnf_sm_table;
sm_table_t *pfsmcnf_sm_table = &fsmcnf_sm_table;


const char *
fsmcnf_state_name (int state)
{
    if ((state <= FSMCNF_S_MIN) || (state >= FSMCNF_S_MAX)) {
        return (get_debug_string(GSM_UNDEFINED));
    }

    return (fsmcnf_state_names[state]);
}


static int
fsmcnf_get_new_cnf_id (void)
{
    static int cnf_id = 0;

    if (++cnf_id < 0) {
        cnf_id = 1;
    }

    return (cnf_id);
}


static void
fsmcnf_init_ccb (fsmcnf_ccb_t *ccb)
{
    if (ccb != NULL) {
        ccb->cnf_id      = FSM_NO_ID;
        ccb->cnf_call_id = CC_NO_CALL_ID;
        ccb->cns_call_id = CC_NO_CALL_ID;
        ccb->cnf_line    = CC_NO_LINE;
        ccb->cns_line    = CC_NO_LINE;
        ccb->bridged     = FALSE;
        ccb->active      = FALSE;
        ccb->flags       = 0;
        ccb->cnf_ftr_ack = FALSE;
    }
}


static fsmcnf_ccb_t *
fsmcnf_get_ccb_by_cnf_id (int cnf_id)
{
    fsmcnf_ccb_t *ccb;
    fsmcnf_ccb_t *ccb_found = NULL;

    FSM_FOR_ALL_CBS(ccb, fsmcnf_ccbs, FSMCNF_MAX_CCBS) {
        if (ccb->cnf_id == cnf_id) {
            ccb_found = ccb;

            break;
        }
    }

    return (ccb_found);
}















static fsmcnf_ccb_t *
fsmcnf_get_new_cnf_context (callid_t cnf_call_id)
{
    static const char fname[] = "fsmcnf_get_new_cnf_context";
    fsmcnf_ccb_t *ccb;

    ccb = fsmcnf_get_ccb_by_cnf_id(FSM_NO_ID);
    if (ccb != NULL) {
        ccb->cnf_id      = fsmcnf_get_new_cnf_id();
        ccb->cnf_call_id = cnf_call_id;
        ccb->cns_call_id = cc_get_new_call_id();

        FSM_DEBUG_SM(get_debug_string(FSMCNF_DBG_PTR), ccb->cnf_id,
                     ccb->cnf_call_id, ccb->cns_call_id, fname, ccb);
    } else {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Failed to get new ccb.\n", fname);
    }

    return (ccb);
}


fsmcnf_ccb_t *
fsmcnf_get_ccb_by_call_id (callid_t call_id)
{
    fsmcnf_ccb_t *ccb;
    fsmcnf_ccb_t *ccb_found = NULL;

    FSM_FOR_ALL_CBS(ccb, fsmcnf_ccbs, FSMCNF_MAX_CCBS) {
        if ((ccb->cnf_call_id == call_id) || (ccb->cns_call_id == call_id)) {
            ccb_found = ccb;

            break;
        }
    }

    return (ccb_found);
}


static void
fsmcnf_update_cnf_context (fsmcnf_ccb_t *ccb, callid_t old_call_id,
                           callid_t new_call_id)
{
    static const char fname[] = "fsmcnf_update_cnf_context";

    if (ccb != NULL) {
        if (old_call_id == ccb->cnf_call_id) {
            ccb->cnf_call_id = new_call_id;
        } else if (old_call_id == ccb->cns_call_id) {
            ccb->cns_call_id = new_call_id;
        }

        FSM_DEBUG_SM(get_debug_string(FSMCNF_DBG_PTR), ccb->cnf_id,
                     ccb->cnf_call_id, ccb->cns_call_id, fname, ccb);
    }
}


callid_t
fsmcnf_get_other_call_id (fsmcnf_ccb_t *ccb, callid_t call_id)
{
    callid_t other_call_id = CC_NO_CALL_ID;

    if (ccb != NULL) {
        if (ccb->cnf_call_id == call_id) {
            other_call_id = ccb->cns_call_id;
        } else if (ccb->cns_call_id == call_id) {
            other_call_id = ccb->cnf_call_id;
        }
    }

    return (other_call_id);
}

static void
fsmcnf_cnf_xfer (fsmcnf_ccb_t *ccb)
{
    fsmdef_dcb_t     *dcb;
    cc_feature_data_t ftr_data;

    dcb = fsm_get_dcb(ccb->cnf_call_id);

    


    ftr_data.xfer.cause          = CC_CAUSE_XFER_CNF;
    ftr_data.xfer.target_call_id = ccb->cns_call_id;
    cc_int_feature(CC_SRC_UI, CC_SRC_GSM, dcb->call_id,
                   dcb->line, CC_FEATURE_XFER, &(ftr_data));
}



















static void
fsmcnf_remove_fcb (fsm_fcb_t *fcb, callid_t call_id)
{
    fsmcnf_ccb_t *ccb = fcb->ccb;

    if (ccb != NULL) {
        fsmcnf_update_cnf_context(ccb, call_id, CC_NO_CALL_ID);

        


        if ((ccb->cnf_call_id == CC_NO_CALL_ID) &&
            (ccb->cns_call_id == CC_NO_CALL_ID)) {
            fsmcnf_init_ccb(ccb);
        }
    }
}


static void
fsmcnf_cleanup (fsm_fcb_t *fcb, int fname, boolean both)
{
    fsmcnf_ccb_t   *ccb;
    fsm_fcb_t      *other_fcb, *fcb_def;
    callid_t        call_id       = fcb->call_id;
    callid_t        other_call_id = CC_NO_CALL_ID;

    
    ccb = fsmcnf_get_ccb_by_call_id(call_id);
    other_call_id = fsmcnf_get_other_call_id(fcb->ccb, call_id);
    
    fcb_def = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);

    if (fcb->ccb && (call_id == fcb->ccb->cnf_call_id)) {

        if (other_call_id != CC_NO_CALL_ID) {
            




            if (ccb == NULL) {
                GSM_DEBUG_ERROR(GSM_F_PREFIX"Failed to get CCB.\n", fname);
            } else {
                cc_call_attribute(other_call_id, ccb->cnf_line, NORMAL_CALL);
            }
        }

    }

    if (fcb_def && fcb_def->dcb)  {
        fcb_def->dcb->session = PRIMARY;
    }
    






    if (both) {
        other_call_id = fsmcnf_get_other_call_id(fcb->ccb, call_id);
        if (other_call_id != CC_NO_CALL_ID) {
            other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_CNF);
            if (other_fcb != NULL) {
                fsmcnf_cleanup(other_fcb, fname, FALSE);
            }
        }
    }

    


    fsmcnf_remove_fcb(fcb, fcb->call_id);

    


    fsm_change_state(fcb, fname, FSMCNF_S_IDLE);

    



    fsm_init_fcb(fcb, fcb->call_id, fcb->dcb, FSM_TYPE_CNF);
}


void
fsmcnf_free_cb (fim_icb_t *icb, callid_t call_id)
{
    fsm_fcb_t *fcb = NULL;

    if (call_id != CC_NO_CALL_ID) {
        fcb = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_CNF);

        if (fcb != NULL) {
            fsmcnf_cleanup(fcb, __LINE__, FALSE);
            fsm_init_fcb(fcb, CC_NO_CALL_ID, FSMDEF_NO_DCB, FSM_TYPE_NONE);
        }
    }
}
















void
fsmcnf_feature_cancel (fsmcnf_ccb_t *ccb, line_t line, callid_t call_id,
                          callid_t target_call_id)
{
    cc_feature_data_t data;
    fsm_fcb_t         *fcb_def;

    fcb_def = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);

    
    
    
    if (
        (fcb_def != NULL) && ((fcb_def->dcb->selected == FALSE) &&
            ((fcb_def->state == FSMDEF_S_OUTGOING_ALERTING) ||
            ((fcb_def->state == FSMDEF_S_CONNECTED) &&
            (fcb_def->dcb->spoof_ringout_requested == TRUE) &&
            (fcb_def->dcb->spoof_ringout_applied == TRUE))))) {

        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, call_id,
                           line, CC_FEATURE_END_CALL, NULL);
    }

    fcb_def = fsm_get_fcb_by_call_id_and_type(target_call_id, FSM_TYPE_DEF);

    if (
        (fcb_def != NULL) && ((fcb_def->dcb->selected == FALSE) &&
            ((fcb_def->state == FSMDEF_S_OUTGOING_ALERTING) ||
            ((fcb_def->state == FSMDEF_S_CONNECTED) &&
            (fcb_def->dcb->spoof_ringout_requested == TRUE) &&
            (fcb_def->dcb->spoof_ringout_applied == TRUE))))) {

        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, target_call_id,
                           line, CC_FEATURE_END_CALL, NULL);
    }

    data.cancel.target_call_id = target_call_id;
    data.cancel.call_id = call_id;
    data.cancel.cause = CC_SK_EVT_TYPE_EXPLI;

    cc_int_feature(CC_SRC_GSM, CC_SRC_SIP, call_id,
                           line, CC_FEATURE_CANCEL, &data);
}






static sm_rcs_t
fsmcnf_ev_idle_setup (sm_event_t *event)
{
    fsm_fcb_t    *fcb     = (fsm_fcb_t *) event->data;
    cc_setup_t   *msg     = (cc_setup_t *) event->msg;
    callid_t     call_id  = msg->call_id;
    fsmcnf_ccb_t *ccb;

    if (!msg->replaces) {
        return (SM_RC_DEF_CONT);
    }

    





    ccb = fsmcnf_get_ccb_by_call_id(call_id);
    if (ccb == NULL) {
        return (SM_RC_DEF_CONT);
    }

    
    fcb->ccb = ccb;           
    fsm_change_state(fcb, __LINE__, FSMCNF_S_CNFING);

    return (SM_RC_CONT);
}


static sm_rcs_t
fsmcnf_ev_idle_feature (sm_event_t *event)
{
    static const char *fname  = "fsmcnf_ev_idle_feature";
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_feature_t     *msg     = (cc_feature_t *) event->msg;
    callid_t          call_id = msg->call_id;
    line_t            line    = msg->line;
    cc_srcs_t         src_id  = msg->src_id;
    cc_features_t     ftr_id  = msg->feature_id;
    fsmdef_dcb_t     *dcb     = fcb->dcb;
    callid_t          cns_call_id;
    sm_rcs_t          sm_rc   = SM_RC_CONT;
    fsmcnf_ccb_t     *ccb;
    int               free_lines;
    cc_feature_data_t data;
    cc_action_data_t  action_data;
    fsmdef_dcb_t     *other_dcb;
    fsm_fcb_t        *other_fcb;
    fsm_fcb_t        *fcb_def, *join_fcb_cnf;
    cc_feature_data_t ftr_data = msg->data;
    cc_feature_data_t *feat_data = &(msg->data);
    fsm_fcb_t        *cns_fcb;
    callid_t          other_call_id;
    fsmxfr_xcb_t     *xcb;
    cc_causes_t       cause;

    memset(&data, 0, sizeof(cc_feature_data_t));

    fsm_sm_ftr(ftr_id, src_id);

    switch (src_id) {
    case CC_SRC_UI:
        switch (ftr_id) {
        case CC_FEATURE_CONF:

            




            if (feat_data && msg->data_valid &&
                (feat_data->cnf.target_call_id != CC_NO_CALL_ID)
                && (cns_fcb = fsm_get_fcb_by_call_id_and_type(feat_data->cnf.target_call_id,
                            FSM_TYPE_CNF)) != NULL) {
                



                ccb = fsmcnf_get_new_cnf_context(feat_data->cnf.target_call_id);

                if (ccb == NULL || ccb->cnf_id == FSM_NO_ID) {
                    return(SM_RC_END);
                }

                ccb->cns_call_id = call_id;
                fcb->ccb = ccb;
                cns_fcb->ccb = ccb;
                ccb->cnf_line = line;
                ccb->cns_line = line;

                fsm_change_state(fcb, __LINE__, FSMCNF_S_CNFING);

                fsm_change_state(cns_fcb, __LINE__, FSMCNF_S_CNFING);

                cc_int_feature(CC_SRC_UI, CC_SRC_GSM, ccb->cns_call_id,
                                   cns_fcb->dcb->line, CC_FEATURE_CONF, NULL);
                return(SM_RC_END);

            }

            










            




            


            fcb_def = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);
            if ((fcb_def != NULL) && (fcb_def->state != FSMDEF_S_CONNECTED)) {
                break;
            }

            


            
            
            free_lines = lsm_get_instances_available_cnt(line, FALSE);
            if (free_lines <= 0) {
                


                fsm_display_no_free_lines();

                break;
            }

            



            ccb = fsmcnf_get_new_cnf_context(call_id);
            if (ccb == NULL || ccb->cnf_id == 0) {
                break;
            }
            fcb->ccb = ccb;
            ccb->cnf_line = line;
            ccb->cns_line = line;

            




            data.hold.call_info.type = CC_FEAT_HOLD;
            data.hold.call_info.data.hold_resume_reason = CC_REASON_CONF;
            data.hold.msg_body.num_parts = 0;
            cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id, dcb->line,
                           CC_FEATURE_HOLD, &data);

            


            data.newcall.cause = CC_CAUSE_CONF;
            cns_call_id = ccb->cns_call_id;
            cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, cns_call_id, line,
                           CC_FEATURE_NEW_CALL, &data);

            FSM_DEBUG_SM(get_debug_string(FSMCNF_DBG_CNF_INITIATED),
                         ccb->cnf_id, call_id, cns_call_id, __LINE__);

            fsm_change_state(fcb, __LINE__, FSMCNF_S_CNFING);

            sm_rc = SM_RC_END;

            break;

        case CC_FEATURE_JOIN:
            


            fcb_def = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);
            if ((fcb_def != NULL) && (fcb_def->state != FSMDEF_S_CONNECTED)) {
                break;
            }

            


            other_dcb = fsmdef_get_other_dcb_by_line(call_id, dcb->line);
            if (other_dcb == NULL) {
                break;
            }

            
            other_fcb = fsm_get_fcb_by_call_id_and_type(other_dcb->call_id,
                                                        FSM_TYPE_DEF);
            if (other_fcb == NULL) {
                break;
            }

            if (other_fcb->state == FSMDEF_S_HOLDING) {
                



                ccb = fsmcnf_get_new_cnf_context(call_id);
                if (ccb == NULL) {
                    break;
                }
                fcb->ccb = ccb;
                fsm_change_state(fcb, __LINE__, FSMCNF_S_CNFED);


                other_fcb = fsm_get_fcb_by_call_id_and_type(other_dcb->call_id,
                                                            FSM_TYPE_CNF);
                if (other_fcb == NULL) {
                    fsmcnf_cleanup(fcb, __LINE__, TRUE);
                    break;
                }
                other_fcb->ccb = ccb;
                fsm_change_state(other_fcb, __LINE__, FSMCNF_S_CNFED);

                ccb->cnf_call_id = dcb->call_id;
                ccb->cns_call_id = other_dcb->call_id;
                ccb->bridged = TRUE;

                
                cause = gsmsdp_encode_sdp_and_update_version(other_dcb,
                                                             &data.resume.msg_body);
                if (cause != CC_CAUSE_OK) {
                    FSM_DEBUG_SM(get_debug_string(FSM_DBG_SDP_BUILD_ERR));
                    sm_rc = SM_RC_END;
                    break;
                }
                data.resume.cause = CC_CAUSE_CONF;
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_dcb->call_id,
                               other_dcb->line, CC_FEATURE_RESUME, &data);

                


                action_data.update_ui.action = CC_UPDATE_CONF_ACTIVE;
                (void)cc_call_action(other_dcb->call_id, other_dcb->line,
                               CC_ACTION_UPDATE_UI, &action_data);
            } else {
                fsm_display_feature_unavailable();
            }

            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            sm_rc = SM_RC_DEF_CONT;

            break;
        }                       

        break;

    case CC_SRC_GSM:
        switch (ftr_id) {
        case CC_FEATURE_NEW_CALL:

            












            


            ccb = fsmcnf_get_ccb_by_call_id(call_id);
            if (ccb == NULL) {
                break;
            }
            fcb->ccb = ccb;

            










            other_call_id = fsmcnf_get_other_call_id(ccb, call_id);
            other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_CNF);

            if(other_fcb == NULL) {
                GSM_DEBUG_ERROR(GSM_F_PREFIX"Failed to get FCB.\n", fname);
            } else {
                fsm_change_state(fcb, __LINE__, other_fcb->state);
            }

            break;

        case CC_FEATURE_JOIN:
            if (fsm_is_joining_call(ftr_data)) {
                



                fcb_def = fsm_get_fcb_by_call_id_and_type(call_id,
                                                          FSM_TYPE_DEF);
                if ((fcb_def == NULL) ||
                    ((fcb_def->state != FSMDEF_S_CONNECTED) &&
                     (fcb_def->state != FSMDEF_S_RESUME_PENDING))) {
                    FSM_DEBUG_SM(DEB_L_C_F_PREFIX"Join target \
                            call is not at connected state\n",
                            DEB_L_C_F_PREFIX_ARGS(FSM, line, call_id, fname));
                    break;
                }

                
                join_fcb_cnf = fsm_get_fcb_by_call_id_and_type(
                                    ftr_data.newcall.join.join_call_id,
                                    FSM_TYPE_CNF);

                
                ccb = fsmcnf_get_new_cnf_context(call_id);
                if (ccb == NULL || join_fcb_cnf == NULL) {
                    FSM_DEBUG_SM(DEB_L_C_F_PREFIX"Could not \
                            find the conference context\n",
                            DEB_L_C_F_PREFIX_ARGS(FSM, line, call_id, fname));
                    break;
                }

                fcb->ccb = ccb;
                join_fcb_cnf->ccb = ccb;
                ccb->cnf_call_id = fcb_def->dcb->call_id;
                
                ccb->cns_call_id = join_fcb_cnf->dcb->call_id;

                join_fcb_cnf->dcb->group_id = fcb_def->dcb->group_id;
                fsm_change_state(fcb, __LINE__, FSMCNF_S_CNFED);
                fsm_change_state(join_fcb_cnf, __LINE__, FSMCNF_S_CNFED);

                softkey_mask_list[0] = skConfrn;
                ui_select_feature_key_set(line, call_id,
                                          FSMCNF_CONNECTED_SET,
                                          softkey_mask_list, 1);

                ccb->flags |= JOINED;

                ccb->bridged = FALSE;
            }
            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            sm_rc = SM_RC_DEF_CONT;

            break;
        }                       

        break;

    case CC_SRC_SIP:
        switch (ftr_id) {
        case CC_FEATURE_NOTIFY:
            if ((msg->data_valid == TRUE) &&
                (msg->data.notify.subscription == CC_SUBSCRIPTIONS_XFER) &&
                (msg->data.notify.method == CC_XFER_METHOD_REFER)) {
                xcb = fsmxfr_get_xcb_by_call_id(call_id);
                if ((xcb != NULL) && (call_id == xcb->cns_call_id)) {
                    







                    ccb = fsmcnf_get_ccb_by_call_id(xcb->xfr_call_id);
                    if (ccb == NULL) {
                        break;
                    }

                    other_fcb =
                        fsm_get_fcb_by_call_id_and_type(xcb->xfr_call_id,
                                                        FSM_TYPE_CNF);
                    if (other_fcb == NULL) {
                        break;
                    }

                    fcb->ccb = ccb;

                    fsmcnf_update_cnf_context(ccb, xcb->xfr_call_id,
                                              xcb->cns_call_id);

                    fsmcnf_cleanup(other_fcb, __LINE__, FALSE);

                    other_call_id = fsmcnf_get_other_call_id(ccb, call_id);
                    other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                                FSM_TYPE_CNF);

                    fsm_change_state(fcb, __LINE__, other_fcb->state);

                    if (other_fcb->state == FSMCNF_S_CNFED) {
                        ccb->bridged = TRUE;

                        



                        other_dcb = fsm_get_dcb(other_call_id);

                        
                        cause = gsmsdp_encode_sdp_and_update_version(other_dcb,
                                                                     &data.resume.msg_body);
                        if (cause != CC_CAUSE_OK) {
                            GSM_DEBUG_ERROR(get_debug_string(FSM_DBG_SDP_BUILD_ERR));
                            fsmcnf_cleanup(fcb, __LINE__, TRUE);
                            sm_rc = SM_RC_END;
                            break;
                        }
                        data.resume.cause = CC_CAUSE_CONF;
                        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM,
                                       other_dcb->call_id, other_dcb->line,
                                       CC_FEATURE_RESUME, &data);
                    }

                    


                    action_data.update_ui.action = CC_UPDATE_CONF_ACTIVE;
                    (void)cc_call_action(fcb->call_id, dcb->line,
                                   CC_ACTION_UPDATE_UI, &action_data);
                }
            }

            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            sm_rc = SM_RC_DEF_CONT;

            break;
        }                       

        break;

    default:
        fsm_sm_ignore_src(fcb, __LINE__, src_id);
        sm_rc = SM_RC_DEF_CONT;

        break;
    }                           

    return (sm_rc);
}


static void
fsmcnf_update_release (sm_event_t *event)
{
    fsm_fcb_t       *fcb = (fsm_fcb_t *) event->data;
    callid_t         other_call_id;
    cc_action_data_t action_data;
    fsm_fcb_t       *other_fcb;

    


    other_call_id = fsmcnf_get_other_call_id(fcb->ccb, fcb->call_id);
    if (other_call_id != CC_NO_CALL_ID) {
        action_data.update_ui.action = CC_UPDATE_CONF_RELEASE;
        (void)cc_call_action(other_call_id, fcb->dcb->line, CC_ACTION_UPDATE_UI,
                       &action_data);

        






        if (fcb->ccb && (fcb->call_id == fcb->ccb->cnf_call_id)) {
            other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_CNF);
            if (other_fcb != NULL) {
                fsm_fcb_t       *b2bcnf_fcb, *xfr_fcb;
                b2bcnf_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_B2BCNF);
                xfr_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_XFR);
                if ((b2bcnf_fcb != NULL && b2bcnf_fcb->b2bccb == NULL)  &&
                        (xfr_fcb != NULL && xfr_fcb->xcb == NULL)) {
                    cc_call_attribute(other_call_id, other_fcb->dcb->line, NORMAL_CALL);
                }
            }
        }
    }

    fsmcnf_cleanup(fcb, __LINE__, TRUE);
}


static sm_rcs_t
fsmcnf_ev_cnfing_release (sm_event_t *event)
{
    fsm_fcb_t    *fcb     = (fsm_fcb_t *) event->data;
    cc_release_t *msg     = (cc_release_t *) event->msg;
    callid_t      call_id = msg->call_id;
    fsmcnf_ccb_t *ccb     = fcb->ccb;
    fsmxfr_xcb_t *xcb;
    fsm_fcb_t    *other_fcb;

    xcb = fsmxfr_get_xcb_by_call_id(call_id);
    if (xcb != NULL) {
        






        fsmcnf_update_cnf_context(ccb, call_id, xcb->cns_call_id);

        fsmcnf_cleanup(fcb, __LINE__, FALSE);

        other_fcb = fsm_get_fcb_by_call_id_and_type(xcb->cns_call_id,
                                                    FSM_TYPE_CNF);

        if (other_fcb != NULL) {
            other_fcb->ccb = ccb;
            fsm_change_state(other_fcb, __LINE__, FSMCNF_S_CNFING);
        }

        return (SM_RC_CONT);
    }

    fsmcnf_update_release(event);

    return (SM_RC_CONT);
}


static sm_rcs_t
fsmcnf_ev_cnfing_feature (sm_event_t *event)
{
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_feature_t     *msg     = (cc_feature_t *) event->msg;
    callid_t          call_id = msg->call_id;
    fsmdef_dcb_t     *dcb     = fcb->dcb;
    fsmcnf_ccb_t     *ccb     = fcb->ccb;
    cc_srcs_t         src_id  = msg->src_id;
    cc_features_t     ftr_id  = msg->feature_id;
    static const char fname[] = "fsmcnf_ev_cnfing_feature";
    cc_feature_data_t *feat_data   = &(msg->data);
    sm_rcs_t          sm_rc   = SM_RC_CONT;
    callid_t          other_call_id;
    fsmdef_dcb_t     *other_dcb;
    fsm_fcb_t        *other_fcb;
    cc_action_data_t  action_data;
    cc_feature_data_t data;
    cc_causes_t       cause;

    fsm_sm_ftr(ftr_id, src_id);

    switch (src_id) {
    case CC_SRC_UI:
        switch (ftr_id) {
        case CC_FEATURE_CANCEL:
            sm_rc = SM_RC_END;
            fsmcnf_feature_cancel(ccb, ccb->cnf_line, ccb->cnf_call_id,
                                  ccb->cns_call_id 
);
            fsmcnf_cleanup(fcb, __LINE__, TRUE);
            break;

        case CC_FEATURE_ANSWER:
            other_call_id = fsmcnf_get_other_call_id(ccb, call_id);
            if (other_call_id == CC_NO_CALL_ID) {
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                return (sm_rc);
            }
            other_dcb = fsm_get_dcb(other_call_id);
            if (other_dcb == NULL) {
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                return (sm_rc);
            }
            








            dcb->group_id = other_dcb->group_id;
            fsm_change_state(fcb, __LINE__, FSMCNF_S_CNFED);
            return (sm_rc);

        default:
            break;
        }
        

    case CC_SRC_GSM:
        switch (ftr_id) {
        case CC_FEATURE_CONF:
            






            ccb->bridged = TRUE;
            ccb->active  = TRUE;
            ccb->flags |= LCL_CNF;

            other_call_id = fsmcnf_get_other_call_id(ccb, call_id);
            other_dcb = fsm_get_dcb(other_call_id);

            




            gsmsdp_update_local_sdp_media_capability(other_dcb, TRUE, FALSE);

            
            cause = gsmsdp_encode_sdp_and_update_version(other_dcb,
                                                         &data.resume.msg_body);
            if (cause != CC_CAUSE_OK) {
                GSM_DEBUG_ERROR(get_debug_string(FSM_DBG_SDP_BUILD_ERR));
                sm_rc = SM_RC_END;
                break;
            }
            data.resume.cause = CC_CAUSE_CONF;

            other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_DEF);
            if (other_fcb && other_fcb->state == FSMDEF_S_HOLDING) {

                other_dcb->session = LOCAL_CONF;

                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_dcb->call_id,
                           other_dcb->line, CC_FEATURE_RESUME, &data);

            } else {
                

                dcb->session = LOCAL_CONF;
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, call_id,
                           ccb->cnf_line, CC_FEATURE_RESUME, &data);


            }

            


            action_data.update_ui.action = CC_UPDATE_CONF_ACTIVE;
            (void)cc_call_action(dcb->call_id, dcb->line, CC_ACTION_UPDATE_UI,
                           &action_data);

            other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_CNF);
            fsm_change_state(other_fcb, __LINE__, FSMCNF_S_CNFED);
            fsm_change_state(fcb, __LINE__, FSMCNF_S_CNFED);

            sm_rc = SM_RC_END;

            break;

        case CC_FEATURE_END_CALL:
            fsmcnf_update_release(event);

            break;

        case CC_FEATURE_HOLD:
            if ((msg->data_valid) &&
                (feat_data->hold.call_info.data.hold_resume_reason != CC_REASON_SWAP &&
                feat_data->hold.call_info.data.hold_resume_reason != CC_REASON_CONF &&
                feat_data->hold.call_info.data.hold_resume_reason != CC_REASON_INTERNAL)) {
                sm_rc = SM_RC_END;
                DEF_DEBUG(DEB_F_PREFIX"Invoke hold call_id = %d t_call_id=%d\n",
                            DEB_F_PREFIX_ARGS(GSM, fname), ccb->cnf_call_id, ccb->cns_call_id);
                
                ui_terminate_feature(dcb->line, ccb->cnf_call_id, ccb->cns_call_id);

                fsmcnf_feature_cancel(ccb, ccb->cnf_line, ccb->cnf_call_id,
                                      ccb->cns_call_id 
);
                fsmcnf_cleanup(fcb, __LINE__, TRUE);
            }
            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);

            break;
        }                       

        break;

    case CC_SRC_SIP:
        switch (ftr_id) {
        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);

            break;
        }                       

        break;

    default:
        fsm_sm_ignore_src(fcb, __LINE__, src_id);

        break;
    }                           

    return (sm_rc);
}


static sm_rcs_t
fsmcnf_ev_cnfing_onhook (sm_event_t *event)
{
    fsmcnf_update_release(event);

    return (SM_RC_CONT);
}


static sm_rcs_t
fsmcnf_ev_cnfed_release (sm_event_t *event)
{
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_release_t     *msg     = (cc_release_t *) event->msg;
    callid_t          call_id = msg->call_id;
    fsmdef_dcb_t     *dcb     = fcb->dcb;
    fsmcnf_ccb_t     *ccb     = fcb->ccb;
    callid_t          other_call_id;
    fsmdef_dcb_t     *other_dcb;
    cc_feature_data_t data;
    cc_action_data_t  action_data;
    fsmxfr_xcb_t     *xcb;
    fsm_fcb_t        *other_fcb;
    cc_causes_t       cause;

    

    ccb->active  = FALSE;

    if( ccb->flags & JOINED ){
        other_call_id = fsmcnf_get_other_call_id(ccb, call_id);
        if(other_call_id != CC_NO_CALL_ID ){
            fsm_fcb_t       *b2bcnf_fcb, *xfr_fcb;
            b2bcnf_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_B2BCNF);
            xfr_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_XFR);
            if ((b2bcnf_fcb != NULL && b2bcnf_fcb->b2bccb == NULL)  &&
                        (xfr_fcb != NULL && xfr_fcb->xcb == NULL)) {
                cc_call_attribute(other_call_id, dcb->line, NORMAL_CALL);
            }
        }
    }
    xcb = fsmxfr_get_xcb_by_call_id(call_id);
    if ((xcb != NULL) && (!(ccb->flags & JOINED))) {
        









        fsmcnf_update_cnf_context(ccb, call_id, xcb->cns_call_id);

        fsmcnf_cleanup(fcb, __LINE__, FALSE);

        






        ccb->bridged = TRUE;

        


        other_call_id = fsmcnf_get_other_call_id(ccb, xcb->cns_call_id);
        other_dcb = fsm_get_dcb(other_call_id);

        
        cause = gsmsdp_encode_sdp_and_update_version(other_dcb,
                                                     &data.resume.msg_body);
        if (cause != CC_CAUSE_OK) {
            GSM_DEBUG_ERROR(get_debug_string(FSM_DBG_SDP_BUILD_ERR));
            return (SM_RC_END);
        }
        data.resume.cause = CC_CAUSE_CONF;
        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_dcb->call_id,
                       other_dcb->line, CC_FEATURE_RESUME, &data);

        


        other_fcb = fsm_get_fcb_by_call_id_and_type(xcb->cns_call_id,
                                                    FSM_TYPE_CNF);

        if (other_fcb != NULL) {
            other_fcb->ccb = ccb;

            fsm_change_state(other_fcb, __LINE__, FSMCNF_S_CNFED);

            action_data.update_ui.action = CC_UPDATE_CONF_ACTIVE;
            cc_call_action(other_fcb->call_id, dcb->line, CC_ACTION_UPDATE_UI,
                           &action_data);
            return (SM_RC_CONT);
        }
    }

    fsmcnf_update_release(event);

    return (SM_RC_CONT);
}


static void
fsmcnf_other_feature (fsm_fcb_t *fcb, cc_features_t ftr_id)
{
    callid_t other_call_id;

    other_call_id = fsmcnf_get_other_call_id(fcb->ccb, fcb->call_id);
    if (other_call_id != CC_NO_CALL_ID) {
        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_call_id,
                       fcb->dcb->line, ftr_id, NULL);
    }
}


static sm_rcs_t
fsmcnf_ev_cnfed_feature (sm_event_t *event)
{
    fsm_fcb_t      *fcb     = (fsm_fcb_t *) event->data;
    cc_feature_t   *msg     = (cc_feature_t *) event->msg;
    callid_t        call_id = msg->call_id;
    cc_srcs_t       src_id  = msg->src_id;
    cc_features_t   ftr_id  = msg->feature_id;
    sm_rcs_t        sm_rc   = SM_RC_CONT;
    fsmcnf_ccb_t   *ccb     = fcb->ccb;
    fsmxfr_xcb_t   *xcb;
    int             join = 1;

    fsm_sm_ftr(ftr_id, src_id);

    switch (src_id) {
    case CC_SRC_UI:
    case CC_SRC_GSM:
        switch (ftr_id) {
        case CC_FEATURE_CANCEL:
            sm_rc = SM_RC_END;
            fsmcnf_cleanup(fcb, __LINE__, TRUE);
            break;

        case CC_FEATURE_END_CALL:
            if ((ccb->flags & JOINED) && (ccb->bridged == FALSE)) {
                
                fsmcnf_cleanup(fcb, __LINE__, TRUE);
                return (sm_rc);
            }
            





            config_get_value(CFGID_CNF_JOIN_ENABLE, &join, sizeof(join));
            if (((ccb->bridged == TRUE) && (join) && !(ccb->flags & JOINED)) ||
                ((ccb->bridged == TRUE) && (ccb->flags & XFER))) {

                fsmcnf_cnf_xfer(ccb);
                sm_rc = SM_RC_END;
            } else {
                




                fsmcnf_other_feature(fcb, ftr_id);
            }

            fsmcnf_cleanup(fcb, __LINE__, TRUE);
            break;

        case CC_FEATURE_HOLD:
            


            if ((ccb->cnf_ftr_ack) && (src_id == CC_SRC_UI)) {
                return (SM_RC_END);
            }

            





            if (ccb->bridged == TRUE) {

                if ((ccb->flags & XFER) && (src_id == CC_SRC_GSM)) {
                    fsmcnf_cleanup(fcb, __LINE__, TRUE);
                    return (sm_rc);
                }
                xcb = fsmxfr_get_xcb_by_call_id(call_id);
                if ((xcb == NULL) || (!(ccb->flags & XFER))) {
                    fsmcnf_other_feature(fcb, ftr_id);
                    ccb->cnf_ftr_ack = TRUE;
                    ccb->bridged = FALSE;
                }
            }
            break;

        case CC_FEATURE_RESUME:
            


            if ((ccb->cnf_ftr_ack) && (src_id == CC_SRC_UI)) {
                return (SM_RC_END);
            }

            




            if (ccb->bridged == FALSE) {
                fsmcnf_other_feature(fcb, ftr_id);
                ccb->cnf_ftr_ack = TRUE;
                ccb->bridged = TRUE;
            }
            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            break;
        }                       
        break;

    case CC_SRC_SIP:
        switch (ftr_id) {
        case CC_FEATURE_CALLINFO:
            if ((ccb->flags & JOINED) &&
                (call_id == ccb->cns_call_id)) {
                



                return (SM_RC_END);
            }
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            break;

        case CC_FEATURE_XFER:
            if (msg->data_valid == FALSE) {
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                break;
            }

            switch (msg->data.xfer.method) {
            case CC_XFER_METHOD_BYE:
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                break;

            case CC_XFER_METHOD_REFER:
                if ((msg->data.xfer.cause == CC_CAUSE_XFER_REMOTE) &&
                    (msg->data.xfer.target_call_id != CC_NO_CALL_ID)) {
                    





                    fsmcnf_update_cnf_context(ccb, call_id,
                                              msg->data.xfer.target_call_id);
                    
                    fsmcnf_cleanup(fcb, __LINE__, FALSE);
                } else {
                    fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                }
                break;

            default:
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                break;
            }
            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            break;
        }
        break;

    default:
        fsm_sm_ignore_src(fcb, __LINE__, src_id);
        break;
    }                           

    return (sm_rc);
}


static sm_rcs_t
fsmcnf_ev_cnfed_feature_ack (sm_event_t *event)
{
    fsm_fcb_t        *fcb    = (fsm_fcb_t *) event->data;
    cc_feature_ack_t *msg    = (cc_feature_ack_t *) event->msg;
    fsmdef_dcb_t     *dcb    = fcb->dcb;
    fsmcnf_ccb_t     *ccb    = fcb->ccb;
    cc_srcs_t         src_id = msg->src_id;
    cc_features_t     ftr_id = msg->feature_id;
    sm_rcs_t          sm_rc  = SM_RC_CONT;
    cc_feature_data_t ftr_data;
    cc_causes_t       cause;
    char              tmp_str[STATUS_LINE_MAX_LEN];

    fsm_sm_ftr(ftr_id, src_id);

    switch (src_id) {
    case CC_SRC_SIP:
        switch (ftr_id) {
        case CC_FEATURE_HOLD:
            



            if (msg->cause == CC_CAUSE_REQUEST_PENDING) {
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            } else {
                ccb->cnf_ftr_ack = FALSE;
            }
            break;

        case CC_FEATURE_RESUME:
            



            if (msg->cause == CC_CAUSE_REQUEST_PENDING) {
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                break;
            }

            ccb->cnf_ftr_ack = FALSE;

            


            if (msg->cause != CC_CAUSE_NORMAL) {
                if ((platGetPhraseText(STR_INDEX_CNFR_FAIL_NOCODEC,
                                             (char *) tmp_str,
                                             STATUS_LINE_MAX_LEN - 1)) == CPR_SUCCESS) {
                    lsm_ui_display_notify(tmp_str, NO_FREE_LINES_TIMEOUT);
                }

                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, ccb->cns_call_id,
                               fcb->dcb->line, CC_FEATURE_END_CALL, NULL);

                dcb = fsmdef_get_dcb_by_call_id(ccb->cnf_call_id);
                if (dcb != NULL) {
                    
                    cause = gsmsdp_encode_sdp_and_update_version(dcb,
                                                                 &ftr_data.resume.msg_body);
                    if (cause != CC_CAUSE_OK) {
                        GSM_DEBUG_ERROR(get_debug_string(FSM_DBG_SDP_BUILD_ERR));
                        return (SM_RC_END);
                    }
                    ftr_data.resume.cause = CC_CAUSE_OK;
                    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id,
                                   dcb->line, CC_FEATURE_RESUME, &ftr_data);
                }

                fsmcnf_cleanup(fcb, __LINE__, TRUE);
            }

            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);

            break;
        }                       

        break;

    default:
        fsm_sm_ignore_src(fcb, __LINE__, src_id);

        break;
    }                           

    return (sm_rc);
}


static sm_rcs_t
fsmcnf_ev_cnfed_onhook (sm_event_t *event)
{
    fsm_fcb_t      *fcb   = (fsm_fcb_t *) event->data;
    sm_rcs_t        sm_rc = SM_RC_CONT;
    fsmcnf_ccb_t   *ccb   = fcb->ccb;
    int             join  = 1;
    fsmdef_dcb_t    *other_dcb;
    boolean     conf_id_valid = FALSE;

    






    if (fcb->call_id == ccb->cnf_call_id) {
        other_dcb = fsm_get_dcb(ccb->cns_call_id);
    } else {
        other_dcb = fsm_get_dcb(ccb->cnf_call_id);
    }
    other_dcb->onhook_received = TRUE;

    
    
    conf_id_valid =  fsmcnd_conf_call_id_valid(ccb);
    config_get_value(CFGID_CNF_JOIN_ENABLE, &join, sizeof(join));
    if (((ccb->bridged == TRUE) && (join) && !(ccb->flags & JOINED) && (conf_id_valid)) ||
        ((ccb->bridged == TRUE) && (ccb->flags & XFER) && (conf_id_valid))) {
        





        fsmcnf_cnf_xfer(ccb);
        sm_rc = SM_RC_END;
    } else {
        




        fsmcnf_other_feature(fcb, CC_FEATURE_END_CALL);
    }

    fsmcnf_cleanup(fcb, __LINE__, TRUE);
    return (sm_rc);
}


cc_int32_t
fsmcnf_show_cmd (cc_int32_t argc, const char *argv[])
{
    fsmcnf_ccb_t *ccb;
    int           i = 0;

    


    if ((argc == 2) && (argv[1][0] == '?')) {
        debugif_printf("show fsmcnf\n");
        return (0);
    }

    debugif_printf("\n-------------------------- FSMCNF ccbs --------------------------");
    debugif_printf("\ni   cnf_id  ccb         cnf_call_id  cns_call_id  active  bridged");
    debugif_printf("\n-----------------------------------------------------------------"
         "\n");

    FSM_FOR_ALL_CBS(ccb, fsmcnf_ccbs, FSMCNF_MAX_CCBS) {
        debugif_printf("%-2d  %-6d  0x%8p  %-11d  %-11d  %-6d  %-7d\n",
                       i++, ccb->cnf_id, ccb, ccb->cnf_call_id,
                       ccb->cns_call_id, ccb->active, ccb->bridged);
    }

    return (0);
}


void
fsmcnf_init (void)
{
    fsmcnf_ccb_t *ccb;
    static const char *fname = "fsmcnf_init";


    


    fsmcnf_ccbs = (fsmcnf_ccb_t *)
        cpr_calloc(FSMCNF_MAX_CCBS, sizeof(fsmcnf_ccb_t));

    if (fsmcnf_ccbs == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Failed to allocate memory for " \
                            "cnf ccbs.\n", fname);
        return;
    }

    FSM_FOR_ALL_CBS(ccb, fsmcnf_ccbs, FSMCNF_MAX_CCBS) {
        fsmcnf_init_ccb(ccb);
    }

    


    fsmcnf_sm_table.min_state = FSMCNF_S_MIN;
    fsmcnf_sm_table.max_state = FSMCNF_S_MAX;
    fsmcnf_sm_table.min_event = CC_MSG_MIN;
    fsmcnf_sm_table.max_event = CC_MSG_MAX;
    fsmcnf_sm_table.table     = (&(fsmcnf_function_table[0][0]));
}

int
cc_is_cnf_call (callid_t call_id)
{
    if (call_id == CC_NO_CALL_ID) {
        return FALSE;
    }
    return fsmutil_is_cnf_leg(call_id, fsmcnf_ccbs, FSMCNF_MAX_CCBS);
}

void
fsmcnf_shutdown (void)
{
    cpr_free(fsmcnf_ccbs);
    fsmcnf_ccbs = NULL;
}

int
fsmutil_is_cnf_consult_call (callid_t call_id)
{
    return fsmutil_is_cnf_consult_leg(call_id, fsmcnf_ccbs, FSMCNF_MAX_CCBS);
}

boolean
fsmcnd_conf_call_id_valid(fsmcnf_ccb_t   *ccb){

	 static const char fname[] = "fsmcnd_conf_call_id_valid";
	if( ccb != NULL){
                FSM_DEBUG_SM(get_debug_string(FSMCNF_DBG_PTR), ccb->cnf_id,
                    ccb->cnf_call_id, ccb->cns_call_id, fname, ccb);
		if((ccb->cnf_call_id != CC_NO_CALL_ID)  && (ccb->cns_call_id != CC_NO_CALL_ID) ){
			return TRUE;
		}
	}
	return FALSE;
}
