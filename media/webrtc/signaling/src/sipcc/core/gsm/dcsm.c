






































#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_string.h"
#include "cpr_errno.h"
#include "phone.h"
#include "phntask.h"
#include "lsm.h"
#include "ccapi.h"
#include "phone_debug.h"
#include "fim.h"
#include "sdp.h"
#include "debug.h"
#include "gsm_sdp.h"
#include "uiapi.h"
#include "gsm.h"
#include "prot_configmgr.h"
#include "singly_link_list.h"

typedef enum dcsm_state {
    DCSM_S_MIN = -1,
    DCSM_S_READY,
    DCSM_S_WAITING,
    DCSM_S_MAX
} dcsm_state_e;

#define DCSM_MAX_CALL_IDS     (LSM_MAX_CALLS)

static struct dcsm_icb_t {
    callid_t            call_ids[DCSM_MAX_CALL_IDS];
    line_t              line;
    int                 gsm_state;
    sll_handle_t        s_msg_list;
    
    dcsm_state_e        state;
} dcsm_cb;

static const char *dcsm_state_names[] = {
    "DCSM_READY",
    "DCSM_WAITING"
};

extern cc_int32_t g_dcsmDebug;

static sm_rcs_t dcsm_wait_ev_feature_handling(void *event, int event_id);
static sm_rcs_t dcsm_wait_ev_offhook_handling(void *event, int event_id);
static sm_rcs_t dcsm_wait_ev_dialstring_handling(void *event, int event_id);

typedef sm_rcs_t (*pdcsm_sm_evt_handler)(void *event, int event_id);

typedef struct _dcsm_table_t {
    int min_state;
    int max_state;
    int min_event;
    int max_event;
    pdcsm_sm_evt_handler *table;
} dcsm_table_t;


static pdcsm_sm_evt_handler dcsm_function_table[DCSM_S_MAX][CC_MSG_MAX] =
{

    {
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
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
     NULL,
     NULL,
     dcsm_wait_ev_feature_handling,
     NULL,
     dcsm_wait_ev_offhook_handling,
     NULL,
     NULL,
     NULL,
     NULL,
     dcsm_wait_ev_dialstring_handling,
     NULL,
     NULL
    },
};

static dcsm_table_t dcsm_sm_table;
static dcsm_table_t *pdcsm_sm_table = &dcsm_sm_table;










static boolean
dcsm_add_event_to_queue (void *event)
{
    (void) sll_append(dcsm_cb.s_msg_list, event);
    
    return TRUE;
}










static void *
dcsm_get_first_event_from_queue (void)
{
    void *msg_ptr = NULL;
    
    msg_ptr = sll_next(dcsm_cb.s_msg_list, NULL);
    
    sll_remove(dcsm_cb.s_msg_list, msg_ptr);

    return(msg_ptr);
}










const char *dcsm_get_state_name (int state)
{
    if ((state <= DCSM_S_MIN) || 
            (state >= DCSM_S_MAX)) {
        return (get_debug_string(GSM_UNDEFINED));
    }
    
    return dcsm_state_names[state];
}









static void dcsm_process_event_to_gsm (void *msg_ptr)
{
    if (fim_process_event(msg_ptr, FALSE) == TRUE) {
    
        fim_free_event(msg_ptr);

        
        cprReleaseBuffer(msg_ptr);
    }
}










static void dcsm_do_ready_state_job (void)
{
    static const char fname[] = "dcsm_do_ready_state_job";
    void            *msg_ptr;
    int             event_id;
    cc_feature_t    *feat_msg = NULL;
    callid_t        call_id = CC_NO_CALL_ID;

    if (dcsm_cb.state != DCSM_S_READY) {
        DEF_DEBUG(DEB_F_PREFIX": not in ready state.\n",
            DEB_F_PREFIX_ARGS("DCSM", fname));
        return;
    }
    
    msg_ptr = dcsm_get_first_event_from_queue();

    
    if (msg_ptr != NULL) {
        event_id = (int)(((cc_setup_t *)msg_ptr)->msg_id);
        if (event_id == CC_MSG_FEATURE) {
            feat_msg = (cc_feature_t *) msg_ptr;
            if (feat_msg != NULL) {
                call_id = feat_msg->call_id;
            }
        }
        
        DEF_DEBUG(DEB_F_PREFIX"%d: event (%s%s)\n",
            DEB_F_PREFIX_ARGS("DCSM", fname), call_id,
                     cc_msg_name((cc_msgs_t)(event_id)),
                     feat_msg ? cc_feature_name(feat_msg->feature_id):" ");
        dcsm_process_event_to_gsm(msg_ptr);
    }
}








