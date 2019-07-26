



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "phntask.h"
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
#include "singly_link_list.h"
#include "platform_api.h"

#define CAC_FAILURE_TIMEOUT 5
cc_int32_t g_cacDebug = 0;


typedef struct {
    callid_t  call_id;
} cac_key_t;


typedef enum {
    FSM_CAC_IDLE = 0,
    FSM_CAC_REQ_PENDING = 1,
    FSM_CAC_REQ_RESP = 2
} fsm_cac_state_e;



typedef struct cac_data_t {
    void                *msg_ptr;
    callid_t            call_id;
    void                *cac_fail_timer;
    fsm_cac_state_e     cac_state;
    uint32_t            sessions;
} cac_data_t;

static sll_handle_t s_cac_list = NULL;













static sll_match_e
fsm_cac_match_call_id (cac_data_t *key_p, cac_data_t *cac_data)
{
    if (cac_data->call_id == key_p->call_id) {

        return SLL_MATCH_FOUND;
    }

    return SLL_MATCH_NOT_FOUND;

}










static cac_data_t *
fsm_get_new_cac_data (void)
{
    static const char *fname="fsm_get_new_cac_data";
    cac_data_t *cac_mem;

    cac_mem = (cac_data_t *) cpr_malloc(sizeof(cac_data_t));

    if (cac_mem == NULL) {
        CAC_ERROR(CAC_F_PREFIX"No memory for CAC data.\n",
                DEB_F_PREFIX_ARGS("CAC", fname));
        return (NULL);
    }

    memset(cac_mem, 0, sizeof(cac_data_t));
    return (cac_mem);
}













static void
fsm_clear_cac_data (cac_data_t *cac_data)
{

    if (cac_data->cac_fail_timer) {
        (void) cprCancelTimer(cac_data->cac_fail_timer);

        (void) cprDestroyTimer(cac_data->cac_fail_timer);
    }

    (void) sll_remove(s_cac_list, cac_data);

    fim_free_event(cac_data->msg_ptr);

    
    cpr_free(cac_data->msg_ptr);

    cpr_free(cac_data);

}












static void fsm_cac_notify_failure (cac_data_t *cac_data)
{
    const char fname[] = "fsm_cac_notify_failure";
    cc_setup_t     *msg = (cc_setup_t *) cac_data->msg_ptr;
    cc_msgs_t       msg_id   = msg->msg_id;
    callid_t        call_id  = msg->call_id;
    line_t          line     = msg->line;
    int             event_id = msg_id;
    cc_srcs_t       src_id  = msg->src_id;

    
    lsm_ui_display_notify_str_index(STR_INDEX_NO_BAND_WIDTH);

    
    if (event_id == CC_MSG_SETUP &&
            src_id == CC_SRC_SIP) {
        DEF_DEBUG(DEB_F_PREFIX"Send CAC failure to SIP %d.\n",
                    DEB_F_PREFIX_ARGS("CAC", fname), cac_data->call_id);
        cc_int_release(CC_SRC_GSM, CC_SRC_SIP, call_id, line,
                       CC_CAUSE_CONGESTION, NULL, NULL);
    } else {
        




        ui_call_state(evOnHook, line, call_id, CC_CAUSE_CONGESTION);
    }

}















static boolean
fsm_init_cac_failure_timer(cac_data_t *cac_data, uint32_t timeout)
{
    const char fname[] = "fsm_init_cac_failure_timer";

    CAC_DEBUG(DEB_F_PREFIX"cac_data call_id=%x\n",
              DEB_F_PREFIX_ARGS("CAC", fname),
              cac_data->call_id);

    cac_data->cac_fail_timer =
        cprCreateTimer("CAC failure timer", GSM_CAC_FAILURE_TIMER, TIMER_EXPIRATION,
                       gsm_msg_queue);

    if (cac_data->cac_fail_timer == NULL) {
        CAC_ERROR(CAC_F_PREFIX"CAC Timer allocation failed.\n",
                                    DEB_F_PREFIX_ARGS("CAC", fname));
        return(FALSE);
    }

    (void) cprStartTimer(cac_data->cac_fail_timer, timeout * 1000,
                         (void *)(long)cac_data->call_id);

    return(TRUE);
}














