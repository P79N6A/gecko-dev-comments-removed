






































#include "ccsip_publish.h"
#include "phntask.h"
#include "sip_common_transport.h"
#include "ccsip_task.h"
#include "ccsip_callinfo.h"
#include "ccsip_macros.h"
#include "util_string.h"
#include "cpr_rand.h"
#include "platform_api.h"
#include "ccsip_register.h"
#include "ccsip_reldev.h"
#include "debug.h"

static sll_handle_t s_PCB_list = NULL; 
static int outgoingPublishes;

static boolean sipSPISendPublish(ccsip_publish_cb_t *pcb_p, boolean authen);
static void free_pending_reqs (sll_handle_t list);






static pub_handle_t generate_new_pub_handle (void)
{
    static pub_handle_t handle = 0;

    handle++;
    if (handle == NULL_PUBLISH_HANDLE) {
        handle++;
    }
    return handle;
}













static sll_match_e is_matching_pcb (void *key, void *data)
{
    pub_handle_t pub_handle = *((pub_handle_t *)key);
    ccsip_publish_cb_t *pcb_p = (ccsip_publish_cb_t *)data;

    if (pub_handle == pcb_p->pub_handle) {
        return SLL_MATCH_FOUND;
    }
    return SLL_MATCH_NOT_FOUND;
}









static ccsip_publish_cb_t *get_new_pcb (void)
{
    ccsip_publish_cb_t *pcb_p;

    


    if (s_PCB_list == NULL) {
        s_PCB_list = sll_create(is_matching_pcb);
        if (s_PCB_list == NULL) {
            return NULL;
        }
    }

    pcb_p = (ccsip_publish_cb_t *)cpr_malloc(sizeof(ccsip_publish_cb_t));
    if (pcb_p == NULL) {
        return NULL;
    }
    memset(pcb_p, 0, sizeof(ccsip_publish_cb_t));
    pcb_p->pub_handle = generate_new_pub_handle();
    pcb_p->hb.cb_type = PUBLISH_CB;
    pcb_p->hb.dn_line = 1; 
    


    ccsip_common_util_set_dest_ipaddr_port(&pcb_p->hb);
    ccsip_common_util_set_src_ipaddr(&pcb_p->hb);
    pcb_p->hb.local_port = sipTransportGetListenPort(pcb_p->hb.dn_line, NULL);
    pcb_p->retry_timer.timer = cprCreateTimer("PUBLISH retry timer",
                                              SIP_PUBLISH_RETRY_TIMER,
                                              TIMER_EXPIRATION,
                                              sip_msgq);
    if (pcb_p->retry_timer.timer == NULL) {
        cpr_free(pcb_p);
        return NULL;
    }
    pcb_p->pending_reqs = sll_create(NULL);
    if (pcb_p->pending_reqs == NULL) {
        (void)cprDestroyTimer(pcb_p->retry_timer.timer);
        cpr_free(pcb_p);
        return NULL;
    }
    (void) sll_append(s_PCB_list, pcb_p);

    return pcb_p;
}











static ccsip_publish_cb_t *find_pcb (pub_handle_t pub_handle)
{
    ccsip_publish_cb_t *pcb_p;

    pcb_p = (ccsip_publish_cb_t *)sll_find(s_PCB_list, &pub_handle);

    return pcb_p;
}











static ccsip_publish_cb_t *find_pcb_by_sip_callid (const char *callID_p)
{
    ccsip_publish_cb_t *pcb_p;

    pcb_p = (ccsip_publish_cb_t *)sll_next(s_PCB_list, NULL);
    while (pcb_p != NULL) {
        if (strncmp(callID_p, pcb_p->hb.sipCallID, (sizeof(pcb_p->hb.sipCallID) -1)) == 0) {
            return pcb_p;
        }
        pcb_p = (ccsip_publish_cb_t *)sll_next(s_PCB_list, pcb_p);
    }
    return NULL;
}










