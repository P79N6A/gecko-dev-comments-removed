



#ifndef _CCSIP_SUBSMANAGER_H_
#define _CCSIP_SUBSMANAGER_H_

#include "cpr_types.h"
#include "cpr_ipc.h"
#include "cpr_timers.h"
#include "cpr_socket.h"
#include "ccsip_core.h"
#include "phone_platform_constants.h"
#include "singly_link_list.h"
#include "ccsip_common_cb.h"




typedef enum {
    SUBS_STATE_IDLE = 0,        
    SUBS_STATE_REGISTERED,

    
    SUBS_STATE_SENT_SUBSCRIBE,  
    SUBS_STATE_RCVD_NOTIFY,     
    SUBS_STATE_SENT_SUBSCRIBE_RCVD_NOTIFY,  

    
    SUBS_STATE_RCVD_SUBSCRIBE,  
    SUBS_STATE_SENT_NOTIFY,     
    SUBS_STATE_RCVD_SUBSCRIBE_SENT_NOTIFY, 

    




    SUBS_STATE_ACTIVE,

    SUBS_STATE_INVALID
} subsStateType_t;

typedef enum {
    SUBSCRIPTION_NULL = 0,
    SUBSCRIPTION_TERMINATE
} subscriptionState;



 
typedef uint32_t sub_id_t;

#define MAX_EVENT_NAME_LEN           32
#define CCSIP_SUBS_START_CSEQ        1000




#define MAX_SCBS                     ((MAX_TEL_LINES * 2) < 32 ? 32 : (MAX_TEL_LINES * 2))
#define LIMIT_SCBS_USAGE             (MAX_SCBS - ((MAX_SCBS * 20) / 100))
#define TMR_PERIODIC_SUBNOT_INTERVAL 5
#define CCSIP_SUBS_INVALID_SUB_ID   (sub_id_t)(-1)
#define MAX_SCB_HISTORY              10

typedef enum {
    SM_REASON_CODE_NORMAL = 0,
    SM_REASON_CODE_ERROR,
    SM_REASON_CODE_SHUTDOWN,
    SM_REASON_CODE_ROLLOVER,
    SM_REASON_CODE_RESET_REG
} ccsip_reason_code_e;





typedef struct {
    int  status_code;
    long expires;
} ccsip_subs_result_data_t;


typedef struct {
    ccsip_event_data_t *eventData;
    int               expires;
    int               line;
    string_t          from;
    string_t          to;
} ccsip_subs_ind_data_t;


typedef struct {
    ccsip_event_data_t     *eventData;
    sip_subs_state_e        subscription_state; 
    sip_subs_state_reason_e subscription_state_reason;
    uint32_t                expires;
    uint32_t                retry_after;
    uint32_t                cseq;
    char                    entity[CC_MAX_DIALSTRING_LEN]; 
} ccsip_notify_ind_data_t;


typedef struct {
    int status_code;
} ccsip_notify_result_data_t;


typedef struct {
    int status_code;
} ccsip_subs_terminate_data_t;


typedef struct ccsip_sub_not_data_t {
    int                 msg_id;
    sub_id_t            sub_id;
    int                 sub_duration;
    cc_subscriptions_t  event;
    line_t              line_id;
    callid_t            gsm_id;
    boolean             norefersub;
    long		request_id;
    ccsip_reason_code_e reason_code;
    union {
        ccsip_subs_ind_data_t       subs_ind_data;
        ccsip_subs_result_data_t    subs_result_data;
        ccsip_notify_ind_data_t     notify_ind_data;
        ccsip_notify_result_data_t  notify_result_data;
        ccsip_subs_terminate_data_t subs_term_data;
    } u;
} ccsip_sub_not_data_t;



typedef void (*ccsipSubsIndCallbackFn_t)(ccsip_sub_not_data_t *msg_data);


typedef void (*ccsipSubsResultCallbackFn_t)(ccsip_sub_not_data_t *msg_data);


typedef void (*ccsipNotifyIndCallbackFn_t)(ccsip_sub_not_data_t *msg_data);


typedef void (*ccsipNotifyResultCallbackFn_t)(ccsip_sub_not_data_t *msg_data);


typedef void (*ccsipSubsTerminateCallbackFn_t)(ccsip_sub_not_data_t *msg_data);
typedef void (*ccsipGenericCallbackFn_t)(ccsip_sub_not_data_t *msg_data);




