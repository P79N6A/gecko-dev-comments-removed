






































#include "cpr_types.h"
#include "ccapi.h"
#include "string_lib.h"
#include "ccsip_pmh.h"
#include "ccsip_messaging.h"



















void sip_cc_mv_msg_body_to_cc_msg (cc_msgbody_info_t *cc_msg,
                                   sipMessage_t *sip_msg)
{
    int i;
    uint32_t num_parts = 0;
    cc_msgbody_t *part;

    if (cc_msg == NULL) {
        
        return;
    }
    if (sip_msg == NULL) {
        
        cc_msg->num_parts = 0;
        return;
    }

    part = &cc_msg->parts[0];
    for (i = 0; (i < sip_msg->num_body_parts) &&
                (i < CC_MAX_BODY_PARTS); i++) {
        if ((sip_msg->mesg_body[i].msgBody != NULL) &&
            (sip_msg->mesg_body[i].msgLength)) {
            
            part->body        = sip_msg->mesg_body[i].msgBody;
            part->body_length = sip_msg->mesg_body[i].msgLength;
            sip_msg->mesg_body[i].msgBody = NULL;

            
            part->content_type =
                    sip2cctype(sip_msg->mesg_body[i].msgContentTypeValue);
            
            part->content_disposition.disposition =
                    sip2ccdisp(sip_msg->mesg_body[i].msgContentDisp);
            part->content_disposition.required_handling =
                    sip_msg->mesg_body[i].msgRequiredHandling;
            
            part->content_id = sip_msg->mesg_body[i].msgContentId;
            sip_msg->mesg_body[i].msgContentId = NULL;

            
            part++;
            num_parts++;
        }
    }
    
    cc_msg->num_parts = num_parts;
}

















boolean sip_cc_create_cc_msg_body_from_sip_msg (cc_msgbody_info_t *cc_msg,
                                                sipMessage_t *sip_msg)
{
    int i, len;
    uint32_t num_parts = 0;
    cc_msgbody_t *part;
    boolean  status = TRUE;

    if (cc_msg == NULL) {
        
        return (FALSE);
    }

    if (sip_msg == NULL) {
        
        cc_msg->num_parts = 0;
        return (FALSE);
    }

    memset(cc_msg, 0, sizeof(cc_msgbody_info_t));
    part = &cc_msg->parts[0];
    for (i = 0; (i < sip_msg->num_body_parts) &&
                (i < CC_MAX_BODY_PARTS) ; i++) {
        if ((sip_msg->mesg_body[i].msgBody != NULL) &&
            (sip_msg->mesg_body[i].msgLength)) {
            
            part->body = (char *) cpr_malloc(sip_msg->mesg_body[i].msgLength);
            if (part->body != NULL) {
                part->body_length = sip_msg->mesg_body[i].msgLength;
                memcpy(part->body,
                       sip_msg->mesg_body[i].msgBody,
                       sip_msg->mesg_body[i].msgLength);
            } else {
                
                status = FALSE;
                break;
            }

            
            part->content_type =
                    sip2cctype(sip_msg->mesg_body[i].msgContentTypeValue);
            
            part->content_disposition.disposition =
                    sip2ccdisp(sip_msg->mesg_body[i].msgContentDisp);
            part->content_disposition.required_handling =
                    sip_msg->mesg_body[i].msgRequiredHandling;

            
            if (sip_msg->mesg_body[i].msgContentId != NULL) {
                
                len = strlen(sip_msg->mesg_body[i].msgContentId) + 1;
                part->content_id = (char *) cpr_malloc(len);
                if (part->content_id != NULL) {
                    memcpy(part->content_id,
                           sip_msg->mesg_body[i].msgContentId,
                           len);
                } else {
                    
                    status = FALSE;
                    break;
                }
            } else {
                
                part->content_id = NULL;
            }
            
            part++;
            num_parts++;
        }
    }
    
    cc_msg->num_parts = num_parts;

    if (!status) {
        



        cc_free_msg_body_parts(cc_msg);
    }
    return (status);
}

