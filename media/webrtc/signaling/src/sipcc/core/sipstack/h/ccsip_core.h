



#ifndef _CCSIP_CORE_H_
#define _CCSIP_CORE_H_

#include "cpr_types.h"
#include "cpr_memory.h"
#include "task.h"

#include "ccapi.h"
#include "ccsip_sdp.h"
#include "ccsip_pmh.h"
#include "config.h"
#include "dns_utils.h"
#include "ccsip_platform.h"
#include "ccsip_platform_timers.h"
#include "cc_constants.h"
#include "sessionConstants.h"

#define SIP_DEFER (-2)
#define SIP_ERROR (-1)
#define SIP_OK    (0)

#define SIP_L_C_F_PREFIX "SIP : %d/%d : %s : " // requires 3 args: line_id, call_id, fname
#define SIP_F_PREFIX "SIP : %s : " // requires 1 arg: fname

#define SUPERVISION_DISCONNECT_TIMEOUT 32000
#define SIP_WARNING_LENGTH 100

#define RPID_DISABLED 0
#define RPID_ENABLED  1

#define MAX_INVITE_RETRY_ATTEMPTS      6
#define MAX_NON_INVITE_RETRY_ATTEMPTS 10

typedef enum {
    SIP_STATE_NONE = -1,
    SIP_STATE_BASE = 0,

    SIP_STATE_IDLE = SIP_STATE_BASE,

    SIP_STATE_SENT_INVITE,
    SIP_STATE_SENT_INVITE_CONNECTED,

    SIP_STATE_RECV_INVITE,
    SIP_STATE_RECV_INVITE_PROCEEDING,
    SIP_STATE_RECV_INVITE_ALERTING,
    SIP_STATE_RECV_INVITE_CONNECTED,

    SIP_STATE_ACTIVE,
    SIP_STATE_SENT_MIDCALL_INVITE,
    SIP_STATE_RECV_MIDCALL_INVITE_CCFEATUREACK_PENDING,
    SIP_STATE_RECV_MIDCALL_INVITE_SIPACK_PENDING,

    SIP_STATE_RELEASE,
    SIP_STATE_BLIND_XFER_PENDING,
    SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING,

    SIP_STATE_SENT_OOD_REFER,
    SIP_STATE_RECV_UPDATEMEDIA_CCFEATUREACK_PENDING,

    SIP_STATE_END = SIP_STATE_RECV_UPDATEMEDIA_CCFEATUREACK_PENDING
} sipSMStateType_t;

typedef enum
{
    SIP_REG_STATE_INV = -1,
    SIP_REG_STATE_NONE = 0,
    SIP_REG_STATE_BASE = 1,

    SIP_REG_STATE_IDLE = SIP_REG_STATE_BASE,
    SIP_REG_STATE_REGISTERING,
    SIP_REG_STATE_REGISTERED,
    SIP_REG_STATE_UNREGISTERING,
    SIP_REG_STATE_IN_FALLBACK,
    SIP_REG_STATE_STABILITY_CHECK,
    SIP_REG_STATE_TOKEN_WAIT,
    SIP_REG_STATE_END = SIP_REG_STATE_TOKEN_WAIT
} sipRegSMStateType_t;