#define SUBSCRIPTION_IN_PROGRESS     1010
#define SUBSCRIPTION_SUCCEEDED       1020
#define SUBSCRIPTION_REJECTED        1030
#define SUBSCRIPTION_FAILED          1040
#define NOTIFY_REQUEST_FAILED        1050
#define SUBSCRIBE_REQUEST_FAILED     1060
#define SUBSCRIBE_FAILED_NORESOURCE  1061
#define SUBSCRIBE_FAILED_BADEVENT    1062
#define SUBSCRIBE_FAILED_BADINFO     1062
#define NETWORK_SUBSCRIPTION_EXPIRED 1070
#define APPLICATION_SUBSCRIPTION_EXPIRED 1080
#define REQUEST_TIMEOUT              1090


typedef struct sipspi_subscribe_reg_t_ {
    cc_subscriptions_t eventPackage; 
    ccsipSubsIndCallbackFn_t subsIndCallback; 
    cc_srcs_t subsIndCallbackTask;   
    int       subsIndCallbackMsgID;  
    ccsipSubsTerminateCallbackFn_t subsTermCallback; 
    int       subsTermCallbackMsgID; 
    long      min_duration; 
    long      max_duration; 
} sipspi_subscribe_reg_t;


typedef struct sipspi_subscribe_t_ {
    sub_id_t           sub_id;       
    cc_subscriptions_t eventPackage; 
    cc_subscriptions_t acceptPackage; 
    long               duration;     
    char               subscribe_uri[CC_MAX_DIALSTRING_LEN];
    char               subscriber_uri[CC_MAX_DIALSTRING_LEN]; 
    long		request_id;   

    ccsipSubsResultCallbackFn_t subsResultCallback;
    ccsipNotifyIndCallbackFn_t notifyIndCallback;
    ccsipSubsTerminateCallbackFn_t subsTermCallback;

    cc_srcs_t          subsNotCallbackTask;
    int                subsResCallbackMsgID;
    int                subsNotIndCallbackMsgID;
    int                subsTermCallbackMsgID;

    cpr_ip_addr_t      dest_sip_addr; 
    uint16_t           dest_sip_port; 

    callid_t           call_id;
    line_t             dn_line;       

    boolean            auto_resubscribe; 
    boolean            norefersub;
    ccsip_event_data_t *eventData;    

} sipspi_subscribe_t;


typedef struct sipspi_subscribe_resp_t_ {
    sub_id_t sub_id;         
    uint16_t response_code;  
    int      duration;       
} sipspi_subscribe_resp_t;


typedef struct sipspi_notify_t_ {
    sub_id_t            sub_id;     
    
    ccsipNotifyResultCallbackFn_t notifyResultCallback;
    int                 subsNotResCallbackMsgID;
    ccsip_event_data_t *eventData;  
    cc_subscriptions_t eventPackage; 
    subscriptionState   subState;
    cc_srcs_t           subsNotCallbackTask; 
} sipspi_notify_t;


typedef struct sipspi_notify_resp_t_ {
    sub_id_t sub_id;
    int      response_code;
    int      duration;
    uint32_t cseq;
} sipspi_notify_resp_t;


typedef struct sipspi_subscribe_term_t_ {
    sub_id_t sub_id;
    long     request_id;
    cc_subscriptions_t eventPackage; 
    boolean immediate;
} sipspi_subscribe_term_t;





















typedef struct sipspi_msg_t_ {
    union {
        sipspi_subscribe_reg_t  subs_reg;
        sipspi_subscribe_t      subscribe;
        sipspi_subscribe_resp_t subscribe_resp;
        sipspi_notify_t         notify;
        sipspi_notify_resp_t    notify_resp;
        sipspi_subscribe_term_t subs_term;
        







    } msg;
} sipspi_msg_t;


typedef struct sipspi_msg_list_t_ {
    uint32_t cmd;
    sipspi_msg_t *msg;
    struct sipspi_msg_list_t_ *next;
} sipspi_msg_list_t;