static void free_pcb (ccsip_publish_cb_t *pcb_p)
{
   if (pcb_p->hb.authen.authorization != NULL) {
       cpr_free(pcb_p->hb.authen.authorization);
   }
   if (pcb_p->hb.authen.sip_authen != NULL) {
       sippmh_free_authen(pcb_p->hb.authen.sip_authen);
   }

    cpr_free(pcb_p->entity_tag);
    free_pending_reqs(pcb_p->pending_reqs);
    (void)cprDestroyTimer(pcb_p->retry_timer.timer);
    free_event_data(pcb_p->hb.event_data_p);
    (void)sll_remove(s_PCB_list, (void *)pcb_p);
    cpr_free(pcb_p);
}













static boolean append_pending_reqs (ccsip_publish_cb_t *pcb_p, pub_req_t *msg_p)
{
    pub_req_t *temp_msg_p;

    temp_msg_p = (pub_req_t *)cpr_malloc(sizeof(pub_req_t));
    if (temp_msg_p == NULL) {
        return FALSE;
    }
    (*temp_msg_p) = (*msg_p);
    (void) sll_append(pcb_p->pending_reqs, temp_msg_p);
    return TRUE;
}









static void free_pending_reqs (sll_handle_t list)
{
    pub_req_t *msg_p;

    if (list == NULL)  {
        return;
    }

    msg_p = (pub_req_t *)sll_next(list, NULL);
    while (msg_p != NULL) {
        free_event_data(msg_p->event_data_p);
        (void)sll_remove(list, (void *)msg_p);
        cpr_free(msg_p);
        msg_p = (pub_req_t *)sll_next(list, NULL);
    }
    sll_destroy(list);
}












static void send_resp_to_app (int resp_code, pub_handle_t pub_handle, pub_handle_t app_handle,
                              cc_srcs_t callback_task, int resp_msg_id)
{
    static const char fname[] = "send_resp_to_app";
    pub_rsp_t rsp;

    rsp.resp_code = resp_code;
    rsp.pub_handle = pub_handle;
    rsp.app_handle = app_handle;
    if (publish_int_response(&rsp, callback_task, resp_msg_id) != CC_RC_SUCCESS) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Failed to post PUBLISH response to the application", fname);
    }
}















