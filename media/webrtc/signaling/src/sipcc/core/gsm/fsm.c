



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "fsm.h"
#include "fim.h"
#include "lsm.h"
#include "sm.h"
#include "gsm.h" 
#include "ccapi.h"
#include "phone_debug.h"
#include "debug.h"
#include "text_strings.h"
#include "sip_interface_regmgr.h"
#include "resource_manager.h"
#include "platform_api.h"

#define FSM_MAX_FCBS (LSM_MAX_CALLS * (FSM_TYPE_MAX - 1))
#define FSM_S_IDLE   0

extern sm_table_t *pfsmcnf_sm_table;
extern sm_table_t *pfsmb2bcnf_sm_table;
extern sm_table_t *pfsmxfr_sm_table;
extern sm_table_t *pfsmdef_sm_table;

static fsm_fcb_t *fsm_fcbs;
static fsmdef_dcb_t fsm_dcb;
extern uint16_t g_numofselected_calls;
extern void dcsm_update_gsm_state(fsm_fcb_t *fcb, callid_t call_id, int state);


static const char *fsm_type_names[] = {
    "HEAD",
    "CNF",
    "B2BCNF",
    "XFR",
    "DEF"
};

static resource_manager_t *ci_map_p[MAX_REG_LINES + 1];
static resource_manager_t *shown_calls_ci_map_p[MAX_REG_LINES + 1];

const char *
fsm_type_name (fsm_types_t type)
{
    if ((type <= FSM_TYPE_MIN) || (type >= FSM_TYPE_MAX)) {
        return (get_debug_string(GSM_UNDEFINED));
    }

    return (fsm_type_names[type]);
}


const char *
fsm_state_name (fsm_types_t type, int id)
{
    switch (type) {
    case FSM_TYPE_DEF:
        return (fsmdef_state_name(id));

    case FSM_TYPE_XFR:
        return (fsmxfr_state_name(id));

    case FSM_TYPE_CNF:
        return (fsmcnf_state_name(id));

    case FSM_TYPE_B2BCNF:
        return (fsmb2bcnf_state_name(id));

    case FSM_TYPE_NONE:
        return ("IDLE");

    default:
        return (get_debug_string(GSM_UNDEFINED));
    }
}


void
fsm_sm_ftr (cc_features_t ftr_id, cc_srcs_t src_id)
{
    FSM_DEBUG_SM(get_debug_string(FSM_DBG_SM_FTR_ENTRY),
                 cc_feature_name(ftr_id), cc_src_name(src_id));
}


void
fsm_sm_ignore_ftr (fsm_fcb_t *fcb, int fname, cc_features_t ftr_id)
{
    FSM_DEBUG_SM(get_debug_string(FSM_DBG_IGNORE_FTR),
                 fsm_type_name(fcb->fsm_type), fcb->call_id, fname,
                 cc_feature_name(ftr_id));
}


void
fsm_sm_ignore_src (fsm_fcb_t *fcb, int fname, cc_srcs_t src_id)
{
    FSM_DEBUG_SM(get_debug_string(FSM_DBG_IGNORE_SRC),
                 fsm_type_name(fcb->fsm_type), fcb->call_id, fname,
                 cc_src_name(src_id));
}


void
fsm_init_fcb (fsm_fcb_t *fcb, callid_t call_id, fsmdef_dcb_t *dcb,
              fsm_types_t type)
{
    fcb->call_id = call_id;

    fcb->state     = FSM_S_IDLE;
    fcb->old_state = FSM_S_IDLE;

    fcb->fsm_type = type;

    fcb->dcb = dcb;

    fcb->xcb = NULL;

    fcb->ccb = NULL;

    fcb->b2bccb = NULL;
}















fsm_fcb_t *
fsm_get_fcb_by_call_id_and_type (callid_t call_id, fsm_types_t type)
{
    static const char fname[] = "fsm_get_fcb_by_call_id_and_type";
    fsm_fcb_t      *fcb;
    fsm_fcb_t      *fcb_found = NULL;

    FSM_FOR_ALL_CBS(fcb, fsm_fcbs, FSM_MAX_FCBS) {
        if ((fcb->call_id == call_id) && (fcb->fsm_type == type)) {
            fcb_found = fcb;
            break;
        }
    }

    FSM_DEBUG_SM(get_debug_string(GSM_DBG_PTR), "FSM", call_id,
                 fname, "fcb", fcb_found);

    return (fcb_found);
}