typedef enum {
    SIPSPI_EV_INVALID = -1,
    SIPSPI_EV_BASE = 0,

    E_SIP_INVITE = SIPSPI_EV_BASE,
    E_SIP_ACK,
    E_SIP_BYE,
    E_SIP_CANCEL,
    E_SIP_1xx,
    E_SIP_2xx,
    E_SIP_3xx,
    E_SIP_NOTIFY,
    E_SIP_FAILURE_RESPONSE,
    E_SIP_REFER,
    E_SIP_OPTIONS,
    E_SIP_SUBSCRIBE,
    E_SIP_UPDATE,

    E_CC_SETUP,
    E_CC_SETUP_ACK,
    E_CC_PROCEEDING,
    E_CC_ALERTING,
    E_CC_CONNECTED,
    E_CC_CONNECTED_ACK,
    E_CC_RELEASE,
    E_CC_RELEASE_COMPLETE,
    E_CC_FEATURE,
    E_CC_FEATURE_ACK,
    E_CC_CAPABILITIES,
    E_CC_CAPABILITIES_ACK,
    E_CC_SUBSCRIBE,
    E_CC_NOTIFY,
    E_CC_INFO,

    E_SIP_INV_EXPIRES_TIMER,
    E_SIP_INV_LOCALEXPIRES_TIMER,
    E_SIP_SUPERVISION_DISCONNECT_TIMER,
    E_SIP_TIMER,
    E_SIP_GLARE_AVOIDANCE_TIMER,

    E_SIP_UPDATE_RESPONSE,
    E_SIP_ICMP_UNREACHABLE,
    SIPSPI_EV_END = E_SIP_ICMP_UNREACHABLE
} sipSMEventType_t;

typedef enum {
    H_INVALID_EVENT = -1,
    SIPSPI_EV_INDEX_BASE = 0,
H_IDLE_EV_SIP_INVITE = SIPSPI_EV_INDEX_BASE,                 
H_IDLE_EV_CC_SETUP,                                          
H_SENTINVITE_EV_SIP_1XX,                                     
H_SENTINVITE_EV_SIP_2XX,                                     
H_SENTINVITE_EV_SIP_FXX,                                     
H_DISCONNECT_LOCAL_EARLY,                                    
H_DISCONNECT_REMOTE,                                         
H_SENTINVITECONNECTED_EV_CC_CONNECTED_ACK,                   
H_DISCONNECT_LOCAL,                                          
H_RECVINVITE_EV_CC_SETUP_ACK,                                
H_RECVINVITE_EV_CC_PROCEEDING,                              
H_RECVINVITE_EV_CC_ALERTING,                                
H_RECVINVITE_EV_CC_CONNECTED,                               
H_DISCONNECT_LOCAL_UNANSWERED,                              
H_RECVINVITE_EV_SIP_ACK,                                    
H_ACTIVE_EV_SIP_INVITE,                                     
H_ACTIVE_EV_CC_FEATURE,                                     
H_ACCEPT_2XX,                                               
H_REFER_SIP_MESSAGE,                                        
H_ACTIVE_EV_CC_FEATURE_ACK,                                 
H_SENTINVITE_MIDCALL_EV_SIP_2XX,                            
H_SENTINVITE_MIDCALL_EV_CC_FEATURE,                         
H_RECVMIDCALLINVITE_CCFEATUREACKPENDING_EV_CC_FEATURE_ACK,  
H_RECVMIDCALLINVITE_SIPACKPENDING_EV_SIP_ACK,               
H_DEFAULT_SIP_MESSAGE,                                      
H_DEFAULT_SIP_RESPONSE,                                     
H_DEFAULT,                                                  
H_SIP_INV_EXPIRES_TIMER,                                    
H_SIP_OPTIONS,                                              
H_SENTINVITE_EV_SIP_3XX,                                    
H_RECV_ERR_EV_SIP_ACK,                                      
H_SENTBYE_EV_SIP_2XX,                                       
H_SENTBYE_EV_SIP_1XX,                                       
H_SENTBYE_EV_SIP_FXX,                                       
H_SENTBYE_EV_SIP_INVITE,                                    
H_SENTBYE_SUPERVISION_DISCONNECT_TIMER,                     
H_RELEASE_COMPLETE,                                         
H_ACTIVE_2xx,                                               
H_BLIND_NOTIFY,                                             
H_SENT_BLINDNTFY,                                           
H_BYE_RELEASE,                                              
H_HANDLE_LOCALEXPIRES_TIMER,                                
H_DEFAULT_SIP_TIMER,                                        
H_EARLY_EV_SIP_UPDATE,                                      
H_EARLY_EV_SIP_UPDATE_RESPONSE,                             
H_EARLY_EV_CC_FEATURE,                                      
H_EARLY_EV_CC_FEATURE_ACK,                                  
H_CONFIRM_EV_SIP_UPDATE,                                    
H_RECVUPDATEMEDIA_CCFEATUREACKPENDING_EV_CC_FEATURE_ACK,    
H_SIP_GLARE_AVOIDANCE_TIMER,                                
H_RECVINVITE_SENTOK_NO_SIP_ACK,                             
H_EV_SIP_UNSOLICITED_NOTIFY,                                
H_RECVINVITE_EV_SIP_2XX,                                    
H_ICMP_UNREACHABLE,                                         
H_DISCONNECT_MEDIA_CHANGE,                                  
H_DEFAULT_EV_CC_FEATURE,                                    
H_DEFAULT_RECVREQ_ACK_PENDING_EV_CC_FEATURE,                
H_OOD_REFER_RESPONSE_EV_SIP_1xx,                            
H_OOD_REFER_RESPONSE_EV_SIP_2xx,                            
H_OOD_REFER_RESPONSE_EV_SIP_fxx,                            
H_RELEASE_EV_CC_FEATURE,                                    
H_EV_CC_INFO,                                               
H_RELEASE_EV_RELEASE,                                       
    SIPSPI_EV_INDEX_END = H_RELEASE_EV_RELEASE
} sipSMAction_t;