void dcsm_process_jobs (void)
{
    dcsm_do_ready_state_job();
}









static void dcsm_add_call_id_to_list (callid_t call_id)
{
    static const char fname[] = "dcsm_add_call_id_to_list";
    int i, loc = -1;
    
    for (i=0; i< DCSM_MAX_CALL_IDS; i++) {
        if (dcsm_cb.call_ids[i] == CC_NO_CALL_ID) {
            loc = i;
        } else if (dcsm_cb.call_ids[i] == call_id) {
            
            return;
        }
    }

    if (loc == -1) {
    
        


        DCSM_ERROR(DCSM_F_PREFIX"DCSM No space to store call_id.\n",
                                    DEB_F_PREFIX_ARGS("DCSM", fname));
        return;                                    
    }

    dcsm_cb.call_ids[loc] = call_id;
}









static boolean dcsm_remove_check_for_empty_list (callid_t call_id)
{
    int         i;
    boolean      call_id_present = FALSE;
    
    for (i=0; i< DCSM_MAX_CALL_IDS; i++) {
        if (dcsm_cb.call_ids[i] == call_id) {
            dcsm_cb.call_ids[i] = CC_NO_CALL_ID;
            
            



            if (call_id_present == TRUE) {
                return FALSE;
            }
            
        } else if (dcsm_cb.call_ids[i] != CC_NO_CALL_ID) {
            call_id_present = TRUE;
        }
    }
    
    if (call_id_present == TRUE) {
        return(FALSE);
    }

    return(TRUE);
}











void
dcsm_update_gsm_state (fsm_fcb_t *fcb, callid_t call_id, int state)
{
    int    last_state;
    static const char fname[] = "dcsm_update_gsm_state";
    fsmdef_dcb_t *dcb;
    
    if (fcb->fsm_type != FSM_TYPE_DEF) {
        DEF_DEBUG(DEB_F_PREFIX"%d: Not handling for %s\n", 
                  DEB_F_PREFIX_ARGS("DCSM", fname), call_id, 
                  fsm_type_name(fcb->fsm_type));
        return;
    }

    last_state = dcsm_cb.state;
    
    switch (state) {
        case FSMDEF_S_RELEASING:
            dcb = fsmdef_get_dcb_by_call_id(call_id);
            if (dcb && dcb->send_release == FALSE) {
                
                break;
            }
        case FSMDEF_S_CONNECTING:
        case FSMDEF_S_HOLD_PENDING:
        case FSMDEF_S_RESUME_PENDING:
            dcsm_add_call_id_to_list(call_id);
           
            dcsm_cb.state = DCSM_S_WAITING;
            break;
        case FSMDEF_S_MIN:
        case FSMDEF_S_IDLE:
        case FSMDEF_S_COLLECT_INFO:
        case FSMDEF_S_CALL_SENT:
        case FSMDEF_S_OUTGOING_PROCEEDING:
        case FSMDEF_S_KPML_COLLECT_INFO:
        case FSMDEF_S_OUTGOING_ALERTING:
        case FSMDEF_S_INCOMING_ALERTING:
        case FSMDEF_S_JOINING:
        case FSMDEF_S_CONNECTED:
        case FSMDEF_S_CONNECTED_MEDIA_PEND:
        case FSMDEF_S_HOLDING:
        case FSMDEF_S_PRESERVED:
        case FSMDEF_S_MAX:
            



            if (dcsm_remove_check_for_empty_list(call_id) == TRUE) {
                dcsm_cb.state = DCSM_S_READY;
                



                if (sll_count(dcsm_cb.s_msg_list) > 0 ) {
                    if (gsm_send_msg(DCSM_EV_READY, NULL, 0) == CPR_FAILURE) {
                        DCSM_ERROR(DCSM_F_PREFIX"send DCSM_EV_READY ERROR.\n",
                                        DEB_F_PREFIX_ARGS(DCSM, fname));
                    }
                }
            }
            
            break;
        default:
            break;
    }
    
    DEF_DEBUG(DEB_F_PREFIX"%d : %s --> %s\n",
            DEB_F_PREFIX_ARGS("DCSM", fname), call_id,
            dcsm_get_state_name(last_state),
            dcsm_get_state_name(dcsm_cb.state));
    return;
}