void 
fsm_get_fcb_by_selected_or_connected_call_fcb (callid_t call_id, fsm_fcb_t **con_fcb_found,
                                               fsm_fcb_t **sel_fcb_found)
{
    static const char fname[] = "fsm_get_fcb_by_selected_or_connected_call_fcb";
    fsm_fcb_t      *fcb;

    *con_fcb_found = NULL;
    *sel_fcb_found = NULL;

    FSM_FOR_ALL_CBS(fcb, fsm_fcbs, FSM_MAX_FCBS) {

        if (fcb->call_id == call_id) {
            
            continue;
        }
        if (fcb->fsm_type == FSM_TYPE_DEF && 
            (fcb->state == FSMDEF_S_CONNECTED ||
             fcb->state == FSMDEF_S_CONNECTED_MEDIA_PEND ||
             fcb->state == FSMDEF_S_OUTGOING_ALERTING)) {
            *con_fcb_found = fcb;
        } else if (fcb->fsm_type == FSM_TYPE_DEF && fcb->dcb->selected) {
            *sel_fcb_found = fcb;
            break;
        }
    }

    FSM_DEBUG_SM(get_debug_string(GSM_DBG_PTR), "FSM", call_id,
                 fname, "fcb", con_fcb_found);

}














fsm_fcb_t *
fsm_get_fcb_by_call_id (callid_t call_id)
{
    static const char fname[] = "fsm_get_fcb_by_call_id";
    fsm_fcb_t      *fcb;
    fsm_fcb_t      *fcb_found = NULL;

    FSM_FOR_ALL_CBS(fcb, fsm_fcbs, FSM_MAX_FCBS) {
        if (fcb->call_id == call_id) {
            fcb_found = fcb;
            break;
        }
    }

    FSM_DEBUG_SM(get_debug_string(GSM_DBG_PTR), "FSM", call_id,
                 fname, "fcb", fcb_found);

    return (fcb_found);
}














fsm_fcb_t *
fsm_get_new_fcb (callid_t call_id, fsm_types_t fsm_type)
{
    static const char fname[] = "fsm_get_new_fcb";
    fsm_fcb_t *fcb;

    



    fcb = fsm_get_fcb_by_call_id(CC_NO_CALL_ID);
    if (fcb != NULL) {
        fsm_init_fcb(fcb, call_id, FSMDEF_NO_DCB, fsm_type);
    }

    FSM_DEBUG_SM(get_debug_string(GSM_DBG_PTR), "FSM", call_id,
                 fname, "fcb", fcb);

    return (fcb);
}


fsmdef_dcb_t *
fsm_get_dcb (callid_t call_id)
{
    fsmdef_dcb_t *dcb;

    dcb = fsmdef_get_dcb_by_call_id(call_id);

    


    if (dcb == NULL) {
        dcb = &fsm_dcb;
    }

    return (dcb);
}


void
fsm_get_fcb (fim_icb_t *icb, callid_t call_id)
{
    icb->cb = fsm_get_new_fcb(call_id, icb->scb->type);
}


void
fsm_init_scb (fim_icb_t *icb, callid_t call_id)
{
    icb->scb->get_cb = &fsm_get_fcb;

    switch (icb->scb->type) {

    case FSM_TYPE_B2BCNF:
        icb->scb->sm = pfsmb2bcnf_sm_table;
        icb->scb->free_cb = fsmb2bcnf_free_cb;

        break;

    case FSM_TYPE_CNF:
        icb->scb->sm = pfsmcnf_sm_table;
        icb->scb->free_cb = fsmcnf_free_cb;

        break;
    case FSM_TYPE_XFR:
        icb->scb->sm = pfsmxfr_sm_table;
        icb->scb->free_cb = fsmxfr_free_cb;

        break;

    case FSM_TYPE_DEF:
        icb->scb->sm = pfsmdef_sm_table;
        icb->scb->free_cb = fsmdef_free_cb;

        break;

    case FSM_TYPE_HEAD:
    default:
        icb->scb->get_cb  = NULL;
        icb->scb->free_cb = NULL;
        icb->scb->sm      = NULL;
    }

}