typedef enum {
    SIP_SM_DIS_METHOD_BYE = 0,
    SIP_SM_DIS_METHOD_CANCEL
} sipSMDisMethod_t;

typedef struct {
    char       last_call_id[MAX_SIP_CALL_ID];
    uint32_t   last_bye_cseq_number;
    cpr_ip_addr_t   last_bye_dest_ipaddr;
    uint16_t   last_bye_dest_port;
    cpr_ip_addr_t   proxy_dest_ipaddr;
    line_t     dn_line;
    char       last_bye_also_string[MAX_SIP_URL_LENGTH];
    char       last_route[MAX_SIP_URL_LENGTH];
    char       last_route_request_uri[MAX_SIP_URL_LENGTH];
    char       via_branch[VIA_BRANCH_LENGTH];
    sipStatusCodeClass_t last_rspcode_rcvd;
} sipCallHistory_t;

typedef enum {
    SIP_SM_NO_XFR = 0,
    SIP_SM_BLND_XFR,
    SIP_SM_ATTN_XFR
} sipSMXfrType_t;

typedef struct sipRedirectInfo_ {
    sipContact_t *sipContact;  
    uint16_t next_choice;      
} sipRedirectInfo_t;

typedef struct {
    int  retries_401_407;
    int cred_type;
    char *authorization;
    int status_code;
    sip_authen_t *sip_authen;
    char cnonce[9];
    int nc_count;
    boolean new_flag;
} sipAuthenticate_t;


