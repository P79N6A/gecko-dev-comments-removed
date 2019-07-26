



#include "cpr_types.h"
#include "cpr_string.h"
#include "ccsip_reldev.h"
#include "ccsip_macros.h"
#include "phone_debug.h"
#include "ccsip_pmh.h"
#include "ccsip_messaging.h"
#include "sip_common_transport.h"
#include "util_string.h"


#define SIP_RRLIST_LENGTH (MAX_TEL_LINES)


sipRelDevMessageRecord_t gSIPRRList[SIP_RRLIST_LENGTH];


void
sipRelDevMessageStore (sipRelDevMessageRecord_t * pMessageRecord)
{
    static int counter = 0;

    
    if (counter > (SIP_RRLIST_LENGTH - 1)) {
        counter = 0;
    }

    gSIPRRList[counter] = *pMessageRecord;
    gSIPRRList[counter].valid_coupled_message = FALSE;
    counter++;
}


boolean
sipRelDevMessageIsDuplicate (sipRelDevMessageRecord_t *pMessageRecord,
                             int *idx)
{
    int i = 0;

    for (i = 0; i < SIP_RRLIST_LENGTH; i++) {
        if ((strcmp(pMessageRecord->call_id, gSIPRRList[i].call_id) == 0) &&
            (pMessageRecord->cseq_number == gSIPRRList[i].cseq_number) &&
            (pMessageRecord->cseq_method == gSIPRRList[i].cseq_method) &&
            (strcasecmp_ignorewhitespace(pMessageRecord->tag, gSIPRRList[i].tag) == 0) &&
            (strcmp(pMessageRecord->from_user, gSIPRRList[i].from_user) == 0) &&
            (strcmp(pMessageRecord->from_host, gSIPRRList[i].from_host) == 0) &&
            (strcmp(pMessageRecord->to_user, gSIPRRList[i].to_user) == 0)) {

            if (pMessageRecord->is_request) {
                *idx = i;
                return (TRUE);
            } else {
                if (pMessageRecord->response_code == gSIPRRList[i].response_code) {
                    *idx = i;
                    return (TRUE);
                }
            }
        }
    }

    *idx = -1;
    return (FALSE);
}



























int
sipRelDevCoupledMessageStore (sipMessage_t *pCoupledMessage,
                              const char *call_id,
                              uint32_t cseq_number,
                              sipMethod_t cseq_method,
                              boolean is_request,
                              int response_code,
                              cpr_ip_addr_t *dest_ipaddr,
                              uint16_t dest_port,
                              boolean ignore_tag)
{
    static const char *fname = "sipRelDevCoupledMessageStore";
    int i = 0;
    char to_tag[MAX_SIP_TAG_LENGTH];

    sipGetMessageToTag(pCoupledMessage, to_tag, MAX_SIP_TAG_LENGTH);
    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Storing for reTx (cseq=%d, method=%s, "
                        "to_tag=<%s>)\n", DEB_F_PREFIX_ARGS(SIP_STORE, fname), cseq_number,
                        sipGetMethodString(cseq_method), to_tag);

    for (i = 0; i < SIP_RRLIST_LENGTH; i++) {
        if ((strcmp(call_id, gSIPRRList[i].call_id) == 0) &&
            (cseq_number == gSIPRRList[i].cseq_number) &&
            (cseq_method == gSIPRRList[i].cseq_method) &&
            ((ignore_tag) ? TRUE : (strcasecmp_ignorewhitespace(to_tag,
                                                     gSIPRRList[i].tag)
                                    == 0))) {
            hStatus_t sippmh_write_status = STATUS_FAILURE;
            uint32_t nbytes = SIP_UDP_MESSAGE_SIZE;

            



            if (is_request == FALSE ||
                (is_request == TRUE && gSIPRRList[i].response_code == response_code)) {
                gSIPRRList[i].coupled_message.message_buf[0] = '\0';
                sippmh_write_status =
                    sippmh_write(pCoupledMessage,
                                 gSIPRRList[i].coupled_message.message_buf,
                                 &nbytes);
                if (sippmh_write_status == STATUS_FAILURE) {
                    CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sippmh_write() failed.\n", fname);
                    return (RELDEV_NO_STORED_MSG);
                }
                if ((gSIPRRList[i].coupled_message.message_buf[0] == '\0') ||
                    (nbytes == 0)) {
                    CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sippmh_write() returned empty buffer string.\n",
                                      fname);
                    return (RELDEV_NO_STORED_MSG);
                }
                gSIPRRList[i].coupled_message.message_buf_len = nbytes;
                gSIPRRList[i].coupled_message.dest_ipaddr = *dest_ipaddr;
                gSIPRRList[i].coupled_message.dest_port = dest_port;
                gSIPRRList[i].valid_coupled_message = TRUE;
                
                return (i);
            }
        }
    }
    return (RELDEV_NO_STORED_MSG);
}


