typedef struct {
    ccsip_common_cb_t       hb; 

    line_t             line;
    
    sub_id_t           sub_id; 

    
    boolean            pendingClean;
    unsigned char      pendingCount;

    
    boolean            internal; 

    
    ccsipSubsIndCallbackFn_t subsIndCallback;
    cc_srcs_t          subsIndCallbackTask;
    cc_srcs_t          subsNotCallbackTask;
    int                subsIndCallbackMsgID;
    ccsipSubsResultCallbackFn_t subsResultCallback;
    int                subsResCallbackMsgID;
    ccsipNotifyIndCallbackFn_t notifyIndCallback;
    int                notIndCallbackMsgID;
    ccsipSubsTerminateCallbackFn_t subsTermCallback;
    int                subsTermCallbackMsgID;
    ccsipNotifyResultCallbackFn_t notifyResultCallback;
    int                notResCallbackMsgID;

    short              sip_socket_handle;
    boolean            useDeviceAddressing;
    callid_t           gsm_id;
    long		request_id;

    
    subsStateType_t    smState;
    subsStateType_t    outstandingIncomingNotifyTrxns; 
    unsigned long      min_expires;
    unsigned long      max_expires;
    ccsipCCB_t        *ccbp;             
    char               event_name[MAX_EVENT_NAME_LEN];
    boolean            auto_resubscribe; 
    boolean            norefersub;

    
    uint32_t           last_sent_request_cseq;
    sipMethod_t        last_sent_request_cseq_method;
    uint32_t           last_recv_request_cseq;
    sipMethod_t        last_recv_request_cseq_method;

    
    char               SubURI[MAX_SIP_URL_LENGTH];
    char               SubURIOriginal[MAX_SIP_URL_LENGTH];
    char               SubscriberURI[MAX_SIP_URL_LENGTH];
    string_t           sip_from;
    string_t           sip_to;
    string_t           sip_to_tag;
    string_t           sip_from_tag;
    string_t           sip_contact;
    string_t           cached_record_route;
    sipContact_t      *contact_info;
    sipRecordRoute_t  *record_route_info;
    sll_handle_t       incoming_trxns; 
    string_t           callingNumber;

    
    sip_subs_state_e  subscription_state;
    sip_subs_state_reason_e subscription_state_reason;
    uint32_t          retry_after;


    
    sipspi_msg_list_t *pendingRequests;
} sipSCB_t;


typedef struct {
    ccsip_common_cb_t       hb; 
    char                    full_ruri[MAX_SIP_URL_LENGTH];
    cprTimer_t              timer; 
    uint32_t                trxn_id;
} sipTCB_t;


typedef struct {
    char               last_call_id[MAX_SIP_CALL_ID];
    char               last_from_tag[MAX_SIP_TAG_LENGTH];
    cc_subscriptions_t eventPackage;
} sipSubsHistory_t;

#define MAX_SUB_EVENTS    5
#define MAX_SUB_EVENT_NAME_LEN 16
extern const char eventNames[MAX_SUB_EVENTS][MAX_SUB_EVENT_NAME_LEN];





int sip_subsManager_init();
int sip_subsManager_shut();


int subsmanager_handle_ev_cc_feature_subscribe(sipSMEvent_t *);
int subsmanager_handle_ev_cc_feature_notify(sipSMEvent_t *);

int subsmanager_handle_ev_app_subscribe_register(cprBuffer_t buf);
int subsmanager_handle_ev_app_subscribe(cprBuffer_t buf);
int subsmanager_handle_ev_app_subscribe_response(cprBuffer_t buf);
int subsmanager_handle_ev_app_notify(cprBuffer_t buf);
void subsmanager_handle_ev_app_unsolicited_notify(cprBuffer_t buf, line_t line);
int subsmanager_handle_ev_app_notify_response(cprBuffer_t buf);
int subsmanager_handle_ev_app_subscription_terminated(cprBuffer_t buf);
int subsmanager_handle_retry_timer_expire(int scb_index);
void subsmanager_handle_periodic_timer_expire(void);
int subsmanager_test_start_routine();










int subsmanager_handle_ev_sip_subscribe(sipMessage_t *pSipMessage,
                                        sipMethod_t sipMethod,
                                        boolean in_dialog);
int subsmanager_handle_ev_sip_subscribe_notify(sipMessage_t *pSipMessage);


int subsmanager_handle_ev_sip_response(sipMessage_t *pSipMessage);

void free_event_data(ccsip_event_data_t *event_data);
int sip_subsManager_rollover(void);
int sip_subsManager_reset_reg(void);
void submanager_update_ccb_addr(ccsipCCB_t *ccb);
boolean add_content(ccsip_event_data_t *eventData, sipMessage_t *request, const char *fname);
void pres_unsolicited_notify_ind(ccsip_sub_not_data_t * msg_data);
sipTCB_t *find_tcb_by_sip_callid(const char *callID_p);
int subsmanager_handle_ev_sip_unsolicited_notify_response(sipMessage_t *pSipMessage, sipTCB_t *tcbp);
void subsmanager_unsolicited_notify_timeout(void *data);

#endif