typedef struct {
    int registered;
    int tmr_expire;
    int act_time;
    char proxy[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t  addr;
    uint16_t port;
    uint8_t  rereg_pending;
} sipRegister_t;


typedef struct {
    int payload_type; 
} sipAvtPayloadType_t;


typedef enum {
    SIP_NONE_CCB,
    SIP_REG_CCB,
    SIP_CALL_CCB
} sipCCBTypes_t;




typedef enum {
    CC_CCM = CC_MODE_CCM,
    CC_OTHER = CC_MODE_NONCCM,
    MAX_CC_TYPES
} CC_ID;




typedef enum {
    OA_IDLE,
    OA_OFFER_SENT,
    OA_OFFER_RECEIVED,
    OA_ANSWER_SENT,
    OA_ANSWER_RECEIVED
} sipOfferAnswerState;


typedef enum {
    SIP_PROXY_DEFAULT,  
    SIP_PROXY_BACKUP,   
    SIP_PROXY_DO_NOT_CHANGE_MIDCALL 
} sipCCBProxySelection;

typedef struct
{
    union {
        string_t sip_via_header; 
        string_t sip_via_branch; 
    } u;
    string_t    sip_via_sentby;
    sipMethod_t cseq_method;
    uint32_t    cseq_number;
} sipTransaction_t;

typedef struct
{
    char             sipCallID[MAX_SIP_CALL_ID];
    callid_t         gsm_id;
    callid_t         con_call_id;
    callid_t         blind_xfer_call_id;

    sipSMStateType_t state;
    line_t           index;
    line_t           dn_line;
    boolean          hold_initiated;
    uint32_t         retx_counter;
    sipCCBTypes_t    type;

    




    boolean              first_backup;
    sipCCBProxySelection proxySelection;      
    cpr_ip_addr_t        outBoundProxyAddr;   

    uint32_t             outBoundProxyPort;   

    srv_handle_t          SRVhandle;           
    srv_handle_t          ObpSRVhandle;        
    int                  routeMode;           
    void                 *udpId;              

    




    sipContact_t *contact_info;

    

    





    sipRecordRoute_t *record_route_info;

    string_t          calledDisplayedName;
    string_t          callingNumber;
    string_t          altCallingNumber;
    string_t          callingDisplayName;
    string_t          calledNumber;
    boolean           displayCalledNumber;
    boolean           displayCallingNumber;
    uint16_t          calledNumberLen;
    boolean           calledNumberFirstDigitDialed;

    






    uint32_t          flags;

#define LOOPBACK            1
#define INBAND_ALERTING     (1<<1)
#define SIP_TCP             (1<<2)
#define INCOMING            (1<<3)
#define ADDED_TO_TABLE      (1<<4)
#define DO_CALL_HISTORY     (1<<5)
#define RSVP_RESERVED       (1<<6)
#define SENT_BYE            (1<<7)
#define SENT_CANCEL         (1<<8)
#define RECD_BYE            (1<<9)
#define SENT_3456XX         (1<<10)
#define RECD_456XX          (1<<11)
#define HARIKIRI            (1<<12)
#define TIMED_OUT           (1<<13)
#define SD_IN_ACK           (1<<14)
#define MSG_PASSTHRU        (1<<15)
#define SIGO_CALL           (1<<16)
#define RECD_1xx            (1<<17)
#define SEND_CANCEL         (1<<18)
#define FINAL_NOTIFY        (1<<19)
#define SENT_INVITE_REPLACE (1<<20)

    


    cpr_ip_addr_t       src_addr;            
    cpr_ip_addr_t       dest_sip_addr;       


    uint32_t            local_port;          
    uint32_t            dest_sip_port;       
    int16_t             sip_socket_handle;

    


      cc_msgbody_info_t local_msg_body; 





      char              *old_session_id;
      char              *old_version_id;





    


    char             ReqURI[MAX_SIP_URL_LENGTH];   
    string_t         ReqURIOriginal;  
    string_t         sip_from;
    string_t         sip_to;
    string_t         sip_to_tag;
    string_t         sip_from_tag;
    string_t         sip_contact;
    string_t         sip_remote_party_id;
    string_t         sip_reqby;
    string_t         sip_require;
    string_t         sip_unsupported;
    char             *diversion[MAX_DIVERSION_HEADERS];
#define MAX_REQ_OUTSTANDING 3
    sipTransaction_t sent_request[MAX_REQ_OUTSTANDING];
    sipTransaction_t recv_request[MAX_REQ_OUTSTANDING];
    uint32_t         last_recv_request_cseq;
    sipMethod_t      last_recv_request_cseq_method;
    uint32_t         last_used_cseq;
    uint32_t         last_recv_invite_cseq;
    string_t         sip_referTo;
    string_t         sip_referredBy;
    string_t         referto;
    string_t         sipxfercallid;
    boolean          wastransferred;
    boolean          blindtransferred;
    unsigned int     xfer_status;
    
    sipDiversionInfo_t *div_info;

    cc_call_type_e   call_type;

    
    sipRemotePartyIdInfo_t *rpid_info;
    
    sipRemotePartyId_t *best_rpid;

    
    sipMessage_t *last_request;

    


    sipSMXfrType_t xfr_inprogress;
    cc_features_t  featuretype;

    sipAvtPayloadType_t avt;

    


    sipRegister_t     reg;
    sipAuthenticate_t authen;

    


    char *refer_proxy_auth;
#ifdef SIP_ACC_CONT
    char *refer_acc_cont;
#endif
    



    sipRedirectInfo_t *redirect_info;

    


    cc_alerting_type alert_info;

    


    cc_call_info_t *in_call_info;
    cc_call_info_t *out_call_info;

    


    vcm_ring_mode_t alerting_ring;
    vcm_tones_t alerting_tone;

    


    boolean call_entered_into_pd;
    boolean wait_for_ack;
    boolean send_delayed_bye;
    boolean retx_flag;
    boolean early_transfer;
    boolean first_pass_3xx;
    CC_ID cc_type;
    void *cc_cfg_table_entry;
    


#define replaces_tag      1
#define rel_tag           (1<<1)
#define early_session_tag (1<<2)
#define join_tag          (1<<3)
#define path_tag          (1<<4)
#define precondition_tag  (1<<5)
#define pref_tag          (1<<6)
#define privacy_tag       (1<<7)
#define sec_agree_tag     (1<<8)
#define timer_tag         (1<<9)
#define norefersub_tag    (1<<10)
#define cisco_callinfo_tag (1<< 11)
#define cisco_srtp_fallback_tag (1<< 12)
#define extended_refer_tag (1<<16)
#define cisco_serviceuri_tag (1<<18)
#define cisco_escapecodes_tag (1<<19)
#define cisco_service_control_tag (1<<20)
#define sdp_anat_tag            (1<< 21)
#define unrecognized_tag        (1<<31)

    uint32_t supported_tags;
    uint32_t required_tags;

#define SUPPORTED_TAGS  replaces_tag | join_tag | sdp_anat_tag | norefersub_tag


    


#define ALLOW_ACK          1
#define ALLOW_BYE          (1<<1)
#define ALLOW_CANCEL       (1<<2)
#define ALLOW_INFO         (1<<3)
#define ALLOW_INVITE       (1<<4)
#define ALLOW_MESSAGE      (1<<5)
#define ALLOW_NOTIFY       (1<<6)
#define ALLOW_OPTIONS      (1<<7)
#define ALLOW_PRACK        (1<<8)
#define ALLOW_PUBLISH      (1<<9)
#define ALLOW_REFER        (1<<10)
#define ALLOW_REGISTER     (1<<11)
#define ALLOW_SUBSCRIBE    (1<<12)
#define ALLOW_UPDATE       (1<<13)

    uint16_t allow_methods;

    sipOfferAnswerState oa_state;
    int last_recvd_response_code;
    sipJoinInfo_t       *join_info;
    cc_feature_data_t   *feature_data;
    int dup_flags;
    void *mother_ccb;

#define DUP_NO_FLAGS              0x00
#define DUP_CCB                   0x01
#define DUP_CCB_NEW_CALLID        0x02
#define DUP_CCB_INIT_STATE        0x04
#define DUP_CCB_REINIT_DNS        0x08
#define DUP_CCB_STOLEN_FEAT_DATA  0x10

    cc_kfact_t *kfactor_ptr;

    boolean send_reason_header;

    uint32_t callref;

} ccsipCCB_t;


typedef struct {
    ccsipCCB_t ccbs[MAX_CCBS];
    int        backup_active; 
} ccsipGlobInfo_t;


typedef struct {
    sipSMEventType_t type;
    ccsipCCB_t *ccb;
    union {
        sipMessage_t *pSipMessage;
        cc_msg_t *cc_msg;
        cpr_ip_addr_t UsrInfo;
    } u;
} sipSMEvent_t;

typedef void (*sipSMEventActionFn_t)(ccsipCCB_t *ccb, sipSMEvent_t *event);
typedef void (*shutdown_callback_fn)(void *data);

typedef struct {
    shutdown_callback_fn callback;
    void                 *data;
} shutdown_t;

typedef enum {
    SIP_SDP_SUCCESS = 0,
    SIP_SDP_SESSION_AUDIT,
    SIP_SDP_DNS_FAIL,
    SIP_SDP_NO_MEDIA,
    SIP_SDP_ERROR,
    SIP_SDP_NOT_PRESENT
} sipsdp_status_t;

extern ccsipGlobInfo_t gGlobInfo;
sipSMAction_t get_handler_index(sipSMStateType_t isipsmstate,
                                sipSMEventType_t isipsmevent);

void ccsip_handle_idle_ev_sip_invite(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_idle_ev_cc_setup(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_sentinvite_ev_sip_1xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentinvite_ev_sip_2xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentinvite_ev_sip_3xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentinvite_ev_sip_fxx(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_sentinviteconnected_ev_cc_connected_ack(ccsipCCB_t *ccb,
                                                          sipSMEvent_t *event);

void ccsip_handle_recvinvite_ev_cc_setup_ack(ccsipCCB_t *ccb,
                                             sipSMEvent_t *event);
void ccsip_handle_recvinvite_ev_cc_proceeding(ccsipCCB_t *ccb,
                                              sipSMEvent_t *event);
void ccsip_handle_recvinvite_ev_cc_alerting(ccsipCCB_t *ccb,
                                            sipSMEvent_t *event);
void ccsip_handle_recvinvite_ev_cc_connected(ccsipCCB_t *ccb,
                                             sipSMEvent_t *event);
void ccsip_handle_recvinvite_ev_sip_ack(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_active_ev_cc_feature(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_active_ev_cc_feature_hold(ccsipCCB_t *ccb,
                                            sipSMEvent_t *event);
void ccsip_handle_active_ev_cc_feature_resume_or_media(ccsipCCB_t *ccb,
                                              sipSMEvent_t *event);
void ccsip_handle_active_ev_cc_feature_other(ccsipCCB_t *ccb,
                                             sipSMEvent_t event);
void ccsip_handle_active_ev_sip_invite(ccsipCCB_t *ccb, sipSMEvent_t *event);


void ccsip_handle_recvmidcallinvite_ccfeatureackpending_ev_cc_feature_ack(
                                                          ccsipCCB_t *ccb,
                                                          sipSMEvent_t *event);
void ccsip_handle_recvmidcallinvite_sipackpending_ev_sip_ack(ccsipCCB_t *ccb,
                                                           sipSMEvent_t *event);

void ccsip_handle_accept_2xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_active_2xx(ccsipCCB_t *ccb, sipSMEvent_t *event);


void ccsip_handle_default(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_default_sip_message(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_default_sip_response(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_default_sip_timer(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_disconnect_local(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_disconnect_local_early(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_disconnect_local_unanswered(ccsipCCB_t *ccb,
                                              sipSMEvent_t *event);
void ccsip_handle_disconnect_remote(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_refer_sip_message(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_active_ev_cc_feature_ack(ccsipCCB_t *ccb,
                                            sipSMEvent_t *event);

void ccsip_handle_active_ev_cc_feature_xfer(ccsipCCB_t *ccb,
                                            sipSMEvent_t *event);
void ccsip_handle_active_ev_cc_feature_indication(ccsipCCB_t *ccb,
                                            sipSMEvent_t *event);

void ccsip_handle_sentbye_recvd_invite(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentbye_ev_sip_fxx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentbye_ev_sip_2xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentbye_ev_sip_1xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sendbye_ev_supervision_disconnect(ccsipCCB_t *ccb,
                                                    sipSMEvent_t *event);

void ccsip_handle_recv_error_response_ev_sip_ack(ccsipCCB_t *ccb,
                                                  sipSMEvent_t *event);

void ccsip_handle_release_complete(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_send_blind_notify(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentblindntfy_ev_sip_2xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_release_ev_sip_bye(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_process_in_call_options_request(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_ev_cc_answer_options_request(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_ev_cc_answer_audit_request(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_localexpires_timer(ccsipCCB_t *ccb, sipSMEvent_t *event);

void ccsip_handle_early_ev_sip_update(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_early_ev_sip_update_response(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_early_ev_cc_feature(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_early_ev_cc_feature_ack(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_active_ev_sip_update(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_recvupdatemedia_ccfeatureackpending_ev_cc_feature_ack(
                                                           ccsipCCB_t *ccb,
                                                           sipSMEvent_t *event);
void ccsip_handle_timer_glare_avoidance(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_recvinvite_ev_expires_timer(ccsipCCB_t *ccb,
                                              sipSMEvent_t *event);
void ccsip_handle_unsolicited_notify(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_recvinvite_ev_sip_2xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_icmp_unreachable(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_disconnect_media_change(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sent_ood_refer_ev_sip_1xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sent_ood_refer_ev_sip_2xx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sent_ood_refer_ev_sip_fxx(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_default_ev_cc_feature(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_default_recvreq_ack_pending_ev_cc_feature(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_sentinvite_midcall_ev_cc_feature(ccsipCCB_t *ccb,
                                                   sipSMEvent_t *event);
void ccsip_handle_sentinvite_midcall_ev_sip_2xx(ccsipCCB_t *ccb,
                                                sipSMEvent_t *event);
void ccsip_handle_release_ev_cc_feature(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_ev_cc_info(ccsipCCB_t *ccb, sipSMEvent_t *event);
void ccsip_handle_release_ev_release(ccsipCCB_t *ccb, sipSMEvent_t *event);

int sip_sm_init(void);
void sip_shutdown(void);
void sip_shutdown_phase1(int, int reason);
void sip_shutdown_phase2(int);
void sip_restart(void);
int sip_sm_ccb_init(ccsipCCB_t *ccb, line_t index, int DN,
                    sipRegSMStateType_t initial_state);
ccsipCCB_t *sip_sm_get_ccb_by_index(line_t index);
ccsipCCB_t *sip_sm_get_ccb_by_ccm_id_and_index(int ccm_id, line_t idx);
ccsipCCB_t *sip_sm_get_ccb_by_callid(const char *callid);
ccsipCCB_t *sip_sm_get_ccb_next_available(line_t *line_number);
ccsipCCB_t *sip_sm_get_ccb_by_gsm_id(callid_t gsm_id);
ccsipCCB_t *sip_sm_get_ccb_by_target_call_id(callid_t con_id);
ccsipCCB_t *sip_sm_get_target_call_by_gsm_id(callid_t gsm_id);
ccsipCCB_t *sip_sm_get_target_call_by_con_call_id(callid_t con_call_id);
boolean sip_is_releasing(ccsipCCB_t* ccb);
callid_t sip_sm_get_blind_xfereror_ccb_by_gsm_id(callid_t gsm_id);
uint16_t sip_sm_determine_ccb(const char *callid,
                              sipCseq_t *sipCseq,
                              sipMessage_t *pSipMessage,
                              boolean is_request,
                              ccsipCCB_t **ccb);
void sip_sm_call_cleanup(ccsipCCB_t *ccb);
void free_duped(ccsipCCB_t *dupCCB);


int sip_sm_process_event(sipSMEvent_t *pEvent);
int sip_sm_process_cc_event(cprBuffer_t buf);
void sip_sm_util_normalize_name(ccsipCCB_t *ccb, char *dialString);


const char *sip_util_state2string(sipSMStateType_t state);
const char *sip_util_event2string(sipSMEventType_t event);
const char *sip_util_method2string(sipMethod_t method);
boolean sip_sm_is_bye_or_cancel_response(sipMessage_t *response);

sipSMEventType_t sip_util_ccevent2sipccevent(cc_msgs_t cc_msg_type);
const char *sip_util_feature2string(cc_features_t feature);

void sip_create_new_sip_call_id(char *sipCallID, uint8_t *mac_address,
                                char *pSrcAddrStr);
void sip_util_get_new_call_id(ccsipCCB_t *ccb);
boolean sip_sm_is_previous_call_id(const char *pCallID,
                                   line_t *pPreviousCallLine);
boolean sip_sm_util_is_timeinterval(const char *pStr);

void sip_decrement_backup_active_count(ccsipCCB_t *ccb);

#ifdef DEBUG
void print_ccb_memoryusage(ccsipCCB_t *ccb);
#endif

void sip_sm_200and300_update(ccsipCCB_t *ccb, sipMessage_t *response,
                             int response_code);
char *sip_sm_purify_tag(char *tag);
boolean sip_sm_is_invite_response(sipMessage_t *response);
boolean sip_sm_is_refer_response(sipMessage_t *response);
boolean sip_sm_is_notify_response(sipMessage_t *response);

void sip_sm_dequote_string(char *str, int max_size);
void sip_sm_check_retx_timers(ccsipCCB_t *ccb, sipMessage_t *message);
int strcasecmp_ignorewhitespace(const char *cs, const char *ct);

void sip_util_make_ccmsgsdp(cc_sdp_t *pCcMsgSdp, ccsipCCB_t *ccb);

int sip_dns_gethostbysrv(char *domain,
                         cpr_ip_addr_t *ipaddr_ptr,
                         uint16_t *port,
                         srv_handle_t *srv_order,
                         boolean retried_addr);
int sip_dns_gethostbysrvorname(char *hname,
                               cpr_ip_addr_t *ipaddr_ptr,
                               uint16_t *port);
void sip_util_make_tag(char *tag_str);
void get_sip_error_string(char *errortext, int response);
int ccsip_cc_to_sip_cause(cc_causes_t cause, char **phrase);
void sip_sm_update_to_on_midcall_200(ccsipCCB_t *ccb, sipMessage_t *response);

sipServiceControl_t *ccsip_get_notify_service_control(sipMessage_t *pSipMessage);
boolean ccsip_is_special_name_to_mask_display_number(const char *name);
void sip_sm_change_state(ccsipCCB_t *ccb, sipSMStateType_t new_state);

#define SIP_SM_CALL_SETUP_NOT_COMPLETED(x) \
        ((x->state == SIP_STATE_RECV_INVITE) || \
         (x->state == SIP_STATE_RECV_INVITE_PROCEEDING) || \
         (x->state == SIP_STATE_RECV_INVITE_ALERTING) || \
         (x->state == SIP_STATE_RECV_INVITE_CONNECTED))
#define SIP_SM_CALL_SETUP_RESPONDING(x) \
        ((x->state == SIP_STATE_RECV_INVITE_PROCEEDING) || \
         (x->state == SIP_STATE_RECV_INVITE_ALERTING))

#define ccsip_is_replace_setup(replace) (replace)

extern char *ccsip_find_preallocated_sip_local_tag(line_t dn_line);
extern void ccsip_free_preallocated_sip_local_tag(line_t dn_line);
extern char *getPreallocatedSipCallID(line_t dn_line);
extern char *getPreallocatedSipLocalTag(line_t dn_line);
extern ccsipCCB_t* create_dupCCB(ccsipCCB_t *origCCB, int dup_flags);


#define MAX_INFO_HANDLER    32







extern char *g_registered_info[];

typedef void (*info_package_handler_t)(line_t line, callid_t call_id,
                                       const char *info_package,
                                       const char *content_type,
                                       const char *message_body);

int ccsip_info_package_handler_init(void);
void ccsip_info_package_handler_shutdown(void);
int ccsip_register_info_package_handler(const char *info_package,
                                        const char *content_type,
                                        info_package_handler_t handler);
int ccsip_deregister_info_package_handler(const char *info_package,
                                          const char *content_type,
                                          info_package_handler_t handler);
void ccsip_parse_send_info_header(sipMessage_t *pSipMessage, string_t *recv_info_list);
int ccsip_handle_info_package(ccsipCCB_t *ccb, sipMessage_t *pSipMessage);


#endif