void
fsm_change_state (fsm_fcb_t *fcb, int fname, int new_state)
{

    DEF_DEBUG(DEB_L_C_F_PREFIX"%s: %s -> %s\n",
                 DEB_L_C_F_PREFIX_ARGS(FSM, ((fcb->dcb == NULL)? CC_NO_LINE: fcb->dcb->line),
                 fcb->call_id, "fsm_change_state"),
                 fsm_type_name(fcb->fsm_type), 
                 fsm_state_name(fcb->fsm_type, fcb->state),
                 fsm_state_name(fcb->fsm_type, new_state));

    fcb->old_state = fcb->state;
    fcb->state = new_state;
    NOTIFY_STATE_CHANGE(fcb, fcb->call_id, new_state);

}


void
fsm_release (fsm_fcb_t *fcb, int fname, cc_causes_t cause)
{
    fsm_change_state(fcb, fname, FSM_S_IDLE);

    
    fsm_cac_call_release_cleanup(fcb->call_id);

    fsm_init_fcb(fcb, CC_NO_CALL_ID, FSMDEF_NO_DCB, FSM_TYPE_NONE);
}


cc_int32_t
fsm_show_cmd (cc_int32_t argc, const char *argv[])
{
    fsm_fcb_t      *fcb;
    int             i = 0;
    void           *cb = NULL;

    


    if ((argc == 2) && (argv[1][0] == '?')) {
        debugif_printf("show fsm\n");
        return 0;
    }

    


    debugif_printf("\n----------------------------- FSM fcbs -------------------------------");
    debugif_printf("\ni    call_id  fcb         type       state      dcb         cb        ");
    debugif_printf("\n----------------------------------------------------------------------\n");

    FSM_FOR_ALL_CBS(fcb, fsm_fcbs, FSM_MAX_FCBS) {
        switch (fcb->fsm_type) {
        case FSM_TYPE_CNF:
            cb = fcb->ccb;
            break;

        case FSM_TYPE_B2BCNF:
            cb = fcb->ccb;
            break;

        case FSM_TYPE_XFR:
            cb = fcb->xcb;
            break;

        case FSM_TYPE_DEF:
            cb = fcb->dcb;
            break;

        default:
            cb = NULL;
        }

        debugif_printf("%-3d  %-7d  0x%8p  %-9s  %-9s  0x%8p  0x%8p\n",
                       i++, fcb->call_id, fcb, fsm_type_name(fcb->fsm_type),
                       fsm_state_name(fcb->fsm_type, fcb->state),
                       fcb->dcb, cb);
    }

    return (0);
}


void
fsm_init (void)
{
    fsm_fcb_t *fcb;

    fsmdef_init_dcb(&fsm_dcb, 0, FSMDEF_CALL_TYPE_NONE, NULL, LSM_NO_LINE,
                    NULL);

    fsmdef_init();
    fsmb2bcnf_init();
    fsmcnf_init();
    fsmxfr_init();

    fsm_cac_init();

    


    fsm_fcbs = (fsm_fcb_t *) cpr_calloc(FSM_MAX_FCBS, sizeof(fsm_fcb_t));
    if (fsm_fcbs == NULL) {
        GSM_ERR_MSG(GSM_F_PREFIX"Failed to allcoate FSM FCBs.\n", "fsm_init");
        return;
    }

    FSM_FOR_ALL_CBS(fcb, fsm_fcbs, FSM_MAX_FCBS) {
        fsm_init_fcb(fcb, CC_NO_CALL_ID, FSMDEF_NO_DCB, FSM_TYPE_NONE);
    }

    


    fsmutil_init_ci_map();
}

void
fsm_shutdown (void)
{
    fsmdef_shutdown();

    fsmb2bcnf_shutdown();

    fsmcnf_shutdown();
    fsmxfr_shutdown();

    fsm_cac_shutdown();

    cpr_free(fsm_fcbs);
    fsm_fcbs = NULL;

    


    fsmutil_free_ci_map();
}