int publish_handle_ev_app_publish (cprBuffer_t buf)
{
    static const char fname[] = "publish_handle_ev_app_publish";
    pub_req_t *msg_p = (pub_req_t *)buf;
    ccsip_publish_cb_t *pcb_p;


    


 
    if (msg_p->pub_handle != NULL_PUBLISH_HANDLE) {
        pcb_p = find_pcb(msg_p->pub_handle);
        
        if (pcb_p == NULL) {
            send_resp_to_app(PUBLISH_FAILED_NOCONTEXT, msg_p->pub_handle, msg_p->app_handle,
                             msg_p->callback_task, msg_p->resp_msg_id);
            free_event_data(msg_p->event_data_p);
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX
                              "Modification PUBLISH cannot be sent as the PCB is missing\n",
                              fname);
            return SIP_ERROR;
        }

        



        if (pcb_p->outstanding_trxn == TRUE) {
            if (append_pending_reqs(pcb_p, msg_p) == TRUE) {
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"deffering as there is an outstanding transaction\n",
                                 DEB_F_PREFIX_ARGS(SIP_PUB, fname));
                return SIP_DEFER;
            }
            
            free_pcb (pcb_p);
            send_resp_to_app(PUBLISH_FAILED_NORESOURCE, msg_p->pub_handle, msg_p->app_handle,
                             msg_p->callback_task, msg_p->resp_msg_id);
            free_event_data(msg_p->event_data_p);
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Queueing outgoing PUBLISH request failed\n", fname);
            return SIP_ERROR;
        }
        



        free_event_data(pcb_p->hb.event_data_p);
        pcb_p->hb.event_data_p = msg_p->event_data_p;
        if ((msg_p->event_data_p == NULL) && (msg_p->expires == 0)) { 
            pcb_p->hb.orig_expiration = 0;
        }
    } else {
        pcb_p = get_new_pcb();
        if (pcb_p == NULL) {
            send_resp_to_app(PUBLISH_FAILED_NORESOURCE, msg_p->pub_handle, msg_p->app_handle,
                             msg_p->callback_task, msg_p->resp_msg_id);
            free_event_data(msg_p->event_data_p);
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"PCB allocation failed\n", fname);
            return SIP_ERROR;
        }
        pcb_p->app_handle = msg_p->app_handle;
        sstrncpy(pcb_p->ruri, msg_p->ruri, MAX_URI_LENGTH);
        sstrncpy(pcb_p->esc, msg_p->esc, MAX_URI_LENGTH);
        pcb_p->hb.orig_expiration = msg_p->expires;
        pcb_p->hb.event_type = msg_p->event_type;
        pcb_p->hb.event_data_p = msg_p->event_data_p;
        pcb_p->callback_task = msg_p->callback_task;
        pcb_p->resp_msg_id = msg_p->resp_msg_id;
    }

    pcb_p->hb.authen.cred_type = 0;

    if (sipSPISendPublish(pcb_p, FALSE) == TRUE) {
        pcb_p->outstanding_trxn = TRUE;
        outgoingPublishes++;
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"PUBLISH request sent successfully\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname));
        return SIP_OK;
    }

    
    free_pcb (pcb_p);
    send_resp_to_app(PUBLISH_FAILED_SEND, msg_p->pub_handle, msg_p->app_handle,
                     msg_p->callback_task, msg_p->resp_msg_id);
    CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Failed to send PUBLISH request\n", fname);
    return SIP_ERROR;
    
}