static cac_data_t *
fsm_cac_get_data_by_call_id (callid_t call_id)
{
    const char fname[] = "fsm_cac_get_data_by_call_id";
    cac_data_t *cac_data;

    cac_data = (cac_data_t *) sll_next(s_cac_list, NULL);

    while (cac_data != NULL) {

        if (cac_data->call_id == call_id) {
            CAC_DEBUG(DEB_F_PREFIX"cac_data found call_id=%x\n",
              DEB_F_PREFIX_ARGS("CAC", fname),
              cac_data->call_id);
            return(cac_data);
        }

        cac_data = (cac_data_t *) sll_next(s_cac_list, cac_data);

    }

    CAC_DEBUG(DEB_F_PREFIX"cac_data NOT found.\n",
        DEB_F_PREFIX_ARGS("CAC", fname));
    return(NULL);
}











void fsm_cac_init (void)
{
    const char fname[] = "fsm_cac_init";


    
    s_cac_list = sll_create((sll_match_e(*)(void *, void *))
                            fsm_cac_match_call_id);

    if (s_cac_list == NULL) {
        CAC_ERROR(CAC_F_PREFIX"CAC list creation failed.\n",
                                    DEB_F_PREFIX_ARGS("CAC", fname));

    }
}











void fsm_cac_clear_list (void)
{
    const char fname[] = "fsm_cac_clear_list";
    cac_data_t *cac_data;
    cac_data_t *prev_cac_data;

    DEF_DEBUG(DEB_F_PREFIX"Clear all pending CAC dat.\n",
                DEB_F_PREFIX_ARGS("CAC", fname));

    cac_data = (cac_data_t *) sll_next(s_cac_list, NULL);

    while (cac_data != NULL) {

        prev_cac_data = cac_data;
        cac_data = (cac_data_t *) sll_next(s_cac_list, cac_data);

        fsm_cac_notify_failure(prev_cac_data);
        fsm_clear_cac_data(prev_cac_data);
    }

}











void fsm_cac_shutdown (void)
{

    fsm_cac_clear_list();

    sll_destroy(s_cac_list);

    s_cac_list = NULL;
}











static cac_data_t *
fsm_cac_check_if_pending_req (void)
{
    cac_data_t *cac_data;

    cac_data = (cac_data_t *) sll_next(s_cac_list, NULL);

    while (cac_data != NULL) {

        if (cac_data->cac_state == FSM_CAC_REQ_PENDING ||
                cac_data->cac_state == FSM_CAC_IDLE) {

            return(cac_data);
        }

        cac_data = (cac_data_t *) sll_next(s_cac_list, cac_data);

    }

    return(NULL);
}











static cc_causes_t
fsm_cac_process_bw_allocation (cac_data_t *cac_data)
{
    const char fname[] = "fsm_cac_process_bw_allocation";

    if (lsm_allocate_call_bandwidth(cac_data->call_id, cac_data->sessions) ==
            CC_CAUSE_CONGESTION) {

        DEF_DEBUG(DEB_F_PREFIX"CAC Allocation failed.\n",
                DEB_F_PREFIX_ARGS("CAC", fname));

        fsm_cac_notify_failure(cac_data);

        fsm_clear_cac_data(cac_data);

        return(CC_CAUSE_CONGESTION);
    }

    cac_data->cac_state = FSM_CAC_REQ_PENDING;

    return(CC_CAUSE_OK);
}
















cc_causes_t
fsm_cac_call_bandwidth_req (callid_t call_id, uint32_t sessions,
                            void *msg)
{
    const char fname[] = "fsm_cac_call_bandwidth_req";
    cac_data_t *cac_data, *cac_pend_data;

    
    cac_data = fsm_get_new_cac_data();

    if (cac_data == NULL) {

        return(CC_CAUSE_CONGESTION);
    }

    cac_data->msg_ptr = msg;
    cac_data->call_id = call_id;
    cac_data->cac_state = FSM_CAC_IDLE;
    cac_data->sessions = sessions;

    fsm_init_cac_failure_timer(cac_data, CAC_FAILURE_TIMEOUT);

    


    if ((cac_pend_data = fsm_cac_check_if_pending_req()) == NULL) {

        



        DEF_DEBUG(DEB_F_PREFIX"CAC request for %d sessions %d.\n",
                DEB_F_PREFIX_ARGS("CAC", fname), call_id, sessions);

        if (fsm_cac_process_bw_allocation(cac_data) == CC_CAUSE_CONGESTION) {

            return(CC_CAUSE_CONGESTION);
        }

        cac_data->cac_state = FSM_CAC_REQ_PENDING;

    } else if (cac_pend_data->cac_state == FSM_CAC_IDLE) {

        if (fsm_cac_process_bw_allocation(cac_pend_data) ==
                    CC_CAUSE_CONGESTION) {

            
            fsm_cac_clear_list();

            return(CC_CAUSE_CONGESTION);
        }

    }

    (void) sll_append(s_cac_list, cac_data);

    return(CC_CAUSE_OK);

}