cc_causes_t
fsm_set_fcb_dcbs (fsmdef_dcb_t *dcb)
{
    callid_t        call_id = dcb->call_id;
    fsm_fcb_t      *fcb;
    fsm_types_t     i;

    for (i = FSM_TYPE_CNF; i < FSM_TYPE_MAX; i++) {
        fcb = fsm_get_fcb_by_call_id_and_type(call_id, i);
        if (fcb == NULL) {
            return CC_CAUSE_ERROR;
        }
        fcb->dcb = dcb;
    }

    return CC_CAUSE_OK;
}


















cc_causes_t
fsm_get_new_outgoing_call_context (callid_t call_id, line_t line,
                                  fsm_fcb_t *fcb, boolean expline)
{
    static const char fname[] = "fsm_get_new_outgoing_call_context";
    fsmdef_dcb_t   *dcb;
    cc_causes_t     cause = CC_CAUSE_OK;
    cc_causes_t     lsm_rc;

    


    dcb = fsmdef_get_new_dcb(call_id);
    if (dcb == NULL) {
        return CC_CAUSE_NO_RESOURCE;
    }

    


    lsm_rc = lsm_get_facility_by_line(call_id, line, expline, dcb);
    if (lsm_rc != CC_CAUSE_OK) {
        FSM_DEBUG_SM(get_debug_string(FSM_DBG_FAC_ERR), call_id, fname,
                     "lsm_get_facility_by_line failed", cc_cause_name(lsm_rc));
    }

    



    if (lsm_rc != CC_CAUSE_OK) {
        line = LSM_NO_LINE;
    }
    fsmdef_init_dcb(dcb, call_id, FSMDEF_CALL_TYPE_OUTGOING, NULL, line, fcb);

    cause = fsm_set_fcb_dcbs(dcb);
    if (cause == CC_CAUSE_OK) {
        cause = lsm_rc;
    }

    FSM_DEBUG_SM(get_debug_string(FSM_DBG_FAC_FOUND), call_id, fname,
                 dcb->line);

    return cause;
}

















cc_causes_t
fsm_get_new_incoming_call_context (callid_t call_id, fsm_fcb_t *fcb,
                                   const char *called_number, boolean expline)
{
    static const char fname[] = "fsm_get_new_incoming_call_context";
    fsmdef_dcb_t   *dcb;
    line_t          free_line;
    cc_causes_t     cause;
    cc_causes_t     lsm_rc;


    


    dcb = fsmdef_get_new_dcb(call_id);
    if (dcb == NULL) {
        return CC_CAUSE_NO_RESOURCE;
    }

    


    if ((lsm_rc = lsm_get_facility_by_called_number(call_id, called_number,
                                                    &free_line, expline, dcb))
        != CC_CAUSE_OK) {
        




        free_line = 1;
        FSM_DEBUG_SM(get_debug_string(FSM_DBG_FAC_ERR), call_id, fname,
                     "lsm_get_facility_by_called_number",
                     cc_cause_name(lsm_rc));
    }

    fsmdef_init_dcb(dcb, call_id, FSMDEF_CALL_TYPE_INCOMING, called_number,
                    free_line, fcb);

    cause = fsm_set_fcb_dcbs(dcb);
    if (cause == CC_CAUSE_OK) {
        cause = lsm_rc;
    }

    FSM_DEBUG_SM(get_debug_string(FSM_DBG_FAC_FOUND), call_id, fname,
                 dcb->line);

    return cause;
}

















int
fsmutil_is_cnf_leg (callid_t call_id, fsmcnf_ccb_t *fsmcnf_ccbs,
                    unsigned short max_ccbs)
{
    fsmcnf_ccb_t *ccb;

    FSM_FOR_ALL_CBS(ccb, fsmcnf_ccbs, max_ccbs) {
        if ((ccb->cnf_call_id == call_id) || (ccb->cns_call_id == call_id)) {
            return TRUE;
        }
    }
    return FALSE;
}

















