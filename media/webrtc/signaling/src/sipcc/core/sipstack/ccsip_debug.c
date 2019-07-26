






































#include "debug.h"
#include "phone_debug.h"
#include "ccsip_publish.h"
#include "util_string.h"
#include "sip_interface_regmgr.h"

extern boolean dump_reg_msg;


cc_int32_t SipDebugMessage          = 1; 
cc_int32_t SipDebugState            = 0; 
cc_int32_t SipDebugTask             = 1; 
cc_int32_t SipDebugRegState         = 0; 






int32_t SipRelDevEnabled           = 1; 

int32_t SipDebugGenContactHeader = 0; 
cc_int32_t SipDebugTrx              = 0; 

cc_int32_t GSMDebug                 = 0;
cc_int32_t FIMDebug                 = 0;
cc_int32_t LSMDebug                 = 0;
cc_int32_t FSMDebugSM               = 0;
int32_t CSMDebugSM               = 0;
cc_int32_t CCDebug                  = 0;
cc_int32_t CCDebugMsg               = 0;
cc_int32_t AuthDebug                = 0;
cc_int32_t g_DEFDebug               = 1;
cc_int32_t TNPDebug                 = 1;
cc_int32_t VCMDebug                 = 0;
cc_int32_t CCEVENTDebug             = 0;
cc_int32_t PLATDebug                = 0;
cc_int32_t TMRDebug                 = 0;
cc_int32_t g_dcsmDebug              = 0;
















void ccsip_dump_send_msg_info (char *msg, sipMessage_t *pSIPMessage, 
                               cpr_ip_addr_t *cc_remote_ipaddr,
                               uint16_t cc_remote_port)
{
    char *disp_buf;
    const char *req_uri;
    const char *cseq;
    const char *callid;
    char ipaddr_str[MAX_IPADDR_STR_LEN];
    static const char fname[] = "ccsip_dump_send_msg_info";

    ipaddr2dotted(ipaddr_str, cc_remote_ipaddr);

    req_uri = sippmh_get_header_val(pSIPMessage, SIP_HEADER_TO, NULL);
    if (req_uri == NULL) {
        
        req_uri = "";
    }
    cseq = sippmh_get_header_val(pSIPMessage, SIP_HEADER_CSEQ, NULL);
    if (cseq == NULL) {
        
        cseq = "";
    }
    callid = sippmh_get_header_val(pSIPMessage, SIP_HEADER_CALLID, NULL);
    if (callid == NULL) {
        
        callid = "";
    } 

    



    if (msg != NULL) {
        if (msg[0] == 'S' &&
            msg[1] == 'I' &&
            msg[2] == 'P') {
            disp_buf = &msg[8];
        } else {
            disp_buf = msg;
        } 
        if ((strncmp(disp_buf, SIP_METHOD_REGISTER, sizeof(SIP_METHOD_REGISTER)-1) == 0) &&
            (!dump_reg_msg)) {
            return;
        }
    } else {
        
        disp_buf = NULL;
    }

        
    if (disp_buf != NULL) {
        DEF_DEBUG(DEB_F_PREFIX"<%s:%-4d>:%c%c%c%c%c%c%c: %-10s :%-6s::%s\n",
                    DEB_F_PREFIX_ARGS(SIP_MSG_SEND, fname),
                    ipaddr_str, cc_remote_port,
                    disp_buf[0], 
                    disp_buf[1], 
                    disp_buf[2], 
                    disp_buf[3], 
                    disp_buf[4], 
                    disp_buf[5], 
                    disp_buf[6], 
                    req_uri,
                    cseq, callid);
    } else {
        
        DEF_DEBUG(DEB_F_PREFIX"<%s:%-4d>: empty message\n",
                  DEB_F_PREFIX_ARGS(SIP_MSG_SEND, fname),
                  ipaddr_str, cc_remote_port);
    }
}
