static boolean sipSPISendPublish (ccsip_publish_cb_t *pcb_p, boolean authen) 
{
    static const char fname[] = "sipSPISendPublish";
    static uint32_t   cseq = 0;
    char              dest_sip_addr_str[MAX_IPADDR_STR_LEN];
    char             *domainloc;
    char              src_addr_str[MAX_IPADDR_STR_LEN];
    char              sip_temp_str[MAX_SIP_URL_LENGTH];
    char              sip_temp_tag[MAX_SIP_URL_LENGTH];
    uint8_t           mac_address[MAC_ADDRESS_LENGTH];
    char              via[SIP_MAX_VIA_LENGTH];
    int               max_forwards_value = 70;
    static uint16_t   count = 1;
    sipMessage_t     *request = NULL;
    int               timeout = 0;
   
    request = GET_SIP_MESSAGE();
    if (!request) {
        return FALSE;
    }

    


    if (pcb_p->full_ruri[0] == 0) {
        strncpy(pcb_p->full_ruri, "sip:", MAX_SIP_URL_LENGTH);
        strncat(pcb_p->full_ruri, pcb_p->ruri, MAX_SIP_URL_LENGTH - 5);
        
        domainloc = strchr(pcb_p->full_ruri, '@');
        if (domainloc == NULL) {
            domainloc = pcb_p->full_ruri + strlen(pcb_p->full_ruri);
            if ((domainloc - pcb_p->full_ruri) < (MAX_SIP_URL_LENGTH - 1)) {
                
                if (pcb_p->ruri[0] != '\0') {
                    *domainloc++ = '@';
                }
                ipaddr2dotted(dest_sip_addr_str, &pcb_p->hb.dest_sip_addr);
                sstrncpy(domainloc, dest_sip_addr_str,
                         MAX_SIP_URL_LENGTH - (domainloc - (pcb_p->full_ruri)));
            }
        }
    }

    ipaddr2dotted(src_addr_str, &pcb_p->hb.src_addr);

    
    if (HSTATUS_SUCCESS != sippmh_add_request_line(request,
                                                   sipGetMethodString(sipMethodPublish),
                                                   pcb_p->full_ruri, SIP_VERSION)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Request line\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    snprintf(via, sizeof(via), "SIP/2.0/%s %s:%d;%s=%s%.8x",
             sipTransportGetTransportType(1, TRUE, NULL),
             src_addr_str, pcb_p->hb.local_port, VIA_BRANCH,
             VIA_BRANCH_START, (unsigned int) cpr_rand());
    if (HSTATUS_SUCCESS != sippmh_add_text_header(request, SIP_HEADER_VIA, via)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding VIA header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    snprintf(sip_temp_str, MAX_SIP_URL_LENGTH, "<%s>", pcb_p->full_ruri);
    if (HSTATUS_SUCCESS != sippmh_add_text_header(request, SIP_HEADER_TO, sip_temp_str)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding TO header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    strncat(sip_temp_str, ";tag=", MAX_SIP_URL_LENGTH - strlen(sip_temp_str) - 1);
    sip_util_make_tag(sip_temp_tag);
    strncat(sip_temp_str, sip_temp_tag, MAX_SIP_URL_LENGTH - strlen(sip_temp_str) - 1);
    if (HSTATUS_SUCCESS != sippmh_add_text_header(request, SIP_HEADER_FROM, sip_temp_str)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding FROM header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    platform_get_wired_mac_address(mac_address);
    count++;
    snprintf(pcb_p->hb.sipCallID, MAX_SIP_CALL_ID, "%.4x%.4x-%.4x%.4x-%.8x-%.8x@%s", 
                 mac_address[0] * 256 + mac_address[1],
                 mac_address[2] * 256 + mac_address[3],
                 mac_address[4] * 256 + mac_address[5], count,
                 (unsigned int) cpr_rand(),
                 (unsigned int) cpr_rand(),
                 src_addr_str);
    if (HSTATUS_SUCCESS != sippmh_add_text_header(request, SIP_HEADER_CALLID, pcb_p->hb.sipCallID)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding CALLID header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    snprintf(sip_temp_str, MAX_SIP_URL_LENGTH, "<sip:%.4x%.4x%.4x@%s:%d>",
             mac_address[0] * 256 + mac_address[1],
             mac_address[2] * 256 + mac_address[3],
             mac_address[4] * 256 + mac_address[5],
             src_addr_str, pcb_p->hb.local_port);
    if (HSTATUS_SUCCESS != sippmh_add_text_header(request, SIP_HEADER_CONTACT, sip_temp_str)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Contact header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    cseq++;
    if (cseq == 0) {
        cseq = 1;
    }
    if (HSTATUS_SUCCESS != sippmh_add_cseq(request, sipGetMethodString(sipMethodPublish), cseq)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding CSEQ header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    (void) sippmh_add_text_header(request, SIP_HEADER_USER_AGENT,
                                  sipHeaderUserAgent);


    
    if (pcb_p->entity_tag != NULL) {
        if (HSTATUS_SUCCESS != sippmh_add_text_header(request, SIP_HEADER_SIPIFMATCH,
                                                      pcb_p->entity_tag)) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Event header\n", fname);
            free_sip_message(request);
            return (FALSE);
        }
    }

    
    if (HSTATUS_SUCCESS != sippmh_add_int_header(request, SIP_HEADER_EXPIRES,
                pcb_p->hb.orig_expiration)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Expires header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    config_get_value(CFGID_SIP_MAX_FORWARDS, &max_forwards_value,
                     sizeof(max_forwards_value));
    if (HSTATUS_SUCCESS !=
        sippmh_add_int_header(request, SIP_HEADER_MAX_FORWARDS,
            max_forwards_value)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Max-Forwards header\n", fname);
        free_sip_message(request);
        return (FALSE);
    }

    
    if (authen) {
        if (HSTATUS_SUCCESS != sippmh_add_text_header(request, AUTHOR_HDR(pcb_p->hb.authen.status_code),
                                                      pcb_p->hb.authen.authorization)) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Authorization header\n", fname);
            free_sip_message(request);
            return (FALSE);
        }
    }

    
    if (pcb_p->hb.event_data_p) {
        if (add_content(pcb_p->hb.event_data_p, request, fname) == FALSE) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Content\n", fname);
            free_sip_message(request);
            return (FALSE);
        }
    } else {
        if (HSTATUS_SUCCESS != sippmh_add_int_header(request, SIP_HEADER_CONTENT_LENGTH, 0)) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error in adding Content-Len\n", fname);
            free_sip_message(request);
            return (FALSE);
        }
    }

    ccsip_common_util_set_retry_settings(&pcb_p->hb, &timeout);
    if (sipTransportCreateSendMessage(NULL, request, sipMethodPublish,
                                      &(pcb_p->hb.dest_sip_addr),
                                      (int16_t) pcb_p->hb.dest_sip_port,
                                      FALSE, TRUE, timeout, pcb_p,
                                      RELDEV_NO_STORED_MSG) < 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"failed to send PUBLISH message\n", fname);
        return (FALSE);
    }

    return (TRUE);
    
}











int publish_handle_retry_timer_expire (uint32_t handle)
{
    static const char fname[] = "publish_handle_retry_timer_expire";
    pub_handle_t pub_handle = handle;
    ccsip_publish_cb_t *pcb_p;
    uint32_t        max_retx = 0;
    uint32_t        time_t1 = 0;
    uint32_t        time_t2 = 0;
    uint32_t        timeout = 0;

    


    pcb_p = find_pcb(pub_handle);
    if (pcb_p == NULL) {
        
        return 0;
    }
    if (pcb_p->hb.retx_flag == FALSE) {
        
        return 0;
    }
    config_get_value(CFGID_SIP_RETX, &max_retx, sizeof(max_retx));
    if (max_retx > MAX_NON_INVITE_RETRY_ATTEMPTS) {
        max_retx = MAX_NON_INVITE_RETRY_ATTEMPTS;
    }
    if (pcb_p->hb.retx_counter < max_retx) {
        pcb_p->hb.retx_counter++;
        config_get_value(CFGID_TIMER_T1, &time_t1, sizeof(time_t1));
        timeout = time_t1 * (1 << pcb_p->hb.retx_counter);
        config_get_value(CFGID_TIMER_T2, &time_t2, sizeof(time_t2));
        if (timeout > time_t2) {
            timeout = time_t2;
        }
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Resending message #%d\n",
                         DEB_F_PREFIX_ARGS(SIP_PUB, fname), pcb_p->hb.retx_counter);
            if (sipTransportSendMessage(NULL,
                                        pcb_p->retry_timer.message_buffer,
                                        pcb_p->retry_timer.message_buffer_len,
                                        pcb_p->retry_timer.message_type,
                                        &(pcb_p->retry_timer.ipaddr),
                                        pcb_p->retry_timer.port,
                                        FALSE, TRUE, timeout, pcb_p) < 0) {
                
                send_resp_to_app(PUBLISH_FAILED_SEND, pcb_p->pub_handle, pcb_p->app_handle,
                                 pcb_p->callback_task, pcb_p->resp_msg_id);
                free_pcb (pcb_p);
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"failed to send message", fname);
                return (-1);
            }

    } else {
        


        send_resp_to_app(SIP_CLI_ERR_REQ_TIMEOUT, pcb_p->pub_handle, pcb_p->app_handle,
                         pcb_p->callback_task, pcb_p->resp_msg_id);
        free_pcb (pcb_p);
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"reached MAX retries", fname);
    }
    return 0;
}