static sm_rcs_t
dcsm_wait_ev_offhook_handling (void *event, int event_id)
{
    static const char fname[] = "dcsm_wait_ev_offhook_handling";

    DEF_DEBUG(DEB_F_PREFIX": offhook\n",
                     DEB_F_PREFIX_ARGS("DCSM", fname));

    dcsm_add_event_to_queue(event);
    return (SM_RC_END);
}










static sm_rcs_t
dcsm_wait_ev_feature_handling (void *event, int event_id)
{
    static const char fname[] = "dcsm_wait_ev_feature_handling";
    cc_feature_t     *feat_msg     = (cc_feature_t *) event;
    sm_rcs_t   rc = SM_RC_END;
    cc_features_t      ftr_id = CC_FEATURE_UNDEFINED;
    callid_t           call_id = CC_NO_CALL_ID; 

    if (feat_msg != NULL) {
        ftr_id  = feat_msg->feature_id;
        call_id = feat_msg->call_id;
    }

    DEF_DEBUG(DEB_F_PREFIX"%d: id= %s%s\n",
                     DEB_F_PREFIX_ARGS("DCSM", fname), call_id,
                    cc_msg_name((cc_msgs_t)(event_id)),
                     feat_msg ? cc_feature_name(feat_msg->feature_id):" ");

    switch (ftr_id) {
        case CC_FEATURE_ANSWER:
        case CC_FEATURE_NEW_CALL:
        case CC_FEATURE_REDIAL:
        case CC_FEATURE_RESUME:
        case CC_FEATURE_JOIN:
            dcsm_add_event_to_queue(event);
            DEF_DEBUG(DEB_F_PREFIX"%d: Event queued\n",
                     DEB_F_PREFIX_ARGS("DCSM", fname), call_id);
            rc = SM_RC_END;
            break;
        default:
            DEF_DEBUG(DEB_F_PREFIX"%d: Feature msg not handled\n",
                     DEB_F_PREFIX_ARGS("DCSM", fname), call_id);
            
            rc = SM_RC_CONT;
            break;
    }
    
    
    return (rc);

}











static sm_rcs_t
dcsm_wait_ev_dialstring_handling (void *event, int event_id)
{
    static const char fname[] = "dcsm_wait_ev_dialstring_handling";

    DEF_DEBUG(DEB_F_PREFIX": dialstring\n",
                     DEB_F_PREFIX_ARGS("DCSM", fname));

    dcsm_add_event_to_queue(event);
    return (SM_RC_END);
}











sm_rcs_t
dcsm_process_event (void *event, int event_id)
{
    static const char fname[] = "dcsm_process_event";
    callid_t   call_id;
    int        state_id;
    sm_rcs_t   rc       = SM_RC_CONT;
    fsm_fcb_t *fcb      = (fsm_fcb_t *) event;
    cc_feature_t  *feat_msg = NULL;
    pdcsm_sm_evt_handler hdlr; 

    call_id  = fcb->call_id;

    if (event_id == CC_MSG_FEATURE) {
        feat_msg = (cc_feature_t *) event;

        if (feat_msg != NULL){
            call_id = feat_msg->call_id;
        }
    }
    
    DEF_DEBUG(DEB_F_PREFIX"DCSM %-4d:(%s:%s%s)\n",
                     DEB_F_PREFIX_ARGS("DCSM", fname), call_id,
                     dcsm_get_state_name(dcsm_cb.state),
                     cc_msg_name((cc_msgs_t)(event_id)),
                     feat_msg ? cc_feature_name(feat_msg->feature_id):" ");
    
    state_id = dcsm_cb.state; 
                     
   



    if ((state_id > pdcsm_sm_table->min_state) &&
        (state_id < pdcsm_sm_table->max_state) &&
        (event_id > pdcsm_sm_table->min_event) &&
        (event_id < pdcsm_sm_table->max_event)) {
        
        if ((hdlr = pdcsm_sm_table->table[pdcsm_sm_table->max_event * state_id + 
                            event_id]) != NULL) {
            DEF_DEBUG(DEB_F_PREFIX"%-4d: dcsm entry: (%s)\n",
                         DEB_F_PREFIX_ARGS("DCSM", fname), call_id,
                     cc_msg_name((cc_msgs_t)(event_id)));

            rc = hdlr(event, event_id);
        }
        
    }

    return (rc);

}










