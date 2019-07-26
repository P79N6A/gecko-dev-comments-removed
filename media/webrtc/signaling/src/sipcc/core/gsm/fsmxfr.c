



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_string.h"
#include "fim.h"
#include "lsm.h"
#include "sm.h"
#include "ccapi.h"
#include "phone_debug.h"
#include "text_strings.h"
#include "fsm.h"
#include "uiapi.h"
#include "debug.h"
#include "regmgrapi.h"
#include "platform_api.h"

extern fsmdef_dcb_t *fsmdef_dcbs;

#define FSMXFR_NULL_DIALSTRING '\0'
static fsmxfr_xcb_t *fsmxfr_xcbs;

typedef enum fsmxfr_states_t_ {
    FSMXFR_S_MIN = -1,
    FSMXFR_S_IDLE,
    FSMXFR_S_ACTIVE,
    FSMXFR_S_MAX
} fsmxfr_states_t;

static const char *fsmxfr_state_names[] = {
    "IDLE",
    "ACTIVE"
};


static sm_rcs_t fsmxfr_ev_idle_setup(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_idle_feature(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_idle_dialstring(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_connected_ack(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_release(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_release_complete(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_feature(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_feature_ack(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_onhook(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_dialstring(sm_event_t *event);
static sm_rcs_t fsmxfr_ev_active_proceeding(sm_event_t *event);

static sm_function_t fsmxfr_function_table[FSMXFR_S_MAX][CC_MSG_MAX] =
{

    {
     fsmxfr_ev_idle_setup,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     fsmxfr_ev_idle_feature,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     fsmxfr_ev_idle_dialstring,
     NULL,
     NULL
    },


    {
     NULL,
     NULL,
     fsmxfr_ev_active_proceeding,
     NULL,
     NULL,
     fsmxfr_ev_active_connected_ack,
     fsmxfr_ev_active_release,
     fsmxfr_ev_active_release_complete,
     fsmxfr_ev_active_feature,
     fsmxfr_ev_active_feature_ack,
     NULL,
     fsmxfr_ev_active_onhook,
     NULL,
     NULL,
     NULL,
     fsmxfr_ev_active_dialstring,
     NULL,
     NULL
    }
};

static sm_table_t fsmxfr_sm_table;
sm_table_t *pfsmxfr_sm_table = &fsmxfr_sm_table;

const char *
fsmxfr_state_name (int state)
{
    if ((state <= FSMXFR_S_MIN) || (state >= FSMXFR_S_MAX)) {
        return (get_debug_string(GSM_UNDEFINED));
    }

    return (fsmxfr_state_names[state]);
}


static int
fsmxfr_get_new_xfr_id (void)
{
    static int xfr_id = 0;

    if (++xfr_id < 0) {
        xfr_id = 1;
    }

    return (xfr_id);
}













void fsmxfr_mark_dcb_for_xfr_complete(callid_t cns_call_id, callid_t xfr_call_id,
        boolean set_flag)
{
    fsm_fcb_t    *cns_fcb, *xfr_fcb;
    
    cns_fcb = fsm_get_fcb_by_call_id_and_type(cns_call_id, 
                                    FSM_TYPE_DEF);
    xfr_fcb = fsm_get_fcb_by_call_id_and_type(xfr_call_id, 
                                    FSM_TYPE_DEF);
    if (set_flag) {
        if (cns_fcb && cns_fcb->dcb) {
            FSM_SET_FLAGS(cns_fcb->dcb->flags, FSMDEF_F_XFER_COMPLETE);
        }
        if (xfr_fcb && xfr_fcb->dcb) {
            FSM_SET_FLAGS(xfr_fcb->dcb->flags, FSMDEF_F_XFER_COMPLETE);
        }
    } else {
        if (cns_fcb && cns_fcb->dcb) {
            FSM_RESET_FLAGS(cns_fcb->dcb->flags, FSMDEF_F_XFER_COMPLETE);
        }
        if (xfr_fcb && xfr_fcb->dcb) {
            FSM_RESET_FLAGS(xfr_fcb->dcb->flags, FSMDEF_F_XFER_COMPLETE);
        }
    }
}














fsm_fcb_t *fsmxfr_get_active_xfer(void)
{
    fsm_fcb_t    *fcb;
    fsmxfr_xcb_t *xcb;

    FSM_FOR_ALL_CBS(xcb, fsmxfr_xcbs, FSMXFR_MAX_XCBS) {
        fcb = fsm_get_fcb_by_call_id_and_type(xcb->xfr_call_id,
                                               FSM_TYPE_XFR);        
        if (fcb && fcb->state == FSMXFR_S_ACTIVE) {
            return(fcb);
        }
    }

    return(NULL);
}


static void
fsmxfr_init_xcb (fsmxfr_xcb_t *xcb)
{
    if (xcb != NULL) {
        xcb->xfr_id        = FSM_NO_ID;
        xcb->xfr_call_id   = CC_NO_CALL_ID;
        xcb->cns_call_id   = CC_NO_CALL_ID;
        xcb->xfr_line      = CC_NO_LINE;
        xcb->cns_line      = CC_NO_LINE;
        xcb->type          = FSMXFR_TYPE_NONE;
        xcb->method        = CC_XFER_METHOD_NONE;
        xcb->cnf_xfr       = FALSE;
        xcb->active        = FALSE;
        xcb->mode          = FSMXFR_MODE_TRANSFEROR;
        xcb->xfer_comp_req = FALSE;
        xcb->xfr_orig      = CC_SRC_MIN;

        if (xcb->xcb2 != NULL) {
            fsmxfr_init_xcb(xcb->xcb2);
            xcb->xcb2 = NULL;
        }

        if (xcb->dialstring != NULL) {
            cpr_free(xcb->dialstring);
            xcb->dialstring = NULL;
        }
        if (xcb->queued_dialstring != NULL) {
            cpr_free(xcb->queued_dialstring);
            xcb->queued_dialstring = NULL;
        }
        if (xcb->referred_by != NULL) {
            cpr_free(xcb->referred_by);
            xcb->referred_by = NULL;
        }
    }
}


static fsmxfr_xcb_t *
fsmxfr_get_xcb_by_xfr_id (int xfr_id)
{
    static const char fname[] = "fsmxfr_get_xcb_by_xfr_id";
    fsmxfr_xcb_t *xcb;
    fsmxfr_xcb_t *xcb_found = NULL;

    FSM_FOR_ALL_CBS(xcb, fsmxfr_xcbs, FSMXFR_MAX_XCBS) {
        if (xcb->xfr_id == xfr_id) {
            xcb_found = xcb;

            FSM_DEBUG_SM(get_debug_string(FSMXFR_DBG_PTR), xcb->xfr_id,
                         xcb->xfr_call_id, xcb->cns_call_id, fname, xcb);

            break;
        }
    }


    return (xcb_found);
}


















static fsmxfr_xcb_t *
fsmxfr_get_new_xfr_context (callid_t xfr_call_id, line_t line, fsmxfr_types_t type,
                           cc_xfer_methods_t method, fsmxfr_modes_t mode)
{
    static const char fname[] = "fsmxfr_get_new_xfr_context";
    fsmxfr_xcb_t *xcb;

    xcb = fsmxfr_get_xcb_by_xfr_id(FSM_NO_ID);
    if (xcb != NULL) {
        xcb->xfr_id      = fsmxfr_get_new_xfr_id();
        xcb->xfr_call_id = xfr_call_id;
        xcb->cns_call_id = cc_get_new_call_id();
        xcb->xfr_line    = line;
        xcb->cns_line    = line;
        xcb->type        = type;
        xcb->method      = method;
        xcb->mode        = mode;

        FSM_DEBUG_SM(get_debug_string(FSMXFR_DBG_PTR), xcb->xfr_id,
                     xcb->xfr_call_id, xcb->cns_call_id, fname, xcb);
    }

    return (xcb);
}


fsmxfr_xcb_t *
fsmxfr_get_xcb_by_call_id (callid_t call_id)
{
    fsmxfr_xcb_t *xcb;
    fsmxfr_xcb_t *xcb_found = NULL;

    FSM_FOR_ALL_CBS(xcb, fsmxfr_xcbs, FSMXFR_MAX_XCBS) {
        if ((xcb->xfr_call_id == call_id) || (xcb->cns_call_id == call_id)) {
            xcb_found = xcb;
            break;
        }
    }

    return (xcb_found);
}












void
fsmxfr_feature_cancel (fsmxfr_xcb_t *xcb, line_t line, callid_t call_id, 
                       callid_t target_call_id,
                       cc_rcc_skey_evt_type_e cause)
{
    static const char fname[] = "fsmxfr_feature_cancel";
    cc_feature_data_t data;
    fsm_fcb_t         *fcb_def;

    DEF_DEBUG(DEB_F_PREFIX"Sending cancel call_id = %d, t_id=%d, cause = %d\n", 
            DEB_F_PREFIX_ARGS(GSM, fname), call_id, target_call_id, cause);

    fcb_def = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);

    if ((cause == CC_SK_EVT_TYPE_EXPLI) &&
        (fcb_def != NULL) && ((fcb_def->dcb->selected == FALSE) &&
        ((fcb_def->state == FSMDEF_S_OUTGOING_ALERTING) ||
            ((fcb_def->state == FSMDEF_S_CONNECTED) && 
            (fcb_def->dcb->spoof_ringout_requested == TRUE) &&
            (fcb_def->dcb->spoof_ringout_applied == TRUE))))) {

        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, call_id,
                           line, CC_FEATURE_END_CALL, NULL);
    }

    fcb_def = fsm_get_fcb_by_call_id_and_type(target_call_id, FSM_TYPE_DEF);

    if ((cause == CC_SK_EVT_TYPE_EXPLI) &&
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
    data.cancel.cause = cause;

    cc_int_feature(CC_SRC_GSM, CC_SRC_SIP, call_id,
                           line, CC_FEATURE_CANCEL, &data);
}


void
fsmxfr_update_xfr_context (fsmxfr_xcb_t *xcb, callid_t old_call_id,
                           callid_t new_call_id)
{
    static const char fname[] = "fsmxfr_update_xfr_context";
    FSM_DEBUG_SM(DEB_F_PREFIX"Entered. \n", DEB_F_PREFIX_ARGS(FSM, "fsmxfr_update_xfr_context"));

    if (xcb != NULL) {
        if (old_call_id == xcb->xfr_call_id) {
            xcb->xfr_call_id = new_call_id;
        } else if (old_call_id == xcb->cns_call_id) {
            xcb->cns_call_id = new_call_id;
        }

        FSM_DEBUG_SM(get_debug_string(FSMXFR_DBG_PTR), xcb->xfr_id,
                     xcb->xfr_call_id, xcb->cns_call_id, fname, xcb);
    }
}










line_t
fsmxfr_get_other_line (fsmxfr_xcb_t *xcb, callid_t call_id)
{
    line_t other_line = CC_NO_LINE;

    if (xcb != NULL) {
        if (xcb->xfr_call_id == call_id) {
            other_line = xcb->cns_line;
        } else if (xcb->cns_call_id == call_id) {
            other_line = xcb->xfr_line;
        }
    }

    return (other_line);
}

callid_t
fsmxfr_get_other_call_id (fsmxfr_xcb_t *xcb, callid_t call_id)
{
    callid_t other_call_id = CC_NO_CALL_ID;

    if (xcb != NULL) {
        if (xcb->xfr_call_id == call_id) {
            other_call_id = xcb->cns_call_id;
        } else if (xcb->cns_call_id == call_id) {
            other_call_id = xcb->xfr_call_id;
        }
    }

    return (other_call_id);
}



















static void
fsmxfr_remove_fcb (fsm_fcb_t *fcb, callid_t call_id)
{
    fsmxfr_xcb_t *xcb = fcb->xcb;
    FSM_DEBUG_SM(DEB_F_PREFIX"Entered. \n", DEB_F_PREFIX_ARGS(FSM, "fsmxfr_remove_fcb"));

    if (xcb != NULL) {
        fsmxfr_update_xfr_context(xcb, call_id, CC_NO_CALL_ID);

        


        if ((xcb->xfr_call_id == CC_NO_CALL_ID) &&
            (xcb->cns_call_id == CC_NO_CALL_ID)) {
            fsmxfr_init_xcb(xcb);
        }
    }
}


static void
fsmxfr_cnf_cleanup (fsmxfr_xcb_t *xcb)
{
    fsmdef_dcb_t     *xfr_dcb;
    fsmdef_dcb_t     *cns_dcb;
    cc_feature_data_t ftr_data;

    cns_dcb = fsm_get_dcb(xcb->cns_call_id);
    xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);

    ftr_data.endcall.cause = CC_CAUSE_NORMAL;
    ftr_data.endcall.dialstring[0] = '\0';
    



    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM,
                   cns_dcb->call_id, cns_dcb->line,
                   CC_FEATURE_END_CALL, &ftr_data);
    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM,
                   xfr_dcb->call_id, xfr_dcb->line,
                   CC_FEATURE_END_CALL, &ftr_data);
}


static void
fsmxfr_cleanup (fsm_fcb_t *fcb, int fname, boolean both)
{
    fsm_fcb_t   *other_fcb = NULL;
    callid_t     call_id = fcb->call_id;
    callid_t     other_call_id = CC_NO_CALL_ID;
    line_t       other_line;


    FSM_DEBUG_SM(DEB_F_PREFIX"Entered. \n", DEB_F_PREFIX_ARGS(FSM, "fsmxfr_cleanup"));
    other_call_id = fsmxfr_get_other_call_id(fcb->xcb, call_id);
    other_line    = fsmxfr_get_other_line(fcb->xcb, call_id);

    if (other_call_id != CC_NO_CALL_ID) {
        other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                    FSM_TYPE_XFR);
    }

    if (fcb->xcb && (fcb->xcb->cnf_xfr != TRUE) &&
        (call_id == fcb->xcb->xfr_call_id)) {

        if (other_call_id != CC_NO_CALL_ID) {
            




            cc_call_attribute(other_call_id, other_line, NORMAL_CALL);
        }
    }
    






    if (both) {
        FSM_DEBUG_SM(DEB_F_PREFIX"clean both. \n", DEB_F_PREFIX_ARGS(FSM, "fsmxfr_cleanup"));

        if (other_call_id != CC_NO_CALL_ID) {
            if (other_fcb != NULL) {
                fsmxfr_cleanup(other_fcb, fname, FALSE);
            } else {
                



                fsmxfr_update_xfr_context(fcb->xcb, other_call_id,
                                          CC_NO_CALL_ID);
            }
        }
    }

    


    fsmxfr_remove_fcb(fcb, fcb->call_id);

    


    fsm_change_state(fcb, fname, FSMXFR_S_IDLE);

    



    fsm_init_fcb(fcb, fcb->call_id, fcb->dcb, FSM_TYPE_XFR);
}


void
fsmxfr_free_cb (fim_icb_t *icb, callid_t call_id)
{
    fsm_fcb_t *fcb = NULL;

    if (call_id != CC_NO_CALL_ID) {
        fcb = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_XFR);

        if (fcb != NULL) {
            fsmxfr_cleanup(fcb, __LINE__, FALSE);
            fsm_init_fcb(fcb, CC_NO_CALL_ID, FSMDEF_NO_DCB, FSM_TYPE_NONE);
        }
    }
}


fsmxfr_types_t
fsmxfr_get_xfr_type (callid_t call_id)
{
    fsmxfr_xcb_t   *xcb;
    fsmxfr_types_t  type = FSMXFR_TYPE_BLND_XFR;

    xcb = fsmxfr_get_xcb_by_call_id(call_id);
    if (xcb != NULL) {
        type = xcb->type;
    }

    return (type);
}


cc_features_t
fsmxfr_type_to_feature (fsmxfr_types_t type)
{
    cc_features_t feature;

    if (type == FSMXFR_TYPE_XFR) {
        feature = CC_FEATURE_XFER;
    } else {
        feature = CC_FEATURE_BLIND_XFER;
    }

    return (feature);
}







static sm_rcs_t
fsmxfr_ev_idle_setup (sm_event_t *event)
{
    fsm_fcb_t    *fcb     = (fsm_fcb_t *) event->data;
    cc_setup_t   *msg     = (cc_setup_t *) event->msg;
    fsmxfr_xcb_t *xcb;

    








    


    xcb = fsmxfr_get_xcb_by_call_id(msg->call_id);
    if (xcb == NULL) {
        return (SM_RC_DEF_CONT);
    }
    fcb->xcb = xcb;

    fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);

    return (SM_RC_CONT);
}


static boolean
fsmxfr_copy_dialstring (char **saved_dialstring, char *dialstring)
{
    char         *tempstring;
    int           len;

    if (saved_dialstring == NULL) {
        return (FALSE);
    }

    


    if ((dialstring == NULL) || (dialstring[0] == '\0')) {
        return (FALSE);
    }

    




    len = (strlen(dialstring) + 1) * sizeof(char);
    tempstring = (char *) cpr_malloc(len);
    if (tempstring != NULL) {
        sstrncpy(tempstring, dialstring, len);

        *saved_dialstring = tempstring;
    }

    return (TRUE);
}

static void
fsmxfr_set_xfer_data (cc_causes_t cause, cc_xfer_methods_t method,
                      callid_t target_call_id, char *dialstring,
                      cc_feature_data_xfer_t *xfer)
{
    xfer->cause          = cause;
    xfer->method         = method;
    xfer->target_call_id = target_call_id;
    sstrncpy(xfer->dialstring, dialstring, CC_MAX_DIALSTRING_LEN);
}


static boolean
fsmxfr_remote_transfer (fsm_fcb_t *fcb, cc_features_t ftr_id,
                        callid_t call_id, line_t line, char *dialstring,
                        char *referred_by)
{
    fsmxfr_types_t    type;
    int               free_lines;
    cc_feature_data_t data;
    fsmxfr_xcb_t     *xcb;
    fsmxfr_xcb_t     *primary_xcb;
    callid_t          cns_call_id;
    line_t            newcall_line = 0;

    memset(&data, 0, sizeof(cc_feature_data_t));

    







    if (ftr_id == CC_FEATURE_XFER) {
        



        free_lines = lsm_get_instances_available_cnt(line, TRUE);
        if (free_lines <= 0) {
            



            fsm_display_no_free_lines();

            return (FALSE);
        }
    }

    newcall_line = lsm_get_newcall_line(line);
    if (newcall_line == NO_LINES_AVAILABLE) {
        


        lsm_ui_display_notify_str_index(STR_INDEX_ERROR_PASS_LIMIT);
         return (FALSE);
    }

    



    type = ((ftr_id == CC_FEATURE_XFER) ?
            (FSMXFR_TYPE_XFR) : (FSMXFR_TYPE_BLND_XFR));

    primary_xcb = fsmxfr_get_xcb_by_call_id(call_id);

    xcb = fsmxfr_get_new_xfr_context(call_id, line, type, CC_XFER_METHOD_REFER,
                                     FSMXFR_MODE_TRANSFEREE);
    if (xcb == NULL) {
        return (FALSE);
    }

    if (primary_xcb) {
        fcb->xcb->xcb2 = xcb;
        fcb->xcb->active = TRUE;
    } else {
        fcb->xcb = xcb;
    }

    xcb->cns_line = newcall_line;
    cns_call_id = xcb->cns_call_id;
    fsmxfr_set_xfer_data(CC_CAUSE_OK, CC_XFER_METHOD_REFER, cns_call_id,
                         FSMXFR_NULL_DIALSTRING, &(data.xfer));

    cc_int_feature_ack(CC_SRC_GSM, CC_SRC_SIP, call_id,
                       line, ftr_id, &data, CC_CAUSE_NORMAL);

    if (ftr_id == CC_FEATURE_XFER) {
        








        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, call_id,
                       0xFF, CC_FEATURE_HOLD, NULL);

        



        


        if ((dialstring == NULL) || (dialstring[0] == '\0')) {
            return (FALSE);
        }

        




        memset(data.newcall.redirect.redirects[0].number, 0,
               sizeof(CC_MAX_DIALSTRING_LEN));
        data.newcall.cause = CC_CAUSE_XFER_REMOTE;
        data.newcall.redirect.redirects[0].redirect_reason =
            CC_REDIRECT_REASON_DEFLECTION;
        sstrncpy(data.newcall.dialstring, dialstring, CC_MAX_DIALSTRING_LEN);

        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, cns_call_id, newcall_line,
                       CC_FEATURE_NEW_CALL, &data);

        FSM_DEBUG_SM(get_debug_string(FSMXFR_DBG_XFR_INITIATED),
                     xcb->xfr_id, call_id, cns_call_id, __LINE__);

        fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);
    } else { 
        


        if ((fsmxfr_copy_dialstring(&xcb->dialstring, dialstring) == TRUE) &&
            (fsmxfr_copy_dialstring(&xcb->referred_by, referred_by) == TRUE)) {

            fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);
        } else {
            fsmxfr_cleanup(fcb, __LINE__, TRUE);
        }
    }

    return (TRUE);
}