void publish_handle_periodic_timer_expire (void)
{
    static const char fname[] = "publish_handle_periodic_timer_expire";
    int                 delta = 0;
    ccsip_publish_cb_t *pcb_p;
    pub_req_t           msg;

    config_get_value(CFGID_TIMER_SUBSCRIBE_DELTA, &delta,
                     sizeof(delta));
    pcb_p = (ccsip_publish_cb_t *)sll_next(s_PCB_list, NULL);
    while (pcb_p != NULL) {
        if (pcb_p->outstanding_trxn == FALSE) {
            if (pcb_p->hb.expires >= TMR_PERIODIC_PUBLISH_INTERVAL) {
                pcb_p->hb.expires -= TMR_PERIODIC_PUBLISH_INTERVAL;
            }
            if (pcb_p->hb.expires <= (delta + TMR_PERIODIC_PUBLISH_INTERVAL)) {
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"sending REFRESH PUBLISH", DEB_F_PREFIX_ARGS(SIP_PUB, fname));
                memset (&msg, 0, sizeof(msg));
                
                msg.pub_handle = pcb_p->pub_handle;
                msg.expires = pcb_p->hb.orig_expiration;
                (void)publish_handle_ev_app_publish(&msg);               
            }
        }
        pcb_p = (ccsip_publish_cb_t *)sll_next(s_PCB_list, pcb_p);
    }
}