int
fsmutil_is_xfr_leg (callid_t call_id, fsmxfr_xcb_t *fsmxfr_xcbs,
                    unsigned short max_xcbs)
{
    fsmxfr_xcb_t *xcb;

    FSM_FOR_ALL_CBS(xcb, fsmxfr_xcbs, max_xcbs) {
        if ((xcb->xfr_call_id == call_id) || (xcb->cns_call_id == call_id)) {
            return xcb->mode;
        }
    }
    return FSMXFR_MODE_MIN;
}

void
fsm_display_no_free_lines (void)
{
    char tmp_str[STATUS_LINE_MAX_LEN];

    if ((platGetPhraseText(STR_INDEX_NO_FREE_LINES,
                                 (char *)tmp_str,
                                 (STATUS_LINE_MAX_LEN - 1))) == CPR_SUCCESS) {
        lsm_ui_display_notify(tmp_str, NO_FREE_LINES_TIMEOUT);
    }
}












void
fsm_display_use_line_or_join_to_complete (void)
{
    char tmp_str[STATUS_LINE_MAX_LEN];

    if ((platGetPhraseText(STR_INDEX_USE_LINE_OR_JOIN_TO_COMPLETE,
                                 (char *)tmp_str,
                                 (STATUS_LINE_MAX_LEN - 1))) == CPR_SUCCESS) {
        lsm_ui_display_notify(tmp_str, NO_FREE_LINES_TIMEOUT);
    }
}

void
fsm_display_feature_unavailable (void)
{
    char tmp_str[STATUS_LINE_MAX_LEN];

    if ((platGetPhraseText(STR_INDEX_FEAT_UNAVAIL,
                                 (char *)tmp_str,
                                 (STATUS_LINE_MAX_LEN - 1))) == CPR_SUCCESS) {
        lsm_ui_display_notify(tmp_str, NO_FREE_LINES_TIMEOUT);
    }
}

void
fsm_set_call_status_feature_unavailable (callid_t call_id, line_t line)
{
    char tmp_str[STATUS_LINE_MAX_LEN];

    if ((platGetPhraseText(STR_INDEX_FEAT_UNAVAIL,
                                 (char *)tmp_str,
                                 (STATUS_LINE_MAX_LEN - 1))) == CPR_SUCCESS) {
        ui_set_call_status(tmp_str, line, lsm_get_ui_id(call_id));
    }
}











uint16_t fsmutil_get_num_selected_calls (void)
{
    return(g_numofselected_calls);
}








void fsm_display_control_ringin_calls (boolean hide)
{
    fsm_fcb_t      *fcb;

    FSM_FOR_ALL_CBS(fcb, fsm_fcbs, FSM_MAX_FCBS) {
        if ((fcb->state == FSMDEF_S_INCOMING_ALERTING) &&
            (lsm_is_it_priority_call(fcb->call_id) == FALSE)) { 
            lsm_display_control_ringin_call (fcb->call_id, fcb->dcb->line, hide);
            if (hide == TRUE) {
                fsmutil_clear_shown_calls_ci_element(fcb->dcb->caller_id.call_instance_id, fcb->dcb->line);
            } else {
                fsmutil_set_shown_calls_ci_element(fcb->dcb->caller_id.call_instance_id, fcb->dcb->line);
            }
        }
    }
}















void
fsmutil_init_groupid (fsmdef_dcb_t *dcb, callid_t call_id,
                      fsmdef_call_types_t call_type)
{
    fsmcnf_ccb_t *ccb = NULL;

    


    dcb->group_id = CC_NO_GROUP_ID;
    if (call_type != FSMDEF_CALL_TYPE_NONE) {
        ccb = fsmcnf_get_ccb_by_call_id(call_id);
        if (ccb) {
            
            fsmdef_dcb_t *other_dcb = NULL;

            other_dcb =
                fsmdef_get_dcb_by_call_id(fsmcnf_get_other_call_id(ccb, call_id));
            if (other_dcb) {
                dcb->group_id = other_dcb->group_id;
            }
        } else {
            

            dcb->group_id = dcb->call_id;
        }
    }
    return;
}