static sm_rcs_t
fsmxfr_ev_idle_feature (sm_event_t *event)
{
    const char        *fname    = "fsmxfr_ev_idle_feature";
    fsm_fcb_t         *fcb     = (fsm_fcb_t *) event->data;
    cc_feature_t      *msg     = (cc_feature_t *) event->msg;
    callid_t           call_id = msg->call_id;
    line_t             line    = msg->line;
    cc_srcs_t          src_id  = msg->src_id;
    cc_features_t      ftr_id  = msg->feature_id;
    cc_feature_data_t *ftr_data = &(msg->data);
    fsmdef_dcb_t      *dcb     = fcb->dcb;
    fsm_fcb_t         *other_fcb;
    sm_rcs_t           sm_rc   = SM_RC_CONT;
    fsmxfr_types_t     type;
    int                free_lines;
    cc_feature_data_t  data;
    cc_causes_t        cause   = msg->data.xfer.cause;
    cc_xfer_methods_t  method;
    fsmxfr_xcb_t      *xcb;
    fsm_fcb_t         *fcb_def;
    fsm_fcb_t         *cns_fcb, *con_fcb, *sel_fcb;
    boolean            int_rc = FALSE;
    callid_t           cns_call_id;
    line_t             newcall_line = 0;

    memset(&data, 0, sizeof(data));
    fsm_sm_ftr(ftr_id, src_id);

    


    if ((ftr_id == CC_FEATURE_BLIND_XFER) || (ftr_id == CC_FEATURE_XFER)) {
        sm_rc = SM_RC_END;
    }

    switch (src_id) {
    case CC_SRC_UI:
        switch (ftr_id) {
        case CC_FEATURE_DIRTRXFR:

            


            other_fcb = fsmxfr_get_active_xfer();
            if (other_fcb) {
                if (other_fcb->xcb == NULL) {
                    GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
                    return (SM_RC_END);
                }
                


                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_fcb->xcb->cns_call_id,
                       other_fcb->xcb->cns_line, CC_FEATURE_END_CALL, NULL);
                other_fcb->xcb->cns_call_id = call_id;
               
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_fcb->xcb->xfr_call_id,
                               other_fcb->xcb->xfr_line, CC_FEATURE_DIRTRXFR, NULL);

                return(SM_RC_END);
            }
            fsm_get_fcb_by_selected_or_connected_call_fcb(call_id, &con_fcb, &sel_fcb);

            
            if (sel_fcb) {
                other_fcb = sel_fcb;
            } else if (con_fcb) {
                other_fcb = con_fcb;
            } else {
                
                return(SM_RC_CONT);
            }

            


            if ((fsmutil_get_num_selected_calls() > 1) && (dcb->selected == FALSE)) {
                
                return(SM_RC_CONT);
            }

            if (other_fcb->xcb == NULL || other_fcb->xcb->xfr_line != line) {
                
                GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
                return(SM_RC_CONT);
            }

            xcb = fsmxfr_get_new_xfr_context(call_id, line, FSMXFR_TYPE_XFR,
                                             CC_XFER_METHOD_REFER,
                                             FSMXFR_MODE_TRANSFEROR);
            if (xcb == NULL) {
                break;
            }

            xcb->xfr_orig = src_id;
            fcb->xcb = xcb;

            xcb->type = FSMXFR_TYPE_DIR_XFR;
            xcb->cns_call_id = other_fcb->dcb->call_id;

            other_fcb->xcb = xcb;
            



            cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id,
                               dcb->line, CC_FEATURE_DIRTRXFR, NULL);
            
            fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);

            return(SM_RC_END);

        case CC_FEATURE_BLIND_XFER:
        case CC_FEATURE_XFER:

            




            if ((cause != CC_CAUSE_XFER_CNF) &&
                (ftr_data && msg->data_valid) && 
                (ftr_data->xfer.target_call_id != CC_NO_CALL_ID) &&
                ((cns_fcb = fsm_get_fcb_by_call_id_and_type(ftr_data->xfer.target_call_id,
                                                    FSM_TYPE_XFR)) != NULL)) {
                


                xcb = fsmxfr_get_new_xfr_context(call_id, line, FSMXFR_TYPE_XFR,
                                                 CC_XFER_METHOD_REFER,
                                                 FSMXFR_MODE_TRANSFEROR);
                if (xcb == NULL) {
                    return(SM_RC_END);
                }
               
                fcb->xcb = xcb;
                cns_fcb->xcb = xcb;
                xcb->type = FSMXFR_TYPE_DIR_XFR;

                xcb->cns_call_id = ftr_data->xfer.target_call_id;

                fcb_def = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);

                

                if (fcb_def && fcb_def->dcb) {

                    xcb->cns_line = fcb_def->dcb->line;

                } else {

                    return(SM_RC_END);
                }

                fsm_change_state(cns_fcb, __LINE__, FSMXFR_S_ACTIVE);
                fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);

                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, xcb->xfr_call_id,
                               line, CC_FEATURE_DIRTRXFR, NULL);

                return(SM_RC_END);
            }

            










            




            


            fcb_def = fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);
            if ((fcb_def != NULL) &&
                (fcb_def->state != FSMDEF_S_CONNECTED) &&
                (cause != CC_CAUSE_XFER_CNF)) {
                break;
            }

            



            if (cause != CC_CAUSE_XFER_CNF) {
                
                
                free_lines = lsm_get_instances_available_cnt(line, FALSE);
                if (free_lines <= 0) {
                    


                    fsm_display_no_free_lines();

                    break;
                }

                newcall_line = lsm_get_newcall_line(line);
                if (newcall_line == NO_LINES_AVAILABLE) {
                    


                    lsm_ui_display_notify_str_index(STR_INDEX_ERROR_PASS_LIMIT);
                    return (FALSE);
                }
            }

            



            type = ((ftr_id == CC_FEATURE_XFER) ?
                    (FSMXFR_TYPE_XFR) : (FSMXFR_TYPE_BLND_XFR));

            xcb = fsmxfr_get_new_xfr_context(call_id, line, type,
                                             CC_XFER_METHOD_REFER,
                                             FSMXFR_MODE_TRANSFEROR);
            if (xcb == NULL) {
                break;
            }
            xcb->xfr_orig = src_id;
            fcb->xcb = xcb;
            



            if (cause != CC_CAUSE_XFER_CNF) {
                


                xcb->cns_line = newcall_line;
                


                if (ftr_id == CC_FEATURE_BLIND_XFER) {
                    dcb->active_feature = ftr_id;
                }

                




                data.hold.call_info.type = CC_FEAT_HOLD;
                data.hold.call_info.data.hold_resume_reason = CC_REASON_XFER;
                data.hold.msg_body.num_parts = 0;
                data.hold.call_info.data.call_info_feat_data.protect = TRUE;
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id, dcb->line,
                               CC_FEATURE_HOLD, &data);

                cns_call_id = xcb->cns_call_id;
                if (ftr_data->xfer.cause == CC_CAUSE_XFER_LOCAL_WITH_DIALSTRING) {
                    cc_int_dialstring(CC_SRC_GSM, CC_SRC_GSM, cns_call_id, newcall_line,
                                      ftr_data->xfer.dialstring, NULL, 0);
                } else {
                    data.newcall.cause = CC_CAUSE_XFER_LOCAL;
                    if (ftr_data->xfer.dialstring[0] != 0) {
                        data.newcall.cause = CC_CAUSE_XFER_BY_REMOTE;
                        sstrncpy(data.newcall.dialstring, ftr_data->xfer.dialstring,
                                 CC_MAX_DIALSTRING_LEN);
                    }

                    if (ftr_data->xfer.global_call_id[0] != 0) {
                        sstrncpy(data.newcall.global_call_id,
                                 ftr_data->xfer.global_call_id, CC_GCID_LEN);
                    }
                    data.newcall.prim_call_id = xcb->xfr_call_id;
                    data.newcall.hold_resume_reason = CC_REASON_XFER;

                    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, cns_call_id, newcall_line,
                                   CC_FEATURE_NEW_CALL, &data);
                }
                FSM_DEBUG_SM(get_debug_string(FSMXFR_DBG_XFR_INITIATED),
                             xcb->xfr_id, call_id, cns_call_id, __LINE__);
            } else {
                other_fcb =
                    fsm_get_fcb_by_call_id_and_type(msg->data.xfer.target_call_id,
                                                    FSM_TYPE_XFR);
                if (other_fcb == NULL) {
                    GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
                    break;
                }

                other_fcb->xcb = xcb;
                



                fsmxfr_update_xfr_context(xcb, xcb->cns_call_id,
                                          msg->data.xfer.target_call_id);
                xcb->cnf_xfr = TRUE;
                fsm_change_state(other_fcb, __LINE__, FSMXFR_S_ACTIVE);
                



                cc_int_feature(CC_SRC_UI, CC_SRC_GSM, dcb->call_id,
                               dcb->line, CC_FEATURE_XFER, NULL);
            }

            fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);
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

            













            


            xcb = fsmxfr_get_xcb_by_call_id(call_id);
            if (xcb == NULL) {
                break;
            }
            fcb->xcb = xcb;

            fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);

            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            sm_rc = SM_RC_DEF_CONT;

            break;
        }                       

        break;

    case CC_SRC_SIP:
        switch (ftr_id) {
        case CC_FEATURE_BLIND_XFER:
        case CC_FEATURE_XFER:
            if (msg->data_valid == FALSE) {
                break;
            }

            method = msg->data.xfer.method;

            switch (method) {
            case CC_XFER_METHOD_BYE:
                













                


                if (msg->data.xfer.cause != CC_CAUSE_XFER_REMOTE) {
                    break;
                }

                




                



                type = ((ftr_id == CC_FEATURE_XFER) ?
                        (FSMXFR_TYPE_XFR) : (FSMXFR_TYPE_BLND_XFR));

                xcb = fsmxfr_get_new_xfr_context(call_id, line, type, method,
                                                 FSMXFR_MODE_TRANSFEREE);
                if (xcb == NULL) {
                    break;
                }
                xcb->xfr_orig = src_id;
                fcb->xcb = xcb;

                fsmxfr_set_xfer_data(CC_CAUSE_OK, method,
                                     xcb->cns_call_id,
                                     FSMXFR_NULL_DIALSTRING, &(data.xfer));

                cc_int_feature_ack(CC_SRC_GSM, CC_SRC_SIP, dcb->call_id,
                                   dcb->line, ftr_id, &data, CC_CAUSE_NORMAL);

                


                if (fsmxfr_copy_dialstring(&xcb->dialstring,
                                           msg->data.xfer.dialstring) == TRUE) {
                    fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);
                } else {
                    fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id,
                            xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                    fsmxfr_cleanup(fcb, __LINE__, TRUE);
                }

                break;

            case CC_XFER_METHOD_REFER:
                

























                




                if ((msg->data.xfer.cause == CC_CAUSE_XFER_REMOTE) &&
                    (msg->data.xfer.target_call_id != CC_NO_CALL_ID)) {
                    type = ((ftr_id == CC_FEATURE_XFER) ?
                            (FSMXFR_TYPE_XFR) : (FSMXFR_TYPE_BLND_XFR));

                    xcb = fsmxfr_get_new_xfr_context(call_id, line, type,
                                                     CC_XFER_METHOD_REFER,
                                                     FSMXFR_MODE_TARGET);
                    if (xcb == NULL) {
                        break;
                    }
                    xcb->xfr_orig = src_id;
                    fcb->xcb = xcb;

                    



                    fsmxfr_update_xfr_context(xcb, xcb->cns_call_id,
                                              msg->data.xfer.target_call_id);
                    cns_call_id = xcb->cns_call_id;
                    




                    fsmxfr_set_xfer_data(CC_CAUSE_XFER_REMOTE,
                                         method,
                                         CC_NO_CALL_ID,
                                         FSMXFR_NULL_DIALSTRING, &(data.xfer));

                    cc_int_feature_ack(CC_SRC_GSM, CC_SRC_SIP, dcb->call_id,
                                       dcb->line, ftr_id, &data,
                                       CC_CAUSE_NORMAL);

                    fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);

                    break;
                }

                




                fcb_def =
                    fsm_get_fcb_by_call_id_and_type(call_id, FSM_TYPE_DEF);
                if ((fcb_def->state == FSMDEF_S_CONNECTED) ||
                    (fcb_def->state == FSMDEF_S_CONNECTED_MEDIA_PEND) ||
                    (fcb_def->state == FSMDEF_S_RESUME_PENDING) ||
                    (fcb_def->state == FSMDEF_S_HOLD_PENDING) ||
                    (fcb_def->state == FSMDEF_S_HOLDING)) {
                    int_rc =
                        fsmxfr_remote_transfer(fcb, ftr_id, call_id, dcb->line,
                                               msg->data.xfer.dialstring,
                                               msg->data.xfer.referred_by);
                }

                if (int_rc == FALSE) {
                    fsmxfr_set_xfer_data(CC_CAUSE_ERROR, CC_XFER_METHOD_REFER,
                                         CC_NO_CALL_ID,
                                         FSMXFR_NULL_DIALSTRING, &(data.xfer));

                    cc_int_feature_ack(CC_SRC_GSM, CC_SRC_SIP, call_id,
                                       line, ftr_id, &data, CC_CAUSE_ERROR);
                }
                break;

            default:
                break;
            }                   

            break;
        case CC_FEATURE_NOTIFY:
            if (ftr_data->notify.subscription != CC_SUBSCRIPTIONS_XFER) {
                
                break;
            }
            data.notify.cause        = msg->data.notify.cause;
            data.notify.cause_code   = msg->data.notify.cause_code;
            data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
            data.notify.method       = CC_XFER_METHOD_REFER;
            data.notify.blind_xferror_gsm_id =
                msg->data.notify.blind_xferror_gsm_id;
            data.notify.final        = TRUE;

            if (data.notify.blind_xferror_gsm_id == CC_NO_CALL_ID) {
                if (msg->data.notify.cause == CC_CAUSE_OK) {
                    data.endcall.cause = CC_CAUSE_OK;
                    sm_rc = SM_RC_END;
                    







                    if (dcb) {
                        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM,
                                       dcb->call_id, dcb->line,
                                       CC_FEATURE_END_CALL, &data);
                    }
                }
            } else {
                cc_int_feature(CC_SRC_GSM, CC_SRC_SIP,
                               data.notify.blind_xferror_gsm_id,
                               line, CC_FEATURE_NOTIFY, &data);
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

static sm_rcs_t
fsmxfr_ev_idle_dialstring (sm_event_t *event)
{
    fsm_fcb_t         *fcb     = (fsm_fcb_t *) event->data;
    cc_feature_t      *msg     = (cc_feature_t *) event->msg;
    callid_t           call_id = msg->call_id;
    fsmxfr_xcb_t      *xcb;
    sm_rcs_t           sm_rc   = SM_RC_CONT;

    


    xcb = fsmxfr_get_xcb_by_call_id(call_id);
    if (xcb == NULL) {
        return sm_rc;
    }
    fcb->xcb = xcb;

    fsm_change_state(fcb, __LINE__, FSMXFR_S_ACTIVE);
    return (fsmxfr_ev_active_dialstring(event));
}




static sm_rcs_t
fsmxfr_ev_active_proceeding (sm_event_t *event)
{
    cc_proceeding_t   *msg     = (cc_proceeding_t *) event->msg;
    callid_t           call_id = msg->call_id;
    line_t             line    = msg->line;
    cc_feature_data_t  data;
    fsmxfr_xcb_t      *xcb;
    fsm_fcb_t         *other_fcb;
    callid_t           other_call_id;
    line_t             other_line;

    



    xcb = fsmxfr_get_xcb_by_call_id(call_id);
    if ((xcb == NULL) || (xcb->mode != FSMXFR_MODE_TARGET)) {
        return (SM_RC_CONT);
    }

    other_call_id = fsmxfr_get_other_call_id(xcb, call_id);
    other_line = fsmxfr_get_other_line(xcb, call_id);
    other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id, FSM_TYPE_DEF);

    







    data.endcall.cause = CC_CAUSE_REPLACE;
    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_call_id,
                   other_line, CC_FEATURE_END_CALL, &data);

    




    
    if (other_fcb && (other_fcb->old_state == FSMDEF_S_CONNECTED ||
        other_fcb->old_state == FSMDEF_S_CONNECTED_MEDIA_PEND ||
        other_fcb->old_state == FSMDEF_S_RESUME_PENDING ||
        other_fcb->state == FSMDEF_S_CONNECTED ||
        other_fcb->state == FSMDEF_S_CONNECTED_MEDIA_PEND ||
        other_fcb->state == FSMDEF_S_RESUME_PENDING)) {
        cc_int_feature(CC_SRC_UI, CC_SRC_GSM, call_id,
                       line, CC_FEATURE_ANSWER, NULL);
    }

    return (SM_RC_CONT);
}