static sll_match_e
dcsm_match_event (void *key_p, void *msg_data)
{
    return SLL_MATCH_FOUND;
}










cc_int32_t
dcsm_show_cmd (cc_int32_t argc, const char *argv[])
{
    void *msg_ptr;
    int i;
    cc_setup_t     *msg;
    cc_msgs_t       msg_id;
    line_t    line;
    callid_t  call_id;
    cc_feature_t  *feat_msg = NULL;


    


    if ((argc == 2) && (argv[1][0] == '?')) {
        debugif_printf("show dcsm\n");
        return (0);
    }

   
    if (dcsm_cb.s_msg_list == NULL) {
        return(0);
    }
    
    debugif_printf("\n-------------------------- DCSM Data --------------------------");
    debugif_printf("\nDCSM State = %s",dcsm_get_state_name(dcsm_cb.state));
    debugif_printf("\nDCSM waiting calls \n");

    for (i=0; i< DCSM_MAX_CALL_IDS; i++) {
        if (dcsm_cb.call_ids[i] != CC_NO_CALL_ID) {
            debugif_printf("%d ", dcsm_cb.call_ids[i]);
        }
    }
    debugif_printf("\n");
    
    debugif_printf("\nDCSM waiting events \n");
    i = 0;
    msg_ptr = sll_next(dcsm_cb.s_msg_list, NULL);
    while (msg_ptr) {
        msg_ptr = sll_next(dcsm_cb.s_msg_list, msg_ptr);
        
        if (msg_ptr) {
            msg = (cc_setup_t *) msg_ptr;
            msg_id   = msg->msg_id;
            call_id  = msg->call_id;
            line     = msg->line;
            if ((int)msg_id == CC_MSG_FEATURE) {
                feat_msg = (cc_feature_t *) msg_ptr;
            }
            
            debugif_printf("Event %d (%d/%d): (%s%s)\n",
                       i++, line, call_id, cc_msg_name(msg_id),
                        feat_msg ? cc_feature_name(feat_msg->feature_id):" ");
        }
    }
    debugif_printf("\n-------------------------- DCSM Data Done-----------------------");
        
    return (0);
}












void
dcsm_init (void)
{
    static const char fname[] = "dcsm_init";
    int i;
    
    


    dcsm_sm_table.min_state = DCSM_S_MIN;
    dcsm_sm_table.max_state = DCSM_S_MAX;
    dcsm_sm_table.min_event = CC_MSG_MIN;
    dcsm_sm_table.max_event = CC_MSG_MAX;
    dcsm_sm_table.table     = (&(dcsm_function_table[0][0]));
    
    dcsm_cb.state = DCSM_S_READY;

    for (i=0; i< DCSM_MAX_CALL_IDS; i++) {
        dcsm_cb.call_ids[i] = CC_NO_CALL_ID;
    }
    
    
    dcsm_cb.s_msg_list = sll_create((sll_match_e(*)(void *, void *))
                            dcsm_match_event);

    if (dcsm_cb.s_msg_list == NULL) {
        DCSM_ERROR(DCSM_F_PREFIX"DCSM CB creation failed.\n",
                                    DEB_F_PREFIX_ARGS("DCSM", fname));

    }
   
}











void
dcsm_shutdown (void)
{
    void *msg_ptr;
    
    if (dcsm_cb.s_msg_list == NULL) {
        return;
    }
    
    msg_ptr = sll_next(dcsm_cb.s_msg_list, NULL);
    while (msg_ptr) {
        msg_ptr = sll_next(dcsm_cb.s_msg_list, msg_ptr);
        
        if (msg_ptr) {
            fim_free_event(msg_ptr);

            
            cprReleaseBuffer(msg_ptr);
        }
    }
    
    sll_destroy(dcsm_cb.s_msg_list);
    dcsm_cb.s_msg_list = NULL;
}

