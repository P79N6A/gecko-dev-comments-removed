



#include "cpr_strings.h"
#include "pres_sub_not_handler.h"
#include "singly_link_list.h"
#include "ccsip_subsmanager.h"
#include "ccsip_messaging.h"
#include "ccapi.h"
#include "tnp_blf.h"
#include "debug.h"
#include "phone_debug.h"
#include "phntask.h"
#include "subapi.h"
#include "phone_platform_constants.h"
#include "misc_apps_task.h"
#include "cc_blf_listener.h"
#include "uiapi.h"

cc_int32_t g_blfDebug = 0;

#define DEFAULT_RETRY_AFTER_MILLISECS 5000
typedef enum {
    PRES_RETRYAFTER_TIMER = 1
} pres_timers_e;

#define BLF_PICKUP_FEATURE   0x1
typedef struct {
    int      request_id;        
    sub_id_t sub_id;            
    int      duration;          
    char     presentity[CC_MAX_DIALSTRING_LEN];
    
    char     watcher[CC_MAX_DIALSTRING_LEN];
    
    int      app_id;            
    uint32_t highest_cseq;      
    int      feature_mask;
    int      blf_state; 
} pres_subscription_req_t;

typedef struct {
    char                presentity[CC_MAX_DIALSTRING_LEN];
    ccsip_event_data_t *event_data_p;
} pres_pending_notify_t;

static void subscribe_response_ind(ccsip_sub_not_data_t *msg_data);
static void notify_ind_cb(ccsip_sub_not_data_t *msg_data);
static void terminate_cb(ccsip_sub_not_data_t *msg_data);
static int extract_blf_state(Presence_ext_t *event_body_p, int feature_mask);
static void free_sub_request(pres_subscription_req_t *sup_req_p);
static void process_timer_expiration(void *msg_p);
static boolean apply_presence_state_to_matching_feature_keys(char *presentity,
                                                             Presence_ext_t *event_body_p);
static void append_notification_to_pending_queue(ccsip_event_data_t *event_body_p);
static void sub_handler_initialized(void);

static sll_handle_t s_pres_req_list = NULL; 
static sll_handle_t s_pending_notify_list = NULL;
static boolean s_subs_hndlr_initialized = FALSE;







static cprTimer_t s_retry_after_timers[MAX_REG_LINES];











cc_rcs_t
send_subscribe_ev_to_sip_task (pres_subscription_req_t *sub_req_p)
{
    sipspi_msg_t subscribe_msg;

    


    memset(&subscribe_msg, 0, sizeof(sipspi_msg_t));
    subscribe_msg.msg.subscribe.eventPackage = CC_SUBSCRIPTIONS_PRESENCE;
    subscribe_msg.msg.subscribe.sub_id = sub_req_p->sub_id;
    subscribe_msg.msg.subscribe.auto_resubscribe = TRUE;
    subscribe_msg.msg.subscribe.request_id = sub_req_p->request_id;
    subscribe_msg.msg.subscribe.duration = sub_req_p->duration;
    sstrncpy(subscribe_msg.msg.subscribe.subscribe_uri, sub_req_p->presentity,
             CC_MAX_DIALSTRING_LEN);
    sstrncpy(subscribe_msg.msg.subscribe.subscriber_uri, sub_req_p->watcher,
             CC_MAX_DIALSTRING_LEN);
    subscribe_msg.msg.subscribe.dn_line =
        get_dn_line_from_dn(sub_req_p->watcher);
    subscribe_msg.msg.subscribe.subsNotCallbackTask = CC_SRC_MISC_APP;
    subscribe_msg.msg.subscribe.subsResCallbackMsgID =
        SUB_MSG_PRESENCE_SUBSCRIBE_RESP;
    subscribe_msg.msg.subscribe.subsNotIndCallbackMsgID =
        SUB_MSG_PRESENCE_NOTIFY;
    subscribe_msg.msg.subscribe.subsTermCallbackMsgID =
        SUB_MSG_PRESENCE_TERMINATE;
    return (sub_int_subscribe(&subscribe_msg));
}











static sll_match_e
find_matching_node (void *key, void *data)
{
    int request_id = *((int *)key);
    pres_subscription_req_t *req_p = (pres_subscription_req_t *) data;

    if (request_id == req_p->request_id) {
        return SLL_MATCH_FOUND;
    }

    return SLL_MATCH_NOT_FOUND;
}

typedef struct {
    int request_id;
    int duration;
    char watcher[CC_MAX_DIALSTRING_LEN];
    char presentity[CC_MAX_DIALSTRING_LEN];
    int app_id;
    int feature_mask;
} pres_req_msg;





