static sm_rcs_t
fsmxfr_ev_active_connected_ack (sm_event_t *event)
{
    fsm_fcb_t          *fcb = (fsm_fcb_t *) event->data;
    cc_connected_ack_t *msg = (cc_connected_ack_t *) event->msg;
    fsmxfr_xcb_t       *xcb;

    






    


    xcb = fsmxfr_get_xcb_by_call_id(msg->call_id);
    if ((xcb == NULL) || (xcb->mode != FSMXFR_MODE_TARGET)) {
        return (SM_RC_CONT);
    }

    


    fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
            xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
    fsmxfr_cleanup(fcb, __LINE__, FALSE);

    return (SM_RC_CONT);
}


static sm_rcs_t
fsmxfr_ev_active_release (sm_event_t *event)
{
    static const char fname[] = "fsmxfr_ev_active_release";
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_release_t     *msg     = (cc_release_t *) event->msg;
    callid_t          call_id = msg->call_id;
    fsmdef_dcb_t     *dcb     = fcb->dcb;
    callid_t          new_call_id;
    callid_t          other_call_id;
    line_t            other_line;
    fsmxfr_xcb_t     *xcb     = fcb->xcb;
    cc_feature_data_t data;
    boolean           secondary = FALSE;
    cc_action_data_t  action_data;
    fsm_fcb_t        *other_fcb;
    FSM_DEBUG_SM(DEB_F_PREFIX"Entered. \n", DEB_F_PREFIX_ARGS(FSM, "fsmxfr_ev_active_release"));

    


    memset(&data, 0, sizeof(cc_feature_data_t));

    


    if (xcb == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find a transfer call to cancel.\n", fname);
        return (SM_RC_CONT);
    }

    if ((xcb->active == TRUE) && (xcb->xcb2 != NULL)) {
        xcb = xcb->xcb2;
        secondary = TRUE;
    }

    if ((xcb->dialstring != NULL) && (xcb->dialstring[0] != '\0')) {
        





        if (xcb->active == TRUE) {
            new_call_id = cc_get_new_call_id();
            fsmxfr_update_xfr_context(xcb, call_id, new_call_id);
        } else {
            new_call_id = fsmxfr_get_other_call_id(xcb, call_id);
            if (secondary == TRUE) {
                fsmxfr_update_xfr_context(fcb->xcb, call_id, new_call_id);
            }
        }

        data.newcall.cause = CC_CAUSE_XFER_REMOTE;
        sstrncpy(data.newcall.dialstring, xcb->dialstring,
                 CC_MAX_DIALSTRING_LEN);

        cpr_free(xcb->dialstring);
        xcb->dialstring = NULL;
        memset(data.newcall.redirect.redirects[0].number, 0,
               sizeof(CC_MAX_DIALSTRING_LEN));
        if (xcb->referred_by != NULL) {
            sstrncpy(data.newcall.redirect.redirects[0].number,
                     xcb->referred_by, CC_MAX_DIALSTRING_LEN);

            cpr_free(xcb->referred_by);
            xcb->referred_by = NULL;
        }

        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, new_call_id,
                       dcb->line, CC_FEATURE_NEW_CALL, &data);

        FSM_DEBUG_SM(get_debug_string(FSMXFR_DBG_XFR_INITIATED), xcb->xfr_id,
                     xcb->xfr_call_id, xcb->cns_call_id, __LINE__);

        if (secondary == TRUE) {
            fsmxfr_init_xcb(xcb);
            fcb->xcb->active = FALSE;
            fcb->xcb->xcb2 = NULL;
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
            fsmxfr_cleanup(fcb, __LINE__, FALSE);
        } else if (xcb->active == TRUE) {
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
            fsmxfr_cleanup(fcb, __LINE__, FALSE);
            fcb->xcb->active = FALSE;
        } else {
            




            fsmxfr_update_xfr_context(xcb, new_call_id, CC_NO_CALL_ID);
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
            fsmxfr_cleanup(fcb, __LINE__, TRUE);
        }

        xcb->active = FALSE;
    } else {
        if (secondary == TRUE) {
            











            new_call_id = fsmxfr_get_other_call_id(xcb, call_id);
            fsmxfr_update_xfr_context(fcb->xcb, call_id, new_call_id);

            fcb->xcb->active = FALSE;

            other_fcb = fsm_get_fcb_by_call_id_and_type(new_call_id,
                                                        FSM_TYPE_XFR);
            if (other_fcb == NULL) {
                return (SM_RC_CONT);
            }
            other_fcb->xcb = fcb->xcb;

            fsmxfr_init_xcb(xcb);

            action_data.update_ui.action = CC_UPDATE_XFER_PRIMARY;
            (void)cc_call_action(other_fcb->dcb->call_id, dcb->line,
                                 CC_ACTION_UPDATE_UI, &action_data);

            fsmxfr_cleanup(fcb, __LINE__, FALSE);
        } else {
            



            other_call_id = fsmxfr_get_other_call_id(xcb, call_id);
            other_line = fsmxfr_get_other_line(xcb, call_id);

            other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                        FSM_TYPE_XFR);
            if (xcb->cnf_xfr) {
                



                xcb->cnf_xfr = FALSE;
                if (other_fcb == NULL) {
                    return (SM_RC_CONT);
                }
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_call_id,
                               other_line, CC_FEATURE_END_CALL, NULL);
            }

            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, xcb->cns_call_id,
                        CC_SK_EVT_TYPE_IMPLI);
            fsmxfr_cleanup(fcb, __LINE__, TRUE);

        }
    }

    return (SM_RC_CONT);
}