void sip_cc_setup (callid_t call_id, line_t line,
                   string_t calling_name, string_t calling_number, string_t alt_calling_number,
                   boolean display_calling_number,
                   string_t called_name,  string_t called_number,
                   boolean display_called_number,
                   string_t orig_called_name, string_t orig_called_number,
                   string_t last_redirect_name, string_t last_redirect_number,
                   cc_call_type_e   call_type,
                   cc_alerting_type alert_info,
                   vcm_ring_mode_t alerting_ring,
                   vcm_tones_t alerting_tone, cc_call_info_t *call_info_p,
                   boolean replaces, string_t recv_info_list, sipMessage_t *sip_msg)
{
    cc_caller_id_t caller_id;
    cc_msgbody_info_t cc_body_info;

    caller_id.calling_name = calling_name;
    caller_id.calling_number = calling_number;
    caller_id.alt_calling_number = alt_calling_number;
    caller_id.display_calling_number = display_calling_number;
    caller_id.called_name = called_name;
    caller_id.called_number = called_number;
    caller_id.display_called_number = display_called_number;
    caller_id.last_redirect_name = last_redirect_name;
    caller_id.last_redirect_number = last_redirect_number;
    caller_id.orig_called_name = orig_called_name;
    caller_id.orig_called_number = orig_called_number;
    caller_id.orig_rpid_number = strlib_empty();
    caller_id.call_type = call_type;

    
    sip_cc_mv_msg_body_to_cc_msg(&cc_body_info, sip_msg);


    cc_setup(CC_SRC_SIP, call_id, line, &caller_id, alert_info,
             alerting_ring, alerting_tone, NULL, call_info_p, replaces,
             recv_info_list, &cc_body_info);
}


#ifdef REMOVED_UNUSED_FUNCTION
void sip_cc_setup_ack (callid_t call_id, line_t line,
                       cc_msgbody_info_t *msg_body)
{
    cc_setup_ack(CC_SRC_SIP, call_id, line, NULL, msg_body);
}
#endif

void sip_cc_proceeding (callid_t call_id, line_t line)
{
    cc_proceeding(CC_SRC_SIP, call_id, line, NULL);
}

void sip_cc_alerting (callid_t call_id, line_t line,
                      sipMessage_t *sip_msg, int inband)
{
    cc_msgbody_info_t cc_body_info;

    
    sip_cc_mv_msg_body_to_cc_msg(&cc_body_info, sip_msg);

    cc_alerting(CC_SRC_SIP, call_id, line, NULL, &cc_body_info,
                (boolean)inband);
}


void sip_cc_connected (callid_t call_id, line_t line, string_t recv_info_list, sipMessage_t *sip_msg)
{
    cc_msgbody_info_t cc_body_info;

    
    sip_cc_mv_msg_body_to_cc_msg(&cc_body_info, sip_msg);

    cc_connected(CC_SRC_SIP, call_id, line, NULL, recv_info_list, &cc_body_info);
}


void sip_cc_connected_ack (callid_t call_id, line_t line,
                           sipMessage_t *sip_msg)
{
    cc_msgbody_info_t cc_body_info;

    
    sip_cc_mv_msg_body_to_cc_msg(&cc_body_info, sip_msg);

    cc_connected_ack(CC_SRC_SIP, call_id, line, NULL, &cc_body_info);
}


void sip_cc_release (callid_t call_id, line_t line, cc_causes_t cause,
                     const char *dialstring)
{
    cc_release(CC_SRC_SIP, call_id, line, cause, dialstring, NULL);
}


void sip_cc_release_complete (callid_t call_id, line_t line, cc_causes_t cause)
{
    cc_release_complete(CC_SRC_SIP, call_id, line, cause, NULL);
}


void sip_cc_feature (callid_t call_id, line_t line, cc_features_t feature, void *data)
{
    cc_feature(CC_SRC_SIP, call_id, line, feature, (cc_feature_data_t *)data);
}


void sip_cc_feature_ack (callid_t call_id, line_t line, cc_features_t feature,
                         void *data, cc_causes_t cause)
{
    cc_feature_ack(CC_SRC_SIP, call_id, line, feature, (cc_feature_data_t *)data, cause);
}


void sip_cc_mwi (callid_t call_id, line_t line, boolean on, int type,
                 int newCount, int oldCount, int hpNewCount, int hpOldCount)
{
    cc_mwi(CC_SRC_SIP, call_id, line, on, type, newCount, oldCount, hpNewCount, hpOldCount);
}

void sip_cc_options (callid_t call_id, line_t line, sipMessage_t *pSipMessage)
{
    cc_options_sdp_req(CC_SRC_SIP, call_id, line, pSipMessage);
}

void sip_cc_audit (callid_t call_id, line_t line, boolean apply_ringout)
{
    cc_audit_sdp_req(CC_SRC_SIP, call_id, line, apply_ringout);
}