void
pres_get_state (int request_id,
                int duration,
                const char *watcher,
                const char *presentity,
                int app_id,
                int feature_mask)
{
    pres_req_msg msg;

    msg.request_id = request_id;
    msg.duration = duration;
    sstrncpy(msg.presentity, presentity, CC_MAX_DIALSTRING_LEN);
    sstrncpy(msg.watcher, watcher, CC_MAX_DIALSTRING_LEN);
    msg.app_id = app_id;
    msg.feature_mask = feature_mask;

    (void) app_send_message(&msg, sizeof(pres_req_msg), CC_SRC_MISC_APP,
                            SUB_MSG_PRESENCE_GET_STATE);
}























static void
get_state (int request_id,
           int duration,
           const char *watcher,
           const char *presentity,
           int app_id,
           int feature_mask)
{
    static const char fname[] = "get_state";
    pres_subscription_req_t *sub_req_p;

    DEF_DEBUG(DEB_F_PREFIX"REQ %d: TM %d: WTR %s: PRT %s: FMSK %d: APP %d\n",
         DEB_F_PREFIX_ARGS(BLF_INFO, fname),
         request_id, duration, watcher, presentity, feature_mask, app_id);
    


    if (s_pres_req_list == NULL) {
        s_pres_req_list = sll_create(find_matching_node);
        if (s_pres_req_list == NULL) {
            
            ui_BLF_notification(request_id, CC_SIP_BLF_REJECTED, app_id);
            BLF_ERROR(MISC_F_PREFIX"Exiting : request list creation failed\n", fname);
            return;
        }
    }
    


    if ((sub_req_p = (pres_subscription_req_t *)
                sll_find(s_pres_req_list, &request_id)) == NULL) {
        


        sub_req_p = (pres_subscription_req_t *)
            cpr_malloc(sizeof(pres_subscription_req_t));
        if (sub_req_p == NULL) {
            BLF_ERROR(MISC_F_PREFIX"Exiting : malloc failed\n", fname);
            return;
        }

        sub_req_p->request_id = request_id;
        sub_req_p->sub_id = CCSIP_SUBS_INVALID_SUB_ID;
        sub_req_p->highest_cseq = 0;
        sub_req_p->duration = duration;
        sstrncpy(sub_req_p->presentity, presentity, CC_MAX_DIALSTRING_LEN);
        sstrncpy(sub_req_p->watcher, watcher, CC_MAX_DIALSTRING_LEN);
        sub_req_p->app_id = app_id;
        sub_req_p->feature_mask = feature_mask;
        sub_req_p->blf_state = CC_SIP_BLF_UNKNOWN;

        (void) sll_append(s_pres_req_list, sub_req_p);
    } else { 
        sub_req_p->duration = duration;
    }

    


    if (send_subscribe_ev_to_sip_task(sub_req_p) != CC_RC_SUCCESS) {
        


        free_sub_request(sub_req_p);
        
        ui_BLF_notification(request_id, CC_SIP_BLF_REJECTED, app_id);
        BLF_ERROR(MISC_F_PREFIX"Exiting : Unable to send SUBSCRIBE\n", fname);
        return;
    }

    BLF_DEBUG(DEB_F_PREFIX"Exiting : request made successfully\n", DEB_F_PREFIX_ARGS(BLF, fname));
    return;
}











void
pres_terminate_req (int request_id)
{
    (void) app_send_message(&request_id, sizeof(request_id), CC_SRC_MISC_APP,
                            SUB_MSG_PRESENCE_TERM_REQ);
}












static void
terminate_req (int request_id)
{
    static const char fname[] = "terminate_req";
    pres_subscription_req_t *sub_req_p;

    BLF_DEBUG(DEB_F_PREFIX"Entering (request_id=%d)\n",
              DEB_F_PREFIX_ARGS(BLF, fname), request_id);

    


    if ((sub_req_p = (pres_subscription_req_t *)
                sll_find(s_pres_req_list, &request_id)) == NULL) {
        BLF_ERROR(MISC_F_PREFIX"request does not exist in the list\n", fname);
        return;
    }

    


    sub_req_p->duration = 0;
    



    (void) send_subscribe_ev_to_sip_task(sub_req_p);

    




    (void) sub_int_subscribe_term(sub_req_p->sub_id, FALSE,
                                  sub_req_p->request_id,
                                  CC_SUBSCRIPTIONS_PRESENCE);

    


    free_sub_request(sub_req_p);

    BLF_DEBUG(DEB_F_PREFIX"Exiting : request terminated\n", DEB_F_PREFIX_ARGS(BLF, fname));
    return;
}











void
pres_terminate_req_all (void)
{
    int dummy = 0;

    (void) app_send_message(&dummy, sizeof(dummy), CC_SRC_MISC_APP,
                            SUB_MSG_PRESENCE_TERM_REQ_ALL);
}








void
pres_sub_handler_initialized (void)
{
    int dummy = 0;

    (void) app_send_message(&dummy, sizeof(dummy), CC_SRC_MISC_APP,
                            SUB_HANDLER_INITIALIZED);
}