static sm_rcs_t
fsmxfr_ev_active_release_complete (sm_event_t *event)
{
    fsmxfr_cleanup((fsm_fcb_t *) event->data, __LINE__, TRUE);

    return (SM_RC_CONT);
}

static char *fsmxfr_get_dialed_num (fsmdef_dcb_t *dcb)
{
    static const char fname[] = "fsmxfr_get_dialed_num";
     char             *tmp_called_number;
    

    tmp_called_number = lsm_get_gdialed_digits();

    DEF_DEBUG(DEB_F_PREFIX"called_dialed_num = %s\n", 
                DEB_F_PREFIX_ARGS(GSM, fname), tmp_called_number);

    


    if (tmp_called_number == NULL || (*tmp_called_number) == NUL) {

        if (dcb->caller_id.called_number[0] != NUL) {
            DEF_DEBUG(DEB_F_PREFIX"called_dcb_num = %s\n", 
                DEB_F_PREFIX_ARGS(GSM, fname), (char *)dcb->caller_id.called_number);
            return((char *)dcb->caller_id.called_number);
            
        } else {
            DEF_DEBUG(DEB_F_PREFIX"calling_dcb_num = %s\n", 
                DEB_F_PREFIX_ARGS(GSM, fname), (char *)dcb->caller_id.calling_number);
            return((char *)dcb->caller_id.calling_number);
        }
    }

    



    if (dcb->caller_id.called_number != NULL &&
        dcb->caller_id.called_number[0] != NUL) {
        
        if (strncmp(tmp_called_number, CC_CISCO_PLAR_STRING, sizeof(CC_CISCO_PLAR_STRING)) == 0) {
            tmp_called_number = (char *)dcb->caller_id.called_number;
        }
    }

    return(tmp_called_number);
}