void ccsip_dump_recv_msg_info (sipMessage_t *pSIPMessage, 
                               cpr_ip_addr_t *cc_remote_ipaddr,
                               uint16_t cc_remote_port)
{
    char *disp_buf;
    const char *req_uri;
    const char *cseq;
    const char *callid;
    char ipaddr_str[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t cc_ipaddr;
    static const char fname[] = "ccsip_dump_recv_msg_info";

    util_ntohl(&cc_ipaddr, cc_remote_ipaddr);
    ipaddr2dotted(ipaddr_str, &cc_ipaddr);

    req_uri = sippmh_get_cached_header_val(pSIPMessage, FROM);
    if (req_uri == NULL) {
        
        req_uri = "";
    }
    cseq = sippmh_get_cached_header_val(pSIPMessage, CSEQ);
    if (cseq == NULL) {
        
        cseq = "";
    }
    callid = sippmh_get_cached_header_val(pSIPMessage, CALLID);
    if (callid == NULL) {
        
        callid = "";
    } 

    if (!dump_reg_msg) {
       if (strstr(cseq, SIP_METHOD_REGISTER) != NULL) {
          return;
       }
    }

    



    if (pSIPMessage->mesg_line != NULL) {
        if (pSIPMessage->mesg_line[0] == 'S' &&
            pSIPMessage->mesg_line[1] == 'I' &&
            pSIPMessage->mesg_line[2] == 'P') {
            disp_buf = &(pSIPMessage->mesg_line[8]);
        } else {
            disp_buf = pSIPMessage->mesg_line;
        }
    } else {
        



        disp_buf = NULL;
    }
        
    if (disp_buf != NULL) {
        DEF_DEBUG(DEB_F_PREFIX"<%s:%-4d>:%c%c%c%c%c%c%c: %-10s :%-6s::%s\n",
                    DEB_F_PREFIX_ARGS(SIP_MSG_RECV, fname),
                    ipaddr_str, cc_remote_port,
                    disp_buf[0], 
                    disp_buf[1], 
                    disp_buf[2], 
                    disp_buf[3], 
                    disp_buf[4], 
                    disp_buf[5], 
                    disp_buf[6], 
                    req_uri,
                    cseq, callid);
    } else {
        
        DEF_DEBUG(DEB_F_PREFIX"<%s:%-4d>: empty message\n",
                  DEB_F_PREFIX_ARGS(SIP_MSG_RECV, fname),
                  ipaddr_str, cc_remote_port);
    }
}












void
platform_print_sip_msg (const char *msg)
{
    char *buf;
    int msg_to_crypto_line_len, msg_to_digits_tag_len, buf_len;
    const char *c_line_begin, *c_line_end;
    static const char crypto_line_tag[] = "a=crypto:";
    static const char crypto_mask[] = "...";
    static const char digits_tag[] = "digits=";

    if (msg == NULL) {
        return;
    }

    buginf_msg("\n");
    
    if (strstr(msg, "kpml-response")) {
        
        c_line_begin = strstr(msg, digits_tag);
        if (c_line_begin == NULL) {
            
            buginf_msg(msg);
            return;
        }
        



        msg_to_digits_tag_len = c_line_begin - msg + sizeof(digits_tag);
        buf_len = msg_to_digits_tag_len + sizeof(crypto_mask);
        buf = (char *) cpr_malloc(buf_len);
        if (buf == NULL) {
            
            return;
        }

        
        memcpy(buf, msg, msg_to_digits_tag_len);

        
        memcpy(&buf[msg_to_digits_tag_len], crypto_mask, sizeof(crypto_mask));
        
        buginf_msg(buf);
        cpr_free(buf);

        c_line_end = c_line_begin + sizeof(digits_tag) + 3; 
        msg = c_line_end;
        buginf_msg(msg);
    } else if (sip_regmgr_get_sec_level(1) == ENCRYPTED) {
        
        while (TRUE) {
            c_line_begin = strstr(msg, crypto_line_tag);
            if (c_line_begin == NULL) {
                
                buginf_msg(msg);
                return;
            } else {
                





























                











                msg_to_crypto_line_len = c_line_begin - msg +
                                         sizeof(crypto_line_tag);
                






                buf_len = msg_to_crypto_line_len + sizeof(crypto_mask);
                buf = (char *) cpr_malloc(buf_len);
                if (buf == NULL) {
                    
                    return;
                }

                
                memcpy(buf, msg, msg_to_crypto_line_len);

                
                memcpy(&buf[msg_to_crypto_line_len], crypto_mask,
                       sizeof(crypto_mask));
                
                buginf_msg(buf);
                cpr_free(buf);

                
                c_line_end = strpbrk(c_line_begin, "\n");
                if (c_line_end != NULL) {
                    
                    msg = c_line_end + 1;
                } else {
                    
                    return;
                }
            }
        }
    } else {
        
        buginf_msg(msg);
    }
}











void
ccsip_debug_init (void)
{
    
}