int
fsmutil_get_call_attr (fsmdef_dcb_t *dcb, 
                    line_t line, callid_t call_id)
{
    int call_attr;

    if (fsmutil_is_cnf_consult_call(call_id) == TRUE) {
        call_attr = LOCAL_CONF_CONSULT;
    } else if (fsmutil_is_b2bcnf_consult_call(call_id) == TRUE) {
        call_attr = CONF_CONSULT;
    } else if (fsmutil_is_xfr_consult_call(call_id) == TRUE) {
        call_attr = XFR_CONSULT;
    } else {
        if (dcb == NULL) {
            return(NORMAL_CALL);
        }

        switch (dcb->active_feature) {
        case CC_FEATURE_CFWD_ALL:
            call_attr = CC_ATTR_CFWD_ALL;
            break;
        default:
            call_attr = NORMAL_CALL;
            break;
        }
    }
    return call_attr;
}















int
fsmutil_is_cnf_consult_leg (callid_t call_id, fsmcnf_ccb_t *fsmcnf_ccbs,
                            uint16_t max_ccbs)
{
    fsmcnf_ccb_t *ccb;

    FSM_FOR_ALL_CBS(ccb, fsmcnf_ccbs, max_ccbs) {
        if (ccb->cns_call_id == call_id) {
            return TRUE;
        }
    }

    return FALSE;
}

















int
fsmutil_is_xfr_consult_leg (callid_t call_id, fsmxfr_xcb_t *fsmxfr_xcbs,
                            uint16_t max_xcbs)
{
    fsmxfr_xcb_t *xcb;

    FSM_FOR_ALL_CBS(xcb, fsmxfr_xcbs, max_xcbs) {
        if ((xcb->mode == FSMXFR_MODE_TRANSFEROR) &&
            (xcb->cns_call_id == call_id)) {
            return TRUE;
        }
    }
    return FALSE;
}


















static void
fsmutil_clear_feature_invocation_state (fsmdef_dcb_t *dcb,
                                       cc_features_t feature_id)
{
    if ((feature_id < CC_FEATURE_NONE) || (feature_id >= CC_FEATURE_MAX)) {
        
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"Invalid feature id -> %d\n", dcb->line, dcb->call_id, "fsmutil_clear_feature_invocation_state", feature_id);
        return;
    }

    rm_clear_element((resource_manager_t *) dcb->feature_invocation_state,
                     (int16_t) feature_id);
}














void
fsmutil_process_feature_ack (fsmdef_dcb_t *dcb, cc_features_t feature_id)
{
    
    fsmutil_clear_feature_invocation_state(dcb, feature_id);
}















void
fsmutil_clear_all_feature_invocation_state (fsmdef_dcb_t *dcb)
{
    rm_clear_all_elements((resource_manager_t *) dcb->feature_invocation_state);
}













void
fsmutil_init_feature_invocation_state (fsmdef_dcb_t *dcb)
{
    static const char fname[] = "fsmutil_init_feature_invocation_state";

    dcb->feature_invocation_state = rm_create(CC_FEATURE_MAX);

    if (!dcb->feature_invocation_state) {
        GSM_ERR_MSG(GSM_L_C_F_PREFIX"failed to allocate feature invocation state table\n", dcb->line, dcb->call_id,
                     fname);
    }
}















void
fsmutil_free_feature_invocation_state (fsmdef_dcb_t *dcb)
{
    rm_destroy((resource_manager_t *) dcb->feature_invocation_state);
    dcb->feature_invocation_state = NULL;
}













void
fsmutil_free_all_ci_id (void)
{
    uint16_t line;

    for (line = 1; line <= MAX_REG_LINES; line++) {
        rm_clear_all_elements((resource_manager_t *) ci_map_p[line]);
    }
}



















void
fsmutil_free_ci_id (uint16_t id, line_t line)
{
    static const char fname[] = "fsmutil_free_ci_id";

    if (id < 1 || id > MAX_CALLS) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified id %d is invalid\n", fname, id);
        return;
    }

    if (line < 1 || line > MAX_REG_LINES) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified line %d is invalid\n", fname, line);
        return;
    }

    rm_clear_element((resource_manager_t *) ci_map_p[line], (int16_t) --id);
}

#ifdef LOCAL_UI_CALLINSTANCE_ID