static void
fsmxfr_initiate_xfr (sm_event_t *event)
{
    static const char fname[] = "fsmxfr_initiate_xfr";
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    fsm_fcb_t        *cns_fcb = NULL;
    fsmdef_dcb_t     *dcb     = fcb->dcb;
    fsmdef_dcb_t     *xfr_dcb;
    fsmdef_dcb_t     *cns_dcb;
    cc_feature_data_t data;
    fsmxfr_xcb_t     *xcb     = fcb->xcb;
    char             *called_num = NULL;
 
    


    if (xcb == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
        return;
    }

    cns_dcb = fsm_get_dcb(xcb->cns_call_id);
    cns_fcb = fsm_get_fcb_by_call_id_and_type(xcb->cns_call_id,
                                              FSM_TYPE_DEF);
    xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);

    



    if (cns_fcb != NULL) {
        







        if ((cns_fcb->state == FSMDEF_S_COLLECT_INFO) ||
            (cns_fcb->state == FSMDEF_S_OUTGOING_PROCEEDING) ||
            (cns_fcb->state == FSMDEF_S_KPML_COLLECT_INFO)) {
            FSM_DEBUG_SM(DEB_L_C_F_PREFIX"Ignore the xfer xid %d cid %d %d\n",
				DEB_L_C_F_PREFIX_ARGS(FSM, xcb->xfr_line, xcb->xfr_call_id, "fsmxfr_initiate_xfr"),
                         xcb->xfr_id, xcb->xfr_call_id, xcb->cns_call_id);
            return;
        }

        


        xcb->xfer_comp_req = TRUE;

        if (cns_fcb->state < FSMDEF_S_CONNECTED) {
            data.endcall.cause = CC_CAUSE_NO_USER_RESP;
            cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, cns_dcb->call_id,
                           cns_dcb->line, CC_FEATURE_END_CALL, &data);
            


            called_num = fsmxfr_get_dialed_num(cns_dcb);
            if (called_num && called_num[0] != '\0') {

                fsmxfr_set_xfer_data(CC_CAUSE_XFER_LOCAL,
                                     xcb->method, cns_dcb->call_id,
                                     called_num,
                                     &(data.xfer));

                cc_int_feature(CC_SRC_GSM, CC_SRC_SIP, xfr_dcb->call_id,
                               xfr_dcb->line, CC_FEATURE_XFER, &data);
            } else {
                



                fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                fsmxfr_cleanup(fcb, __LINE__, TRUE);
                if (xcb->cnf_xfr) {
                    



                    fsmxfr_cnf_cleanup(xcb);
                }
            }
        } else {
            







            data.hold.call_info.type = CC_FEAT_NONE;
            data.hold.msg_body.num_parts = 0;
            if (((cns_fcb->state == FSMDEF_S_HOLDING) ||
                (cns_fcb->state == FSMDEF_S_HOLD_PENDING)) &&
                ((fcb->state != FSMDEF_S_HOLDING) &&
                (fcb->state != FSMDEF_S_HOLD_PENDING))) {
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id, dcb->line,
                               CC_FEATURE_HOLD, &data);
            } else {
                
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM,
                               cns_dcb->call_id, cns_dcb->line,
                               CC_FEATURE_HOLD, &data);
            }
        }
    }
}