static void
terminate_req_all (void)
{
    static const char fname[] = "terminate_req_all";
    pres_subscription_req_t *sub_req_p;

    BLF_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(BLF, fname));
    if (s_pres_req_list == NULL) {
        BLF_DEBUG(DEB_F_PREFIX"Exiting : no outstanding requests\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }

    while ((sub_req_p = (pres_subscription_req_t *)
                sll_next(s_pres_req_list, NULL)) != NULL) {
        


        (void) sub_int_subscribe_term(sub_req_p->sub_id, TRUE,
                                      sub_req_p->request_id,
                                      CC_SUBSCRIPTIONS_PRESENCE);

        


        free_sub_request(sub_req_p);
    }
    



    s_subs_hndlr_initialized = FALSE;
    BLF_DEBUG(DEB_F_PREFIX"Exiting\n", DEB_F_PREFIX_ARGS(BLF, fname));
}















static void
subscribe_response_ind (ccsip_sub_not_data_t *msg_data)
{
    static const char fname[] = "subscribe_response_ind";
    int status_code = msg_data->u.subs_result_data.status_code;
    int request_id = msg_data->request_id;
    sub_id_t sub_id = msg_data->sub_id;
    pres_subscription_req_t *sub_req_p;

    BLF_DEBUG(DEB_F_PREFIX"Entering (status_code=%d)\n",
              DEB_F_PREFIX_ARGS(BLF, fname), status_code);

    if ((s_pres_req_list == NULL) ||
        ((sub_req_p = (pres_subscription_req_t *)
          sll_find(s_pres_req_list, &request_id)) == NULL)) {
        



        (void) sub_int_subscribe_term(sub_id, TRUE, request_id,
                                      CC_SUBSCRIPTIONS_PRESENCE);
        BLF_DEBUG(DEB_F_PREFIX"Exiting : subscription does not exist\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }

    


    sub_req_p->sub_id = sub_id;

    


    if ((status_code >= 100) && (status_code < 300)) {
        
        BLF_DEBUG(DEB_F_PREFIX"Exiting :100-299 response\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }

    


    if (status_code == SIP_CLI_ERR_INTERVAL_TOO_SMALL) {
        sub_req_p->duration = msg_data->u.subs_result_data.expires;
        if (send_subscribe_ev_to_sip_task(sub_req_p) != CC_RC_SUCCESS) {
            
            ui_BLF_notification(request_id, CC_SIP_BLF_REJECTED, sub_req_p->app_id);
            


            free_sub_request(sub_req_p);
            BLF_ERROR(MISC_F_PREFIX"Exiting : Unable to send SUBSCRIBE\n", fname);
            return;
        }
        BLF_DEBUG(DEB_F_PREFIX"Exiting : subscribed again with double duration\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }

    



    if ((status_code == SIP_CLI_ERR_CALLEG) && (sub_req_p->app_id > 0)) {
        ui_BLF_notification(request_id, CC_SIP_BLF_UNKNOWN, sub_req_p->app_id); 
        sub_req_p->blf_state = CC_SIP_BLF_UNKNOWN;
        


        (void) sub_int_subscribe_term(sub_req_p->sub_id, TRUE,
                                      sub_req_p->request_id,
                                      CC_SUBSCRIPTIONS_PRESENCE);
        


        sub_req_p->sub_id = CCSIP_SUBS_INVALID_SUB_ID;
        sub_req_p->highest_cseq = 0;
        if (send_subscribe_ev_to_sip_task(sub_req_p) != CC_RC_SUCCESS) {
            
            ui_BLF_notification(request_id, CC_SIP_BLF_REJECTED, sub_req_p->app_id);
            


            free_sub_request(sub_req_p);
            BLF_ERROR(MISC_F_PREFIX"Exiting : Unable to send SUBSCRIBE\n", fname);
            return;
        }
        BLF_DEBUG(DEB_F_PREFIX"Exiting : subscribed again after receiving 481\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }

    





    ui_BLF_notification(request_id, CC_SIP_BLF_REJECTED, sub_req_p->app_id);
    


    (void) sub_int_subscribe_term(sub_req_p->sub_id, TRUE,
                                  sub_req_p->request_id,
                                  CC_SUBSCRIPTIONS_PRESENCE);

    


    free_sub_request(sub_req_p);

    BLF_DEBUG(DEB_F_PREFIX"Exiting : request terminated\n", DEB_F_PREFIX_ARGS(BLF, fname));
    return;
}











static void
notify_ind_cb (ccsip_sub_not_data_t * msg_data)
{
    static const char fname[] = "notify_ind_cb";
    int sub_state = msg_data->u.notify_ind_data.subscription_state;
    sip_subs_state_reason_e reason =
    msg_data->u.notify_ind_data.subscription_state_reason;
    uint32_t retry_after = msg_data->u.notify_ind_data.retry_after;
    int request_id = msg_data->request_id;
    sub_id_t sub_id = msg_data->sub_id;
    pres_subscription_req_t *sub_req_p;
    Presence_ext_t *event_body_p = NULL;
    uint32_t cseq = msg_data->u.notify_ind_data.cseq;
    int blf_state;

    BLF_DEBUG(DEB_F_PREFIX"Entering (subscription_state=%d)\n",
              DEB_F_PREFIX_ARGS(BLF, fname), sub_state);

    



    if ((msg_data->u.notify_ind_data.eventData != NULL) &&
        (msg_data->u.notify_ind_data.eventData->type != EVENT_DATA_PRESENCE)) {
        BLF_ERROR(MISC_F_PREFIX"NOTIFY does not contain presence body\n", fname);
        free_event_data(msg_data->u.notify_ind_data.eventData);
        msg_data->u.notify_ind_data.eventData = NULL;
    }

    event_body_p = (msg_data->u.notify_ind_data.eventData == NULL) ?
        NULL : &(msg_data->u.notify_ind_data.eventData->u.presence_rpid);


    if ((s_pres_req_list == NULL) ||
        ((sub_req_p = (pres_subscription_req_t *)
          sll_find(s_pres_req_list, &request_id)) == NULL)) {
        




        (void) sub_int_notify_ack(sub_id, SIP_CLI_ERR_CALLEG, cseq);

        (void) sub_int_subscribe_term(sub_id, TRUE, request_id,
                                      CC_SUBSCRIPTIONS_PRESENCE);
        free_event_data(msg_data->u.notify_ind_data.eventData);
        BLF_DEBUG(DEB_F_PREFIX"Exiting : subscription does not exist\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }

    


    (void) sub_int_notify_ack(sub_id, SIP_STATUS_SUCCESS, cseq);

    


    if (cseq < sub_req_p->highest_cseq) {
        free_event_data(msg_data->u.notify_ind_data.eventData);
        BLF_ERROR(MISC_F_PREFIX"Exiting : out of sequence NOTIFY received\n", fname);
        return;
    } else {
        sub_req_p->highest_cseq = cseq;
    }


    


    if (sub_state == SUBSCRIPTION_STATE_TERMINATED) {
        


        (void) sub_int_subscribe_term(sub_id, TRUE, sub_req_p->request_id,
                                      CC_SUBSCRIPTIONS_PRESENCE);
        if (reason == SUBSCRIPTION_STATE_REASON_DEACTIVATED) {
            
            sub_req_p->sub_id = CCSIP_SUBS_INVALID_SUB_ID;
            sub_req_p->highest_cseq = 0;
            


            if (send_subscribe_ev_to_sip_task(sub_req_p) != CC_RC_SUCCESS) {
                
                ui_BLF_notification(request_id, CC_SIP_BLF_REJECTED,
                                 sub_req_p->app_id);
                


                free_sub_request(sub_req_p);
            }
        } else if ((reason == SUBSCRIPTION_STATE_REASON_TIMEOUT) ||
                   (reason == SUBSCRIPTION_STATE_REASON_PROBATION) ||
                   (reason == SUBSCRIPTION_STATE_REASON_GIVEUP)) {
            
            ui_BLF_notification(request_id, CC_SIP_BLF_EXPIRED, sub_req_p->app_id);
            sub_req_p->blf_state = CC_SIP_BLF_EXPIRED;
            if (sub_req_p->app_id > 0) {
                


                sub_req_p->sub_id = CCSIP_SUBS_INVALID_SUB_ID;
                sub_req_p->highest_cseq = 0;
                if ((reason == SUBSCRIPTION_STATE_REASON_PROBATION) ||
                    (reason == SUBSCRIPTION_STATE_REASON_GIVEUP)) {
                    



                    if (retry_after == 0) {
                        retry_after = DEFAULT_RETRY_AFTER_MILLISECS;
                    } else {
                        retry_after = (retry_after * 1000); 
                    }
                    if ((cprCancelTimer(s_retry_after_timers[sub_req_p->app_id - 1])
                                == CPR_SUCCESS) &&
                        (cprStartTimer(s_retry_after_timers[sub_req_p->app_id - 1],
                          retry_after, (void *) sub_req_p) == CPR_SUCCESS)) {
                        


                        free_event_data(msg_data->u.notify_ind_data.eventData);
                        BLF_DEBUG(DEB_F_PREFIX"Exiting : retry_after Timer started\n",
                                  DEB_F_PREFIX_ARGS(BLF, fname));
                        return;
                    }

                }
                if (send_subscribe_ev_to_sip_task(sub_req_p) != CC_RC_SUCCESS) {
                    


                    free_sub_request(sub_req_p);
                    BLF_ERROR(MISC_F_PREFIX"Unable to send SUBSCRIBE\n", fname);
                }
                BLF_DEBUG(DEB_F_PREFIX"subscribed again after expiration\n", DEB_F_PREFIX_ARGS(BLF, fname));
            } else {
                


                free_sub_request(sub_req_p);
            }
        } else {
            ui_BLF_notification(request_id, CC_SIP_BLF_REJECTED, sub_req_p->app_id);
            


            free_sub_request(sub_req_p);
        }
    } else {
        
        blf_state = extract_blf_state(event_body_p, sub_req_p->feature_mask);
        ui_BLF_notification(request_id, blf_state, sub_req_p->app_id);
        sub_req_p->blf_state = blf_state;
        



        if (blf_state == CC_SIP_BLF_ALERTING) {
            


            cc_feature(CC_SRC_MISC_APP, CC_NO_CALL_ID, 0, CC_FEATURE_BLF_ALERT_TONE, NULL);
        }
        DEF_DEBUG(DEB_F_PREFIX"SUB %d: BLF %d\n",
            DEB_F_PREFIX_ARGS(BLF_INFO, fname), sub_state, blf_state);
    }

    free_event_data(msg_data->u.notify_ind_data.eventData);
    BLF_DEBUG(DEB_F_PREFIX"Exiting : acted based on subscription state\n", DEB_F_PREFIX_ARGS(BLF, fname));
    return;
}











static void
unsolicited_notify_ind_cb (ccsip_sub_not_data_t *msg_data)
{
    Presence_ext_t *event_body_p = NULL;
    char  *presentity_url = NULL;
    char  presentity_user[CC_MAX_DIALSTRING_LEN];
    static const char fname[] = "unsolicited_notify_ind_cb";

    BLF_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(BLF, fname));
    



    if ((msg_data->u.notify_ind_data.eventData != NULL) &&
        (msg_data->u.notify_ind_data.eventData->type != EVENT_DATA_PRESENCE)) {
        BLF_ERROR(MISC_F_PREFIX"NOTIFY does not contain presence body\n", fname);
        free_event_data(msg_data->u.notify_ind_data.eventData);
        msg_data->u.notify_ind_data.eventData = NULL;
    }

    event_body_p = (msg_data->u.notify_ind_data.eventData == NULL) ?
        NULL : &(msg_data->u.notify_ind_data.eventData->u.presence_rpid);

    if (event_body_p == NULL) {
        BLF_DEBUG("Exiting pres_sub_not_handler.c:%s(): no presence body", fname);
        return;
    }

    if (s_subs_hndlr_initialized == FALSE) {
        



        append_notification_to_pending_queue(msg_data->u.notify_ind_data.eventData);
        BLF_DEBUG("MSC: 0/0: %s: appended presence notification to the pending queue",
                  fname);
        return;

    }

    if (s_pres_req_list == NULL) {
        free_event_data(msg_data->u.notify_ind_data.eventData);
        BLF_DEBUG(DEB_F_PREFIX"Exiting : no pres requests\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }
    
    presentity_url = strchr(event_body_p->presence_body.entity, ':');
    if (presentity_url == NULL)
    {
        BLF_ERROR("MSC:  Error parsing presentity_url", fname);
        return;
    }

    presentity_url = presentity_url + 1;

    



    if (apply_presence_state_to_matching_feature_keys(presentity_url, event_body_p) != TRUE) {
        ccsip_util_extract_user(event_body_p->presence_body.entity, presentity_user);
        if (apply_presence_state_to_matching_feature_keys(presentity_user, event_body_p) != TRUE) {
            BLF_DEBUG("pres_sub_not_handler.c:%s(): no matching BLF feature keys found", fname);
        }
    }

    free_event_data(msg_data->u.notify_ind_data.eventData);
    BLF_DEBUG("Exiting pres_sub_not_handler.c:%s(): pres state processed successfully", fname);
    return;
}











static
boolean apply_presence_state_to_matching_feature_keys (char *presentity,
                                                       Presence_ext_t *event_body_p)
{
    pres_subscription_req_t *sub_req_p;
    int blf_state;
    boolean match_found = FALSE;

    sub_req_p = (pres_subscription_req_t *)sll_next(s_pres_req_list, NULL);
    while (sub_req_p != NULL) { 
        if ((sub_req_p->app_id > 0) &&
            (strncmp(sub_req_p->presentity, presentity, CC_MAX_DIALSTRING_LEN - 1) == 0)) {
            match_found = TRUE;
            
            blf_state = extract_blf_state(event_body_p, sub_req_p->feature_mask);
            ui_BLF_notification(sub_req_p->request_id, blf_state, sub_req_p->app_id);
            sub_req_p->blf_state = blf_state;
            



            if (blf_state == CC_SIP_BLF_ALERTING) {
                


                cc_feature(CC_SRC_MISC_APP, CC_NO_CALL_ID, 0, CC_FEATURE_BLF_ALERT_TONE, NULL);
            }
        }
        sub_req_p = (pres_subscription_req_t *)sll_next(s_pres_req_list, sub_req_p);
    }
    return match_found;
}










static void append_notification_to_pending_queue (ccsip_event_data_t *event_data_p)
{
    static const char fname[] = "append_notification_to_pending_queue";
    pres_pending_notify_t *pending_notify_p;
    Presence_ext_t *event_body_p = &(event_data_p->u.presence_rpid);

    


    if (s_pending_notify_list == NULL) {
        s_pending_notify_list = sll_create(NULL);
        if (s_pending_notify_list == NULL) {
            err_msg("MSC: 0/0: %s: out of memory", fname);
            free_event_data(event_data_p);
            return;
        }
    }

    pending_notify_p = (pres_pending_notify_t *)sll_next(s_pending_notify_list, NULL);
    while (pending_notify_p != NULL) {
        if (strncmp(pending_notify_p->presentity, event_body_p->presence_body.entity,
                    CC_MAX_DIALSTRING_LEN - 1) == 0) {
            
            free_event_data(pending_notify_p->event_data_p);
            pending_notify_p->event_data_p = event_data_p;
            return;
        }
        pending_notify_p = (pres_pending_notify_t *)sll_next(s_pending_notify_list,
                                                             pending_notify_p);
    }

    



    if (sll_count(s_pending_notify_list) == MAX_REG_LINES) {
        err_msg("MSC: 0/0: %s: ignoring the NOTIFY to protect from DoS attack", fname);
        free_event_data(event_data_p);
        return;
    }
    pending_notify_p = (pres_pending_notify_t *)
                       cpr_malloc(sizeof(pres_pending_notify_t));
    if (pending_notify_p == NULL) {
        err_msg("MSC: 0/0: %s: out of memory", fname);
        free_event_data(event_data_p);
        return;
    }
    sstrncpy(pending_notify_p->presentity, event_body_p->presence_body.entity,
             CC_MAX_DIALSTRING_LEN);
    pending_notify_p->event_data_p = event_data_p;
    (void) sll_append(s_pending_notify_list, pending_notify_p);
    return;
}








static void sub_handler_initialized (void)
{
    static const char fname[] = "sub_handler_initialized";
    pres_pending_notify_t *pending_notify_p;
    char  *presentity_url = NULL;
    char  presentity_user[CC_MAX_DIALSTRING_LEN];
    Presence_ext_t *event_body_p = NULL;

    BLF_DEBUG("MSC: 0/0: %s: invoked\n", fname);
    s_subs_hndlr_initialized = TRUE;

    if (s_pending_notify_list == NULL) {
        BLF_DEBUG("MSC: 0/0: %s: no pending notfications\n", fname);
        return;
    }

    


    pending_notify_p = (pres_pending_notify_t *)sll_next(s_pending_notify_list, NULL);
    while (pending_notify_p != NULL) {
        
        presentity_url = strchr(pending_notify_p->presentity, ':');
        if (presentity_url == NULL)
        {
            BLF_ERROR("MSC:  Error parsing presentity_url", fname);
            return;
        }

        presentity_url = presentity_url + 1;

        



        event_body_p = &(pending_notify_p->event_data_p->u.presence_rpid);
        if (apply_presence_state_to_matching_feature_keys(presentity_url, event_body_p)
            != TRUE) {
            ccsip_util_extract_user(pending_notify_p->presentity, presentity_user);
            if (apply_presence_state_to_matching_feature_keys(presentity_user,
                event_body_p) != TRUE) {
                BLF_DEBUG("MSC: 0/0: %s: no matching BLF feature keys found", fname);
            }
        }
        BLF_DEBUG("MSC: 0/0: %s: processed a pending notfication for %s\n",
                  fname, presentity_url);
        free_event_data(pending_notify_p->event_data_p);
        (void) sll_remove(s_pending_notify_list, (void *)pending_notify_p);
        cpr_free(pending_notify_p);

        pending_notify_p = (pres_pending_notify_t *)sll_next(s_pending_notify_list,
                                                             NULL);
    }
    (void)sll_destroy(s_pending_notify_list);
    s_pending_notify_list = NULL;
}














static void
terminate_cb (ccsip_sub_not_data_t *msg_data)
{
    static const char fname[] = "terminate_cb";
    ccsip_reason_code_e reason_code = msg_data->reason_code;
    int status_code = msg_data->u.subs_term_data.status_code;
    int request_id = msg_data->request_id;
    sub_id_t sub_id = msg_data->sub_id;
    pres_subscription_req_t *sub_req_p = NULL;
    int orig_duration = 0;

    BLF_DEBUG(DEB_F_PREFIX"Entering (reason_code=%d, status_code=%d)\n",
              DEB_F_PREFIX_ARGS(BLF, fname), reason_code, status_code);

    if (s_pres_req_list != NULL) {
        sub_req_p = (pres_subscription_req_t *)
            sll_find(s_pres_req_list, &request_id);
    }
    if (sub_req_p == NULL) {
        



        (void) sub_int_subscribe_term(sub_id, TRUE, request_id,
                                      CC_SUBSCRIPTIONS_PRESENCE);
        BLF_DEBUG(DEB_F_PREFIX"Exiting : subscription does not exist\n", DEB_F_PREFIX_ARGS(BLF, fname));
        return;
    }

    orig_duration = sub_req_p->duration;
    if (reason_code == SM_REASON_CODE_ERROR) { 
        


        sub_req_p->duration = 0;
        



        (void) send_subscribe_ev_to_sip_task(sub_req_p);
    }
    




    if ((reason_code != SM_REASON_CODE_RESET_REG) &&
        (reason_code != SM_REASON_CODE_ROLLOVER) &&
        (reason_code != SM_REASON_CODE_SHUTDOWN)) {
        (void) sub_int_subscribe_term(sub_id, TRUE, request_id,
                                  CC_SUBSCRIPTIONS_PRESENCE);
    }

    
    ui_BLF_notification(request_id, CC_SIP_BLF_UNKNOWN, sub_req_p->app_id);

    if ((reason_code == SM_REASON_CODE_ERROR) ||
        (reason_code == SM_REASON_CODE_RESET_REG)) {
        


        sub_req_p->sub_id = CCSIP_SUBS_INVALID_SUB_ID;
        sub_req_p->highest_cseq = 0;
        sub_req_p->duration = orig_duration;
        (void) send_subscribe_ev_to_sip_task(sub_req_p);
    } else {
        


        free_sub_request(sub_req_p);
    }

    BLF_DEBUG(DEB_F_PREFIX"Exiting : terminated subscription\n", DEB_F_PREFIX_ARGS(BLF, fname));
    return;
}











static int
extract_blf_state (Presence_ext_t *event_body_p, int feature_mask)
{
    static const char fname[] = "extract_blf_state";
    char *basic_p = NULL;
    boolean on_the_phone;
    boolean alerting;
    int return_code = CC_SIP_BLF_UNKNOWN;

    BLF_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(BLF, fname));

    if (event_body_p == NULL) {
        BLF_DEBUG(DEB_F_PREFIX
                  "Exiting with return value %d because there is no event body\n",
                  DEB_F_PREFIX_ARGS(BLF, fname), CC_SIP_BLF_UNKNOWN);
        return CC_SIP_BLF_UNKNOWN;
    }
    basic_p = event_body_p->presence_body.person.personStatus.basic;
    if (basic_p[0] == '\0') {
        basic_p = event_body_p->presence_body.tuple[0].status.basic;
    }
    on_the_phone = event_body_p->onThePhone;
    alerting = event_body_p->alerting;
    BLF_DEBUG(DEB_F_PREFIX"basic: %s, onThePhone:%d, alerting:%d\n",
              DEB_F_PREFIX_ARGS(BLF, fname), basic_p, on_the_phone, alerting);

    if ((on_the_phone == FALSE) &&
        (cpr_strcasecmp(basic_p, "closed") == 0)) {
        BLF_DEBUG(DEB_F_PREFIX"Exiting with return value %d\n",
                  DEB_F_PREFIX_ARGS(BLF, fname), CC_SIP_BLF_UNKNOWN);
        return(CC_SIP_BLF_UNKNOWN);
    }

    if (feature_mask & BLF_PICKUP_FEATURE) {
        if (alerting == TRUE) {
            BLF_DEBUG(DEB_F_PREFIX"Exiting with return value %d\n",
                      DEB_F_PREFIX_ARGS(BLF, fname), CC_SIP_BLF_ALERTING);
            return CC_SIP_BLF_ALERTING;

        }
    }
    if (on_the_phone == FALSE) {
        if (cpr_strcasecmp(basic_p, "open") == 0) {
            return_code = CC_SIP_BLF_IDLE;
        }
    } else {
        return_code = CC_SIP_BLF_INUSE;
    }

    BLF_DEBUG(DEB_F_PREFIX"Exiting with return value %d\n",
              DEB_F_PREFIX_ARGS(BLF, fname), return_code);
    return return_code;
}

void
pres_process_msg_from_msgq (uint32_t cmd, void *msg_p)
{
    static const char fname[] = "pres_process_msg_from_msgq";
    pres_req_msg *pres_req_p;

    BLF_DEBUG(DEB_F_PREFIX"Entering (cmd=%d, msg_p=0x%X)\n",
              DEB_F_PREFIX_ARGS(BLF, fname), cmd, msg_p);

    switch (cmd) {
    case SUB_HANDLER_INITIALIZED:
        sub_handler_initialized();
        break;

    case SUB_MSG_PRESENCE_GET_STATE:
        pres_req_p = (pres_req_msg *) msg_p;
        get_state(pres_req_p->request_id,
                  pres_req_p->duration,
                  pres_req_p->watcher,
                  pres_req_p->presentity,
                  pres_req_p->app_id,
                  pres_req_p->feature_mask);
        break;

    case SUB_MSG_PRESENCE_TERM_REQ:
        terminate_req(*((int *)msg_p));
        break;

    case SUB_MSG_PRESENCE_TERM_REQ_ALL:
        terminate_req_all();
        break;

    case SUB_MSG_PRESENCE_SUBSCRIBE_RESP:
        subscribe_response_ind((ccsip_sub_not_data_t *)msg_p);
        break;

    case SUB_MSG_PRESENCE_NOTIFY:
        notify_ind_cb((ccsip_sub_not_data_t *)msg_p);
        break;

    case SUB_MSG_PRESENCE_UNSOLICITED_NOTIFY:
        unsolicited_notify_ind_cb((ccsip_sub_not_data_t *)msg_p);
        break;

    case SUB_MSG_PRESENCE_TERMINATE:
        terminate_cb((ccsip_sub_not_data_t *)msg_p);
        break;

    case TIMER_EXPIRATION:
        process_timer_expiration(msg_p);
        break;

    default:
        BLF_ERROR(MISC_F_PREFIX"bad Cmd received: %d\n", fname, cmd);
        break;
    }
    BLF_DEBUG(DEB_F_PREFIX"Exiting\n", DEB_F_PREFIX_ARGS(BLF, fname));
}










cpr_status_e
pres_create_retry_after_timers (void)
{
    int i;
    int j;

    


    for (i = 0; i < MAX_REG_LINES; i++) {
        s_retry_after_timers[i] =
            cprCreateTimer("Presence/BLF Retry After Timer",
                           PRES_RETRYAFTER_TIMER, TIMER_EXPIRATION,
                           s_misc_msg_queue);
        if (!s_retry_after_timers[i]) {
            


            for (j = 0; j < i; j++) {
                (void) cprDestroyTimer(s_retry_after_timers[j]);
                s_retry_after_timers[j] = NULL;
            }
            return CPR_FAILURE;
        }
    }
    return CPR_SUCCESS;
}









void
pres_destroy_retry_after_timers (void)
{
    int i;

    


    for (i = 0; i < MAX_REG_LINES; i++) {
        if (s_retry_after_timers[i] != NULL) {
            (void) cprDestroyTimer(s_retry_after_timers[i]);
            s_retry_after_timers[i] = NULL;
        }
    }
}










static void
free_sub_request (pres_subscription_req_t *sub_req_p)
{
    


    (void) sll_remove(s_pres_req_list, (void *)sub_req_p);

    


    if (sub_req_p->app_id > 0) {
        (void) cprCancelTimer(s_retry_after_timers[sub_req_p->app_id - 1]);
    }

    


    cpr_free(sub_req_p);
}










static void
process_timer_expiration (void *msg_p)
{
    static const char fname[] = "process_timer_expiration";
    cprCallBackTimerMsg_t *timerMsg;
    pres_subscription_req_t *sub_req_p;

    BLF_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(BLF, fname));

    timerMsg = (cprCallBackTimerMsg_t *) msg_p;
    switch (timerMsg->expiredTimerId) {
    case PRES_RETRYAFTER_TIMER:
        sub_req_p = (pres_subscription_req_t *) (timerMsg->usrData);
        if (send_subscribe_ev_to_sip_task(sub_req_p) != CC_RC_SUCCESS) {
            


            free_sub_request(sub_req_p);
            BLF_DEBUG(DEB_F_PREFIX"Unable to send SUBSCRIBE", DEB_F_PREFIX_ARGS(BLF, fname));
        }
        BLF_DEBUG(DEB_F_PREFIX"resubscribed after retry-after seconds\n", DEB_F_PREFIX_ARGS(BLF, fname));
        break;
    default:
        BLF_ERROR(MISC_F_PREFIX"unknown timer:%d expired\n", fname,
                  timerMsg->expiredTimerId);
        break;
    }
    BLF_DEBUG(DEB_F_PREFIX"Exiting\n", DEB_F_PREFIX_ARGS(BLF, fname));
}











void pres_unsolicited_notify_ind (ccsip_sub_not_data_t *msg_data)
{
    static const char fname[] = "pres_unsolicited_notify_ind";
    ccsip_sub_not_data_t *pmsg;
    cpr_status_e rc;

    pmsg = (ccsip_sub_not_data_t *) cc_get_msg_buf(sizeof(*pmsg));

    if (!pmsg) {
        BLF_ERROR(MISC_F_PREFIX"malloc failed\n", fname);
        return;
    }
    memcpy(pmsg, msg_data, sizeof(*pmsg));

    rc = MiscAppTaskSendMsg(SUB_MSG_PRESENCE_UNSOLICITED_NOTIFY, pmsg, sizeof(*pmsg));
    if (rc == CPR_FAILURE) {
        cpr_free(pmsg);
    }

}









void pres_play_blf_audible_alert (void)
{
    pres_subscription_req_t *sub_req_p;

    sub_req_p = (pres_subscription_req_t *)sll_next(s_pres_req_list, NULL);
    while (sub_req_p != NULL) {
        if ((sub_req_p->app_id > 0) && (sub_req_p->blf_state == CC_SIP_BLF_ALERTING)) {
            


            cc_feature(CC_SRC_MISC_APP, CC_NO_CALL_ID, 0, CC_FEATURE_BLF_ALERT_TONE, NULL);
            break;
        }
        sub_req_p = (pres_subscription_req_t *)sll_next(s_pres_req_list, sub_req_p);
    }
}