uint16_t
fsmutil_get_ci_id (line_t line)
{
    static const char fname[] = "fsmutil_get_ci_id";
    int16_t         id;
    uint16_t        return_id = 0;

    if (line < 1 || line > MAX_REG_LINES) {
        GSM_ERR_MSG("specified line %d is invalid\n", fname, line);
        return 0;
    }

    id = rm_get_free_element(ci_map_p[line]);
    if (id >= 0) {
        return_id = ++id;
    }
    return (return_id);
}
#endif






















void
fsmutil_set_ci_id (uint16_t id, line_t line)
{
    static const char fname[] = "fsmutil_set_ci_id";

    if (id < 1 || id > MAX_CALLS) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified id %d is invalid\n", fname, id);
        return;
    }

    if (line < 1 || line > MAX_REG_LINES) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified line %d is invalid\n", fname, line);
        return;
    }

    rm_set_element(ci_map_p[line], (int16_t) --id);
}













void
fsmutil_init_ci_map (void)
{
    static const char fname[] = "fsmutil_init_ci_map";
    uint16_t line;

    for (line = 1; line <= MAX_REG_LINES; line++) {
        ci_map_p[line] = rm_create(MAX_CALLS);

        if (!ci_map_p[line]) {
            GSM_ERR_MSG(GSM_F_PREFIX"failed to allocate call instance id map for line %d",
                         fname, line);
        }
    }
}






void fsmutil_init_shown_calls_ci_map (void)
{
    static const char fname[] = "fsmutil_init_shown_calls_ci_map";
    uint16_t line;

    for (line = 1; line <= MAX_REG_LINES; line++) {
        shown_calls_ci_map_p[line] = rm_create(MAX_CALLS);

        if (!shown_calls_ci_map_p[line]) {
            GSM_ERR_MSG(GSM_F_PREFIX"failed to allocate shown calls call instance id map for line %d",
                         fname, line);
        }
    }
}






void fsmutil_free_all_shown_calls_ci_map (void)
{
    uint16_t line;

    for (line = 1; line <= MAX_REG_LINES; line++) {
        rm_clear_all_elements((resource_manager_t *) shown_calls_ci_map_p[line]);
    }
}









void fsmutil_clear_shown_calls_ci_element (uint16_t id, line_t line)
{
    static const char fname[] = "fsmutil_clear_shown_calls_ci_element";

    if (id < 1 || id > MAX_CALLS) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified id %d is invalid\n", fname, id);
        return;
    }

    if (line < 1 || line > MAX_REG_LINES) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified line %d is invalid\n", fname, line);
        return;
    }

    rm_clear_element((resource_manager_t *) shown_calls_ci_map_p[line], (int16_t)(id - 1));
}









void fsmutil_set_shown_calls_ci_element (uint16_t id, line_t line)
{
    static const char fname[] = "fsmutil_set_shown_calls_ci_element";

    if (id < 1 || id > MAX_CALLS) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified id %d is invalid\n", fname, id);
        return;
    }

    if (line < 1 || line > MAX_REG_LINES) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified line %d is invalid\n", fname, line);
        return;
    }

    rm_set_element(shown_calls_ci_map_p[line], (int16_t)(id - 1));
}









boolean fsmutil_is_shown_calls_ci_element_set (uint16_t id, line_t line)
{
    static const char fname[] = "fsmutil_is_shown_calls_ci_element_set";

    if (id < 1 || id > MAX_CALLS) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified id %d is invalid\n", fname, id);
        return FALSE;
    }

    if (line < 1 || line > MAX_REG_LINES) {
        GSM_ERR_MSG(GSM_F_PREFIX"specified line %d is invalid\n", fname, line);
        return FALSE;
    }

    if (rm_is_element_set(shown_calls_ci_map_p[line], (int16_t)(id - 1))) {
        return (TRUE);
    }

    return (FALSE);
}













void
fsmutil_free_ci_map (void)
{
    uint16_t line;

    for (line = 1; line <= MAX_REG_LINES; line++) {
        rm_destroy((resource_manager_t *) ci_map_p[line]);
        ci_map_p[line] = NULL;
    }
}














void
fsmutil_show_ci_map (void)
{
    uint16_t line;

    for (line = 1; line <= MAX_REG_LINES; line++) {
        rm_show(ci_map_p[line]);
    }
}