static sm_rcs_t
fsmxfr_ev_active_feature (sm_event_t *event)
{
    static const char fname[] = "fsmxfr_ev_active_feature";
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_feature_t     *msg     = (cc_feature_t *) event->msg;
    callid_t          call_id = msg->call_id;
    line_t            line    = msg->line;
    fsmdef_dcb_t     *dcb     = fcb->dcb;
    cc_srcs_t         src_id  = msg->src_id;
    cc_features_t     ftr_id  = msg->feature_id;
    cc_feature_data_t *feat_data   = &(msg->data);
    fsmdef_dcb_t     *xfr_dcb, *cns_dcb;
    cc_feature_data_t data;
    sm_rcs_t          sm_rc   = SM_RC_CONT;
    fsmxfr_xcb_t     *xcb     = fcb->xcb;
    boolean           int_rc;
    char              tmp_str[STATUS_LINE_MAX_LEN];
    char             *called_num = NULL;
    fsm_fcb_t        *cns_fcb = NULL;

    fsm_sm_ftr(ftr_id, src_id);

    if (xcb == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
        return (SM_RC_CONT);
    }

    



    if ((ftr_id == CC_FEATURE_BLIND_XFER) ||
        (ftr_id == CC_FEATURE_XFER) || (ftr_id == CC_FEATURE_NOTIFY)) {
        if (ftr_id == CC_FEATURE_NOTIFY) {
            if (msg->data_valid &&
                (msg->data.notify.subscription != CC_SUBSCRIPTIONS_XFER)) {
                
                return (SM_RC_CONT);
            }
        }
        sm_rc = SM_RC_END;
    }

    switch (src_id) {
    case CC_SRC_UI:
        switch (ftr_id) {
        case CC_FEATURE_CANCEL:
            sm_rc = SM_RC_END;
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, xcb->cns_call_id,
                            CC_SK_EVT_TYPE_EXPLI);
            fsmxfr_cleanup(fcb, __LINE__, TRUE);
            break;

        case CC_FEATURE_XFER:
            




            DEF_DEBUG(DEB_F_PREFIX"ACTIVE XFER call_id = %d, cns_id = %d, t_id=%d\n", 
                DEB_F_PREFIX_ARGS(GSM, fname), xcb->xfr_call_id, 
                    feat_data->xfer.target_call_id, xcb->cns_call_id);
            
            if (feat_data && msg->data_valid &&
                (xcb->cns_call_id != feat_data->xfer.target_call_id)) {

                cns_fcb = fsm_get_fcb_by_call_id_and_type(xcb->cns_call_id,
                        FSM_TYPE_DEF);

                if (cns_fcb != NULL) {
                    DEF_DEBUG(DEB_F_PREFIX"INVOKE ACTIVE XFER call_id = %d, t_id=%d\n", 
                        DEB_F_PREFIX_ARGS(GSM, fname), xcb->xfr_call_id, 
                        feat_data->xfer.target_call_id);
                    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, xcb->xfr_call_id,
                               xcb->xfr_line, CC_FEATURE_DIRTRXFR, NULL);
                }

                return(SM_RC_END);
            }
            switch (xcb->method) {
            case CC_XFER_METHOD_BYE:
            case CC_XFER_METHOD_REFER:
                







                fsmxfr_initiate_xfr(event);
                lsm_set_hold_ringback_status(xcb->cns_call_id, FALSE);
                break;
             
            default:
                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                break;
            }                   
            break;
        case CC_FEATURE_RESUME:
            break;

        case CC_FEATURE_END_CALL:
            if (xcb->mode == FSMXFR_MODE_TRANSFEREE) {
                xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);
                if (call_id == xcb->cns_call_id) {
                    




                    data.notify.cause        = CC_CAUSE_ERROR;
                    data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
                    data.notify.method       = CC_XFER_METHOD_REFER;
                    data.notify.final        = TRUE;
                    cc_int_feature(CC_SRC_GSM, CC_SRC_SIP, xfr_dcb->call_id,
                                   xfr_dcb->line, CC_FEATURE_NOTIFY, &data);
                }
            }
            lsm_set_hold_ringback_status(xcb->cns_call_id, TRUE);
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, xcb->cns_call_id,
                                CC_SK_EVT_TYPE_IMPLI);
            fsmxfr_cleanup(fcb, __LINE__, TRUE);
            break;

        case CC_FEATURE_HOLD:
            if ((msg->data_valid) &&
                (feat_data->hold.call_info.data.hold_resume_reason == CC_REASON_SWAP ||
                feat_data->hold.call_info.data.hold_resume_reason == CC_REASON_XFER ||
                feat_data->hold.call_info.data.hold_resume_reason == CC_REASON_INTERNAL))
            {
                feat_data->hold.call_info.data.call_info_feat_data.protect = TRUE;
            } else {
                DEF_DEBUG(DEB_F_PREFIX"Invoke hold call_id = %d t_call_id=%d\n", 
                        DEB_F_PREFIX_ARGS(GSM, fname), xcb->xfr_call_id, xcb->cns_call_id);
                
                ui_terminate_feature(xcb->xfr_line, xcb->xfr_call_id, xcb->cns_call_id);
                fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id,
                            xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                fsmxfr_cleanup(fcb, __LINE__, TRUE);
            }
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            break;
        }                       

        break;

    case CC_SRC_GSM:
        switch (ftr_id) {
        case CC_FEATURE_DIRTRXFR:
            xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);
            called_num = fsmxfr_get_dialed_num(xfr_dcb);

            if (called_num && called_num[0] != '\0') {
                fsmxfr_set_xfer_data(CC_CAUSE_XFER_LOCAL,
                        xcb->method, xcb->cns_call_id,
                        called_num,
                        &(data.xfer));

                data.xfer.method = CC_XFER_METHOD_DIRXFR;

                cc_int_feature(CC_SRC_GSM, CC_SRC_SIP,
                        call_id, line,
                        CC_FEATURE_XFER, &data);
            } else {
                



                fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                fsmxfr_cleanup(fcb, __LINE__, TRUE);
            }
            return(SM_RC_END);
        case CC_FEATURE_END_CALL:
            






            if (xcb->type == FSMXFR_TYPE_BLND_XFR) {
                fsmxfr_cleanup(fcb, __LINE__, FALSE);
            } else if ((xcb->type == FSMXFR_TYPE_XFR) &&
                       (msg->data.endcall.cause == CC_CAUSE_NO_USER_RESP)) {

                DEF_DEBUG(DEB_F_PREFIX"Xfer type =%d\n", 
                        DEB_F_PREFIX_ARGS(GSM, fname), xcb->type);

                if ((platGetPhraseText(STR_INDEX_TRANSFERRING,
                                             (char *) tmp_str,
                                             STATUS_LINE_MAX_LEN - 1)) == CPR_SUCCESS) {
                    lsm_ui_display_status(tmp_str, xcb->xfr_line, xcb->xfr_call_id);
                }

                if (xcb->xfer_comp_req == FALSE) {
                    fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                } else {
                    
                    fsmxfr_mark_dcb_for_xfr_complete(xcb->cns_call_id,
                                    xcb->xfr_call_id, TRUE);
                }
                fsmxfr_cleanup(fcb, __LINE__, FALSE);
            } else if (xcb->mode == FSMXFR_MODE_TARGET) {
                break;
            } else {
                


                if (xcb->xfer_comp_req == FALSE) {
                    fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                }
                fsmxfr_cleanup(fcb, __LINE__, TRUE);
            }
            break;

        case CC_FEATURE_HOLD:
            ui_set_local_hold(dcb->line, dcb->call_id);
            if(msg->data_valid) {
                feat_data->hold.call_info.data.call_info_feat_data.protect = TRUE;
            }
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            break;

        default:
            fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
            break;
        }                       
        break;

    case CC_SRC_SIP:
        switch (ftr_id) {
        case CC_FEATURE_CALL_PRESERVATION:
            DEF_DEBUG(DEB_F_PREFIX"Preservation call_id = %d t_call_id=%d\n", 
                        DEB_F_PREFIX_ARGS(GSM, fname), xcb->xfr_call_id, xcb->cns_call_id);
            ui_terminate_feature(xcb->xfr_line, xcb->xfr_call_id, xcb->cns_call_id);
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id,
                            xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
            fsmxfr_cleanup(fcb, __LINE__, TRUE);
            break;

        case CC_FEATURE_BLIND_XFER:
        case CC_FEATURE_XFER:
            if (msg->data_valid == FALSE) {
                break;
            }

            


            if (msg->data.xfer.cause != CC_CAUSE_XFER_REMOTE) {
                break;
            }

            




            switch (msg->data.xfer.method) {
            case CC_XFER_METHOD_BYE:
                




                fsmxfr_set_xfer_data(CC_CAUSE_OK, CC_XFER_METHOD_BYE,
                                     CC_NO_CALL_ID,
                                     FSMXFR_NULL_DIALSTRING, &(data.xfer));

                cc_int_feature_ack(CC_SRC_GSM, CC_SRC_SIP, dcb->call_id,
                                   dcb->line, ftr_id, &data, CC_CAUSE_NORMAL);

                


                if (fsmxfr_copy_dialstring(&xcb->dialstring,
                                           msg->data.xfer.dialstring) == FALSE) {
                    fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, xcb->cns_call_id,
                                CC_SK_EVT_TYPE_IMPLI);
                    fsmxfr_cleanup(fcb, __LINE__, TRUE);
                    break;
                }

                




                xcb->active = TRUE;

                break;

            case CC_XFER_METHOD_REFER:
                int_rc = fsmxfr_remote_transfer(fcb, ftr_id, call_id, dcb->line,
                                                msg->data.xfer.dialstring,
                                                msg->data.xfer.referred_by);

                if (int_rc == FALSE) {
                    fsmxfr_set_xfer_data(CC_CAUSE_ERROR, CC_XFER_METHOD_REFER,
                                         CC_NO_CALL_ID,
                                         FSMXFR_NULL_DIALSTRING, &(data.xfer));

                    cc_int_feature_ack(CC_SRC_GSM, CC_SRC_SIP, call_id,
                                       dcb->line, ftr_id, &data,
                                       CC_CAUSE_ERROR);
                }
                break;

            default:
                break;
            }                   

            break;

        case CC_FEATURE_NOTIFY:
            











            if (msg->data_valid == FALSE) {
                break;
            }

            


            cc_int_feature_ack(CC_SRC_GSM, CC_SRC_SIP, dcb->call_id,
                               dcb->line, CC_FEATURE_NOTIFY, NULL,
                               CC_CAUSE_NORMAL);

            switch (xcb->type) {
            case FSMXFR_TYPE_BLND_XFR:
                switch (msg->data.notify.method) {
                case CC_XFER_METHOD_BYE:
                    






                    


                    data.endcall.cause = CC_CAUSE_OK;
                    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id,
                                   dcb->line, CC_FEATURE_END_CALL, &data);
                    break;

                case CC_XFER_METHOD_REFER:
                    if (msg->data.notify.cause == CC_CAUSE_OK) {
                        


                        


                        fsmxfr_mark_dcb_for_xfr_complete(xcb->cns_call_id,
                                    xcb->xfr_call_id, TRUE);

                        data.endcall.cause = CC_CAUSE_OK;
                        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id,
                                       dcb->line, CC_FEATURE_END_CALL, &data);
                    } else {
                        fsmxfr_cleanup(fcb, __LINE__, TRUE);
                        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, dcb->call_id,
                                       dcb->line, CC_FEATURE_RESUME, NULL);
                    }
                    break;

                default:
                    break;
                }               

                break;
            case FSMXFR_TYPE_DIR_XFR:
                




                xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);
                if (msg->data.notify.cause == CC_CAUSE_OK) {
                        data.endcall.cause = CC_CAUSE_OK;

                    fsmxfr_mark_dcb_for_xfr_complete(xcb->cns_call_id,
                                    xcb->xfr_call_id, TRUE);
                    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM,
                        xfr_dcb->call_id,
                        xfr_dcb->line, CC_FEATURE_END_CALL,
                        &data);
                } else {
                    lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                            dcb->line, xcb->xfr_call_id);
                    lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                            dcb->line, xcb->cns_call_id);
                }
                if (xcb->cnf_xfr) {
                    



                    fsmxfr_cnf_cleanup(xcb);
                }
                break;

            case FSMXFR_TYPE_XFR:
                switch (msg->data.notify.method) {
                case CC_XFER_METHOD_BYE:
                    


                    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, xcb->cns_call_id,
                                   dcb->line, CC_FEATURE_END_CALL, NULL);

                    


                    cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, xcb->xfr_call_id,
                                   dcb->line, CC_FEATURE_END_CALL, NULL);
                    break;

                case CC_XFER_METHOD_REFER:
                    




















                    xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);
                    if (call_id == xcb->cns_call_id) {
                        



                        data.notify.cause        = msg->data.notify.cause;
                        data.notify.cause_code   = msg->data.notify.cause_code;
                        data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
                        data.notify.method       = CC_XFER_METHOD_REFER;
                        data.notify.blind_xferror_gsm_id =
                            msg->data.notify.blind_xferror_gsm_id;
                        data.notify.final        = TRUE;
                        if (data.notify.blind_xferror_gsm_id == CC_NO_CALL_ID) {
                            cc_int_feature(CC_SRC_GSM, CC_SRC_SIP,
                                           xfr_dcb->call_id, xfr_dcb->line,
                                           CC_FEATURE_NOTIFY, &data);
                        } else {
                            cc_int_feature(CC_SRC_GSM, CC_SRC_SIP,
                                           data.notify.blind_xferror_gsm_id,
                                           msg->line, CC_FEATURE_NOTIFY, &data);
                        }
                    } else {
                        




                        if (xcb == NULL) {
                            GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
                            break;
                        }

                        if (msg->data.notify.cause == CC_CAUSE_OK) {
                            data.endcall.cause = CC_CAUSE_OK;

                            fsmxfr_mark_dcb_for_xfr_complete(xcb->cns_call_id,
                                        xcb->xfr_call_id, TRUE);
                            cc_int_feature(CC_SRC_GSM, CC_SRC_GSM,
                                           xfr_dcb->call_id,
                                           xfr_dcb->line, CC_FEATURE_END_CALL,
                                           &data);
                        } else {
                            lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                                                  dcb->line, xcb->xfr_call_id);
                            fsmxfr_mark_dcb_for_xfr_complete(xcb->cns_call_id,
                                        xcb->xfr_call_id, FALSE);
                            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                                      xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                            



                            if (xcb->cns_call_id != CC_NO_CALL_ID) {
                               	lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                                                      dcb->line, xcb->cns_call_id);
                                cns_dcb = fsm_get_dcb (xcb->cns_call_id);
                                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, 
                                               xcb->cns_call_id, cns_dcb->line,
                                               CC_FEATURE_RESUME, NULL);
                            }

                            if (xcb->cnf_xfr) {
                                



                                fsmxfr_cnf_cleanup(xcb);
                            }
                        }
                        fsmxfr_cleanup(fcb, __LINE__, TRUE);
                    }           
                    break;

                default:
                    break;
                }               

                break;

            default:
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