uint32_t
sipRelDevGetStoredCoupledMessage (int idx,
                                  char *dest_buffer,
                                  uint32_t dest_buf_size)
{
    sipRelDevMessageRecord_t *record;

    if (dest_buffer == NULL) {
        
        return (0);
    }
    if ((idx < 0) || (idx >= SIP_RRLIST_LENGTH)) {
        
        return (0);
    }

    record = &gSIPRRList[idx];

    if (!record->valid_coupled_message) {
        
        return (0);
    }
    if ((record->coupled_message.message_buf_len > dest_buf_size) ||
        (record->coupled_message.message_buf_len == 0)) {
        



        return (0);
    }
    
    memcpy(dest_buffer, &record->coupled_message.message_buf[0],
           record->coupled_message.message_buf_len);
    return (record->coupled_message.message_buf_len);
}

int
sipRelDevCoupledMessageSend (int idx)
{
    static const char *fname = "sipRelDevCoupledMessageSend";
    char dest_ipaddr_str[MAX_IPADDR_STR_LEN];

    if ((idx < 0) || (idx >= SIP_RRLIST_LENGTH)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Argument Check: idx (=%d) out of bounds.\n",
                          fname, idx);
        return SIP_ERROR;
    }

    if (gSIPRRList[idx].valid_coupled_message) {
        ipaddr2dotted(dest_ipaddr_str,
                      &gSIPRRList[idx].coupled_message.dest_ipaddr);

        CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Sending stored coupled message (idx=%d) to "
                            "<%s>:<%d>\n", DEB_F_PREFIX_ARGS(SIP_MSG_SEND, fname), idx, dest_ipaddr_str,
                            gSIPRRList[idx].coupled_message.dest_port);
        if (sipTransportChannelSend(NULL,
                       gSIPRRList[idx].coupled_message.message_buf,
                       gSIPRRList[idx].coupled_message.message_buf_len,
                       sipMethodInvalid,
                       &(gSIPRRList[idx].coupled_message.dest_ipaddr),
                       gSIPRRList[idx].coupled_message.dest_port,
                       0) < 0) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sipTransportChannelSend() failed."
                              " Stored message not sent.\n", fname);
            return SIP_ERROR;
        }
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Duplicate message detected but failed to"
                          " find valid coupled message. Stored message not sent.\n",
                          fname);
        return SIP_ERROR;
    }
    return SIP_OK;
}







void
sipRelDevMessagesClear (const char *call_id,
                        const char *from_user,
                        const char *from_host,
                        const char *to_user)
{
    int i = 0;

    for (i = 0; i < SIP_RRLIST_LENGTH; i++) {
        if ((strcmp(call_id, gSIPRRList[i].call_id) == 0) &&
            (strcmp(from_user, gSIPRRList[i].from_user) == 0) &&
            (strcmp(from_host, gSIPRRList[i].from_host) == 0) &&
            (strcmp(to_user, gSIPRRList[i].to_user) == 0)) {
            memset(&gSIPRRList[i], 0, sizeof(sipRelDevMessageRecord_t));
        }
    }
    return;
}




void sipRelDevAllMessagesClear(){
	int i = 0;
	for (i = 0; i < SIP_RRLIST_LENGTH; i++) {
		memset(&gSIPRRList[i], 0, sizeof(sipRelDevMessageRecord_t));
	}
	return;
}