int publish_handle_ev_sip_response (sipMessage_t *pSipMessage)
{
    static const char fname[] = "publish_handle_ev_sip_response";
    const char     *callID_p = NULL;
    int             response_code = 0;
    const char     *expires = NULL;
    const char     *sip_etag = NULL;
    long            expiry_time;
    pub_req_t      *msg_p;
    ccsip_publish_cb_t *pcb_p;

    callID_p = sippmh_get_cached_header_val(pSipMessage, CALLID);
    if (!callID_p) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Cannot obtain SIP Call ID.\n", fname);
        return SIP_ERROR;
    }

    


    pcb_p = find_pcb_by_sip_callid(callID_p);
    if (pcb_p == NULL) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"No matching PCB found\n", fname);
        return SIP_ERROR;
    }

    


    sip_platform_msg_timer_subnot_stop(&(pcb_p->retry_timer));
    pcb_p->hb.retx_flag = FALSE;
    
    
    (void) sipGetResponseCode(pSipMessage, &response_code);

    if (response_code >= 200) {
        pcb_p->outstanding_trxn = FALSE;
    }


    if ((response_code == SIP_CLI_ERR_UNAUTH) ||
        (response_code == SIP_CLI_ERR_PROXY_REQD)) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Authentication Required\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname));
        if (ccsip_common_util_generate_auth(pSipMessage, &pcb_p->hb, SIP_METHOD_PUBLISH,
                                            response_code, pcb_p->full_ruri) == TRUE) {
            if (sipSPISendPublish(pcb_p, TRUE) == TRUE) {
                pcb_p->outstanding_trxn = TRUE;
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"sent request with Auth header\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname));
                return SIP_OK;
             }
        }
        


        send_resp_to_app(PUBLISH_FAILED_SEND, pcb_p->pub_handle, pcb_p->app_handle,
                         pcb_p->callback_task, pcb_p->resp_msg_id);
        free_pcb (pcb_p);
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"failed to respond to auth challenge\n", fname);
        return SIP_ERROR;
    }

    


    if (response_code == SIP_CLI_ERR_INTERVAL_TOO_SMALL) {
        expires = sippmh_get_header_val(pSipMessage,
                                        (const char *)SIP_HEADER_MIN_EXPIRES,
                                        NULL);
        if (expires) {
            expiry_time = strtoul(expires, NULL, 10);
            
            if ((long) expiry_time > pcb_p->hb.expires) {
                pcb_p->hb.expires = expiry_time;
                pcb_p->hb.orig_expiration = expiry_time;
            }
            if (sipSPISendPublish(pcb_p, FALSE) == TRUE) {
                pcb_p->outstanding_trxn = TRUE;
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"sent request with increased expires\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname));
                return SIP_OK;
             }
        }
        


        send_resp_to_app(PUBLISH_FAILED_SEND, pcb_p->pub_handle, pcb_p->app_handle,
                         pcb_p->callback_task, pcb_p->resp_msg_id);
        free_pcb (pcb_p);
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"failed to respond to 423\n", fname);
        return SIP_ERROR;
    }
    
    


    if (response_code > 299) {
        send_resp_to_app(response_code, pcb_p->pub_handle, pcb_p->app_handle,
                         pcb_p->callback_task, pcb_p->resp_msg_id);
        free_pcb (pcb_p);
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"received %d response\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname), response_code);
        return SIP_OK;
    }

    


    if (response_code < 200) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"received %d response\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname), response_code);
        return SIP_OK;
    }
    
    


    if (pcb_p->hb.orig_expiration == 0) {
        send_resp_to_app(response_code, pcb_p->pub_handle, pcb_p->app_handle,
                         pcb_p->callback_task, pcb_p->resp_msg_id);
        free_pcb (pcb_p);
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"removed PCB as this was a terminating PUBLISH\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname));
        return SIP_OK;
    }

    


    expires = sippmh_get_header_val(pSipMessage, SIP_HEADER_EXPIRES, NULL);
    if (expires) {
        expiry_time = strtoul(expires, NULL, 10);
        pcb_p->hb.expires = expiry_time;
    }
    sip_etag = sippmh_get_header_val(pSipMessage, SIP_HEADER_SIPETAG, NULL);
    if (sip_etag != NULL) {
        cpr_free(pcb_p->entity_tag);
        pcb_p->entity_tag = cpr_malloc(strlen(sip_etag) + 1);
        if (pcb_p->entity_tag != NULL) {
            strcpy(pcb_p->entity_tag, sip_etag);
        } else {
            free_pcb (pcb_p);
            send_resp_to_app(PUBLISH_FAILED_NORESOURCE, pcb_p->pub_handle, pcb_p->app_handle,
                             pcb_p->callback_task, pcb_p->resp_msg_id);
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"memory allocation failed\n", fname);
            return SIP_ERROR;
            
        }
    }

    


    msg_p = (pub_req_t *)sll_next(pcb_p->pending_reqs, NULL);
    if (msg_p != NULL) {
        (void)sll_remove(pcb_p->pending_reqs, msg_p);
        (void)publish_handle_ev_app_publish(msg_p);
        cpr_free(msg_p);
        return SIP_OK;
    }
    send_resp_to_app(response_code, pcb_p->pub_handle, pcb_p->app_handle,
                     pcb_p->callback_task, pcb_p->resp_msg_id);
    CCSIP_DEBUG_TASK(DEB_F_PREFIX"sent response %d to app\n", DEB_F_PREFIX_ARGS(SIP_PUB, fname), response_code);
    return SIP_OK;
}












void publish_reset (void)
{
    ccsip_publish_cb_t *pcb_p;

    pcb_p = (ccsip_publish_cb_t *)sll_next(s_PCB_list, NULL);
    while (pcb_p != NULL) {
        send_resp_to_app(PUBLISH_FAILED_RESET, pcb_p->pub_handle, pcb_p->app_handle,
                         pcb_p->callback_task, pcb_p->resp_msg_id);
        free_pcb(pcb_p);
        pcb_p = (ccsip_publish_cb_t *)sll_next(s_PCB_list, NULL);
    }
}








cc_int32_t show_publish_stats (cc_int32_t argc, const char *argv[])
{
    debugif_printf("------ Current PUBLISH Statistics ------\n");
    if (s_PCB_list != NULL) {
        debugif_printf("Number of PCBs allocated: %d\n", sll_count(s_PCB_list));
    } else {
        debugif_printf("Number of PCBs allocated: 0\n");
    }
    debugif_printf("Total outgoing PUBLISH requests: %d\n", outgoingPublishes);
    return 0;
}