static void
fsmxfr_requeue_blind_xfer_dialstring (fsmdef_dcb_t *dcb, fsmxfr_xcb_t *xcb)
{
    



    dcb->active_feature = CC_FEATURE_NONE;

    







    if (xcb->queued_dialstring && xcb->queued_dialstring[0] != '\0') {
        cc_dialstring(CC_SRC_UI, xcb->cns_call_id, dcb->line,
                      xcb->queued_dialstring);
        cpr_free(xcb->queued_dialstring);
        xcb->queued_dialstring = NULL;
    }
}

static sm_rcs_t
fsmxfr_ev_active_feature_ack (sm_event_t *event)
{
    static const char fname[] = "fsmxfr_ev_active_feature_ack";
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_feature_ack_t *msg     = (cc_feature_ack_t *) event->msg;
    cc_srcs_t         src_id  = msg->src_id;
    cc_features_t     ftr_id  = msg->feature_id;
    fsmdef_dcb_t     *dcb     = fcb->dcb;
    cc_feature_data_t data;
    sm_rcs_t          sm_rc   = SM_RC_CONT;
    fsmxfr_xcb_t     *xcb     = fcb->xcb;
    fsmdef_dcb_t     *xfr_dcb = NULL;
    char             *called_num = NULL;

    fsm_sm_ftr(ftr_id, src_id);

    


    if ((ftr_id == CC_FEATURE_BLIND_XFER) || (ftr_id == CC_FEATURE_XFER)) {
        sm_rc = SM_RC_END;
    }

    if (xcb == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
        return (sm_rc);
    }

    switch (src_id) {
    case CC_SRC_SIP:
    case CC_SRC_GSM:
        switch (ftr_id) {
        case CC_FEATURE_BLIND_XFER:
            switch (xcb->type) {
            case FSMXFR_TYPE_BLND_XFR:
                switch (msg->data.xfer.method) {
                case CC_XFER_METHOD_REFER:
                    



                    if (msg->cause == CC_CAUSE_OK) {
                        
                        

                        
                                       
                    } else {
                        fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                            xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                        fsmxfr_cleanup(fcb, __LINE__, TRUE);
                    }
                    break;

                default:
                    fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                    break;
                }               

                break;

            default:
                break;
            }                   
            break;

        case CC_FEATURE_HOLD:
            if (msg->cause == CC_CAUSE_REQUEST_PENDING) {
                



                fsm_sm_ignore_ftr(fcb, __LINE__, ftr_id);
                break;
            }

            



            if (xcb->type == FSMXFR_TYPE_BLND_XFR &&
                xcb->xfr_call_id == fcb->call_id) {
                fsmxfr_requeue_blind_xfer_dialstring(dcb, xcb);
                break;
            }

            



            if (!xcb->xfer_comp_req) {
                break;
            }


            




            if (msg->cause == CC_CAUSE_ERROR) {

                lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                                      xcb->xfr_line, xcb->xfr_call_id);
                lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                                      xcb->cns_line, xcb->cns_call_id);
                fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, xcb->cns_call_id,
                                CC_SK_EVT_TYPE_IMPLI);
                fsmxfr_cleanup(fcb, __LINE__, TRUE);

                break;
            }

            


            if (xcb->type == FSMXFR_TYPE_XFR) {
                





                if (xcb->cns_call_id == fcb->call_id) {
                    xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);

                    called_num = fsmxfr_get_dialed_num(fcb->dcb);

                    if (called_num && called_num[0] != '\0') {
                        fsmxfr_set_xfer_data(CC_CAUSE_XFER_LOCAL,
                                             xcb->method, fcb->dcb->call_id,
                                             called_num,
                                             &(data.xfer));
                        cc_int_feature(CC_SRC_GSM, CC_SRC_SIP, xfr_dcb->call_id,
                                       xfr_dcb->line, CC_FEATURE_XFER, &data);
                    } else {
                        



                        fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                                xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                        fsmxfr_cleanup(fcb, __LINE__, TRUE);
                    }
                } else {
                    






                    
                    
                    

                    


                    xfr_dcb = fsm_get_dcb(xcb->cns_call_id);

                    






                    if (xfr_dcb->fcb->state != FSMDEF_S_HOLDING) {
                        break;
                    } else {

                        called_num = fsmxfr_get_dialed_num(xfr_dcb);

                        if (called_num && called_num[0] != '\0') {
                            fsmxfr_set_xfer_data(CC_CAUSE_XFER_LOCAL,
                                                 xcb->method, xfr_dcb->call_id,
                                                 called_num,
                                                 &(data.xfer));
                            cc_int_feature(CC_SRC_GSM, CC_SRC_SIP,
                                           fcb->dcb->call_id, fcb->dcb->line,
                                           CC_FEATURE_XFER, &data);
                        } else {
                            



                            fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                            fsmxfr_cleanup(fcb, __LINE__, TRUE);
                        }
                    }
                }
            }
            break;

        case CC_FEATURE_XFER:

            if (msg->cause != CC_CAUSE_OK) {
                lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                                      xcb->xfr_line, xcb->xfr_call_id);
                lsm_ui_display_status(platform_get_phrase_index_str(TRANSFER_FAILED),
                                      xcb->cns_line, xcb->cns_call_id);
                fsmxfr_feature_cancel(xcb, xcb->xfr_line, xcb->xfr_call_id, 
                                    xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                fsmxfr_cleanup(fcb, __LINE__, TRUE);
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
        fsm_sm_ignore_src(fcb, __LINE__, src_id);

        break;
    }                           

    return (sm_rc);
}