void fsm_cac_call_release_cleanup (callid_t call_id)
{
    cac_data_t *cac_data;

    cac_data = fsm_cac_get_data_by_call_id(call_id);

    if (cac_data) {

        sll_remove(s_cac_list, cac_data);

        fsm_clear_cac_data(cac_data);
    }

}
















cc_causes_t
fsm_cac_process_bw_avail_resp (void)
{
    const char      fname[] = "fsm_cac_process_bw_avail_resp";
    cac_data_t      *cac_data = NULL;
    cac_data_t      *next_cac_data = NULL;


    cac_data = (cac_data_t *) sll_next(s_cac_list, NULL);

    if (cac_data != NULL) {

        switch (cac_data->cac_state) {
        default:
        case FSM_CAC_IDLE:
            DEF_DEBUG(DEB_F_PREFIX"No Pending CAC request.\n",
                DEB_F_PREFIX_ARGS("CAC", fname));
            



            if (fsm_cac_process_bw_allocation(cac_data) == CC_CAUSE_CONGESTION) {

                sll_remove(s_cac_list, cac_data);

                return(CC_CAUSE_NO_RESOURCE);
            }


            break;
        case FSM_CAC_REQ_PENDING:

            next_cac_data = (cac_data_t *) sll_next(s_cac_list, cac_data);
            sll_remove(s_cac_list, cac_data);

            
            DEF_DEBUG(DEB_F_PREFIX"Process pending responses %d.\n",
                DEB_F_PREFIX_ARGS("CAC", fname), cac_data->call_id);

            
            fim_process_event(cac_data->msg_ptr, TRUE);

            fsm_clear_cac_data(cac_data);

            if (next_cac_data != NULL) {
                



                DEF_DEBUG(DEB_F_PREFIX"Requesting next allocation %d.\n",
                    DEB_F_PREFIX_ARGS("CAC", fname), next_cac_data->call_id);

                if (fsm_cac_process_bw_allocation(next_cac_data) ==
                                CC_CAUSE_CONGESTION) {

                    


                    if (next_cac_data->cac_state == FSM_CAC_IDLE) {
                        
                        fsm_cac_clear_list();
                    } else {

                        sll_remove(s_cac_list, next_cac_data);
                    }

                    return(CC_CAUSE_NO_RESOURCE);
                }

            }

            break;
        }

    }

    return(CC_CAUSE_NO_RESOURCE);

}














cc_causes_t
fsm_cac_process_bw_failed_resp (void)
{
    const char      fname[] = "fsm_cac_process_bw_avail_resp";
    cac_data_t      *cac_data = NULL;
    cac_data_t      *next_cac_data = NULL;


    cac_data = (cac_data_t *) sll_next(s_cac_list, NULL);

    if (cac_data != NULL) {

        switch (cac_data->cac_state) {
        default:
        case FSM_CAC_IDLE:
            DEF_DEBUG(DEB_F_PREFIX"No Pending request.\n",
                DEB_F_PREFIX_ARGS("CAC", fname));
            



            if (fsm_cac_process_bw_allocation(cac_data) == CC_CAUSE_CONGESTION) {

                sll_remove(s_cac_list, cac_data);

                return(CC_CAUSE_NO_RESOURCE);
            }

            break;
        case FSM_CAC_REQ_PENDING:

            next_cac_data = (cac_data_t *) sll_next(s_cac_list, cac_data);

            sll_remove(s_cac_list, cac_data);

            
            DEF_DEBUG(DEB_F_PREFIX"Process pending responses even after failure.\n",
                DEB_F_PREFIX_ARGS("CAC", fname));

            
            fsm_cac_notify_failure(cac_data);

            fsm_clear_cac_data(cac_data);

            if (next_cac_data != NULL) {

                



                if (fsm_cac_process_bw_allocation(next_cac_data) == CC_CAUSE_CONGESTION) {

                    


                    if (next_cac_data->cac_state == FSM_CAC_IDLE) {
                        
                        fsm_cac_clear_list();
                    } else {

                        sll_remove(s_cac_list, next_cac_data);
                    }

                    return(CC_CAUSE_NO_RESOURCE);
                }

            }

            break;
        }

    }

    return(CC_CAUSE_NO_RESOURCE);
}












void
fsm_cac_process_bw_fail_timer (void *tmr_data)
{
    const char      fname[] = "fsm_cac_process_bw_fail_timer";

    DEF_DEBUG(DEB_F_PREFIX"CAC request timedout %d.\n",
                    DEB_F_PREFIX_ARGS("CAC", fname), (callid_t)(long)tmr_data);

    

    fsm_cac_process_bw_failed_resp();

}