static sm_rcs_t
fsmxfr_ev_active_onhook (sm_event_t *event)
{
    static const char fname[] = "fsmxfr_ev_active_onhook";
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_onhook_t      *msg     = (cc_onhook_t *) event->msg;
    callid_t          call_id = msg->call_id;
    callid_t          other_call_id;
    fsmxfr_xcb_t     *xcb     = fcb->xcb;
    fsm_fcb_t        *other_fcb;
    fsmdef_dcb_t     *xfr_dcb;
    cc_feature_data_t data;
    fsm_fcb_t        *cns_fcb, *xfr_fcb;
    int               onhook_xfer = 0;

    if (xcb == NULL) {
        GSM_DEBUG_ERROR(GSM_F_PREFIX"Cannot find the active xfer\n", fname);
        return (SM_RC_CONT);
    }

    cns_fcb = fsm_get_fcb_by_call_id_and_type(xcb->cns_call_id, FSM_TYPE_DEF);
    xfr_fcb = fsm_get_fcb_by_call_id_and_type(xcb->xfr_call_id, FSM_TYPE_DEF);

    if (xcb->cnf_xfr) {
        



        xcb->cnf_xfr = FALSE;
        other_call_id = fsmxfr_get_other_call_id(xcb, call_id);
        other_fcb = fsm_get_fcb_by_call_id_and_type(other_call_id,
                                                    FSM_TYPE_XFR);
        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, other_call_id,
                       other_fcb ? other_fcb->dcb->line:CC_NO_LINE, CC_FEATURE_END_CALL, NULL);
        fsmxfr_cleanup(fcb, __LINE__, TRUE);
        return (SM_RC_CONT);
    }

    if (xcb->mode == FSMXFR_MODE_TRANSFEREE) {
        xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);
        if (call_id == xcb->cns_call_id) {
            











            data.notify.cause        = CC_CAUSE_ERROR;
            data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
            data.notify.method       = CC_XFER_METHOD_REFER;
            data.notify.final        = TRUE;
            cc_int_feature(CC_SRC_GSM, CC_SRC_SIP, xfr_dcb->call_id,
                           xfr_dcb->line, CC_FEATURE_NOTIFY, &data);
            if (cns_fcb && cns_fcb->state != FSMDEF_S_HOLDING &&
                cns_fcb->state != FSMDEF_S_HOLD_PENDING) {
                fsmxfr_feature_cancel(xcb, xfr_dcb->line, 
                                    xcb->xfr_call_id, xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                                        
                fsmxfr_cleanup(fcb, __LINE__, TRUE);
            }
            


            if( xfr_dcb->fcb->state == FSMDEF_S_HOLDING )
                cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, xfr_dcb->call_id,xfr_dcb->line, CC_FEATURE_END_CALL, NULL);
            return (SM_RC_CONT);
        }
    }

    if (msg->softkey) {
        



        if ((call_id == xcb->cns_call_id) &&
            (cns_fcb->state == FSMDEF_S_HOLDING ||
             cns_fcb->state == FSMDEF_S_HOLD_PENDING)) {
            
        } if (msg->active_list == CC_REASON_ACTIVECALL_LIST) {
            



            xcb->cns_call_id = CC_NO_CALL_ID;
            xcb->cns_line = CC_NO_LINE;

        }else {
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, 
                                    xcb->xfr_call_id, xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                                        
            fsmxfr_cleanup(fcb, __LINE__, TRUE);
        }
        return (SM_RC_CONT);
    } else {
        




        config_get_value(CFGID_XFR_ONHOOK_ENABLED, &onhook_xfer,
                         sizeof(onhook_xfer));
        if (onhook_xfer && ((cns_fcb->state == FSMDEF_S_OUTGOING_ALERTING)||
            (cns_fcb->state == FSMDEF_S_CONNECTED))) {
            fsmxfr_initiate_xfr(event);
            return (SM_RC_END);
        } else if (onhook_xfer && xfr_fcb && 
                ((xfr_fcb->state == FSMDEF_S_OUTGOING_ALERTING)||
                (xfr_fcb->state == FSMDEF_S_CONNECTED))) {
            fsmxfr_initiate_xfr(event);
            return (SM_RC_END);
        } else {
            fsmxfr_feature_cancel(xcb, xcb->xfr_line, 
                                    xcb->xfr_call_id, xcb->cns_call_id, CC_SK_EVT_TYPE_IMPLI);
                                        
            fsmxfr_cleanup(fcb, __LINE__, TRUE);
            return (SM_RC_CONT);
        }
    }
}





static sm_rcs_t
fsmxfr_ev_active_dialstring (sm_event_t *event)
{
    static const char fname[] = "fsmxfr_ev_active_dialstring";
    fsm_fcb_t        *fcb     = (fsm_fcb_t *) event->data;
    cc_dialstring_t  *msg     = (cc_dialstring_t *) event->msg;
    callid_t          call_id = msg->call_id;
    line_t             line    = msg->line;
    fsmdef_dcb_t     *xfr_dcb;
    fsmxfr_xcb_t     *xcb     = fcb->xcb;
    cc_feature_data_t data;
    char             *dialstring;

    


    dialstring = msg->dialstring;
    if ((dialstring == NULL) || (dialstring[0] == '\0')) {
        FSM_DEBUG_SM(DEB_L_C_F_PREFIX"dialstring= %c\n",
			DEB_L_C_F_PREFIX_ARGS(FSM, msg->line, call_id, fname), '\0');
        return (SM_RC_END);
    }

    FSM_DEBUG_SM(DEB_L_C_F_PREFIX"dialstring= %s\n", 
		DEB_L_C_F_PREFIX_ARGS(FSM, msg->line, call_id, fname), dialstring);

    







    if (xcb == NULL) {
        return (SM_RC_END);
    }

    xfr_dcb = fsm_get_dcb(xcb->xfr_call_id);
    if (xfr_dcb == NULL) {
        return (SM_RC_END);
    }

    if (xfr_dcb->active_feature == CC_FEATURE_BLIND_XFER) {
        if (!fsmxfr_copy_dialstring(&xcb->queued_dialstring, dialstring)) {
            GSM_DEBUG_ERROR(GSM_L_C_F_PREFIX"unable to copy dialstring\n",
                            msg->line, call_id, fname);
        }
        return (SM_RC_END);
    }

    if (xcb->type == FSMXFR_TYPE_BLND_XFR) {
        lsm_set_hold_ringback_status(xcb->cns_call_id, FALSE);
    }
    






    if ((xcb->cns_call_id != call_id) || (xcb->type != FSMXFR_TYPE_BLND_XFR)) {
        return (SM_RC_CONT);
    }

    switch (xcb->method) {
    case CC_XFER_METHOD_BYE:
    case CC_XFER_METHOD_REFER:
        





        


        data.endcall.cause = CC_CAUSE_NORMAL;
        cc_int_feature(CC_SRC_GSM, CC_SRC_GSM, call_id,
                       line, CC_FEATURE_END_CALL, &data);
        

        



        fsmxfr_set_xfer_data(CC_CAUSE_XFER_LOCAL, xcb->method, CC_NO_CALL_ID,
                             dialstring, &(data.xfer));

        cc_int_feature(CC_SRC_GSM, CC_SRC_SIP, xcb->xfr_call_id,
                       line, fsmxfr_type_to_feature(xcb->type), &data);

        FSM_DEBUG_SM(get_debug_string(FSMXFR_DBG_XFR_INITIATED),
                     xcb->xfr_id, xcb->xfr_call_id, xcb->cns_call_id, __LINE__);

        break;

    default:
        break;
    }                           

    return (SM_RC_END);
}


cc_int32_t
fsmxfr_show_cmd (cc_int32_t argc, const char *argv[])
{
    fsmxfr_xcb_t   *xcb;
    int             i = 0;

    


    if ((argc == 2) && (argv[1][0] == '?')) {
        debugif_printf("show fsmxfr\n");
        return (0);
    }

    debugif_printf("\n------------------------ FSMXFR xcbs -------------------------");
    debugif_printf("\ni   xfr_id  xcb         type  method  xfr_call_id  cns_call_id");
    debugif_printf("\n--------------------------------------------------------------\n");

    FSM_FOR_ALL_CBS(xcb, fsmxfr_xcbs, FSMXFR_MAX_XCBS) {
        debugif_printf("%-2d  %-6d  0x%8p  %-4d  %-6d  %-11d  %-11d\n",
                       i++, xcb->xfr_id, xcb, xcb->type, xcb->method,
                       xcb->xfr_call_id, xcb->cns_call_id);
    }

    return (0);
}


void
fsmxfr_init (void)
{
    fsmxfr_xcb_t *xcb;


    


    fsmxfr_xcbs = (fsmxfr_xcb_t *)
        cpr_calloc(FSMXFR_MAX_XCBS, sizeof(fsmxfr_xcb_t));

    FSM_FOR_ALL_CBS(xcb, fsmxfr_xcbs, FSMXFR_MAX_XCBS) {
        fsmxfr_init_xcb(xcb);
    }

    


    fsmxfr_sm_table.min_state = FSMXFR_S_MIN;
    fsmxfr_sm_table.max_state = FSMXFR_S_MAX;
    fsmxfr_sm_table.min_event = CC_MSG_MIN;
    fsmxfr_sm_table.max_event = CC_MSG_MAX;
    fsmxfr_sm_table.table     = (&(fsmxfr_function_table[0][0]));
}

cc_transfer_mode_e
cc_is_xfr_call (callid_t call_id)
{
    static const char fname[] = "cc_is_xfr_call";
    int mode;

    if (call_id == CC_NO_CALL_ID) {
        return CC_XFR_MODE_NONE;
    }
    mode = fsmutil_is_xfr_leg(call_id, fsmxfr_xcbs, FSMXFR_MAX_XCBS);

    switch (mode) {
    case FSMXFR_MODE_TRANSFEROR:
        FSM_DEBUG_SM(DEB_F_PREFIX"xfer mode is transferor for call id = %d\n", DEB_F_PREFIX_ARGS(FSM, fname), call_id);
        return CC_XFR_MODE_TRANSFEROR;
    case FSMXFR_MODE_TRANSFEREE:
        FSM_DEBUG_SM(DEB_F_PREFIX"xfer mode is transferee for call id = %d\n", DEB_F_PREFIX_ARGS(FSM, fname), call_id);
        return CC_XFR_MODE_TRANSFEREE;
    case FSMXFR_MODE_TARGET:
        FSM_DEBUG_SM(DEB_F_PREFIX"xfer mode is target for call id = %d\n", DEB_F_PREFIX_ARGS(FSM, fname), call_id);
        return CC_XFR_MODE_TARGET;
    default:
        FSM_DEBUG_SM(DEB_F_PREFIX"invalid xfer mode %d for call id = %d\n", DEB_F_PREFIX_ARGS(FSM, fname), mode, call_id);
        return CC_XFR_MODE_NONE;
    }
}

void
fsmxfr_shutdown (void)
{
    cpr_free(fsmxfr_xcbs);
    fsmxfr_xcbs = NULL;
}

int
fsmutil_is_xfr_consult_call (callid_t call_id)
{
    return fsmutil_is_xfr_consult_leg(call_id, fsmxfr_xcbs, FSMXFR_MAX_XCBS);
}

callid_t
fsmxfr_get_consult_call_id (callid_t call_id)
{
    fsmxfr_xcb_t *xcb;

    xcb = fsmxfr_get_xcb_by_call_id(call_id);

    if (xcb && call_id == xcb->xfr_call_id) {
        return (fsmxfr_get_other_call_id(xcb, call_id));
    } else {
        return (CC_NO_CALL_ID);
    }
}

callid_t
fsmxfr_get_primary_call_id (callid_t call_id)
{
    fsmxfr_xcb_t *xcb;

    xcb = fsmxfr_get_xcb_by_call_id(call_id);

    if (xcb && (xcb->cns_call_id == call_id)) {
        return (fsmxfr_get_other_call_id(xcb, call_id));
    } else {
        return (CC_NO_CALL_ID);
    }
}
