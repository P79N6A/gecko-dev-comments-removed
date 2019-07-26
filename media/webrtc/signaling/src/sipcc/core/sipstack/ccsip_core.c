



#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "cpr_in.h"
#include "cpr_rand.h"

#include "ccsip_core.h"
#include "text_strings.h"
#include "util_string.h"
#include "ccsip_messaging.h"
#include "ccsip_platform_udp.h"
#include "ccsip_platform.h"
#include "ccsip_macros.h"
#include "ccsip_pmh.h"
#include "ccsip_spi_utils.h"
#include "phone_debug.h"
#include "ccsip_register.h"
#include "ccsip_credentials.h"
#include "ccsip_callinfo.h"
#include "ccsip_cc.h"
#include "ccsip_task.h"
#include "config.h"
#include "string_lib.h"
#include "dialplan.h"
#include "fsm.h"
#include "sip_interface_regmgr.h"
#include "ccsip_subsmanager.h"
#include "ccsip_publish.h"
#include "sdp.h"
#include "sip_common_transport.h"
#include "sip_common_regmgr.h"
#include "rtp_defs.h"
#include "uiapi.h"
#include "text_strings.h"
#include "platform_api.h"
#include "misc_util.h"






extern void sip_platform_handle_service_control_notify(sipServiceControl_t *scp);
extern uint32_t IPNameCk(char *name, char *addr_error);


#define ADD_TO_ARP_CACHE(dest_sip_addr)
#define UNBIND_UDP_ICMP_HANDLER(udp_id)


extern boolean sip_mode_quiet;

extern void ccsip_debug_init(void);
extern void shutdownCCAck(int action);
void ccsip_remove_wlan_classifiers(void);

#define USECALLMANAGER_LEN 14




extern const char *tone_names[];
const char *ring_names[] = {
    "Bellcore-dr1",
    "Bellcore-dr2",
    "Bellcore-dr3",
    "Bellcore-dr4",
    "Bellcore-dr5"
};



static int sip_sm_request_check_and_store(ccsipCCB_t *ccb, sipMessage_t *request,
                                               sipMethod_t request_method,
                                               boolean midcall,
                                               uint16_t *request_check_reason_code,
                                               char *request_check_reason_phrase,
                                               boolean store_invite);
void sip_sm_update_to_from_on_callsetup_finalresponse(ccsipCCB_t *ccb,
                                                                 sipMessage_t *response);
void sip_sm_update_contact_recordroute(ccsipCCB_t *ccb, sipMessage_t *response,
                                                  int response_code, boolean midcall);
static boolean ccsip_set_replace_info(ccsipCCB_t *ccb, cc_setup_t * setup);
static boolean ccsip_handle_cc_select_event(sipSMEvent_t *sip_sm_event);
static boolean ccsip_handle_cc_b2bjoin_event(sipSMEvent_t *sip_sm_event);
static void ccsip_set_join_info(ccsipCCB_t *ccb, cc_setup_t * setup);
static boolean ccsip_get_join_info(ccsipCCB_t *ccb, sipMessage_t *request);
static char *ccsip_find_preallocated_sip_call_id(line_t dn_line);
static void ccsip_free_preallocated_sip_call_id(line_t dn_line);
static boolean ccsip_handle_cc_hook_event(sipSMEvent_t *sip_sm_event);

extern cc_int32_t dnsGetHostByName (const char *hname, cpr_ip_addr_t *ipaddr_ptr, cc_int32_t timeout, cc_int32_t retries);


extern char *Basic_is_phone_forwarded(line_t line);


extern sipPlatformUITimer_t sipPlatformUISMTimers[];
extern sipGlobal_t sip;
extern sipCallHistory_t gCallHistory[];


int      dns_error_code;  
uint16_t server_caps = 0; 
boolean  sip_reg_all_failed;

ccsipGlobInfo_t  gGlobInfo;
sipCallHistory_t gCallHistory[MAX_TEL_LINES];

typedef struct {
    int16_t sipValidEvent;
    int16_t actionIndex;
} subStateEvent_t;

#define MAX_STATE_EVENTS 13
typedef struct {
    int16_t sipState;
    subStateEvent_t validEvent[MAX_STATE_EVENTS];
} sipSMfunctable_t;

static char *preAllocatedSipCallID[MAX_REG_LINES] = { NULL };
static char *preAllocatedTag[MAX_REG_LINES] = { NULL };
boolean g_disable_mass_reg_debug_print = FALSE;

static const sipSMfunctable_t g_sip_table[SIP_STATE_END - SIP_STATE_BASE + 1] =
{
    


    { SIP_STATE_IDLE,
        {
      
      {E_SIP_INVITE, H_IDLE_EV_SIP_INVITE},
      
      {E_CC_SETUP, H_IDLE_EV_CC_SETUP},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_FEATURE, H_DEFAULT_EV_CC_FEATURE},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },
    


    {SIP_STATE_SENT_INVITE,
     {
      
      {E_SIP_1xx, H_SENTINVITE_EV_SIP_1XX},
      
      {E_SIP_2xx, H_SENTINVITE_EV_SIP_2XX},
      
      {E_SIP_3xx, H_SENTINVITE_EV_SIP_3XX},
      
      {E_SIP_FAILURE_RESPONSE, H_SENTINVITE_EV_SIP_FXX},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL_EARLY},
      
      {E_SIP_INV_EXPIRES_TIMER, H_DISCONNECT_LOCAL_EARLY},
      
      {E_SIP_UPDATE, H_EARLY_EV_SIP_UPDATE},
      
      {E_SIP_UPDATE_RESPONSE, H_EARLY_EV_SIP_UPDATE_RESPONSE},
      
      {E_CC_FEATURE, H_EARLY_EV_CC_FEATURE},
      
      {E_CC_FEATURE_ACK, H_EARLY_EV_CC_FEATURE_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },

    


    {SIP_STATE_SENT_INVITE_CONNECTED,
     {
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_CC_CONNECTED_ACK, H_SENTINVITECONNECTED_EV_CC_CONNECTED_ACK},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL},
      
      {E_SIP_UPDATE, H_EARLY_EV_SIP_UPDATE},
      
      {E_SIP_UPDATE_RESPONSE, H_EARLY_EV_SIP_UPDATE_RESPONSE},
      
      {E_CC_FEATURE, H_EARLY_EV_CC_FEATURE},
      
      {E_CC_FEATURE_ACK, H_EARLY_EV_CC_FEATURE_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },
    


    {SIP_STATE_RECV_INVITE,
     {
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_SIP_CANCEL, H_DISCONNECT_REMOTE},
      
      {E_CC_SETUP_ACK, H_RECVINVITE_EV_CC_SETUP_ACK},
      
      {E_CC_PROCEEDING, H_RECVINVITE_EV_CC_PROCEEDING},
      
      {E_CC_ALERTING, H_RECVINVITE_EV_CC_ALERTING},
      
      {E_CC_CONNECTED, H_RECVINVITE_EV_CC_CONNECTED},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL_UNANSWERED},
      
      {E_SIP_INV_LOCALEXPIRES_TIMER, H_HANDLE_LOCALEXPIRES_TIMER},
      
      {E_SIP_UPDATE, H_EARLY_EV_SIP_UPDATE},
      
      {E_SIP_UPDATE_RESPONSE, H_EARLY_EV_SIP_UPDATE_RESPONSE},
      
      {E_CC_FEATURE, H_EARLY_EV_CC_FEATURE},
      
      {E_CC_FEATURE_ACK, H_EARLY_EV_CC_FEATURE_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY}

      
      }
     },

    


    {SIP_STATE_RECV_INVITE_PROCEEDING,
     {
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_SIP_CANCEL, H_DISCONNECT_REMOTE},
      
      {E_CC_ALERTING, H_RECVINVITE_EV_CC_ALERTING},
      
      {E_CC_CONNECTED, H_RECVINVITE_EV_CC_CONNECTED},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL_UNANSWERED},
      
      {E_SIP_INV_LOCALEXPIRES_TIMER, H_HANDLE_LOCALEXPIRES_TIMER},
      
      {E_SIP_UPDATE, H_EARLY_EV_SIP_UPDATE},
      
      {E_SIP_UPDATE_RESPONSE, H_EARLY_EV_SIP_UPDATE_RESPONSE},
      
      {E_CC_FEATURE, H_EARLY_EV_CC_FEATURE},
      
      {E_CC_FEATURE_ACK, H_EARLY_EV_CC_FEATURE_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },
    


    {SIP_STATE_RECV_INVITE_ALERTING,
     {
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_SIP_CANCEL, H_DISCONNECT_REMOTE},
      
      {E_CC_CONNECTED, H_RECVINVITE_EV_CC_CONNECTED},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL_UNANSWERED},
      
      {E_SIP_INV_LOCALEXPIRES_TIMER, H_HANDLE_LOCALEXPIRES_TIMER},
      
      {E_SIP_UPDATE, H_EARLY_EV_SIP_UPDATE},
      
      {E_SIP_UPDATE_RESPONSE, H_EARLY_EV_SIP_UPDATE_RESPONSE},
      
      {E_CC_FEATURE, H_EARLY_EV_CC_FEATURE},
      
      {E_CC_FEATURE_ACK, H_EARLY_EV_CC_FEATURE_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_SIP_2xx, H_RECVINVITE_EV_SIP_2XX},
      
      {E_SIP_FAILURE_RESPONSE, H_SENTINVITE_EV_SIP_FXX},

      
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },
    


    {SIP_STATE_RECV_INVITE_CONNECTED,
     {
      
      {E_SIP_ACK, H_RECVINVITE_EV_SIP_ACK},
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL},
      
      {E_SIP_UPDATE, H_EARLY_EV_SIP_UPDATE},
      
      {E_SIP_UPDATE_RESPONSE, H_EARLY_EV_SIP_UPDATE_RESPONSE},
      
      {E_CC_FEATURE, H_EARLY_EV_CC_FEATURE},
      
      {E_CC_FEATURE_ACK, H_EARLY_EV_CC_FEATURE_ACK},
      
      {E_SIP_INV_EXPIRES_TIMER, H_RECVINVITE_SENTOK_NO_SIP_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },

    


    {SIP_STATE_ACTIVE,
     {
      
      {E_SIP_INVITE, H_ACTIVE_EV_SIP_INVITE},
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_SIP_2xx, H_ACTIVE_2xx},
      
      {E_SIP_REFER, H_REFER_SIP_MESSAGE},
      
      {E_SIP_FAILURE_RESPONSE, H_SENTINVITE_EV_SIP_FXX},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL},
      
      {E_CC_FEATURE, H_ACTIVE_EV_CC_FEATURE},
      
      {E_CC_FEATURE_ACK, H_ACTIVE_EV_CC_FEATURE_ACK},
      
      {E_SIP_UPDATE, H_CONFIRM_EV_SIP_UPDATE},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_INFO, H_EV_CC_INFO},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },
    


    {SIP_STATE_SENT_MIDCALL_INVITE,
     {
      
      {E_SIP_INVITE, H_ACTIVE_EV_SIP_INVITE},
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_SIP_2xx, H_SENTINVITE_MIDCALL_EV_SIP_2XX},
      
      {E_SIP_FAILURE_RESPONSE, H_SENTINVITE_EV_SIP_FXX},
      
      {E_SIP_INV_EXPIRES_TIMER, H_DISCONNECT_LOCAL},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL},
      
      {E_CC_FEATURE, H_SENTINVITE_MIDCALL_EV_CC_FEATURE},
      
      {E_SIP_UPDATE, H_CONFIRM_EV_SIP_UPDATE},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_INFO, H_EV_CC_INFO},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },
    


    {SIP_STATE_RECV_MIDCALL_INVITE_CCFEATUREACK_PENDING,
     {
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_CC_RELEASE, H_DISCONNECT_MEDIA_CHANGE},
      
      {E_CC_FEATURE_ACK, H_RECVMIDCALLINVITE_CCFEATUREACKPENDING_EV_CC_FEATURE_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_FEATURE, H_DEFAULT_RECVREQ_ACK_PENDING_EV_CC_FEATURE},
      
      {E_CC_INFO, H_EV_CC_INFO},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },

    


    {SIP_STATE_RECV_MIDCALL_INVITE_SIPACK_PENDING,
     {
      
      {E_SIP_ACK, H_RECVMIDCALLINVITE_SIPACKPENDING_EV_SIP_ACK},
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_CC_RELEASE, H_DISCONNECT_LOCAL},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_FEATURE, H_DEFAULT_RECVREQ_ACK_PENDING_EV_CC_FEATURE},
      
      {E_CC_INFO, H_EV_CC_INFO},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },
    


    {SIP_STATE_RELEASE,
     {
      
      {E_SIP_BYE, H_BYE_RELEASE},
      
      {E_SIP_ACK, H_RECV_ERR_EV_SIP_ACK},
      
      {E_SIP_INVITE, H_SENTBYE_EV_SIP_INVITE},
      
      {E_SIP_1xx, H_SENTBYE_EV_SIP_1XX},
      
      {E_SIP_2xx, H_SENTBYE_EV_SIP_2XX},
      
      {E_SIP_FAILURE_RESPONSE, H_SENTBYE_EV_SIP_FXX},
      

      {E_SIP_SUPERVISION_DISCONNECT_TIMER, H_SENTBYE_SUPERVISION_DISCONNECT_TIMER},
      
      {E_CC_RELEASE_COMPLETE, H_RELEASE_COMPLETE},
      
      {E_SIP_UPDATE, H_SENTBYE_EV_SIP_INVITE},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_FEATURE, H_RELEASE_EV_CC_FEATURE},
      
      {E_CC_RELEASE, H_RELEASE_EV_RELEASE},
      
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },

    



    {SIP_STATE_BLIND_XFER_PENDING,
     {
      
      {E_SIP_BYE, H_BYE_RELEASE},
      
      {E_SIP_2xx, H_SENT_BLINDNTFY},
      
      {E_SIP_FAILURE_RESPONSE, H_SENTBYE_EV_SIP_FXX},
      
      {E_CC_FEATURE, H_BLIND_NOTIFY},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}

      }
     },

    {SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING,
     {
      
      {E_SIP_TIMER, H_DEFAULT_SIP_TIMER},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_FEATURE, H_DEFAULT_EV_CC_FEATURE},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },

    {SIP_STATE_SENT_OOD_REFER,
     {
      {E_SIP_1xx, H_OOD_REFER_RESPONSE_EV_SIP_1xx},
      {E_SIP_2xx, H_OOD_REFER_RESPONSE_EV_SIP_2xx},
      {E_SIP_3xx, H_OOD_REFER_RESPONSE_EV_SIP_fxx},
      {E_SIP_FAILURE_RESPONSE, H_OOD_REFER_RESPONSE_EV_SIP_fxx},
      
      {E_CC_INFO, H_EV_CC_INFO},
      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },

    {SIP_STATE_RECV_UPDATEMEDIA_CCFEATUREACK_PENDING,
     {
      
      {E_SIP_BYE, H_DISCONNECT_REMOTE},
      
      {E_CC_RELEASE, H_DISCONNECT_MEDIA_CHANGE},
      
      {E_CC_FEATURE_ACK, H_RECVUPDATEMEDIA_CCFEATUREACKPENDING_EV_CC_FEATURE_ACK},
      
      {E_SIP_NOTIFY, H_EV_SIP_UNSOLICITED_NOTIFY},
      
      {E_CC_FEATURE, H_DEFAULT_RECVREQ_ACK_PENDING_EV_CC_FEATURE},
      
      {E_CC_INFO, H_EV_CC_INFO},

      
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT},
      {H_INVALID_EVENT, H_DEFAULT}
      }
     },

};

static const sipSMEventActionFn_t
    gSIPHandlerTable[SIPSPI_EV_INDEX_END - SIPSPI_EV_INDEX_BASE + 1] = {

    
        ccsip_handle_idle_ev_sip_invite,

    
        ccsip_handle_idle_ev_cc_setup,

    
        ccsip_handle_sentinvite_ev_sip_1xx,

    
        ccsip_handle_sentinvite_ev_sip_2xx,

    
        ccsip_handle_sentinvite_ev_sip_fxx,

    
        ccsip_handle_disconnect_local_early,

    
        ccsip_handle_disconnect_remote,

    
        ccsip_handle_sentinviteconnected_ev_cc_connected_ack,

    
        ccsip_handle_disconnect_local,

    
        ccsip_handle_recvinvite_ev_cc_setup_ack,

    
        ccsip_handle_recvinvite_ev_cc_proceeding,

    
        ccsip_handle_recvinvite_ev_cc_alerting,

    
        ccsip_handle_recvinvite_ev_cc_connected,

    
        ccsip_handle_disconnect_local_unanswered,

    
        ccsip_handle_recvinvite_ev_sip_ack,

    
        ccsip_handle_active_ev_sip_invite,

    
        ccsip_handle_active_ev_cc_feature,

    
        ccsip_handle_accept_2xx,

    
        ccsip_handle_refer_sip_message,

    
        ccsip_handle_active_ev_cc_feature_ack,

    
        ccsip_handle_sentinvite_midcall_ev_sip_2xx,

    
        ccsip_handle_sentinvite_midcall_ev_cc_feature,

    
        ccsip_handle_recvmidcallinvite_ccfeatureackpending_ev_cc_feature_ack,

    
        ccsip_handle_recvmidcallinvite_sipackpending_ev_sip_ack,

    
        ccsip_handle_default_sip_message,

    
        ccsip_handle_default_sip_response,

    
        ccsip_handle_default,

    
        ccsip_handle_disconnect_local_early,

    
        ccsip_handle_process_in_call_options_request,

    
        ccsip_handle_sentinvite_ev_sip_3xx,

    
        ccsip_handle_recv_error_response_ev_sip_ack,

    
        ccsip_handle_sentbye_ev_sip_2xx,

    
        ccsip_handle_sentbye_ev_sip_1xx,

    
        ccsip_handle_sentbye_ev_sip_fxx,

    
        ccsip_handle_sentbye_recvd_invite,

    
        ccsip_handle_sendbye_ev_supervision_disconnect,

    
        ccsip_handle_release_complete,

    
        ccsip_handle_active_2xx,

    
        ccsip_handle_send_blind_notify,

    
        ccsip_handle_sentblindntfy_ev_sip_2xx,

    
        ccsip_handle_release_ev_sip_bye,

    
        ccsip_handle_localexpires_timer,

    
        ccsip_handle_default_sip_timer,

    
        ccsip_handle_early_ev_sip_update,

    
        ccsip_handle_early_ev_sip_update_response,

    
        ccsip_handle_early_ev_cc_feature,

    
        ccsip_handle_early_ev_cc_feature_ack,

    
        ccsip_handle_active_ev_sip_update,

    
        ccsip_handle_recvupdatemedia_ccfeatureackpending_ev_cc_feature_ack,

    
        ccsip_handle_timer_glare_avoidance,

    
        ccsip_handle_recvinvite_ev_expires_timer,

    
        ccsip_handle_unsolicited_notify,

    
        ccsip_handle_recvinvite_ev_sip_2xx,

    
        ccsip_handle_icmp_unreachable,

    
        ccsip_handle_disconnect_media_change,

    
        ccsip_handle_default_ev_cc_feature,

    
        ccsip_handle_default_recvreq_ack_pending_ev_cc_feature,

    
        ccsip_handle_sent_ood_refer_ev_sip_1xx,

    
        ccsip_handle_sent_ood_refer_ev_sip_2xx,

    
        ccsip_handle_sent_ood_refer_ev_sip_fxx,

    
        ccsip_handle_release_ev_cc_feature,

    
        ccsip_handle_ev_cc_info,

    
        ccsip_handle_release_ev_release,
};

static uint32_t get_callref(const char *tag) {
    int32_t i, callref = 0;
    const char * ref;

    for ( i = (strlen(tag)-1); i >= 0; i--) {
        ref = &tag[i];
        if ( *ref == '-' ) {
           sscanf ( ref, "-%d", &callref);
           break;
        }
    }
    return callref;
}

sipSMAction_t
get_handler_index (sipSMStateType_t isipsmstate, sipSMEventType_t isipsmevent)
{
    int16_t i;

    if ((isipsmstate < SIP_STATE_BASE) || (isipsmstate > SIP_STATE_END) ||
        (isipsmevent < SIPSPI_EV_BASE) || (isipsmevent > SIPSPI_EV_END)) {
        buginf("\nvalue of event passed isipsmevent=%d value of state = %d, "
               "SIP_STATE_BASE = %d, SIP_STATE_END = %d, SIPSPI_EV_BASE = %d,"
               " SIPSPI_EV_END = %d",
               isipsmstate, isipsmevent, SIP_STATE_BASE, SIP_STATE_END,
               SIPSPI_EV_BASE, SIPSPI_EV_END);

        return H_INVALID_EVENT;
    }

    for (i = 0; i < MAX_STATE_EVENTS; i++) {
        if (g_sip_table[isipsmstate].validEvent[i].sipValidEvent == (int16_t)isipsmevent) {
            return ((sipSMAction_t) g_sip_table[isipsmstate].validEvent[i].actionIndex);
        }
    }
    switch (isipsmevent) {
    case E_SIP_INVITE:
    case E_SIP_UPDATE:
    case E_SIP_ACK:
    case E_SIP_BYE:
    case E_SIP_REFER:
    case E_SIP_CANCEL:
        return H_DEFAULT_SIP_MESSAGE;
    case E_SIP_1xx:
    case E_SIP_2xx:
    case E_SIP_3xx:
    case E_SIP_FAILURE_RESPONSE:
        return H_DEFAULT_SIP_RESPONSE;
    case E_SIP_TIMER:
        return H_DEFAULT_SIP_TIMER;

    case E_SIP_OPTIONS:
        return H_SIP_OPTIONS;

    case E_SIP_GLARE_AVOIDANCE_TIMER:
        return H_SIP_GLARE_AVOIDANCE_TIMER;

    case E_SIP_ICMP_UNREACHABLE:
        return H_ICMP_UNREACHABLE;

    default:
        break;
    }
    return H_DEFAULT;
}


void
sip_sm_change_state (ccsipCCB_t *ccb, sipSMStateType_t new_state)
{
    CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Change state %s -> %s\n",
            DEB_L_C_F_PREFIX_ARGS(SIP_STATE, ccb->dn_line, ccb->gsm_id, "sip_sm_change_state"),
            sip_util_state2string(ccb->state),
            sip_util_state2string(new_state));

    if (ccb->state == SIP_STATE_RELEASE &&
        new_state == SIP_STATE_IDLE) {
        
        DEF_DEBUG("===================================================\n");
    }

    



    if (ccb->state == SIP_STATE_RELEASE) {
        (void) sip_platform_supervision_disconnect_timer_stop(ccb->index);
    }

    ccb->state = new_state;

    if (ccb->state == SIP_STATE_RELEASE) {
        (void) sip_platform_supervision_disconnect_timer_start(SUPERVISION_DISCONNECT_TIMEOUT,
                                                               ccb->index);
    }
}


















static sipsdp_status_t
sip_util_extract_sdp (ccsipCCB_t *ccb, sipMessage_t *message)
{
    const char *fname = "sip_util_extract_sdp";
    
    uint8_t         content_type = 0;
    int             content_length = 0;
    cc_sdp_t       *sip_msg_sdp = NULL;
    u16             i = 0;
    sipsdp_status_t retval = SIP_SDP_NOT_PRESENT;
    char           *sdp_data = NULL; 
    uint16_t        num_m_lines;

    


    
    
    if (message->num_body_parts == 0) {
        content_length = 0;
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"\nmultipart/mixed No SDP Found!\n", DEB_F_PREFIX_ARGS(SIP_SDP, fname));
    } else {
        for (i = 0; i < message->num_body_parts; i++) {
            if (message->mesg_body[i].msgContentTypeValue ==
                SIP_CONTENT_TYPE_SDP_VALUE) {
                content_type = SIP_CONTENT_TYPE_SDP_VALUE;
                content_length = message->mesg_body[i].msgLength;
                sdp_data = message->mesg_body[i].msgBody;
                break;
            }
        }
    }

    


    if ((content_type != SIP_CONTENT_TYPE_SDP_VALUE) ||
        (content_length <= 0)) {
        
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY), ccb->index,
                          ccb->dn_line, fname, "No SDP");
        return (SIP_SDP_NOT_PRESENT);
    }

    





    sipsdp_src_dest_create("", CCSIP_DEST_SDP_BIT, &sip_msg_sdp);
    if ((sip_msg_sdp == NULL) || (sip_msg_sdp->dest_sdp == NULL)) {
        
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SDP_CREATE_BUF_ERROR),
                          fname);
        return (SIP_SDP_ERROR);
    }
    


    if (sdp_parse(sip_msg_sdp->dest_sdp, &sdp_data,
                  (uint16_t)content_length) != SDP_SUCCESS) {
        
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_PARSE_SDP_ERROR), fname);

        sipsdp_src_dest_free(CCSIP_DEST_SDP_BIT, &sip_msg_sdp);
        return (SIP_SDP_ERROR);
    }

    


    num_m_lines = sdp_get_num_media_lines(sip_msg_sdp->dest_sdp);
    if (num_m_lines == 0) {
        
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                          ccb->index, ccb->dn_line, fname,
                          "Process SDP, no media");
        retval =  SIP_SDP_NO_MEDIA;
    } else {
        
        retval = SIP_SDP_SUCCESS;

        





        if (sip_msg_sdp->dest_sdp) {
            const char *new_session_id = NULL;
            const char *new_version_id = NULL;

            new_session_id = sdp_get_owner_sessionid(sip_msg_sdp->dest_sdp);
            new_version_id = sdp_get_owner_version(sip_msg_sdp->dest_sdp);

            if ((ccb->old_session_id) && (ccb->old_version_id) &&
                (new_session_id) && (new_version_id)) {
                if ((!(strcmp(ccb->old_session_id, new_session_id))) &&
                    (!(strcmp(ccb->old_version_id, new_version_id)))) {
                     retval = SIP_SDP_SESSION_AUDIT;
                }
            }

            if (ccb->old_session_id != NULL) {
                cpr_free(ccb->old_session_id);
                ccb->old_session_id = NULL;
            }
            if (ccb->old_version_id != NULL) {
                cpr_free(ccb->old_version_id);
                ccb->old_version_id = NULL;
            }
            if ((new_session_id) && (new_version_id)) {
                ccb->old_session_id = cpr_strdup(new_session_id);
                ccb->old_version_id = cpr_strdup(new_version_id);
            }
        }
    }
    
    sipsdp_src_dest_free(CCSIP_DEST_SDP_BIT, &sip_msg_sdp);
    return (retval);
}















static void
ccsip_save_local_msg_body (ccsipCCB_t *ccb, cc_msgbody_info_t *msg_body)
{
    if ((msg_body == NULL) || (msg_body->num_parts == 0)) {
        return;
    }

    



    cc_mv_msg_body_parts(&ccb->local_msg_body, msg_body);
}















void
sip_redirect (ccsipCCB_t *ccb, sipMessage_t *response, uint16_t status_code)
{
    const char     *fname = "sip_redirect";
    sipRedirectInfo_t *redirect_info;
    const char     *contact;
    sipLocation_t  *sipLocation;
    sipUrl_t       *pContactURL = NULL;
    char           *diversion[MAX_DIVERSION_HEADERS];
    char           *str_temp = NULL;
    char            ui_str[STATUS_LINE_MAX_LEN];
    int             i = 0;
    boolean         sent_invite = FALSE;
    cpr_ip_addr_t   cc_remote_ipaddr;
    uint16_t        diversion_count, cc_diversion_count, max_headers;
    size_t          to_length;
    char            tmp_str[STATUS_LINE_MAX_LEN];
    char            stored_to_header[MAX_SIP_URL_LENGTH];
    char           *temp = NULL;


    CPR_IP_ADDR_INIT(cc_remote_ipaddr);

    while (!sent_invite) {
        if (ccb->redirect_info == NULL) {

            
            redirect_info = (sipRedirectInfo_t *)
                cpr_malloc(sizeof(sipRedirectInfo_t));
            if (redirect_info == NULL) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                                  ccb->dn_line, fname, "malloc(redirect_info)");
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
                return;
            }

            redirect_info->sipContact = NULL;
            redirect_info->next_choice = 0;
            ccb->redirect_info = redirect_info;
        } else {
            redirect_info = ccb->redirect_info;
        }

        





        if (ccb->record_route_info) {
            sippmh_free_record_route(ccb->record_route_info);
            ccb->record_route_info = NULL;
        }

        if (redirect_info->next_choice >= SIP_MAX_LOCATIONS) {
            
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "Exhausted all Contacts");
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        }


        if ((status_code >= SIP_RED_MULT_CHOICES &&
             status_code <= SIP_RED_USE_PROXY) &&
            (ccb->first_pass_3xx == TRUE)) {
            

            




            ccb->first_pass_3xx = FALSE;

            if ((platGetPhraseText(STR_INDEX_CALL_REDIRECTED,
                                          (char *)tmp_str,
                                          STATUS_LINE_MAX_LEN - 1)) == CPR_SUCCESS) {
                memset(ui_str, 0, sizeof(ui_str));
                snprintf(ui_str, sizeof(ui_str), "%s (in %d)",
                         tmp_str, status_code);
                ui_set_call_status(ui_str, ccb->dn_line, ccb->gsm_id);
            }

            if (redirect_info->sipContact) {
                
                sippmh_free_contact(redirect_info->sipContact);
                redirect_info->sipContact = NULL;
            }

            contact = sippmh_get_cached_header_val(response, CONTACT);
            redirect_info->sipContact = sippmh_parse_contact(contact);
            if ((redirect_info->sipContact == NULL)
                || (contact == NULL)) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                  ccb->index, ccb->dn_line, fname,
                                  "sippmh_parse_contact()");
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
                return;
            }
            






            sipLocation = redirect_info->sipContact->locations[0];
            redirect_info->next_choice = 1;

            
            
            
            ccb->sip_referTo = strlib_update(ccb->sip_referTo, contact);

        } else {
            

            if ((redirect_info->sipContact == NULL) ||
                (redirect_info->next_choice >=
                 redirect_info->sipContact->num_locations)) {
                
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                  ccb->index, ccb->dn_line, fname,
                                  "Exhausted all Contacts");
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
                return;
            }

            sipLocation = redirect_info->sipContact->locations
                              [redirect_info->next_choice++];
        }

        


        for (i = 0; i < MAX_DIVERSION_HEADERS; i++) {
            diversion[i] = NULL;
        }
        









        diversion_count = sippmh_get_num_particular_headers(response,
                                                            SIP_HEADER_DIVERSION,
                                                            SIP_HEADER_DIVERSION,
                                                            diversion, MAX_DIVERSION_HEADERS);
        max_headers = MAX_DIVERSION_HEADERS - diversion_count;
        cc_diversion_count = sippmh_get_num_particular_headers(response,
                                                               SIP_HEADER_CC_DIVERSION,
                                                               SIP_HEADER_CC_DIVERSION,
                                                               &diversion[diversion_count],
                                                               max_headers);
        max_headers = MAX_DIVERSION_HEADERS - diversion_count - cc_diversion_count;
        (void) sippmh_get_num_particular_headers(response,
                                                 SIP_HEADER_CC_REDIRECT,
                                                 SIP_HEADER_CC_REDIRECT,
                                                 &diversion[diversion_count + cc_diversion_count],
                                                 max_headers);
        for (i = 0; i < MAX_DIVERSION_HEADERS; i++) {
            if (diversion[i]) {
                int len = 0;

                len = strlen(diversion[i]);
                if (ccb->diversion[i]) {
                    cpr_free(ccb->diversion[i]);
                    ccb->diversion[i] = NULL;
                }
                ccb->diversion[i] = (char *) cpr_malloc(len + 2);
                if (ccb->diversion[i]) {
                    sstrncpy(ccb->diversion[i], diversion[i], len + 1);
                } else {
                    CCSIP_DEBUG_ERROR(DEB_L_C_F_PREFIX"No memory left;"
                                      "Ignoring CC-Diversion header #%d %d\n",
                                      ccb->dn_line, ccb->gsm_id, fname,
                                      ccb->index, i + 1);
                }
            }
        }

        


        while ((sipLocation) &&
               (redirect_info->next_choice < SIP_MAX_LOCATIONS) &&
               (sipLocation->genUrl->schema != URL_TYPE_SIP)) {
            sipLocation = redirect_info->sipContact->locations
                              [redirect_info->next_choice++];
        }

        if (!sipLocation) {
            
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "Exhausted all Contacts");
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        }


        pContactURL = sipLocation->genUrl->u.sipUrl;

        




        ccb->dest_sip_port = pContactURL->port;

        if (!pContactURL->port_present) {
            
            dns_error_code = sipTransportGetServerAddrPort(pContactURL->host,
                                                           &cc_remote_ipaddr,
                                                           (uint16_t *) &ccb->dest_sip_port,
                                                           NULL, FALSE);
        } else {
            dns_error_code = dnsGetHostByName(pContactURL->host, &cc_remote_ipaddr, 100, 1);
        }
        if (dns_error_code == 0) {
            util_ntohl(&(ccb->dest_sip_addr), &cc_remote_ipaddr);
        } else {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipTransportGetServerAddrPort or dnsGetHostByName()");

            



            if (redirect_info->next_choice < SIP_MAX_LOCATIONS) {
                continue;
            } else {
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            }
            return;
        }

        



        clean_method_request_trx(ccb, sipMethodInvite, TRUE);

        



        temp = strstr(ccb->sip_to, ";tag");
        to_length = (temp != NULL) ? (strlen(ccb->sip_to) - strlen(temp)) : MAX_SIP_URL_LENGTH;
        if (to_length < MAX_SIP_URL_LENGTH) {
            sstrncpy(stored_to_header, ccb->sip_to, to_length + 1);
        } else {
            sstrncpy(stored_to_header, ccb->sip_to, MAX_SIP_URL_LENGTH);
        }

        str_temp = strlib_open(ccb->sip_to, MAX_SIP_URL_LENGTH);
        snprintf(str_temp, MAX_SIP_URL_LENGTH, "%s", stored_to_header);
        ccb->sip_to = strlib_close(str_temp);

        if (pContactURL->maddr) {
            if (pContactURL->user) {
                snprintf(ccb->ReqURI, MAX_SIP_URL_LENGTH,
                         "sip:%s%s%s@%s:%d;maddr=%s%s",
                         pContactURL->user,
                         (pContactURL->password ? ":" : ""),
                         (pContactURL->password ? pContactURL->password : ""),
                         pContactURL->host,
                         ccb->dest_sip_port,
                         pContactURL->maddr,
                         (pContactURL->is_phone ? ";user=phone" : ""));
            } else {
                snprintf(ccb->ReqURI, MAX_SIP_URL_LENGTH,
                         pContactURL->is_phone ? "sip:%s:%d;maddr=%s;user=phone"
                         : "sip:%s:%d;maddr=%s",
                         pContactURL->host, ccb->dest_sip_port,
                         pContactURL->maddr);
            }
        } else {
            if (pContactURL->user) {
                snprintf(ccb->ReqURI, MAX_SIP_URL_LENGTH,
                         "sip:%s%s%s@%s:%d%s",
                         pContactURL->user,
                         (pContactURL->password ? ":" : ""),
                         (pContactURL->password ? pContactURL->password : ""),
                         pContactURL->host,
                         ccb->dest_sip_port,
                         (pContactURL->is_phone ? ";user=phone" : ""));
            } else {
                snprintf(ccb->ReqURI, MAX_SIP_URL_LENGTH,
                         pContactURL->is_phone ? "sip:%s:%d;user=phone" : "sip:%s:%d",
                         pContactURL->host, ccb->dest_sip_port);
            }
        }


        ccb->ReqURIOriginal = strlib_update(ccb->ReqURIOriginal, ccb->ReqURI);

        ccb->sip_to_tag = strlib_update(ccb->sip_to_tag, ""); 
        ccb->authen.cred_type = 0;

        ccb->last_recv_request_cseq        = 0;
        ccb->last_recv_request_cseq_method = sipMethodInvalid;

        


        if (sipSPISendInvite(ccb, SIP_INVITE_TYPE_REDIRECTED, FALSE) == TRUE) {
            sent_invite = TRUE;
        } else {
            status_code = SIP_1XX_TRYING;
        }
    }
    return;

}

















void
parseAlertingHeader (ccsipCCB_t *ccb, const char *header)
{
    char alertHeader[MAX_SIP_URL_LENGTH];
    char *pAlertHeader;
    char *input = NULL;
    unsigned int counter = 0;

    





    sstrncpy(alertHeader, header, MAX_SIP_URL_LENGTH);
    pAlertHeader = alertHeader;

    



    if (*pAlertHeader == '<') {
        pAlertHeader++;

        
        while (isspace((int)*pAlertHeader)) {
            pAlertHeader++;
        }

        
        input = pAlertHeader;
        pAlertHeader = strchr(input, '>');
        if (pAlertHeader != NULL) {
            



            *pAlertHeader = NUL;

            






            for (counter = 0; counter <= VCM_BELLCORE_MAX - VCM_RING_OFFSET; counter++) {
                if (strstr(input, ring_names[counter]) != NULL) {
                    ccb->alert_info = ALERTING_RING;
                    ccb->alerting_ring = (vcm_ring_mode_t) (counter +
                                                            VCM_RING_OFFSET);
                    return;
                }
            }

            



            for (counter = 0; counter < VCM_MAX_DIALTONE; counter++) {
                if (strstr(input, tone_names[counter]) != NULL) {
                    ccb->alert_info = ALERTING_TONE;
                    ccb->alerting_tone = (vcm_tones_t) counter;
                    return;
                }
            }
        }
    }

    




    ccb->alert_info = ALERTING_OLD;
    ccb->alerting_ring = VCM_INSIDE_RING;
}













boolean
ccsip_is_special_name_to_mask_display_number (const char *name)
{
    const char *special_string;

    if (name == NULL) {
        
        return (FALSE);
    }
    




    special_string = platform_get_phrase_index_str(UI_CONFERENCE);
    if ((cpr_strncasecmp(name, special_string, strlen(special_string)) == 0) ||
        (cpr_strncasecmp(name, CONFERENCE_STR, CONFERENCE_STR_LEN) == 0)) {
        return (TRUE);
    }

    



    if (cpr_strncasecmp(name, INDEX_STR_BARGE, 2) == 0) {
        return (TRUE);
    }

    


    if (cpr_strncasecmp(name, INDEX_STR_PRIVATE, 2) == 0) {
        return (TRUE);
    }

    


    if (cpr_strncasecmp(name, INDEX_STR_MONITORING, 2) == 0) {
        return (TRUE);
    }

    


    if (cpr_strncasecmp(name, INDEX_STR_COACHING, 2) == 0) {
        return (TRUE);
    }

    return (FALSE);
}























static boolean
unescape_UserInfo (const char *esc_str, char *unesc_str,
                   unsigned int unesc_str_len)
{
    char *user_info;

    
    user_info = strpbrk(esc_str, "%");

    if (user_info) {
        if (unesc_str_len > strlen(esc_str)) {
            unesc_str_len = strlen(esc_str);
        }
        sippmh_convertEscCharToChar(esc_str, unesc_str_len, unesc_str);
        return TRUE;
    }

    return (FALSE);
}















static boolean
ccsip_identify_best_rpid (ccsipCCB_t *ccb, boolean request)
{
    unsigned int j;
    sipRemotePartyId_t *rpid;

    ccb->best_rpid = ccb->rpid_info->rpid[0];

    if (!ccb->best_rpid) {
        return FALSE;
    }

    for (j = 0; j < ccb->rpid_info->num_rpid; j++) {
        rpid = ccb->rpid_info->rpid[j];
        if (rpid &&
            
            
            
            rpid->loc->genUrl->schema == URL_TYPE_SIP) {
            
            



            
            
            
            
            
            
            ccb->best_rpid = rpid;
            return TRUE;
            
            
        }
    }
    return FALSE;
}

static void
ccsip_phrase_specifier (int16_t phrase, string_t * string, uint16_t len)
{
    char           *temp_str;
    char            tmp_str[STATUS_LINE_MAX_LEN];

    temp_str = strlib_open(*string, len);
    if (temp_str) {
        if (phrase == STR_INDEX_ANONYMOUS_SPACE) {
            if ((platGetPhraseText(STR_INDEX_ANONYMOUS_SPACE,
                                          (char *)tmp_str,
                                          STATUS_LINE_MAX_LEN - 1)) == CPR_SUCCESS) {
                sstrncpy(temp_str, tmp_str, len);
            }
        } else {
            sstrncpy(temp_str, platform_get_phrase_index_str(phrase), len);
        }
    }
    *string = strlib_close(temp_str);
}































static boolean
ccsip_check_set_privacy_screen (string_t *name, string_t *number,
                                char *input_name, char *input_num,
                                char *privacy, char *screen,
                                boolean connected_party)
{
    char *name_str;
    boolean number_private = FALSE;

    
    *name = strlib_update(*name, "");

    *number = strlib_update(*number, "");

    if (!privacy || cpr_strncasecmp(privacy, PRIVACY_FULL, sizeof(PRIVACY_FULL)) == 0) {
        


        ccsip_phrase_specifier(UI_PRIVATE, name, MAX_SIP_DISPLAYNAME_LENGTH);
        if (connected_party) {
            


            ccsip_phrase_specifier(STR_INDEX_ANONYMOUS_SPACE, number,
                                   MAX_SIP_URL_LENGTH * 2);
        }
        return TRUE;
    }

    if (cpr_strncasecmp(privacy, PRIVACY_URI, sizeof(PRIVACY_URI)) == 0) {

        if ((input_name != NULL) && (*input_name != NUL)) {
            *name = strlib_update(*name, input_name);
        } else {
            ccsip_phrase_specifier(UI_UNKNOWN, name, MAX_SIP_DISPLAYNAME_LENGTH);
        }
        if (connected_party) {
            


            ccsip_phrase_specifier(STR_INDEX_ANONYMOUS_SPACE, number,
                                   MAX_SIP_URL_LENGTH * 2);
        }
        number_private = TRUE;
    } else if (cpr_strncasecmp(privacy, PRIVACY_NAME, sizeof(PRIVACY_NAME)) == 0) {

        if (input_num) {
            *number = strlib_update(*number, input_num);
        }
        ccsip_phrase_specifier(UI_PRIVATE, name, MAX_SIP_DISPLAYNAME_LENGTH);

    } else {

        if ((input_name != NULL) && (*input_name != NUL)) {
            *name = strlib_update(*name, input_name);
        } else {
            if (!input_num || (*input_num == NUL)) {
                ccsip_phrase_specifier(UI_UNKNOWN, name, MAX_SIP_DISPLAYNAME_LENGTH);
            }
        }

        if (input_num) {
            *number = strlib_update(*number, input_num);
        }
    }

    name_str = strlib_open(*name, MAX_SIP_DISPLAYNAME_LENGTH);
    if (name_str) {
        sip_sm_dequote_string(name_str, MAX_SIP_DISPLAYNAME_LENGTH);
    }
    *name = strlib_close(name_str);
    return number_private;
}















static boolean
ccsip_parse_diversion_header (ccsipCCB_t *ccb, sipMessage_t *msg)
{
    char           *diversion_headers[MAX_DIVERSION_HEADERS];
    unsigned int    diversion_count;
    sipDiversion_t *diversion_header;

    


    sippmh_free_diversion_info(ccb->div_info);

    ccb->div_info = (sipDiversionInfo_t *)
        cpr_malloc(sizeof(sipDiversionInfo_t));

    if (!ccb->div_info) {
        return FALSE;
    }

    memset(ccb->div_info, 0, sizeof(sipDiversionInfo_t));
    memset(diversion_headers, 0, MAX_DIVERSION_HEADERS * sizeof(char *));

    ccb->div_info->last_redirect_name = strlib_empty();
    ccb->div_info->last_redirect_number = strlib_empty();
    ccb->div_info->orig_called_name = strlib_empty();
    ccb->div_info->orig_called_number = strlib_empty();


    diversion_count = sippmh_get_num_particular_headers(msg,
                                                        SIP_HEADER_DIVERSION,
                                                        SIP_HEADER_DIVERSION,
                                                        diversion_headers,
                                                        MAX_DIVERSION_HEADERS);

    if (diversion_count < 1) {
        return FALSE;
    }

    ccb->call_type = CC_CALL_FORWARDED;

    
    diversion_header = sippmh_parse_diversion(diversion_headers[0], SIP_HEADER_DIVERSION);

    if (diversion_header) {
        (void) ccsip_check_set_privacy_screen(&(ccb->div_info->last_redirect_name),
                                              &(ccb->div_info->last_redirect_number),
                                              diversion_header->locations->name,
                                              diversion_header->locations->genUrl->u.sipUrl->user,
                                              diversion_header->privacy,
                                              diversion_header->screen, FALSE);

        sippmh_free_diversion(diversion_header);
    }

    
    diversion_header = sippmh_parse_diversion(diversion_headers[diversion_count - 1],
                                              SIP_HEADER_DIVERSION);

    if (diversion_header) {
        (void) ccsip_check_set_privacy_screen(&(ccb->div_info->orig_called_name),
                                              &(ccb->div_info->orig_called_number),
                                              diversion_header->locations->name,
                                              diversion_header->locations->genUrl->u.sipUrl->user,
                                              diversion_header->privacy,
                                              diversion_header->screen, FALSE);

        sippmh_free_diversion(diversion_header);
    }

    return TRUE;
}













static boolean
ccsip_parse_rpid (ccsipCCB_t *ccb, sipMessage_t *msg)
{
    char           *rpid_headers[MAX_REMOTE_PARTY_ID_HEADERS];
    unsigned int    rpid_line_count;
    const char     *rpid_str = NULL;
    unsigned int    j;

    



    sippmh_free_remote_party_id_info(ccb->rpid_info);
    ccb->best_rpid = NULL;
    ccb->rpid_info = (sipRemotePartyIdInfo_t *)
        cpr_malloc(sizeof(sipRemotePartyIdInfo_t));
    if (!ccb->rpid_info) {
        return FALSE;
    }

    
    memset(ccb->rpid_info, 0, sizeof(sipRemotePartyIdInfo_t));
    memset(rpid_headers, 0, MAX_REMOTE_PARTY_ID_HEADERS * sizeof(char *));

    rpid_line_count = sippmh_get_num_particular_headers(msg,
                                                        SIP_HEADER_REMOTE_PARTY_ID,
                                                        SIP_HEADER_REMOTE_PARTY_ID,
                                                        rpid_headers,
                                                        MAX_REMOTE_PARTY_ID_HEADERS);

    if (rpid_line_count < 1) {
        return FALSE;
    }

    for (j = 0; (j < rpid_line_count) && (j < MAX_REMOTE_PARTY_ID_HEADERS); j++) {
        rpid_str = rpid_headers[j];
        if ((rpid_str) && (rpid_str[0])) {
            ccb->rpid_info->rpid[j] = sippmh_parse_remote_party_id(rpid_str);
        }
    }
    ccb->rpid_info->num_rpid = rpid_line_count;
    return TRUE;
}

























static string_t ccsip_set_url_domain (char *host, string_t callingNumber, string_t calledNumber, line_t line)
{
    char       *target;
    char        addr_error;
    uint32_t    address;
    char        buffer[MAX_SIP_URL_LENGTH];
    boolean     include_domain = FALSE;

    if (host == NULL) {
        return callingNumber;
    }

    


    address = IPNameCk(host, &addr_error);
    if (!address) {
        




        target = cpr_strdup(host);
        if (target != NULL) {
            if (sipSPI_validate_hostname(target)) {
                



                if (line == 0) {
                    



                    line =  sip_config_get_line_by_called_number(1, calledNumber);
                }

                if (line == 0) {
                    



                     include_domain = TRUE;
                } else {
                    buffer[0] = '\0';
                    config_get_line_string(CFGID_PROXY_ADDRESS, buffer, line, MAX_SIP_URL_LENGTH);
                    address = IPNameCk(buffer, &addr_error);
                    if (address) {
                        
                        include_domain = TRUE;
                    } else {
                        if (strncmp(host, buffer, MAX_SIP_URL_LENGTH) != 0) {
                            


                            include_domain = TRUE;
                        }
                    }
                }
                if (include_domain == TRUE) {
                    callingNumber = strlib_append(callingNumber, "@");
                    callingNumber = strlib_append(callingNumber, host);
                }
            }
            cpr_free(target);
        }
    }
    return callingNumber;
}














void ccsip_set_alt_callback_number(ccsipCCB_t *ccb)
{
  int param_idx=0;
  char *ptr;

	while ( (ptr = ccb->best_rpid->loc->genUrl->other_params[param_idx++]) != NULL )
	{
		if ( ! strncasecmp ( ptr, RPID_CALLBACK, RPID_CALLBACK_LEN)) {
			ccb->altCallingNumber =
				strlib_update(ccb->altCallingNumber, ptr + RPID_CALLBACK_LEN);
			return;
		}
	}

        ccb->altCallingNumber = strlib_update(ccb->altCallingNumber, "");
}


















static boolean
ccsip_set_caller_id_from_rpid (ccsipCCB_t *ccb, boolean request, boolean update_ccb, boolean *display_enabled)
{
    int         rpid_flag = RPID_DISABLED;
    char       *sip_rpid_user = NULL;
    char       *pUser = NULL;
    string_t   *name;
    string_t   *number;
    boolean     display_number = TRUE;
    boolean     private_num = FALSE;
    line_t      line = 0;


    *display_enabled = TRUE;

    
    config_get_value(CFGID_REMOTE_PARTY_ID, &rpid_flag, sizeof(rpid_flag));
    if (rpid_flag == RPID_DISABLED) {
        return FALSE;
    }

    
    if (!ccsip_identify_best_rpid(ccb, request)) {
        return FALSE;
    }

    if (ccb->flags & INCOMING) {
        name = &ccb->callingDisplayName;
        number = &ccb->callingNumber;
    } else {
        name = &ccb->calledDisplayedName;
        number = &ccb->calledNumber;
        line = ccb->dn_line;
    }

    sip_rpid_user = ccb->best_rpid->loc->genUrl->u.sipUrl->user;

    pUser = sippmh_parse_user(sip_rpid_user);
    if (pUser) {
        sip_rpid_user = pUser;
    }

    private_num = ccsip_check_set_privacy_screen(name, number,
                                                 ccb->best_rpid->loc->name,
                                                 sip_rpid_user,
                                                 ccb->best_rpid->privacy,
                                                 ccb->best_rpid->screen,
                                                 TRUE);

    




    if (private_num) {
        


        if (cpr_strncasecmp(ccb->best_rpid->privacy, PRIVACY_FULL, sizeof(PRIVACY_FULL)) == 0) {
           


           *display_enabled = display_number = FALSE;
        } else {
            


            display_number = FALSE;
        }
    }

    


    if ( (!private_num) )
    {
        ccsip_set_alt_callback_number(ccb);
    }

      



    if (sip_regmgr_get_cc_mode(1) == REG_MODE_NON_CCM) {
        *number = ccsip_set_url_domain(ccb->best_rpid->loc->genUrl->u.sipUrl->host, *number,
                                       ccb->calledNumber, line);
    }

    if (pUser) {
        cpr_free(pUser);
    }


    if (update_ccb) {
        if (ccb->flags & INCOMING) {
            ccb->displayCallingNumber = display_number;
        } else {
            ccb->displayCalledNumber = display_number;
        }
    }

    return TRUE;
}












static void
ccsip_send_callinfo (ccsipCCB_t *ccb, boolean update_caller_id,
                     boolean delay_update)
{
    cc_feature_data_t data;
    char            unescape_str_temp[MAX_SIP_URL_LENGTH];
    const char     *name;
    const char     *number;
    const char     *altNumber = strlib_empty();
    boolean         display_number;

    if (!ccb->in_call_info) {
        data.call_info.feature_flag = 0;
        data.call_info.security = CC_SECURITY_UNKNOWN;
        data.call_info.policy = CC_POLICY_UNKNOWN;
        if (ccb->flags & INCOMING) {
            data.call_info.orientation = CC_ORIENTATION_FROM;
        } else {
            data.call_info.orientation = CC_ORIENTATION_TO;
        }
        data.call_info.ui_state = CC_UI_STATE_NONE;
        data.call_info.dusting = FALSE;
        data.call_info.global_call_id[0] = 0;
    } else {
        cc_feature_data_call_info_t *feat_data = &ccb->in_call_info->data.call_info_feat_data;

        data.call_info.feature_flag = feat_data->feature_flag;
        data.call_info.security = feat_data->security;
        data.call_info.policy = feat_data->policy;
        data.call_info.orientation = feat_data->orientation;
        data.call_info.ui_state = feat_data->ui_state;
        data.call_info.caller_id.call_instance_id = feat_data->caller_id.call_instance_id;
        data.call_info.dusting = feat_data->dusting;
        sstrncpy(data.call_info.global_call_id,
		  ccb->in_call_info->data.call_info_feat_data.global_call_id, CC_GCID_LEN);
    }

    data.call_info.caller_id.orig_rpid_number = strlib_empty();
    if (!update_caller_id) {
        data.call_info.feature_flag &= ~CC_CALLER_ID;
        data.call_info.caller_id.called_name = strlib_empty();
        data.call_info.caller_id.called_number = strlib_empty();
        data.call_info.caller_id.calling_name = strlib_empty();
        data.call_info.caller_id.calling_number = strlib_empty();
        data.call_info.caller_id.alt_calling_number = strlib_empty();
    } else {
        data.call_info.feature_flag |= CC_CALLER_ID;
        if (ccb->flags & INCOMING) {
            
            if (unescape_UserInfo(ccb->callingDisplayName, unescape_str_temp,
                    MAX_SIP_URL_LENGTH)) {
                ccb->callingDisplayName = strlib_update(ccb->callingDisplayName,
                                                        unescape_str_temp);
            }
            if (unescape_UserInfo(ccb->callingNumber, unescape_str_temp,
                    MAX_SIP_URL_LENGTH)) {
                ccb->callingNumber = strlib_update(ccb->callingNumber,
                                                   unescape_str_temp);
            }
            name = ccb->callingDisplayName;
            number = ccb->callingNumber;
	    strlib_free(altNumber);
            altNumber = ccb->altCallingNumber;
            display_number = ccb->displayCallingNumber;
        } else {
            
            if (unescape_UserInfo(ccb->calledDisplayedName, unescape_str_temp,
                    MAX_SIP_URL_LENGTH)) {
                ccb->calledDisplayedName = strlib_update(ccb->calledDisplayedName,
                                                         unescape_str_temp);
            }
            if (unescape_UserInfo(ccb->calledNumber, unescape_str_temp,
                    MAX_SIP_URL_LENGTH)) {
                ccb->calledNumber = strlib_update(ccb->calledNumber,
                                                  unescape_str_temp);
            }
            name = ccb->calledDisplayedName;
            number = ccb->calledNumber;
            display_number = ccb->displayCalledNumber;
        }

        if (data.call_info.orientation == CC_ORIENTATION_FROM) {
            data.call_info.caller_id.calling_name = name;
            data.call_info.caller_id.calling_number = number;
            data.call_info.caller_id.alt_calling_number = altNumber;
            data.call_info.caller_id.display_calling_number = display_number;

            data.call_info.caller_id.called_name = strlib_empty();
            data.call_info.caller_id.called_number = strlib_empty();
            data.call_info.caller_id.display_called_number = FALSE;
        } else {
            data.call_info.caller_id.called_name = name;
            data.call_info.caller_id.called_number = number;
            data.call_info.caller_id.display_called_number = display_number;

            data.call_info.caller_id.calling_name = strlib_empty();
            data.call_info.caller_id.calling_number = strlib_empty();
            data.call_info.caller_id.alt_calling_number = strlib_empty();
            data.call_info.caller_id.display_calling_number = FALSE;
            if(ccb->best_rpid != NULL && ccb->best_rpid->loc->genUrl->u.sipUrl->user != NULL)
                data.call_info.caller_id.orig_rpid_number = (const char *) ccb->best_rpid->loc->genUrl->u.sipUrl->user;
        }
    }

    



    if (ccb->div_info) {
        data.call_info.caller_id.last_redirect_name = ccb->div_info->last_redirect_name;
        data.call_info.caller_id.last_redirect_number = ccb->div_info->last_redirect_number;
        data.call_info.caller_id.orig_called_name = ccb->div_info->orig_called_name;
        data.call_info.caller_id.orig_called_number = ccb->div_info->orig_called_number;
    } else {
        data.call_info.caller_id.last_redirect_name = strlib_empty();
        data.call_info.caller_id.last_redirect_number = strlib_empty();
        data.call_info.caller_id.orig_called_name = strlib_empty();
        data.call_info.caller_id.orig_called_number = strlib_empty();
    }
    data.call_info.caller_id.call_type = ccb->call_type;

    
    data.call_info.feature_flag &= ~(CC_DELAY_UI_UPDATE);
    if (delay_update) {
        data.call_info.feature_flag |= CC_DELAY_UI_UPDATE;
    }
    data.call_info.callref = ccb->callref;
    sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_CALLINFO, &data);
}



















static void
ccsip_update_callinfo (ccsipCCB_t *ccb, sipMessage_t *msg, boolean check_rpid,
                       boolean request, boolean delay_update)
{
    boolean update_caller_id = FALSE;
    boolean display_number = TRUE;

    if (!msg) {
        return;
    }

    if (check_rpid) {
        if (ccsip_parse_rpid(ccb, msg)) {
            if (ccsip_set_caller_id_from_rpid(ccb, request, TRUE, &display_number)) {
                update_caller_id = TRUE;
            }
        }
    }

    ccsip_process_call_info_header(msg, ccb);

    ccsip_send_callinfo(ccb, update_caller_id, delay_update);
}















static boolean
ccsip_check_display_validity(ccsipCCB_t *ccb, sipMessage_t *request)
{
     int temp = 0;
     boolean display_number = TRUE;
     




     config_get_value(CFGID_ANONYMOUS_CALL_BLOCK, &temp, sizeof(temp));
     if (temp & 1) {
          if (ccsip_parse_rpid(ccb, request)) {
             if (ccsip_set_caller_id_from_rpid(ccb, TRUE, FALSE, &display_number)) {
                 if (!display_number) {
                     return FALSE;
                 }
             }
         }
     }
     return TRUE;
}





void
ccsip_handle_idle_ev_sip_invite (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "idle_ev_sip_invite";
    char           *callingDisplayNameTemp;
    char           *sip_to_tag_temp;
    char           *sip_to_temp;
    sipMessage_t   *request;
    const char     *from = NULL;
    const char     *to = NULL;
    const char     *callID = NULL;
    const char     *contact = NULL;
    const char     *record_route = NULL;
    const char     *expires = NULL;
    const char     *alert_info = NULL, *allow = NULL;
    const char     *require = NULL, *supported = NULL;
    char           *replaceshdr = NULL;
    sipReplaces_t  *replaces_t = NULL;
    sipLocation_t  *to_loc = NULL;
    sipLocation_t  *from_loc = NULL;
    sipUrl_t       *sipFromUrl = NULL;
    sipUrl_t       *sipToUrl = NULL;
    uint32_t        local_expires_timeout = 0;
    int             delta = 0;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    uint32_t        gmt_time;
    uint32_t        diff_time;
    int32_t         gmt_rc;
    callid_t        cc_call_id = CC_NO_CALL_ID;
    ccsipCCB_t     *refererccb = NULL;
    ccsipCCB_t     *replaces_ccb = NULL;
    line_t          dn_line;
    sipsdp_status_t sdp_status;
    boolean         check_send_487 = FALSE;
    char            unescape_str_temp[MAX_SIP_URL_LENGTH];
    boolean         display_number = FALSE;
    string_t        recv_info_list = strlib_empty();

    
    request = event->u.pSipMessage;

    
    if (sip_sm_request_check_and_store(ccb, request, sipMethodInvite, FALSE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       request_check_reason_code,
                                       request_check_reason_phrase, NULL);
        free_sip_message(request);
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    }

    ccb->flags |= INCOMING;

    
    from = sippmh_get_cached_header_val(request, FROM);
    ccb->sip_from = strlib_update(ccb->sip_from, from);
    to = sippmh_get_cached_header_val(request, TO);
    ccb->sip_to = strlib_update(ccb->sip_to, to);

    
    callID = sippmh_get_cached_header_val(request, CALLID);
    sstrncpy(ccb->sipCallID, callID, sizeof(ccb->sipCallID));

    
    require = sippmh_get_cached_header_val(request, REQUIRE);
    if (require) {
        char *unsupported_tokens = NULL;

        ccb->required_tags = sippmh_parse_supported_require(require,
                                                            &unsupported_tokens);

        if (unsupported_tokens != NULL) {
            ccb->sip_unsupported = strlib_update(ccb->sip_unsupported,
                                                 unsupported_tokens);
            cpr_free(unsupported_tokens);
        }

        if (ccb->required_tags & (~(SUPPORTED_TAGS))) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Unsupported Require Header in INVITE\n", fname);
            ccb->sip_require = strlib_update(ccb->sip_require, require);
            sipSPISendInviteResponse(ccb, SIP_CLI_ERR_EXTENSION,
                                     SIP_CLI_ERR_EXTENSION_PHRASE,
                                     0, NULL,
                                     FALSE,  TRUE );
            ccb->wait_for_ack = TRUE;
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        }
    }

    
    supported = sippmh_get_cached_header_val(request, SUPPORTED);
    if (supported) {
        ccb->supported_tags = sippmh_parse_supported_require(supported, NULL);
    }

    
    allow = sippmh_get_header_val(request, SIP_HEADER_ALLOW, NULL);
    if (allow) {
        ccb->allow_methods = sippmh_parse_allow_header(allow);
    }

    
    contact = sippmh_get_cached_header_val(request, CONTACT);
    if (contact) {
        if (ccb->contact_info) {
            sippmh_free_contact(ccb->contact_info);
        }
        ccb->contact_info = sippmh_parse_contact(contact);

        if ((ccb->contact_info == NULL) || 
            (sipSPICheckContact(contact) < 0)) { 
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "sipSPICheckContact()");
            (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                           SIP_CLI_ERR_BAD_REQ_PHRASE,
                                           SIP_WARN_MISC,
                                           SIP_CLI_ERR_BAD_REQ_CONTACT_FIELD,
                                           ccb);
            ccb->wait_for_ack = TRUE;
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        }
    }

    
    record_route = sippmh_get_cached_header_val(request, RECORD_ROUTE);
    if (record_route) {
        if (ccb->record_route_info) {
            sippmh_free_record_route(ccb->record_route_info);
        }
        ccb->record_route_info = sippmh_parse_record_route(record_route);
        if (ccb->record_route_info == NULL) {
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "sippmh_parse_record_route()");
            (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                           SIP_CLI_ERR_BAD_REQ_PHRASE,
                                           SIP_WARN_MISC,
                                           SIP_CLI_ERR_BAD_REQ_RECORD_ROUTE,
                                           ccb);
            ccb->wait_for_ack = TRUE;
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        }
    }
    
    ccb->ReqURIOriginal = strlib_update(ccb->ReqURIOriginal, ccb->ReqURI);

    
    (void) ccsip_parse_rpid(ccb, request);


    
    (void) ccsip_parse_diversion_header(ccb, request);

    


    from_loc = sippmh_parse_from_or_to((char *)ccb->sip_from, TRUE);
    if (!from_loc) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIPPMH_PARSE_FROM));
        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BAD_REQ,
                                 SIP_CLI_ERR_BAD_REQ_PHRASE,
                                 SIP_WARN_MISC,
                                 SIP_CLI_ERR_BAD_REQ_FROMURL_ERROR,
                                 FALSE,  TRUE );
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    }
    if (from_loc->tag && (strlen(from_loc->tag) >= MAX_SIP_TAG_LENGTH)) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname, "Length of From Tag");
        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BAD_REQ,
                                 SIP_CLI_ERR_BAD_REQ_PHRASE,
                                 SIP_WARN_MISC,
                                 SIP_CLI_ERR_BAD_REQ_FROMURL_ERROR,
                                 FALSE,  TRUE );
        sippmh_free_location(from_loc);
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    }

    if (from_loc->genUrl->schema == URL_TYPE_SIP) {
        sipFromUrl = from_loc->genUrl->u.sipUrl;
    }

    


    to_loc = sippmh_parse_from_or_to((char *)ccb->sip_to, TRUE);
    if (!to_loc) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIPPMH_PARSE_TO));
        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BAD_REQ,
                                 SIP_CLI_ERR_BAD_REQ_PHRASE,
                                 SIP_WARN_MISC,
                                 SIP_CLI_ERR_BAD_REQ_ToURL_ERROR,
                                 FALSE,  TRUE );
        sippmh_free_location(from_loc);
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    }

    if (to_loc->genUrl->schema == URL_TYPE_SIP) {
        sipToUrl = to_loc->genUrl->u.sipUrl;
    }
    
    if (to_loc->tag) {
        


        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          "Initial invite with to_tag");
        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_CALLEG,
                                 SIP_CLI_ERR_CALLEG_PHRASE,
                                 0, NULL,
                                 FALSE,  TRUE );
        sippmh_free_location(to_loc);
        sippmh_free_location(from_loc);
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;

    } else {
        sip_to_tag_temp = strlib_open(ccb->sip_to_tag, MAX_SIP_TAG_LENGTH);
        if (sip_to_tag_temp) {
            sip_util_make_tag(sip_to_tag_temp);
        }
        ccb->sip_to_tag = strlib_close(sip_to_tag_temp);

        sip_to_temp = strlib_open(ccb->sip_to, MAX_SIP_URL_LENGTH);
        if (sip_to_temp) {
            sstrncat(sip_to_temp, ";tag=",
                    MAX_SIP_URL_LENGTH - strlen(sip_to_temp));
            if (ccb->sip_to_tag) {
                sstrncat(sip_to_temp, ccb->sip_to_tag,
                        MAX_SIP_URL_LENGTH - strlen(sip_to_temp));
            }
        }
        ccb->sip_to = strlib_close(sip_to_temp);
    }

    if (from_loc->tag) {
        ccb->sip_from_tag = strlib_update(ccb->sip_from_tag,
                                          sip_sm_purify_tag(from_loc->tag));
        ccb->callref = get_callref(ccb->sip_from_tag);
    }

    if (ccb->ReqURI) {
        ccb->calledNumber = strlib_update(ccb->calledNumber, ccb->ReqURI);
    } else if (sipToUrl) {
        if (sipToUrl->user) {
            char *pUser = NULL;

            pUser = sippmh_parse_user(sipToUrl->user);
            if (pUser && (pUser[0] != '\0')) {
                ccb->calledNumber = strlib_update(ccb->calledNumber, pUser);
                cpr_free(pUser);
            } else {
                
                ccb->calledNumber = strlib_update(ccb->calledNumber,
                                                  sipToUrl->user);
                if (pUser)
                    cpr_free(pUser);
            }
        }
    }

    


    if ((ccb->calledNumber == NULL) || (ccb->calledNumber[0] == '\0')) {
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_NOT_FOUND,
                                           SIP_CLI_ERR_NOT_FOUND_PHRASE,
                                           0, NULL, ccb);

        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
        ccb->wait_for_ack = FALSE;
        sip_sm_change_state(ccb, SIP_STATE_IDLE);
        sip_sm_call_cleanup(ccb);
        return;
    }

    if (!ccsip_set_caller_id_from_rpid(ccb, TRUE, TRUE, &display_number)) {
        if (sipFromUrl) {
            if (sipFromUrl->user) {
                char *pUser;

                pUser = sippmh_parse_user(sipFromUrl->user);
                if (pUser) {
                    ccb->callingNumber = strlib_update(ccb->callingNumber, pUser);
                    cpr_free(pUser);
                } else {
                    
                    ccb->callingNumber = strlib_update(ccb->callingNumber, sipFromUrl->user);
                }
                if (from_loc->genUrl->schema == URL_TYPE_SIP &&
                    sipFromUrl->host &&
                    ccb->callingNumber &&
                    sip_regmgr_get_cc_mode(1) == REG_MODE_NON_CCM) {
                    




                    ccb->callingNumber = ccsip_set_url_domain(sipFromUrl->host, ccb->callingNumber,
                                                              ccb->calledNumber, 0);
                }
            } else {
                ccb->callingNumber = strlib_update(ccb->callingNumber,
                                                   "Unknown Number");
            }
        }

        if (from_loc->name) {
            callingDisplayNameTemp = strlib_open(ccb->callingDisplayName,
                                                 MAX_SIP_DISPLAYNAME_LENGTH);
            if (callingDisplayNameTemp) {
                sstrncpy(callingDisplayNameTemp, from_loc->name,
                         MAX_SIP_DISPLAYNAME_LENGTH);
                sip_sm_dequote_string(callingDisplayNameTemp, MAX_SIP_DISPLAYNAME_LENGTH);
            }
            ccb->callingDisplayName = strlib_close(callingDisplayNameTemp);

        } else {
            char *pUser;

            pUser = NULL;
            if (sipFromUrl) {
                pUser = sippmh_parse_user(sipFromUrl->user);
            }

            if (pUser) {
                ccb->callingDisplayName = strlib_update(ccb->callingDisplayName,
                                                        pUser);
                cpr_free(pUser);
            } else {
                
                if (sipFromUrl) {
                    if (sipFromUrl->user) {
                        ccb->callingDisplayName = strlib_update(ccb->callingDisplayName,
                                                                sipFromUrl->user);
                    }
                }
            }
        }
    }

    sippmh_free_location(to_loc);
    sippmh_free_location(from_loc);

    
    
    
    (void) sippmh_get_num_particular_headers(request, SIP_HEADER_REPLACES,
                                             NULL, &replaceshdr, MAX_REPLACES_HEADERS);
    ccb->wastransferred = FALSE;


    dn_line = ccb->dn_line;

    if ((Basic_is_phone_forwarded(dn_line) == NULL) && NULL != replaceshdr) {
        char tempreplace[MAX_SIP_URL_LENGTH];
        boolean is_previous_call_id = FALSE;
        line_t  previous_call_index = 0;

        memset(tempreplace, 0, MAX_SIP_URL_LENGTH);
        sstrncpy(tempreplace, "Replaces=", sizeof(tempreplace));
        sstrncat(tempreplace, replaceshdr, (sizeof(tempreplace) - sizeof("Replaces=")));
        replaces_t = sippmh_parse_replaces(tempreplace, FALSE);
        if (NULL != replaces_t) {
            
            if ((cpr_strcasecmp(replaces_t->toTag, ccb->sip_to_tag)==0
                 && cpr_strcasecmp(replaces_t->fromTag, ccb->sip_from_tag)==0
                 && cpr_strcasecmp(replaces_t->callid, ccb->sipCallID)==0) ||
                ((refererccb = sip_sm_get_ccb_by_callid(replaces_t->callid)) != NULL
                && cpr_strcasecmp(replaces_t->toTag, refererccb->sip_to_tag) == 0
                && cpr_strcasecmp(replaces_t->fromTag, refererccb->sip_from_tag) == 0)) {
                ccb->sipxfercallid = strlib_update(ccb->sipxfercallid, replaces_t->callid);
                sippmh_free_replaces(replaces_t);
                ccb->wastransferred = TRUE;
                check_send_487 = TRUE;
            } else {
                is_previous_call_id = sip_sm_is_previous_call_id(replaces_t->callid,
                                                                 &previous_call_index);
                replaces_ccb = sip_sm_get_ccb_by_callid(replaces_t->callid);
                if (replaces_ccb != NULL && replaces_ccb->state == SIP_STATE_RELEASE) {
                    is_previous_call_id = TRUE;
                    CCSIP_DEBUG_STATE("%s: Replaces Header, matching call found. \n", fname);
                }
                if (is_previous_call_id) {
                    
                    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                      ccb->index, ccb->dn_line, fname,
                                      "Replaces Header, Callid refers to a previous call");
                    sipSPISendInviteResponse(ccb, SIP_FAIL_DECLINE,
                                             SIP_FAIL_DECLINE_PHRASE,
                                             0, NULL,
                                             FALSE,  TRUE );

                } else {
                    
                    
                    
                    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                      ccb->index, ccb->dn_line, fname,
                                      "Replaces Header, No matching call found");
                    sipSPISendInviteResponse(ccb, SIP_CLI_ERR_CALLEG,
                                             SIP_CLI_ERR_CALLEG_PHRASE,
                                             0, NULL,
                                             FALSE,  TRUE );
                }
                ccb->wait_for_ack = TRUE;
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
                sippmh_free_replaces(replaces_t);
                return;
            }
        } else {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "Bad Replaces Header, Ending transferred call");
            sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BAD_REQ,
                                     SIP_CLI_ERR_BAD_REQ_PHRASE,
                                     SIP_WARN_MISC,
                                     SIP_CLI_ERR_BAD_REQ_PHRASE_REPLACES,
                                     FALSE,  TRUE );
            ccb->wait_for_ack = TRUE;
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;

        }
    }

    


    sdp_status = sip_util_extract_sdp(ccb, request);

    switch (sdp_status) {
    case SIP_SDP_SUCCESS:
    case SIP_SDP_SESSION_AUDIT:
        ccb->oa_state = OA_OFFER_RECEIVED;
        break;

    case SIP_SDP_DNS_FAIL:
        sipSPISendInviteResponse(ccb, SIP_SERV_ERR_INTERNAL,
                                 SIP_SERV_ERR_INTERNAL_PHRASE,
                                 SIP_WARN_MISC,
                                 "DNS lookup failed for media destination",
                                 FALSE, FALSE);
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;

    case SIP_SDP_ERROR:
        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BAD_REQ,
                                 SIP_CLI_ERR_BAD_REQ_PHRASE,
                                 SIP_WARN_MISC,
                                 SIP_CLI_ERR_BAD_REQ_SDP_ERROR,
                                 FALSE,  TRUE );
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;

    case SIP_SDP_NO_MEDIA:
        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BAD_REQ,
                                 SIP_CLI_ERR_BAD_REQ_PHRASE,
                                 SIP_WARN_MISC,
                                 SIP_CLI_ERR_BAD_REQ_SDP_ERROR,
                                 FALSE,  TRUE );
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;

    case SIP_SDP_NOT_PRESENT:
    default:
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Waiting for SDP in ACK\n", DEB_F_PREFIX_ARGS(SIP_SDP, fname));
        break;
    }


    
    if (Basic_is_phone_forwarded(dn_line)) {
        size_t  escaped_char_str_len = 8;
        char    pDiversionStr[MAX_SIP_URL_LENGTH];
        int     len = 0;
        int     blocking;
        boolean private_flag = FALSE;
        char    line_name[MAX_LINE_NAME_SIZE];
        char    display_name[MAX_LINE_NAME_SIZE];
        char    src_addr_str[MAX_IPADDR_STR_LEN];


        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                          ccb->index, ccb->dn_line, fname,
                          "Call forwarded, sending redirect");

        ccb->dn_line = dn_line;
        



        config_get_value(CFGID_CALLERID_BLOCKING, &blocking, sizeof(blocking));
        if ((blocking & 1) && (ccb->routeMode != RouteEmergency)) {
            private_flag = TRUE;
        }

        




        config_get_string((CFGID_LINE_NAME + ccb->dn_line - 1), line_name, sizeof(line_name));
        sip_config_get_display_name(ccb->dn_line, display_name, sizeof(display_name));
        ipaddr2dotted(src_addr_str, &ccb->src_addr);
        snprintf(pDiversionStr, MAX_SIP_URL_LENGTH, "\"%s\" <sip:", display_name);
        escaped_char_str_len += strlen(display_name);
        escaped_char_str_len += sippmh_convertURLCharToEscChar(line_name, strlen(line_name),
                                    pDiversionStr + escaped_char_str_len,
                                    MAX_SIP_URL_LENGTH - escaped_char_str_len,
                                    FALSE);
        snprintf(pDiversionStr + escaped_char_str_len,
                 MAX_SIP_URL_LENGTH - escaped_char_str_len,
                 "@%s>;reason=unconditional;privacy=%s;screen=yes",
                 src_addr_str, (private_flag ? "full" : "off"));

        len = strlen(pDiversionStr);
        ccb->diversion[0] = (char *) cpr_malloc(len + 1);

        if (ccb->diversion[0]) {
            sstrncpy(ccb->diversion[0], pDiversionStr, len + 1);
        } else {
            CCSIP_DEBUG_ERROR(DEB_L_C_F_PREFIX "No memory left; %d"
                              "Can not create CC-Diversion header for CFWDAll\n",
                              ccb->dn_line, ccb->gsm_id, fname, ccb->index);
        }
        sipSPISendInviteResponse302(ccb);
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    }

    


    expires = sippmh_get_header_val(request, SIP_HEADER_EXPIRES, NULL);
    config_get_value(CFGID_TIMER_INVITE_EXPIRES, &local_expires_timeout,
                     sizeof(local_expires_timeout));
    if (expires) {

        gmt_rc = gmt_string_to_seconds((char *)expires, (unsigned long *)&gmt_time);
        if (gmt_rc != -1) {
            
            
            
            if (gmt_rc == 1) {
                
                if (gmt_time < local_expires_timeout) {
                    local_expires_timeout = gmt_time;
                }

            } else if (diff_current_time(gmt_time, (unsigned long *) &diff_time) == 0) {
                
                if (diff_time < local_expires_timeout) {
                    local_expires_timeout = diff_time;
                }
            }
        }
    } else {
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                          ccb->index, ccb->dn_line, fname,
                          "Using config timer_expires");
    }

    if (local_expires_timeout > 0) {
        config_get_value(CFGID_TIMER_T1, &delta, sizeof(delta));
        delta = (2 * delta) / 1000;
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Starting INVITE Local Expires %d"
                          "timer (%d sec)\n",
						  DEB_L_C_F_PREFIX_ARGS(SIP_TIMER, ccb->dn_line, ccb->gsm_id, fname),
                          ccb->index, local_expires_timeout + delta);
        




        (void) sip_platform_localexpires_timer_start((local_expires_timeout + delta) * 1000,
                                                     ccb->index, &(ccb->dest_sip_addr),
                                                     (uint16_t) ccb->dest_sip_port);
    }

    
    alert_info = sippmh_get_header_val(request, SIP_HEADER_ALERT_INFO, NULL);
    ccb->alert_info = ALERTING_NONE;
    if (alert_info) {
        parseAlertingHeader(ccb, alert_info);
    }

    
    ccsip_process_call_info_header(request, ccb);

    



    if (ccsip_get_join_info(ccb, request) == FALSE) {
        



        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BAD_REQ,
                                 SIP_CLI_ERR_BAD_REQ_PHRASE,
                                 SIP_WARN_MISC,
                                 SIP_CLI_ERR_BAD_REQ_FROMURL_ERROR,
                                 FALSE,  TRUE );
        ccb->wait_for_ack = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    }

    sipSPISendInviteResponse100(ccb, TRUE);

    
    cc_call_id = cc_get_new_call_id();
    ccb->gsm_id = cc_call_id;
    
    if (ccb->wastransferred) {
        refererccb = sip_sm_get_ccb_by_callid(ccb->sipxfercallid);
        if (NULL != refererccb) {
            cc_feature_data_t data;

            data.xfer.method = CC_XFER_METHOD_REFER;
            data.xfer.cause = CC_CAUSE_XFER_REMOTE;
            data.xfer.target_call_id = cc_call_id;
            data.xfer.dialstring[0] = '\0';
            sip_cc_feature(refererccb->gsm_id, refererccb->dn_line,
                           CC_FEATURE_XFER, (void *) &data);
        }
    }

    if (refererccb == NULL) {
        dn_line = ccb->dn_line;
    } else {
        dn_line = refererccb->dn_line;
    }

    
    if (unescape_UserInfo(ccb->calledNumber, unescape_str_temp, MAX_SIP_URL_LENGTH)) {
        ccb->calledNumber = strlib_update(ccb->calledNumber, unescape_str_temp);
    }

    if (unescape_UserInfo(ccb->callingNumber, unescape_str_temp, MAX_SIP_URL_LENGTH)) {
        ccb->callingNumber = strlib_update(ccb->callingNumber, unescape_str_temp);
    }

    if (unescape_UserInfo(ccb->callingDisplayName, unescape_str_temp, MAX_SIP_URL_LENGTH)) {
        ccb->callingDisplayName = strlib_update(ccb->callingDisplayName, unescape_str_temp);
    }

    if (unescape_UserInfo(ccb->altCallingNumber, unescape_str_temp, MAX_SIP_URL_LENGTH)) {
        ccb->altCallingNumber = strlib_update(ccb->altCallingNumber, unescape_str_temp);
    }

    
    ccsip_parse_send_info_header(request, &recv_info_list);



    if (ccb->wastransferred) {
        sip_cc_setup(cc_call_id, dn_line,
                     ccb->callingDisplayName,
                     ccb->callingNumber,
                     ccb->altCallingNumber,
                     ccb->displayCallingNumber,
                     ccb->calledDisplayedName,
                     ccb->calledNumber,
                     ccb->displayCalledNumber,
                     ccb->div_info->orig_called_name,
                     ccb->div_info->orig_called_number,
                     ccb->div_info->last_redirect_name,
                     ccb->div_info->last_redirect_number,
                     ccb->call_type,
                     ccb->alert_info, ccb->alerting_ring,
                     ccb->alerting_tone, ccb->in_call_info, TRUE,
                     recv_info_list, request);
    } else {
        sip_cc_setup(cc_call_id, dn_line,
                     ccb->callingDisplayName,
                     ccb->callingNumber,
                     ccb->altCallingNumber,
                     ccb->displayCallingNumber,
                     ccb->calledDisplayedName,
                     ccb->calledNumber,
                     ccb->displayCalledNumber,
                     ccb->div_info->orig_called_name,
                     ccb->div_info->orig_called_number,
                     ccb->div_info->last_redirect_name,
                     ccb->div_info->last_redirect_number,
                     ccb->call_type,
                     ccb->alert_info, ccb->alerting_ring,
                     ccb->alerting_tone, ccb->in_call_info, FALSE,
                     recv_info_list, request);
    }

    strlib_free(recv_info_list);

    sip_sm_change_state(ccb, SIP_STATE_RECV_INVITE);

    if (check_send_487) {
        



        ccsipCCB_t *other_ccb;

        other_ccb = sip_sm_get_ccb_by_callid(ccb->sipxfercallid);
        if ((other_ccb != NULL) &&
            ((other_ccb->state >= SIP_STATE_RECV_INVITE) &&
             (other_ccb->state < SIP_STATE_RECV_INVITE_CONNECTED))) {
            sipSPISendInviteResponse(other_ccb, SIP_CLI_ERR_REQ_CANCEL,
                                     SIP_CLI_ERR_REQ_CANCEL_PHRASE, 0, NULL,
                                     FALSE  , TRUE );
            sip_cc_release(other_ccb->gsm_id, other_ccb->dn_line,
                           CC_CAUSE_NORMAL, NULL);
            other_ccb->wait_for_ack = TRUE;
            sip_sm_change_state(other_ccb, SIP_STATE_RELEASE);
        }
    }
}











boolean
ccsip_attempt_backup_proxy (ccsipCCB_t *ccb)
{
    char     ip_addr_str[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t ipaddr;
    int      tempPort = 0;
    char     tmp_str[STATUS_LINE_MAX_LEN];

    CPR_IP_ADDR_INIT(ipaddr);
    


    sipTransportGetBkupServerAddress(&ipaddr, ccb->dn_line, ip_addr_str);
    if (util_check_if_ip_valid(&ipaddr)) {
         util_ntohl(&(ccb->dest_sip_addr), &ipaddr);

        



        tempPort = sipTransportGetBkupServerPort(ccb->dn_line);
        if (tempPort != 0) {
            ccb->dest_sip_port = tempPort;
        } else {
            ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
        }

        ccb->proxySelection = SIP_PROXY_BACKUP;

        







        gGlobInfo.backup_active = 12;

        


        if ((platGetPhraseText(STR_INDEX_PROXY_UNAVAIL,
                                      (char *)tmp_str,
                                      STATUS_LINE_MAX_LEN - 1)) == CPR_SUCCESS) {
            ui_set_call_status(tmp_str, ccb->dn_line, ccb->gsm_id);
        }
        return (TRUE);
    }

    return (FALSE);
}












boolean
send_resume_or_hold_request (ccsipCCB_t *ccb, boolean hold_initiated)
{
    ccb->authen.cred_type = 0;
    ccb->authen.new_flag = TRUE;
    ccb->hold_initiated = hold_initiated;
    if (sipSPISendInviteMidCall(ccb, FALSE ) != TRUE) {
        return FALSE;
    }
    
    ADD_TO_ARP_CACHE(ccb->dest_sip_addr);
    return TRUE;
}

void
ccsip_handle_idle_ev_cc_setup (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "idle_ev_cc_setup";
    char           *dialString;
    char           *referred_by_blind;
    uint32_t        dialStringLength = 0;
    char           *calledNumberTemp;
    char           *outputString = NULL;
    uint32_t        usernameLength = 0;
    char           *hostnameString = NULL;
    uint32_t        hostnameLength = 0;
    char            proxy_ipaddr_str[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t   proxy_ipaddr;

    char           *extraString = NULL;
    uint32_t        extraLength = 0;
    uint32_t        n = 0;
    static char     dialtranslate[MAX_SIP_URL_LENGTH];
    char            temp[MAX_IPADDR_STR_LEN];
    char            line_name[MAX_LINE_NAME_SIZE];
    int             port;
    boolean         sendInvite = FALSE;
    char           *tmpPtr = NULL;
    cpr_ip_type     ip_type = CPR_IP_ADDR_IPV4;
    boolean         replace = FALSE;

    CPR_IP_ADDR_INIT(proxy_ipaddr);

    ccb->gsm_id  = event->u.cc_msg->msg.setup.call_id;
    ccb->dn_line = event->u.cc_msg->msg.setup.line;

    



    if (ccsip_is_replace_setup(event->u.cc_msg->msg.setup.replaces)) {
        replace = TRUE;
        if (!ccsip_set_replace_info(ccb, &event->u.cc_msg->msg.setup)) {
            
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"ignore setup, no replace info for"
                              " replace setup request\n", DEB_F_PREFIX_ARGS(SIP_REP, fname));
            return;
        }
    }

    ccsip_set_join_info(ccb, &event->u.cc_msg->msg.setup);

    dialString = (char *) event->u.cc_msg->msg.setup.caller_id.called_number;
    referred_by_blind = event->u.cc_msg->msg.setup.redirect.redirects[0].number;
    dialStringLength = strlen(dialString);
    if ((0 == dialStringLength) && !replace) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"No Digits to dial", fname);
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    } else if (!dialStringLength && replace) {
        dialString = (char *) event->u.cc_msg->msg.setup.caller_id.calling_number;
        dialStringLength = strlen(dialString);
    }

    









    ccb->calledDisplayedName = strlib_update(ccb->calledDisplayedName,
                                             dialString);

    memset(proxy_ipaddr_str, 0, MAX_IPADDR_STR_LEN);
    memset(temp, 0, sizeof(temp));


    sip_util_get_new_call_id(ccb);
    ccb->featuretype = CC_FEATURE_NONE;
    



    ccb->local_port = sipTransportGetListenPort(ccb->dn_line, ccb);

    





    






    if (event->u.cc_msg->msg.setup.redirect.redirects[0].number[0] != '\0') {
        ccb->sip_referredBy = strlib_update(ccb->sip_referredBy,
                                            referred_by_blind);
        ccb->blindtransferred = TRUE;
    }
    if (event->u.cc_msg->msg.setup.redirect.redirects[0].number[0] != '\0') {
        strlib_free(event->u.cc_msg->msg.setup.redirect.redirects[0].number);
    }

    


    ccb->routeMode = RouteDefault;

#define CAST_N (int32_t *)

    (void) MatchDialTemplate(ccb->calledDisplayedName, ccb->dn_line, CAST_N & n, dialtranslate,
                             sizeof(dialtranslate), (RouteMode *) & (ccb->routeMode), NULL);
    dialString = dialtranslate;
    dialStringLength = strlen(dialString);

    











    tmpPtr = strchr(dialString, '<');
    if (tmpPtr) {
        dialString = tmpPtr;
        dialStringLength = strlen(dialString);
    }

    


    if ((dialStringLength > 0) && (dialString[0] == '<')) {
        dialString++;
        dialStringLength--;
    }

    


    if (!cpr_strncasecmp(dialString, "sip:", 4)) {
        dialStringLength -= 4;
        dialString += 4;
    }

    


    for (n = 0; n <= dialStringLength; n++) {
        





        if (n == dialStringLength) {
            if (hostnameString == NULL) {
                usernameLength = n;
            } else {
                hostnameLength = n - usernameLength - 1; 
            }
        } else if ((dialString[n] == '@') && (hostnameString == NULL)) {
            



            usernameLength = n;
            hostnameString = dialString + n + 1;
        } else if ((dialString[n] == ';') || (dialString[n] == '>')) {
            




            if (hostnameString == NULL) {
                usernameLength = n;
            } else {
                hostnameLength = n - usernameLength - 1; 
            }
            extraString = dialString + n;
            extraLength = dialStringLength - n;
            break;
        }
    }
    









    switch (ccb->routeMode) {
    case RouteEmergency:{
            



            if (ccb->proxySelection != SIP_PROXY_BACKUP) {
                
                sipTransportGetEmerServerAddress(ccb->dn_line, proxy_ipaddr_str);
                if (hostnameLength == 0) {
                    hostnameString = proxy_ipaddr_str;
                    hostnameLength = strlen(proxy_ipaddr_str);
                }
                if (proxy_ipaddr_str[0]) {
                    if (!str2ip((const char *)proxy_ipaddr_str, &proxy_ipaddr)) {
                        
                        util_ntohl(&(ccb->dest_sip_addr), &proxy_ipaddr);
                        



                        port = sipTransportGetEmerServerPort(ccb->dn_line);
                        if (port) {
                            ccb->dest_sip_port = (uint32_t)port;
                        } else {
                            ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
                        }
                        break;
                    }
                }
            }
            
            
         }
    default:
        ip_type = sipTransportGetPrimServerAddress(ccb->dn_line, temp);
        if (hostnameLength == 0) {
            hostnameString = temp;
            hostnameLength = strlen(hostnameString);
        }
        sipTransportGetServerIPAddr(&(ccb->dest_sip_addr),ccb->dn_line);
        if (util_check_if_ip_valid(&(ccb->dest_sip_addr))) {
            ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
        } else {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"Unable to reach proxy, attempting backup.\n",
                             DEB_F_PREFIX_ARGS(SIP_PROXY, fname));
            if (!ccsip_attempt_backup_proxy(ccb)) {
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"Attempt to reach backup proxy failed.\n",
                                 DEB_F_PREFIX_ARGS(SIP_PROXY, fname));
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"INVITE will be broadcast.\n",
                    DEB_F_PREFIX_ARGS(SIP_PROXY, fname));
            }
        }
    }

    
    ADD_TO_ARP_CACHE(ccb->dest_sip_addr);

    


    calledNumberTemp = strlib_open(ccb->calledNumber, (MAX_SIP_URL_LENGTH * 2));
    outputString = calledNumberTemp;
    sstrncpy(outputString, "<sip:", MAX_SIP_URL_LENGTH);
    outputString += 5;
    if (usernameLength) {
        outputString += sippmh_convertURLCharToEscChar(dialString,
                                                       usernameLength,
                                                       outputString,
                                                       (MAX_SIP_URL_LENGTH * 2) - 5, FALSE);
        *outputString++ = '@';
    }
    if ((hostnameLength) && (hostnameString)) {
        if (ip_type == CPR_IP_ADDR_IPV6) {
            *outputString++ = '[';
        }
        memcpy(outputString, hostnameString, hostnameLength);
        outputString += hostnameLength;
        if (ip_type == CPR_IP_ADDR_IPV6) {
            *outputString++ = ']';
        }
    }
    if (extraLength) {
        memcpy(outputString, extraString, extraLength);
        


        outputString[extraLength] = 0;
        


        if (strchr(outputString, '>') == NULL) {
            outputString[extraLength++] = '>';
        }
        outputString += extraLength;
    } else {
        *outputString++ = '>';
    }
    


    *outputString = 0;
    ccb->calledNumber = strlib_close(calledNumberTemp);
    if (ccb->calledNumber) {
        ccb->calledNumberLen = (uint16_t) strlen(ccb->calledNumber);
    }

    CCSIP_DEBUG_STATE(DEB_F_PREFIX"All digits collected.  Placing the call\n",
        DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
    config_get_line_string(CFGID_LINE_NAME, line_name, ccb->dn_line, sizeof(line_name));
    ccb->callingNumber = strlib_update(ccb->callingNumber, line_name);

    CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"SIPSM %d: Setup\n",
                      DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname), ccb->index);

    
    ccsip_store_call_info(&event->u.cc_msg->msg.setup.call_info, ccb);

    

    
    ccsip_save_local_msg_body(ccb, &event->u.cc_msg->msg.setup.msg_body);

    


    
    if (event->u.cc_msg->msg.setup.redirect.redirects[0].redirect_reason
        == CC_REDIRECT_REASON_DEFLECTION) {
        sendInvite = sipSPISendInvite(ccb, SIP_INVITE_TYPE_TRANSFER, TRUE);
    } else {
        sendInvite = sipSPISendInvite(ccb, SIP_INVITE_TYPE_NORMAL, TRUE);
    }

    if (sendInvite == TRUE) {
        sip_sm_change_state(ccb, SIP_STATE_SENT_INVITE);
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sipSPISendInvite failed", fname);
        if (event->u.cc_msg->msg.setup.redirect.redirects[0].redirect_reason
            == CC_REDIRECT_REASON_DEFLECTION) {
            






            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL, NULL);
            sip_sm_call_cleanup(ccb);
        } else {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        }

    }

}







void
ccsip_handle_sentinvite_ev_sip_1xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "sentinvite_ev_sip_1xx";
    sipMessage_t   *response;
    sipRespLine_t  *respLine;
    int             status_code = 0;

    
    response = event->u.pSipMessage;

    
    respLine = sippmh_get_response_line(response);
    if (respLine) {
        status_code = respLine->status_code;
        SIPPMH_FREE_RESPONSE_LINE(respLine);
    }

    



    sip_sm_200and300_update(ccb, response, status_code);

    sip_decrement_backup_active_count(ccb);

    
    ccb->flags |= RECD_1xx;

    if (status_code != SIP_1XX_TRYING) {
        
        ccb->authen.cred_type = 0;
    }

    switch (status_code) {

    case SIP_1XX_TRYING:
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY), ccb->index,
                          ccb->dn_line, fname, sip_util_state2string(ccb->state),
                          "SIP 100 TRYING");
        





        ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);
        free_sip_message(response);
        sip_cc_proceeding(ccb->gsm_id, ccb->dn_line);
        return;

    case SIP_1XX_RINGING:
        {
            sipsdp_status_t sdp_status;
            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: %s <- SIP 180 RINGING\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
							  ccb->index, sip_util_state2string(ccb->state));

            








            sdp_status = sip_util_extract_sdp(ccb, response);

            switch (sdp_status) {
            case SIP_SDP_SUCCESS:
            case SIP_SDP_SESSION_AUDIT:
                ccb->oa_state = OA_IDLE;
                
                ccsip_update_callinfo(ccb, response, TRUE, FALSE, TRUE);
                sip_cc_alerting(ccb->gsm_id, ccb->dn_line, response, TRUE);
                break;

            case SIP_SDP_NOT_PRESENT:
                
                ccsip_update_callinfo(ccb, response, TRUE, FALSE, TRUE);
                sip_cc_alerting(ccb->gsm_id, ccb->dn_line, NULL, 0);
                break;

            case SIP_SDP_DNS_FAIL:
            case SIP_SDP_NO_MEDIA:
            case SIP_SDP_ERROR:
            default:
                sipSPISendCancel(ccb);
                free_sip_message(response);
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
                return;
            }
            




            free_sip_message(response);
            return;

        }                       
    case SIP_1XX_SESSION_PROGRESS:
        {
            sipsdp_status_t sdp_status;

            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY), ccb->index,
                              ccb->dn_line, fname, sip_util_state2string(ccb->state),
                              "SIP 183 IN BAND SESSION PROGRESS");

            sdp_status = sip_util_extract_sdp(ccb, response);

            switch (sdp_status) {
            case SIP_SDP_SUCCESS:
            case SIP_SDP_SESSION_AUDIT:
                ccb->oa_state = OA_IDLE;
                break;

            case SIP_SDP_NOT_PRESENT:
                
                
                
                
                
                ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);
                free_sip_message(response);
                return;

            case SIP_SDP_DNS_FAIL:
            case SIP_SDP_NO_MEDIA:
            case SIP_SDP_ERROR:
            default:
                sipSPISendCancel(ccb);
                free_sip_message(response);
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
                return;
            }

            




            ccsip_update_callinfo(ccb, response, TRUE, FALSE, TRUE);
            sip_cc_alerting(ccb->gsm_id, ccb->dn_line, response, TRUE);

            




            if ((ccb->in_call_info) &&
                (ccb->in_call_info->data.call_info_feat_data.feature_flag & CC_UI_STATE) &&
                (ccb->in_call_info->data.call_info_feat_data.ui_state == CC_UI_STATE_BUSY)) {
                 CCSIP_DEBUG_STATE(DEB_F_PREFIX"DETECTED UI_STATE=BUSY IN 183.\n",
                     DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
                 sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_UI_STATE_BUSY, NULL);
            }
            free_sip_message(response);
            return;
        }                       

    case SIP_1XX_CALL_FWD:
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: %s <- SIP 181 CALL IS BEING"
                          "FORWARDED\n",
						  DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                          ccb->index, sip_util_state2string(ccb->state));
        
        ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);
        free_sip_message(response);
        sip_cc_proceeding(ccb->gsm_id, ccb->dn_line);
        return;

    case SIP_1XX_QUEUED:
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: %s <- SIP 182 QUEUED\n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                          ccb->index, sip_util_state2string(ccb->state));
        
        ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);
        free_sip_message(response);
        sip_cc_proceeding(ccb->gsm_id, ccb->dn_line);
        return;

    default:
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: %s <- SIP BAD 1xx\n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                          ccb->index, sip_util_state2string(ccb->state));
        free_sip_message(response);
        return;
    }
}


void
ccsip_handle_sentinvite_ev_sip_2xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "sentinvite_ev_sip_2xx";
    sipMessage_t   *response;
    const char     *contact = NULL;
    sipsdp_status_t sdp_status;
    string_t        recv_info_list = strlib_empty();

    
    response = event->u.pSipMessage;

    
    if (!sip_sm_is_invite_response(response)) {
        sipMethod_t method = sipMethodInvalid;
        int         response_code = 0;

        
        if (sipGetResponseCode(response, &response_code) < 0) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipGetResponseCode");
            free_sip_message(response);
            return;
        }
        if (sipGetResponseMethod(response, &method) < 0) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipGetResponseMethod");
            free_sip_message(response);
            return;
        }

        if (response_code == SIP_ACCEPTED && method == sipMethodRefer) {
            ccsip_handle_accept_2xx(ccb, event);
            return;
        }
        free_sip_message(response);
        clean_method_request_trx(ccb, method, TRUE);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED), ccb->index,
                          ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        return;
    }

    



    sip_sm_200and300_update(ccb, response, SIP_STATUS_SUCCESS);

    
    ccb->authen.cred_type = 0;

    sip_decrement_backup_active_count(ccb);
    (void) sip_platform_expires_timer_stop(ccb->index);
    
    contact = sippmh_get_cached_header_val(response, CONTACT);
    if (contact) {
        if (sipSPICheckContact(contact) < 0) {
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "sipSPICheckContact()");
            free_sip_message(response);
            ccb->authen.cred_type = 0;
            sipSPISendBye(ccb, NULL, NULL);
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            clean_method_request_trx(ccb, sipMethodAck, FALSE);
            return;
        }
    }

    
    sdp_status = sip_util_extract_sdp(ccb, response);

    switch (sdp_status) {
    case SIP_SDP_SUCCESS:
    case SIP_SDP_SESSION_AUDIT:
        ccb->oa_state = OA_IDLE;
        break;

    case SIP_SDP_NOT_PRESENT:
        break;

    case SIP_SDP_DNS_FAIL:
    case SIP_SDP_NO_MEDIA:
    case SIP_SDP_ERROR:
    default:
        
        if (sipSPISendAck(ccb, response) == FALSE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipSPISendAck");
        }
        
        ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);

        free_sip_message(response);
        sipSPISendBye(ccb, NULL, NULL);
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
        if (ccb->wastransferred) {
            




            cc_feature_data_t data;

            data.notify.cause = CC_CAUSE_ERROR;
            data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
            data.notify.method = CC_XFER_METHOD_REFER;
            data.notify.blind_xferror_gsm_id = 0;
            sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_NOTIFY,
                           (void *) &data);
        }
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        return;
    }

    



    ccsip_parse_diversion_header (ccb, response);

    
    ccsip_parse_send_info_header(response, &recv_info_list);

    




    ccsip_update_callinfo(ccb, response, TRUE, FALSE, TRUE);
    sip_cc_connected(ccb->gsm_id, ccb->dn_line, recv_info_list, response);

    strlib_free(recv_info_list);

    
    free_sip_message(response);
    sip_sm_change_state(ccb, SIP_STATE_SENT_INVITE_CONNECTED);

    




    if ((ccb->wastransferred) || (ccb->blindtransferred == TRUE)) {
        cc_feature_data_t data;

        data.notify.cause = CC_CAUSE_OK;
        data.notify.cause_code = SIP_SUCCESS_SETUP;
        data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
        data.notify.method = CC_XFER_METHOD_REFER;
        data.notify.blind_xferror_gsm_id =
            sip_sm_get_blind_xfereror_ccb_by_gsm_id(ccb->gsm_id);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_NOTIFY,
                       (void *) &data);
        strlib_free(ccb->sipxfercallid);
        ccb->sipxfercallid = strlib_empty();
    } else if (ccb->flags & SENT_INVITE_REPLACE) {
        strlib_free(ccb->sipxfercallid);
        ccb->sipxfercallid = strlib_empty();
    }
}








void
ccsip_handle_sentbye_recvd_invite (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    sipMessage_t *request;

    
    request = event->u.pSipMessage;
    (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_CALLEG,
                                   SIP_CLI_ERR_CALLEG_PHRASE, 0, NULL, NULL);
    free_sip_message(request);
}

void
ccsip_handle_release_complete (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "release_complete";

    if (ccb->blind_xfer_call_id == CC_NO_CALL_ID) {
        if (!ccb->wait_for_ack) {
            if ((ccb->flags & RECD_BYE) && (ccb->last_request)) {
                (void) sipSPISendByeOrCancelResponse(ccb, ccb->last_request, sipMethodBye);
                ccb->flags &= ~RECD_BYE;
            }
            if (!(sip_platform_msg_timer_outstanding_get(ccb->index))) {
                sip_sm_call_cleanup(ccb);
            }
        } else {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"INFO: waiting for Invite Response Ack "
                             "before clearing call\n", DEB_F_PREFIX_ARGS(SIP_ACK, fname));
            



            (void) sip_platform_supervision_disconnect_timer_start(
                       SUPERVISION_DISCONNECT_TIMEOUT, ccb->index);
        }
    } else {
        




        (void) sip_platform_supervision_disconnect_timer_stop(ccb->index);
        sip_sm_change_state(ccb, SIP_STATE_BLIND_XFER_PENDING);
    }
}

void
ccsip_handle_sentbye_ev_sip_1xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char   *fname = "sentbye_ev_sip_1xx";
    sipMessage_t *response;

    
    response = event->u.pSipMessage;

    
    if (!sip_sm_is_bye_or_cancel_response(response)) {
        if (sip_sm_is_invite_response(response)) {
            
            
            
            if (ccb->flags & SEND_CANCEL) {
                sipSPISendCancel(ccb);
            }
        }
        free_sip_message(response);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED), ccb->index,
                          ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        return;
    }
    



    (void) sip_platform_supervision_disconnect_timer_start(
                SUPERVISION_DISCONNECT_TIMEOUT, ccb->index);
    free_sip_message(response);
}

void
ccsip_handle_sentbye_ev_sip_2xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char   *fname = "sentbye_ev_sip_2xx";
    sipMessage_t *response;

    
    response = event->u.pSipMessage;

    
    if (!sip_sm_is_bye_or_cancel_response(response)) {
        

        
        if (sip_sm_is_invite_response(response) &&
            (get_method_request_trx_index(ccb, sipMethodCancel, TRUE) != -1)) {
            











	    char *to_tag, *sip_to_temp;

	    to_tag = strstr(ccb->sip_to,";tag");
	    



	    if (!to_tag) {
                sip_to_temp = strlib_open(ccb->sip_to, MAX_SIP_URL_LENGTH);
                if (sip_to_temp) {
                    sstrncat(sip_to_temp, ";tag=",
                             MAX_SIP_URL_LENGTH - strlen(sip_to_temp));
                    if (ccb->sip_to_tag) {
                        sstrncat(sip_to_temp, ccb->sip_to_tag,
                        MAX_SIP_URL_LENGTH - strlen(sip_to_temp));
		    }
	        }
                ccb->sip_to = strlib_close(sip_to_temp);
	    }

            if (sipSPISendAck(ccb, NULL) == FALSE) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                  fname, "sipSPISendAck");
            }
            sipSPISendBye(ccb, NULL, NULL);

            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX" %d %s Cross-over situation CANCEL/200 OK(INVITE).\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_ACK, ccb->dn_line, ccb->gsm_id, fname),
                              ccb->index, sip_util_state2string(ccb->state));
        } else {
            
            
            sipMethod_t method = sipMethodInvalid;

            if (sipGetResponseMethod(response, &method) < 0) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                  fname, "sipGetResponseMethod");
                free_sip_message(response);
                return;
            }

            clean_method_request_trx(ccb, method, TRUE);
        }

        free_sip_message(response);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        return;
    }
    (void) sip_platform_expires_timer_stop(ccb->index);
    if (!ccb->send_delayed_bye) {
        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
    }

    if (!ccb->wait_for_ack) {
        sip_sm_call_cleanup(ccb);
    } else {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"INFO: waiting for Invite Response Ack "
                         "before clearing call\n", DEB_F_PREFIX_ARGS(SIP_ACK, fname));
        



        (void) sip_platform_supervision_disconnect_timer_start(
                    SUPERVISION_DISCONNECT_TIMEOUT, ccb->index);
    }
    free_sip_message(response);
}





void
ccsip_handle_release_ev_sip_bye (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "blindxfr_ev_sip_bye";
    sipMessage_t   *request;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    sipMethod_t     method = sipMethodInvalid;

    memset(request_check_reason_phrase, 0, SIP_WARNING_LENGTH);

    
    request = event->u.pSipMessage;

    
    sipGetRequestMethod(request, &method);
    if (sip_sm_request_check_and_store(ccb, request, method, TRUE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       request_check_reason_code,
                                       request_check_reason_phrase, NULL);
        free_sip_message(request);
        return;
    }

    (void) sipSPISendByeOrCancelResponse(ccb, request, sipMethodBye);
}


void
ccsip_handle_sentblindntfy_ev_sip_2xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    sipMessage_t *response;

    
    response = event->u.pSipMessage;
    (void) sip_platform_expires_timer_stop(ccb->index);

    if (ccb->flags & FINAL_NOTIFY) {
        sip_sm_call_cleanup(ccb);
    } else {
        clean_method_request_trx(ccb, sipMethodNotify, TRUE);
    }

    free_sip_message(response);
}


void
ccsip_handle_sendbye_ev_supervision_disconnect (ccsipCCB_t *ccb,
                                                sipSMEvent_t *event)
{

    (void) sip_platform_expires_timer_stop(ccb->index);
    sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
    sip_sm_call_cleanup(ccb);
}















void
ccsip_handle_release_ev_cc_feature (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "release_ev_cc_feature";
    cc_features_t   feature_type;

    feature_type = event->u.cc_msg->msg.feature.feature_id;
    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      cc_feature_name(feature_type));

    switch (feature_type) {
    case CC_FEATURE_CANCEL:
        break;
    default:
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type, NULL,
                           CC_CAUSE_ERROR);
        break;
    }
}










void
ccsip_handle_release_ev_release (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "release_ev_release";

    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      "sipstack at SIP_STATE_RELEASE received a RELEASE event from gsm");

    
    ccsip_handle_release_complete(ccb, event);

    
    ccsip_handle_sendbye_ev_supervision_disconnect(ccb, event);
}


void
ccsip_handle_recv_error_response_ev_sip_ack (ccsipCCB_t *ccb,
                                             sipSMEvent_t *event)
{
    sipMessage_t *response;

    
    response = event->u.pSipMessage;

    ccb->wait_for_ack = FALSE;

    if (ccb->send_delayed_bye) {
        
        
        sipSPISendBye(ccb, NULL, NULL);
    } else {
        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
        sip_sm_call_cleanup(ccb);
    }

    free_sip_message(response);
}

void
ccsip_handle_sentbye_ev_sip_fxx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "sentbye_ev_sip_fxx";
    sipMessage_t   *response;
    sipRespLine_t  *respLine = NULL;
    int             status_code = 0;
    const char     *authenticate = NULL;
    credentials_t   credentials;
    sip_authen_t   *sip_authen = NULL;
    char           *author_str = NULL;
    boolean         good_authorization = FALSE;
    const char     *rsp_method = NULL;
    char           *alsoString = NULL;
    sipMethod_t     method = sipMethodInvalid;
    enum {
        INVALID,
        RESP_OF_BYE,
        RESP_OF_CANCEL,
        RESP_OF_NOTIFY,
        RESP_OF_INVITE
    } resp_type = INVALID;

    
    response = event->u.pSipMessage;
    if (sipGetResponseMethod(response, &method) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseMethod");
        free_sip_message(response);
        return;
    }
    switch (method) {
    case sipMethodInvite:
        resp_type = RESP_OF_INVITE;
        rsp_method = SIP_METHOD_INVITE;
        break;
    case sipMethodBye:
        resp_type = RESP_OF_BYE;
        rsp_method = SIP_METHOD_BYE;
        break;
    case sipMethodCancel:
        resp_type = RESP_OF_CANCEL;
        rsp_method = SIP_METHOD_CANCEL;
        break;
    case sipMethodNotify:
        resp_type = RESP_OF_NOTIFY;
        rsp_method = SIP_METHOD_NOTIFY;
        break;
    default:
        resp_type = INVALID;
        break;
    }

    if (INVALID == resp_type) {
        free_sip_message(response);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        if (ccb->state == SIP_STATE_BLIND_XFER_PENDING) {
            


            sip_sm_call_cleanup(ccb);
        }
        return;
    }

    sip_decrement_backup_active_count(ccb);
    
    respLine = sippmh_get_response_line(response);
    if (respLine) {
        status_code = respLine->status_code;
        SIPPMH_FREE_RESPONSE_LINE(respLine);
    }


    if ((strcmp(rsp_method, SIP_METHOD_INVITE) == 0) &&
        (status_code == SIP_SERV_ERR_INTERNAL)) {
        





        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Acking delayed 500 response to INVITE "
                          "request\n", DEB_F_PREFIX_ARGS(SIP_ACK, fname));
        sipSPISendFailureResponseAck(ccb, response, FALSE, 0);
        return;
    }


    switch (status_code) {
    case SIP_CLI_ERR_UNAUTH:
    case SIP_CLI_ERR_PROXY_REQD:
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: %s\n",
						  DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
						  ccb->index, AUTH_BUGINF(status_code));

        if (cred_get_credentials_r(ccb, &credentials) == FALSE) {
            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"retries exceeded: %d/%d\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
							  ccb->index, ccb->authen.cred_type, MAX_RETRIES_401);

            free_sip_message(response);
            sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR);
            sip_sm_call_cleanup(ccb);
            return;
        }

        authenticate = sippmh_get_header_val(response, AUTH_HDR(status_code), NULL);
        if (authenticate != NULL) {
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"Authenticate header %s= %s\n", DEB_F_PREFIX_ARGS(SIP_STATE, fname),
                              AUTH_HDR_STR(status_code), authenticate);
            ccb->retx_counter = 0;
            sip_authen = sippmh_parse_authenticate(authenticate);
            if (sip_authen) {
                ccb->authen.new_flag = FALSE;
                ccb->authen.cnonce[0] = '\0';
                if (sipSPIGenerateAuthorizationResponse(sip_authen, ccb->ReqURI,
                                                        rsp_method, credentials.id,
                                                        credentials.pw,
                                                        &author_str,
                                                        &(ccb->authen.nc_count),
                                                        ccb))
                {
                    good_authorization = TRUE;
                    if (ccb->authen.authorization != NULL) {
                        cpr_free(ccb->authen.authorization);
                        ccb->authen.authorization = NULL;
                    }
                    if (ccb->authen.sip_authen != NULL) {
                        sippmh_free_authen(ccb->authen.sip_authen);
                        ccb->authen.sip_authen = NULL;
                    }
                    ccb->authen.authorization = (char *)
                        cpr_malloc(strlen(author_str) * sizeof(char) + 1);

                    



                    if (ccb->authen.authorization != NULL) {
                        sstrncpy(ccb->authen.authorization, author_str,
                                 strlen(author_str) * sizeof(char) + 1);
                        ccb->authen.status_code = status_code;
                        ccb->authen.sip_authen = sip_authen;
                    }

                    cpr_free(author_str);
                } else {
                    CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Authorization header "
                                      "build unsuccessful\n", fname);
                }
                





                if (good_authorization == FALSE) {
                    sippmh_free_authen(sip_authen);
                }
            }
            if (strcmp(rsp_method, SIP_METHOD_BYE) == 0) {
                clean_method_request_trx(ccb, sipMethodBye, TRUE);
                if (ccb->referto[0]) {
                    alsoString = (char *) cpr_malloc(MAX_SIP_URL_LENGTH);
                    if (!alsoString) {
                        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                          ccb->index, ccb->dn_line, fname,
                                          "malloc(also string)");
                        sipSPISendBye(ccb, NULL, NULL);
                        return;
                    }
                    sstrncpy(alsoString, ccb->referto, MAX_SIP_URL_LENGTH);
                    sipSPISendBye(ccb, alsoString, NULL);
                    cpr_free(alsoString);
                    return;
                }
                
                sipSPISendBye(ccb, NULL, NULL);
            } else if (strcmp(rsp_method, SIP_METHOD_CANCEL) == 0){
                sipSPISendCancel(ccb);
            } else {
                (void) sipSPISendNotify(ccb, ccb->xfer_status);
            }

        } else {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"401/407 response missing "
                              "Authenticate\n", fname);
            sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR);
            sip_sm_call_cleanup(ccb);
        }
        free_sip_message(response);
        return;


    case SIP_CLI_ERR_LOOP_DETECT:
    case SIP_CLI_ERR_BUSY_HERE:
    case SIP_CLI_ERR_MANY_HOPS:
    case SIP_CLI_ERR_AMBIGUOUS:
    case SIP_CLI_ERR_REQ_CANCEL:
        if (sipSPISendAck(ccb, NULL) == FALSE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipSPISendAck");
        }

        (void) sip_platform_expires_timer_stop(ccb->index);
        if (!ccb->send_delayed_bye) {
            sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
        }

        if (!ccb->wait_for_ack) {
            sip_sm_call_cleanup(ccb);
        } else {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"waiting for Invite Response Ack "
                             "before clearing call\n", DEB_F_PREFIX_ARGS(SIP_ACK, fname));
            



            (void) sip_platform_supervision_disconnect_timer_start(
                        SUPERVISION_DISCONNECT_TIMEOUT, ccb->index);
        }

        free_sip_message(response);
        return;

    case SIP_SERV_ERR_NOT_IMPLEM:
        






        if (strcmp (rsp_method,SIP_METHOD_NOTIFY) == 0){
            
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"Ignoring NOTIFY error. But clean up "
                              "transaction.\n", DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
            clean_method_request_trx(ccb, sipMethodNotify, TRUE);

            if (ccb->flags & FINAL_NOTIFY) {
                sip_sm_call_cleanup(ccb);
                CCSIP_DEBUG_STATE(DEB_F_PREFIX"Ignoring NOTIFY error. but "
                                  "cleaning up call.\n", DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
            }
            free_sip_message(response);
            return;
        }
    

    default:
        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR);
        sip_sm_call_cleanup(ccb);
        free_sip_message(response);
        return;
    }
}











void
ccsip_handle_sentinvite_ev_sip_3xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "sentinvite_ev_sip_3xx";
    sipMessage_t   *response;
    sipRespLine_t  *respLine;
    uint16_t        status_code = 0;

    
    response = event->u.pSipMessage;

    sip_decrement_backup_active_count(ccb);

    
    respLine = sippmh_get_response_line(response);
    if (respLine) {
        status_code = respLine->status_code;
        SIPPMH_FREE_RESPONSE_LINE(respLine);
    }

    switch (status_code) {
    case SIP_RED_MULT_CHOICES :
    case SIP_RED_MOVED_PERM   :
    case SIP_RED_MOVED_TEMP   :
    case SIP_RED_USE_PROXY    :
        



        sip_sm_update_to_from_on_callsetup_finalresponse(ccb, response);
        sip_sm_update_contact_recordroute(ccb, response, status_code,
                                          FALSE );

        



        sipSPISendFailureResponseAck(ccb, response, FALSE, 0);

        
        ccb->authen.cred_type = 0;
        ccb->first_pass_3xx = TRUE;
        sip_redirect(ccb, response, status_code);

        break;
    default:
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d %d unsupported\n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                          ccb->index, status_code);
        break;
    }

    free_sip_message(response);
}

void
handle_error_for_state (ccsipCCB_t *ccb, int status_code)
{
    ccsipCCB_t   *referccb = NULL;
    cc_causes_t   fail_reason = CC_CAUSE_MIN;

    if (ccb->state == SIP_STATE_SENT_INVITE) {
        if (status_code == SIP_CLI_ERR_BUSY_HERE) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_BUSY, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);

        } else if (status_code == SIP_SERV_ERR_UNAVAIL) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_CONGESTION, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);

        } else if (status_code == SIP_CLI_ERR_NOT_AVAIL) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_TEMP_NOT_AVAILABLE, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);

        } else if ((status_code == SIP_CLI_ERR_MEDIA) ||
                   (status_code == SIP_CLI_ERR_NOT_ACCEPT_HERE) ||
                   (status_code == SIP_FAIL_NOT_ACCEPT)) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_PAYLOAD_MISMATCH, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);

        } else if (status_code == SIP_SERV_ERR_INTERNAL) {
            
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_REMOTE_SERVER_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);

        } else if (((status_code == SIP_CLI_ERR_BAD_REQ) ||
                    (status_code == SIP_CLI_ERR_CALLEG)) &&
                   (ccb->wastransferred)) {
            




            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL, NULL);
            sip_sm_call_cleanup(ccb);
        } else if (status_code == SIP_CLI_ERR_NOT_FOUND) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NOT_FOUND, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        } else if (status_code == SIP_CLI_ERR_REQ_CANCEL) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NO_USER_ANS, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        } else {
            





            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        }
        if (ccb->blindtransferred == TRUE) {
            cc_feature_data_t data;

            data.notify.cause = CC_CAUSE_ERROR;
            data.notify.cause_code = status_code;
            data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
            data.notify.method = CC_XFER_METHOD_REFER;
            data.notify.blind_xferror_gsm_id = sip_sm_get_blind_xfereror_ccb_by_gsm_id(ccb->gsm_id);
            sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_NOTIFY,
                           (void *) &data);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        }

        


        if (ccb->wastransferred) {
            referccb = sip_sm_get_target_call_by_gsm_id(ccb->gsm_id);
            if (referccb != NULL) {
                ccb->flags |= FINAL_NOTIFY;
                (void) sipSPISendNotify(referccb, status_code);
                ccb->xfer_status = status_code;
            }
        }
    } else if (ccb->state == SIP_STATE_ACTIVE) {
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, CC_FEATURE_NONE, NULL,
                           CC_CAUSE_ERROR);
    } else if (ccb->state == SIP_STATE_SENT_MIDCALL_INVITE) {
        if (status_code == SIP_CLI_ERR_BUSY_HERE) {
            fail_reason = CC_CAUSE_BUSY;
        } else if ((status_code == SIP_CLI_ERR_MEDIA) ||
                   (status_code == SIP_CLI_ERR_NOT_ACCEPT_HERE) ||
                   (status_code == SIP_FAIL_NOT_ACCEPT)) {
            fail_reason = CC_CAUSE_PAYLOAD_MISMATCH;
        } else if (status_code == SIP_CLI_ERR_REQ_PENDING) {
            
            
            
            fail_reason = CC_CAUSE_REQUEST_PENDING;
        } else if (status_code == SIP_SERV_ERR_UNAVAIL) {
            fail_reason = CC_CAUSE_SERV_ERR_UNAVAIL;
        } else {
            fail_reason = CC_CAUSE_ERROR;
        }

        
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, ccb->featuretype, NULL,
                           fail_reason);
        if (status_code == SIP_CLI_ERR_REQ_TIMEOUT) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        } else {
            
            sip_sm_change_state(ccb, SIP_STATE_ACTIVE);
        }
    } else if (ccb->state == SIP_STATE_BLIND_XFER_PENDING) {
        if (status_code == SIP_CLI_ERR_REQ_TIMEOUT) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        }
    }
    if (ccb->wastransferred) {
        




        cc_feature_data_t data;

        data.notify.cause = fail_reason;
        data.notify.cause_code = status_code;
        data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
        data.notify.method = CC_XFER_METHOD_REFER;
        data.notify.blind_xferror_gsm_id = 0;
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_NOTIFY,
                       (void *)&data);
    }
}


void
ccsip_handle_sentinvite_ev_sip_fxx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "sentinvite_ev_sip_fxx";
    sipMessage_t   *response;
    sipRespLine_t  *respLine = NULL;
    uint16_t        status_code = 0;
    const char     *authenticate = NULL;
    credentials_t   credentials;
    sip_authen_t   *sip_authen = NULL;
    char           *author_str = NULL;
    boolean         good_authorization = FALSE;
    sipMethod_t     method = sipMethodInvalid;
    const char     *rsp_method = NULL;

    enum {
        INVALID,
        RESP_OF_INVITE,
        RESP_OF_REFER,
        RESP_OF_NOTIFY
    } resp_type = INVALID;

    response = event->u.pSipMessage;

    
    if (sipGetResponseMethod(response, &method) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseMethod");
        free_sip_message(response);
        return;
    }
    switch (method) {
    case sipMethodInvite:
        resp_type = RESP_OF_INVITE;
        rsp_method = SIP_METHOD_INVITE;

        
        (void) sip_platform_expires_timer_stop(ccb->index);

        
        ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);

        break;
    case sipMethodRefer:
        resp_type = RESP_OF_REFER;
        rsp_method = SIP_METHOD_REFER;
        break;
    case sipMethodNotify:
        resp_type = RESP_OF_NOTIFY;
        rsp_method = SIP_METHOD_NOTIFY;
        break;
    default:
        resp_type = INVALID;
        break;
    }

    if (INVALID == resp_type) {
        free_sip_message(response);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        return;
    }

    
    respLine = sippmh_get_response_line(response);
    if (respLine) {
        status_code = respLine->status_code;
        SIPPMH_FREE_RESPONSE_LINE(respLine);
    }

    



    sip_sm_200and300_update(ccb, response, status_code);

    
    
    
    if (resp_type == RESP_OF_REFER) {
        switch (ccb->featuretype) {
        case CC_FEATURE_B2BCONF:
        case CC_FEATURE_SELECT:
        case CC_FEATURE_CANCEL:
            if (status_code != SIP_CLI_ERR_UNAUTH && status_code != SIP_CLI_ERR_PROXY_REQD) {
                CCSIP_DEBUG_STATE(DEB_F_PREFIX"Received error response for ext refer\n",
                    DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
                sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, ccb->featuretype, NULL,
                                   CC_CAUSE_ERROR);
                clean_method_request_trx(ccb, sipMethodRefer, TRUE);
                free_sip_message(response);
                return;
            }
        
        default:
            break;
        }
    }

    sip_decrement_backup_active_count(ccb);

    switch (status_code) {
    case SIP_CLI_ERR_UNAUTH:
    case SIP_CLI_ERR_PROXY_REQD:
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"SIP_CLI_ERR_PROXY_REQD: %d: %s\n",
						  DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
						  ccb->index, AUTH_BUGINF(status_code));

        
        if (method == sipMethodInvite) {
            sipSPISendFailureResponseAck(ccb, response, FALSE, 0);
        }

        if (cred_get_credentials_r(ccb, &credentials) == FALSE) {
            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"retries exceeded: %d/%d\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                              ccb->authen.cred_type, MAX_RETRIES_401);
            







            handle_error_for_state(ccb, status_code);
            free_sip_message(response);
            return;
        }

        authenticate = sippmh_get_header_val(response, AUTH_HDR(status_code),
                                             NULL);
        if (authenticate != NULL) {
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"Authenticate header %s= %s\n", DEB_F_PREFIX_ARGS(SIP_AUTH, fname),
                              AUTH_HDR_STR(status_code), authenticate);

            ccb->retx_counter = 0;

            sip_authen = sippmh_parse_authenticate(authenticate);
            if (sip_authen) {
                ccb->authen.new_flag = FALSE;
                


                ccb->authen.cnonce[0] = '\0';
                ccb->authen.nc_count = 0; 
                if (sipSPIGenerateAuthorizationResponse(sip_authen,
                                                        ccb->ReqURI,
                                                        rsp_method,
                                                        credentials.id,
                                                        credentials.pw,
                                                        &author_str,
                                                        &(ccb->authen.nc_count), ccb)) {

                    good_authorization = TRUE;

                    if (ccb->authen.authorization != NULL) {
                        cpr_free(ccb->authen.authorization);
                        ccb->authen.authorization = NULL;
                    }

                    if (ccb->authen.sip_authen != NULL) {
                        sippmh_free_authen(ccb->authen.sip_authen);
                        ccb->authen.sip_authen = NULL;
                    }

                    ccb->authen.authorization = (char *) cpr_malloc(strlen(author_str) *
                                                                    sizeof(char) + 1);

                    



                    if (ccb->authen.authorization != NULL) {
                        sstrncpy(ccb->authen.authorization, author_str,
                                 strlen(author_str) * sizeof(char) + 1);
                        ccb->authen.status_code = status_code;
                        ccb->authen.sip_authen  = sip_authen;
                    }

                    
                    if (ccb->state == SIP_STATE_SENT_INVITE) {
                        sip_cc_proceeding(ccb->gsm_id, ccb->dn_line);
                    }
                    cpr_free(author_str);
                } else {
                    CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Authorization header "
                                      "build unsuccessful\n", fname);
                }
                





                if (good_authorization == FALSE) {
                    sippmh_free_authen(sip_authen);
                }
            }

            switch (resp_type) {
            case RESP_OF_REFER:
                
                
                clean_method_request_trx(ccb, sipMethodRefer, TRUE);

                {
                    boolean refer_sent = FALSE;

                    switch (ccb->featuretype) {
                    case CC_FEATURE_B2BCONF:
                    case CC_FEATURE_SELECT:
                    case CC_FEATURE_CANCEL:
                        break;
                    default:
                        break;
                    }

                    if (refer_sent) {
                        break;
                    }
                }

                if (sipSPISendRefer(ccb, (char *) ccb->sip_referTo, SIP_REF_XFER) != TRUE) {
                    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                      ccb->index, ccb->dn_line, fname,
                                      "sipSPISendRefer Failed");
                }
                break;

            case RESP_OF_INVITE:
                if (ccb->state == SIP_STATE_SENT_INVITE) {
                    
                    
                    if (ccb->record_route_info) {
                        sippmh_free_record_route(ccb->record_route_info);
                        ccb->record_route_info = NULL;
                    }

                    (void) sipSPISendInviteMidCall(ccb, TRUE );
                } else {
                    (void) sipSPISendInviteMidCall(ccb, FALSE );
                }
                break;
            case RESP_OF_NOTIFY:
                
                clean_method_request_trx(ccb, sipMethodNotify, TRUE);
                if (sipSPISendNotify(ccb, ccb->xfer_status) != TRUE) {
                    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                      ccb->index, ccb->dn_line, fname,
                                      "sipSPISendNotify Failed");
                }
                break;
            default:
                break;
            }
        } else {
            if ((ccb->redirect_info != NULL)
                && (method == sipMethodInvite)
                && (ccb->state == SIP_STATE_SENT_INVITE)) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"401/407 response missing "
                                  "Authenticate, redirecting to next add.\n", fname);
                sip_redirect(ccb, response, status_code);
            } else {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"401/407 response missing "
                "Authenticate\n", fname);
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX "Clearing call\n", fname);
                
                clean_method_request_trx(ccb, method, TRUE);
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            }
        }

        free_sip_message(response);
        return;

    case SIP_CLI_ERR_NOT_ALLOWED:
    case SIP_SERV_ERR_NOT_IMPLEM:
        if (RESP_OF_REFER == resp_type) {
            
            










            cc_feature_data_t data;

            
            clean_method_request_trx(ccb, sipMethodRefer, TRUE);

            ccb->referto = strlib_update(ccb->referto, ccb->sip_referTo);
            data.notify.cause = CC_CAUSE_OK;
            data.notify.cause_code = status_code;
            data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
            data.notify.method = CC_XFER_METHOD_BYE;
            data.notify.blind_xferror_gsm_id = 0;
            sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_NOTIFY,
                           (void *) &data);
        }
        if (method == sipMethodInvite) {
            sipSPISendFailureResponseAck(ccb, response, FALSE, 0);
            if ((ccb->redirect_info != NULL) &&
                (ccb->state == SIP_STATE_SENT_INVITE)) {
                sip_redirect(ccb, response, status_code);
            } else {
                handle_error_for_state(ccb, status_code);
            }
        }

        if (method == sipMethodNotify) {
            clean_method_request_trx(ccb, sipMethodNotify, TRUE);
        }
        free_sip_message(response);
        break;

    case SIP_CLI_ERR_REQ_PENDING:
        if (method == sipMethodInvite) {
            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d Glare detected!\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
							  ccb->index);
            sipSPISendFailureResponseAck(ccb, response, FALSE, 0);
            
            
            handle_error_for_state(ccb, status_code);
            












        }

        if (method == sipMethodNotify) {
            clean_method_request_trx(ccb, sipMethodNotify, TRUE);
        }

        free_sip_message(response);
        break;

    default:
        if (method == sipMethodInvite) {
            sipSPISendFailureResponseAck(ccb, response, FALSE, 0);

            if ((ccb->redirect_info != NULL)
                && (ccb->state == SIP_STATE_SENT_INVITE)) {
                sip_redirect(ccb, response, status_code);
            } else {
                handle_error_for_state(ccb, status_code);
            }

        } else if (method == sipMethodRefer) {
            ccsipCCB_t *other_ccb;

            




            
            clean_method_request_trx(ccb, sipMethodRefer, TRUE);

            other_ccb = sip_sm_get_ccb_by_target_call_id(ccb->con_call_id);
            if ((other_ccb != NULL) &&
                (other_ccb->state == SIP_STATE_SENT_INVITE)) {
                sipSPISendCancel(other_ccb);
                sip_cc_release(other_ccb->gsm_id, other_ccb->dn_line,
                               CC_CAUSE_NORMAL, NULL);
                sip_sm_change_state(other_ccb, SIP_STATE_RELEASE);
            }
            sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, CC_FEATURE_XFER,
                               NULL, CC_CAUSE_ERROR);

        } else if (method == sipMethodNotify) {
            clean_method_request_trx(ccb, sipMethodNotify, TRUE);
        } else {
            handle_error_for_state(ccb, status_code);
        }
        free_sip_message(response);
    }

}








void
ccsip_handle_sentinviteconnected_ev_cc_connected_ack (ccsipCCB_t *ccb,
                                                      sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_sentinviteconnected_ev_cc_connected_ack";

    



    if (sipSPISendAck(ccb, NULL) == FALSE) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipSPISendAck");
    }
    







    sip_sm_change_state(ccb, SIP_STATE_ACTIVE);
}










void
ccsip_handle_recvinvite_ev_cc_setup_ack (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    sip_sm_change_state(ccb, SIP_STATE_RECV_INVITE);
    ccb->dn_line = event->u.cc_msg->msg.setup_ack.line;
}


void
ccsip_handle_recvinvite_ev_cc_proceeding (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "recvinvite_ev_cc_proceeding";


    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      sip_util_event2string(event->type));

    sip_sm_change_state(ccb, SIP_STATE_RECV_INVITE_PROCEEDING);
}


void
ccsip_handle_recvinvite_ev_cc_alerting (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    sipSPISendInviteResponse180(ccb);
    sip_sm_change_state(ccb, SIP_STATE_RECV_INVITE_ALERTING);
}


void
ccsip_handle_recvinvite_ev_cc_connected (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "recvinvite_ev_cc_connected";
    int timer_h, timer_t1 = 500;

    (void) sip_platform_localexpires_timer_stop(ccb->index);

    CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"SIPSM %d: connected\n",
					  DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname), ccb->index);

    
    ccsip_save_local_msg_body(ccb, &event->u.cc_msg->msg.connected.msg_body);
    sipSPISendInviteResponse200(ccb);

    
    
    config_get_value(CFGID_TIMER_T1, &timer_t1, sizeof(timer_t1));
    timer_h = 64 * timer_t1;

    if (sip_platform_expires_timer_start(timer_h, ccb->index, NULL, 0) != SIP_OK) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED), fname,
                          "sip_platform_expires_timer_start(ACK Timer)");
    }

    sip_sm_change_state(ccb, SIP_STATE_RECV_INVITE_CONNECTED);
}



void
ccsip_handle_recvinvite_ev_expires_timer (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "recvinvite_ev_expires_timer";

    CCSIP_DEBUG_STATE(DEB_F_PREFIX"Sent 200OK but received no ACK\n",
        DEB_F_PREFIX_ARGS(SIP_ACK, fname));

    
    sip_platform_msg_timer_stop(ccb->index);

    
    
    sipSPISendBye(ccb, NULL, NULL);
    sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
    sip_sm_change_state(ccb, SIP_STATE_RELEASE);
}

void
ccsip_handle_recvinvite_ev_sip_ack (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "recvinvite_ev_sip_ack";
    sipMessage_t   *request;
    boolean         no_media = FALSE;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    sipsdp_status_t sdp_status;

    
    request = event->u.pSipMessage;

    
    if (sip_sm_request_check_and_store(ccb, request, sipMethodAck, FALSE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        free_sip_message(request);
        return;
    }
    
    (void) sip_platform_expires_timer_stop(ccb->index);

    ccb->authen.cred_type = 0;

    


    sdp_status = sip_util_extract_sdp(ccb, request);

    switch (sdp_status) {
    case SIP_SDP_SUCCESS:
    case SIP_SDP_SESSION_AUDIT:
        if (ccb->oa_state != OA_OFFER_SENT) {
            


            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                              ccb->index, ccb->dn_line, fname,
                              "Received OFFER SDP in ACK, releasing call");
            sipSPISendBye(ccb, NULL, NULL);
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            clean_method_request_trx(ccb, sipMethodInvite, FALSE);
            clean_method_request_trx(ccb, sipMethodAck, FALSE);
            return;
        } else {
            ccb->oa_state = OA_IDLE;
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"Using the SDP in INVITE\n",
                DEB_F_PREFIX_ARGS(SIP_ACK, fname));
        }
        break;

    case SIP_SDP_DNS_FAIL:
    case SIP_SDP_ERROR:
        sipSPISendBye(ccb, NULL, NULL);
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        clean_method_request_trx(ccb, sipMethodInvite, FALSE);
        clean_method_request_trx(ccb, sipMethodAck, FALSE);
        return;

    case SIP_SDP_NO_MEDIA:
    case SIP_SDP_NOT_PRESENT:
    default:
        no_media = TRUE;
        if (ccb->oa_state == OA_OFFER_SENT) {
            sipSPISendBye(ccb, NULL, NULL);
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            clean_method_request_trx(ccb, sipMethodInvite, FALSE);
            clean_method_request_trx(ccb, sipMethodAck, FALSE);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        } else {
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"Using the SDP in INVITE\n",
                DEB_F_PREFIX_ARGS(SIP_ACK, fname));
        }
    }

    




    ccsip_update_callinfo(ccb, request, TRUE, TRUE, TRUE);

    if (no_media) {
        sip_cc_connected_ack(ccb->gsm_id, ccb->dn_line, NULL);
    } else {
        sip_cc_connected_ack(ccb->gsm_id, ccb->dn_line, request);
    }

    
    if (ccb->wastransferred) {
        ccsipCCB_t *refererccb = NULL;
        cc_feature_data_t data;

        refererccb = sip_sm_get_ccb_by_callid(ccb->sipxfercallid);
        if (NULL != refererccb) {
            data.notify.cause = CC_CAUSE_OK;
            data.notify.cause_code = 200;
            data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
            data.notify.method = CC_XFER_METHOD_REFER;
            data.notify.blind_xferror_gsm_id = 0;
            sip_cc_feature(refererccb->gsm_id, refererccb->dn_line,
                           CC_FEATURE_NOTIFY, (void *) &data);
        }
        strlib_free(ccb->sipxfercallid);
        ccb->sipxfercallid = strlib_empty();
    }

    sip_sm_change_state(ccb, SIP_STATE_ACTIVE);
    clean_method_request_trx(ccb, sipMethodAck, FALSE);
    clean_method_request_trx(ccb, sipMethodInvite, FALSE);
}



void
ccsip_handle_recvinvite_ev_sip_2xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "recvinvite_ev_sip_2xx";
    sipMessage_t   *response;
    sipMethod_t     method = sipMethodInvalid;
    int             response_code = 0;

    
    response = event->u.pSipMessage;

    
    if (sipGetResponseCode(response, &response_code) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseCode");
        free_sip_message(response);
        return;
    }
    if (sipGetResponseMethod(response, &method) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseMethod");
        return;
    }
    if (response_code == SIP_ACCEPTED && method == sipMethodRefer) {
        ccsip_handle_accept_2xx(ccb, event);
        return;
    }
    free_sip_message(response);
    clean_method_request_trx(ccb, method, TRUE);
    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED), ccb->index,
                      ccb->dn_line, fname, sip_util_state2string(ccb->state));
}

void
ccsip_handle_disconnect_local (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "disconnect_local";
    char       *alsoString = NULL;


    



    if (ccb->state == SIP_STATE_SENT_INVITE_CONNECTED) {
        if (sipSPISendAck(ccb, NULL) == FALSE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipSPISendAck");
        }
    }
    
    
    if (ccb->state == SIP_STATE_RECV_INVITE_CONNECTED) {
        
        (void) sip_platform_expires_timer_stop(ccb->index);
        ccb->wait_for_ack = TRUE;
        ccb->send_delayed_bye = TRUE;
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
        return;
    }

    if (ccb->referto[0]) {
        alsoString = (char *) cpr_malloc(MAX_SIP_URL_LENGTH);
        if (alsoString == NULL) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "malloc(also string)");
        } else {
            sstrncpy(alsoString, ccb->referto, MAX_SIP_URL_LENGTH);
        }
    }

    
    ccb->authen.cred_type = 0;

    sipSPISendBye(ccb, alsoString, NULL);
    if (ccb->state == SIP_STATE_SENT_MIDCALL_INVITE) {
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_RESP_TIMEOUT, NULL);
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "MIDCALL INVITE TMR EXPIRED");
    } else {
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
    }
}


void
ccsip_handle_disconnect_media_change (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "ccsip_handle_disconnect_media_change";
    int             code;
    char           *phrase;
    char           *alsoString = NULL;
    cc_causes_t     cause = CC_CAUSE_NORMAL;

    cause = event->u.cc_msg->msg.release.cause;
    if (cause == CC_CAUSE_NO_MEDIA || cause == CC_CAUSE_PAYLOAD_MISMATCH) {
        


        code = ccsip_cc_to_sip_cause(cause, &phrase);
        if (ccb->state == SIP_STATE_RECV_MIDCALL_INVITE_CCFEATUREACK_PENDING) {
            sipSPISendInviteResponse(ccb, (uint16_t) code, phrase, 0, NULL,
                                     FALSE , TRUE );
            ccb->wait_for_ack = TRUE;
            ccb->send_delayed_bye = TRUE;
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
            return;
        } else {
            (void) sipSPISendUpdateResponse(ccb, FALSE, cause, FALSE);
        }
    }

    if (ccb->referto[0]) {
        alsoString = (char *) cpr_malloc(MAX_SIP_URL_LENGTH);
        if (alsoString == NULL) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname, "malloc(also string)");
        } else {
            sstrncpy(alsoString, ccb->referto, MAX_SIP_URL_LENGTH);
        }
    }

    
    ccb->authen.cred_type = 0;
    sipSPISendBye(ccb, alsoString, NULL);
    sip_sm_change_state(ccb, SIP_STATE_RELEASE);
    sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
}

void
ccsip_handle_disconnect_local_early (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "disconnect_local_early";

    





    


    if (event->type == E_CC_RELEASE) {
        if (event->u.cc_msg->msg.release.cause == CC_CAUSE_NO_USER_RESP) {
            ccb->early_transfer = TRUE;
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                              ccb->index, ccb->dn_line, fname,
                              "No action, Early Attended Transfer");
            return;
        }
    }

    



    if (ccb->flags & RECD_1xx) {
        sipSPISendCancel(ccb);
    } else {
        
        
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Received CC disconnect without 1xx from far side\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                          ccb->index, ccb->dn_line, fname,
                          "Stopping retx timer");
        sip_platform_msg_timer_stop(ccb->index);
        ccb->flags |= SEND_CANCEL;
    }

    if (ccb->blindtransferred == TRUE) {
        cc_feature_data_t data;

        data.notify.cause = CC_CAUSE_OK;
        data.notify.cause_code = SIP_CLI_ERR_REQ_CANCEL;
        data.notify.subscription = CC_SUBSCRIPTIONS_XFER;
        data.notify.method = CC_XFER_METHOD_REFER;
        data.notify.blind_xferror_gsm_id = sip_sm_get_blind_xfereror_ccb_by_gsm_id(ccb->gsm_id);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_NOTIFY, (void *)&data);
    } else if (ccb->wastransferred) {
        






        ccsipCCB_t *referccb = NULL;

        referccb = sip_sm_get_target_call_by_gsm_id(ccb->gsm_id);
        if (referccb != NULL) {
            ccb->flags |= FINAL_NOTIFY;
            (void) sipSPISendNotify(referccb, SIP_CLI_ERR_REQ_CANCEL);
            ccb->xfer_status = SIP_CLI_ERR_REQ_CANCEL;
        }
    }

    if (event->type == E_SIP_INV_EXPIRES_TIMER) {
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
    } else {
        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
    }

    sip_sm_change_state(ccb, SIP_STATE_RELEASE);
}

void
ccsip_handle_localexpires_timer (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    
    sipSPISendInviteResponse(ccb, SIP_CLI_ERR_BUSY_HERE,
                             SIP_CLI_ERR_BUSY_HERE_PHRASE,
                             0, NULL, FALSE,  TRUE );

    
    sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL, NULL);

    sip_sm_change_state(ccb, SIP_STATE_RELEASE);
}

static void
ccsip_cc_to_warning (cc_causes_t cause, char **warning_phrase,
                     uint16_t *warning_code)
{
    switch (cause) {

    case CC_CAUSE_PAYLOAD_MISMATCH:
        *warning_phrase = SIP_WARN_INCOMPAT_MEDIA_FORMAT_PHRASE;
        *warning_code = SIP_WARN_INCOMPAT_MEDIA_FORMAT;
        break;

    case CC_CAUSE_NO_MEDIA:
        *warning_phrase = SIP_WARN_MEDIA_TYPE_UNAVAIL_PHRASE;
        *warning_code = SIP_WARN_MEDIA_TYPE_UNAVAIL;
        break;

    default:
        *warning_phrase = NULL;
        *warning_code = 0;
    }

}


int
ccsip_cc_to_sip_cause (cc_causes_t cause, char **phrase)
{
    switch (cause) {

    case CC_CAUSE_OK:
        *phrase = SIP_SUCCESS_SETUP_PHRASE;
        return SIP_SUCCESS_SETUP;

    case CC_CAUSE_ERROR:
        *phrase = SIP_CLI_ERR_BAD_REQ_PHRASE;
        return SIP_CLI_ERR_BAD_REQ;

    case CC_CAUSE_UNASSIGNED_NUM:
        *phrase = SIP_CLI_ERR_NOT_FOUND_PHRASE;
        return SIP_CLI_ERR_NOT_FOUND;

    case CC_CAUSE_NO_RESOURCE:
        *phrase = SIP_SERV_ERR_INTERNAL_PHRASE;
        return SIP_SERV_ERR_INTERNAL;

    case CC_CAUSE_BUSY:
        *phrase = SIP_CLI_ERR_BUSY_HERE_PHRASE;
        return SIP_CLI_ERR_BUSY_HERE;

    case CC_CAUSE_ANONYMOUS:
        *phrase = SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED_PHRASE;
        return SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED;

    case CC_CAUSE_INVALID_NUMBER:
        *phrase = SIP_CLI_ERR_ADDRESS_PHRASE;
        return SIP_CLI_ERR_ADDRESS;

    case CC_CAUSE_PAYLOAD_MISMATCH:
        *phrase = SIP_CLI_ERR_NOT_ACCEPT_HERE_PHRASE;
        return SIP_CLI_ERR_NOT_ACCEPT_HERE;

    case CC_CAUSE_NO_REPLACE_CALL:
        *phrase = SIP_CLI_ERR_CALLEG_PHRASE;
        return SIP_CLI_ERR_CALLEG;

    case CC_CAUSE_REQUEST_PENDING:
        *phrase = SIP_CLI_ERR_REQ_PENDING_PHRASE;
        return SIP_CLI_ERR_REQ_PENDING;

    default:
        *phrase = SIP_CLI_ERR_BAD_REQ_PHRASE;
        return SIP_CLI_ERR_BAD_REQ;
    }
}

void
ccsip_handle_disconnect_local_unanswered (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    int             code;
    char           *phrase;
    uint16_t        warning_code = 0;
    char           *warning_phrase = NULL;

    ccb->dn_line = event->u.cc_msg->msg.release.line;
    code = ccsip_cc_to_sip_cause(event->u.cc_msg->msg.release.cause, &phrase);

    ccsip_cc_to_warning(event->u.cc_msg->msg.release.cause, &warning_phrase, &warning_code);

    
    sipSPISendInviteResponse(ccb, (uint16_t)code, phrase, warning_code,
                             warning_phrase,
                             FALSE  , TRUE  );

    sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
    ccb->wait_for_ack = TRUE;
    sip_sm_change_state(ccb, SIP_STATE_RELEASE);
}


void
ccsip_handle_disconnect_remote (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "disconnect_remote";
    sipMessage_t   *request;
    sipMessage_t   *store_invite_req;
    const char     *alsoString = NULL, *reasonHdr=NULL;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    sipMethod_t     method = sipMethodInvalid;
    cc_causes_t      cause = CC_CAUSE_NORMAL;

    memset(request_check_reason_phrase, 0, SIP_WARNING_LENGTH);

    
    request = event->u.pSipMessage;
    store_invite_req = ccb->last_request;

    sipGetRequestMethod(request, &method);

    
    if (sip_sm_request_check_and_store(ccb, request, method, TRUE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, TRUE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       request_check_reason_code,
                                       request_check_reason_phrase, NULL);
        free_sip_message(request);
        return;
    }

    
    (void) sip_platform_expires_timer_stop(ccb->index);

    
    
    if (ccb->con_call_id != CC_NO_CALL_ID) {
        
        
        
        
        
        
        
        
        
        
        ccsipCCB_t *other_ccb;

        other_ccb = sip_sm_get_ccb_by_target_call_id(ccb->con_call_id);
        if (other_ccb) {
            other_ccb->wastransferred = FALSE;
            strlib_free(other_ccb->sipxfercallid);
            other_ccb->sipxfercallid = strlib_empty();
        }
    }


    



    alsoString = sippmh_get_header_val(request, SIP_HEADER_ALSO, NULL);
    if (alsoString) {
        cc_feature_data_t data;

        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d Far end requested Call Transfer, "
                          "destination=<%s>\n",
						  DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
						  ccb->index, alsoString);
        sstrncpy(data.xfer.dialstring, alsoString, strlen(alsoString) + 1);
        data.xfer.cause  = CC_CAUSE_XFER_REMOTE;
        data.xfer.method = CC_XFER_METHOD_BYE;
        data.xfer.target_call_id = CC_NO_CALL_ID;
        ccb->referto = strlib_update(ccb->referto, alsoString);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_XFER, (void *)&data);
        (void) sipSPISendByeOrCancelResponse(ccb, request, sipMethodBye);
        free_sip_message(store_invite_req);
        return;
    } else {
        
        if (event->type == E_SIP_BYE) {
            



            ccb->flags |= RECD_BYE;
        } else {
            (void) sipSPISendByeOrCancelResponse(ccb, request, sipMethodCancel);
            
            
            reasonHdr = sippmh_get_header_val(request, SIP_HEADER_REASON, NULL);
            if ( reasonHdr && strcasestr(reasonHdr, "cause=200;") ) {
               cause = CC_SIP_CAUSE_ANSWERED_ELSEWHERE;
            }
        }

        if (SIP_SM_CALL_SETUP_RESPONDING(ccb) ||
            ccb->state == SIP_STATE_RECV_INVITE ||
            ccb->state == SIP_STATE_RECV_INVITE_CONNECTED) {
            



            sipGetRequestMethod(store_invite_req, &method);
            if (sip_sm_request_check_and_store(ccb, store_invite_req, method,
                                               TRUE, &request_check_reason_code,
                                               request_check_reason_phrase, FALSE) < 0) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                                  ccb->dn_line, fname,
                                  get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
                free_sip_message(store_invite_req);
                return;
            }
            sipSPISendInviteResponse(ccb, 487, SIP_CLI_ERR_REQ_CANCEL_PHRASE, 0,
                                     NULL, FALSE,  TRUE );
            ccb->wait_for_ack = TRUE;
        } else {
            free_sip_message(store_invite_req);
        }

        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        sip_cc_release(ccb->gsm_id, ccb->dn_line, cause, NULL);
    }
}


void
ccsip_handle_refer_sip_message (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "refer_sip_message";
    sipMessage_t   *request;
    const char     *contact = NULL;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    sipMethod_t     method = sipMethodInvalid;
    cc_feature_data_t data;
    char           *referToString[MAX_REFER_TO_HEADERS] = { NULL };
    char           *referByString;
    sipReferTo_t   *referto = NULL;
    sipReplaces_t  *replaces_t = NULL;
    int             noOfReferTo = 0;
    int             noOfReferBy = 0;
    char            tempreferto[MAX_SIP_URL_LENGTH];
    int             rpid_flag = RPID_DISABLED;
    char            tempreferby[MAX_SIP_URL_LENGTH];
    char           *semi_token = NULL;
    int             rcode;
    sipContact_t   *contact_info = NULL;

    memset(tempreferto, 0, MAX_SIP_URL_LENGTH);

    
    request = event->u.pSipMessage;

    
    ccb->featuretype = CC_FEATURE_BLIND_XFER;  

    sipGetRequestMethod(request, &method);

    
    if (get_method_request_trx_index(ccb, method, FALSE) > -1) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Received REFER while processing an old one!\n", fname);
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_REQ_PENDING,
                                       SIP_CLI_ERR_REQ_PENDING_PHRASE,
                                       0, NULL, NULL);
        free_sip_message(request);
        return;
    }


    
    rcode = sip_sm_request_check_and_store(ccb, request, method, TRUE,
                                           &request_check_reason_code,
                                           request_check_reason_phrase, FALSE);
    if (rcode < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));

        if (rcode == -2) { 
            (void) sipSPISendErrorResponse(request, SIP_SERV_ERR_INTERNAL,
                                           SIP_SERV_ERR_INTERNAL_PHRASE,
                                           request_check_reason_code,
                                           request_check_reason_phrase, NULL);
        } else {
            (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                           SIP_CLI_ERR_BAD_REQ_PHRASE,
                                           request_check_reason_code,
                                           request_check_reason_phrase, NULL);
        }
        free_sip_message(request);
        return;
    }
    contact = sippmh_get_cached_header_val(request, CONTACT);
    if (contact) {
        if (sipSPICheckContact(contact) < 0) { 
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "sipSPICheckContact()");
            (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                           SIP_CLI_ERR_BAD_REQ_PHRASE,
                                           SIP_WARN_MISC,
                                           SIP_CLI_ERR_BAD_REQ_CONTACT_FIELD,
                                           ccb);
            return;
        }
        contact_info = sippmh_parse_contact(contact);
        if (contact_info && contact_info->num_locations > 1) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Received REFER with multiple contacts!\n", fname);
            (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_AMBIGUOUS,
                                           SIP_CLI_ERR_AMBIGUOUS_PHRASE,
                                           SIP_WARN_MISC,
                                           SIP_WARN_REFER_AMBIGUOUS_PHRASE,
                                           ccb);
            sippmh_free_contact(contact_info);
            return;
        }
        if (contact_info) {
            sippmh_free_contact(contact_info);
        }
    } else { 
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       SIP_CLI_ERR_BAD_REQ_CONTACT_FIELD,
                                       ccb);
        return;
    }

    



    ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);

    
    
    noOfReferTo = sippmh_get_num_particular_headers(request, SIP_HEADER_REFER_TO,
                                                    SIP_C_HEADER_REFER_TO,
                                                    referToString, MAX_REFER_TO_HEADERS);

    if (noOfReferTo == 1) {
        referto = sippmh_parse_refer_to(referToString[0]);
    }

    if ((noOfReferTo == 0) || (noOfReferTo > 1) || (referto == NULL)) {

        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname,
                          "Incorrect number of Refer-To headers and/or value.\n");
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE_REFER_TO,
                                       ccb);

        if (referto) {
            cpr_free(referto);
        }
        return;
    }

    sipSPIGenerateTargetUrl(referto->targetUrl, tempreferto);
    ccb->sip_referTo = strlib_update(ccb->sip_referTo, tempreferto);
    if (referto->sip_replaces_hdr != NULL) {
        char tempreplace[MAX_SIP_URL_LENGTH];

        memset(tempreplace, 0, MAX_SIP_URL_LENGTH);
        sstrncpy(tempreplace, "Replaces=", sizeof(tempreplace));
        sstrncat(tempreplace, referto->sip_replaces_hdr, (sizeof(tempreplace) - sizeof("Replaces=")));
        replaces_t = sippmh_parse_replaces(tempreplace, FALSE);
        if (NULL != replaces_t) {
            ccb->sipxfercallid = strlib_update(ccb->sipxfercallid, replaces_t->callid);
            if (ccb->sipxfercallid) {
                ccb->sipxfercallid = strlib_append(ccb->sipxfercallid, ";to-tag=");
                if (ccb->sipxfercallid) {
                    ccb->sipxfercallid = strlib_append(ccb->sipxfercallid, replaces_t->toTag);
                }
                if (ccb->sipxfercallid) {
                    ccb->sipxfercallid = strlib_append(ccb->sipxfercallid, ";from-tag=");
                }
                if (ccb->sipxfercallid) {
                    ccb->sipxfercallid = strlib_append(ccb->sipxfercallid, replaces_t->fromTag);
                }
            }
            ccb->featuretype = CC_FEATURE_XFER;
            sippmh_free_replaces(replaces_t);
        } else {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                              ccb->dn_line, fname,
                              "replaces header in referto missing callid or to/from tags");
            (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                           SIP_CLI_ERR_BAD_REQ_PHRASE,
                                           SIP_WARN_MISC,
                                           SIP_CLI_ERR_BAD_REQ_PHRASE_REFER_TO,
                                           ccb);
            sippmh_free_refer_to(referto);
            return;
        }
    }
#ifdef SIP_ACC_CONT
    if (referto->sip_acc_cont != NULL) {
        
        
        
        if (ccb->refer_acc_cont != NULL) {
            cpr_free(ccb->refer_acc_cont);
            ccb->refer_acc_cont = NULL;
        }
        ccb->refer_acc_cont = cpr_strdup(referto->sip_acc_cont);
    }
#endif

    if (referto->sip_proxy_auth != NULL) {
        if (ccb->refer_proxy_auth != NULL) {
            
            cpr_free(ccb->refer_proxy_auth);
            ccb->refer_proxy_auth = NULL;
        }
        
        ccb->refer_proxy_auth = cpr_strdup(referto->sip_proxy_auth);
    }

    noOfReferBy = sippmh_get_num_particular_headers(request,
                                                    SIP_HEADER_REFERRED_BY,
                                                    SIP_C_HEADER_REFERRED_BY,
                                                    &referByString,
                                                    MAX_REFER_BY_HEADERS);

    if (noOfReferBy == 0) {

        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          "Missing referred by header\n");
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE_REFER_BY,
                                       ccb);
        sippmh_free_refer_to(referto);
        return;
    }

    if (noOfReferTo > 1 || noOfReferBy > 1) {

        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          "Multiple referto or refer by headers found in message");
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE_REFER,
                                       ccb);
        sippmh_free_refer_to(referto);
        return;
    }

    config_get_value(CFGID_REMOTE_PARTY_ID, &rpid_flag, sizeof(rpid_flag));

    if ((rpid_flag == RPID_ENABLED) && (ccb->best_rpid)
        && (cpr_strcasecmp(ccb->best_rpid->privacy, PRIVACY_OFF) != 0)) {
        snprintf(tempreferby, MAX_SIP_URL_LENGTH, "\"%s\" <sip:%s@%s>",
                 SIP_HEADER_ANONYMOUS_STR,
                 ccb->best_rpid->loc->genUrl->u.sipUrl->user,
                 ccb->best_rpid->loc->genUrl->u.sipUrl->host);
        ccb->sip_referredBy = strlib_update(ccb->sip_referredBy, tempreferby);
    } else {
        ccb->sip_referredBy = strlib_update(ccb->sip_referredBy, referByString);
    }

    sstrncpy(data.xfer.dialstring, referToString[0], sizeof(data.xfer.dialstring));
    semi_token = strchr(data.xfer.dialstring, '?');
    if (semi_token) {
        *semi_token++ = '>';
        *semi_token = 0;
    }
    if (data.xfer.dialstring[0] != '<') {
        semi_token = strchr(data.xfer.dialstring, '>');
        if (semi_token) {
            *semi_token = 0;
        }
    }

    
    if (unescape_UserInfo(data.xfer.dialstring, tempreferto, MAX_SIP_URL_LENGTH)) {
        memset(data.xfer.dialstring, 0, CC_MAX_DIALSTRING_LEN);
        sstrncpy(data.xfer.dialstring, tempreferto, CC_MAX_DIALSTRING_LEN);
    }

    data.xfer.method = CC_XFER_METHOD_REFER;
    data.xfer.target_call_id = CC_NO_CALL_ID;
    data.xfer.cause = CC_CAUSE_XFER_REMOTE;
    if (ccb->featuretype == CC_FEATURE_BLIND_XFER) {
        sstrncpy(data.xfer.referred_by, ccb->sip_referredBy,
                 MAX_SIP_URL_LENGTH);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_BLIND_XFER,
                       (void *) &data);
    } else {
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_XFER,
                       (void *) &data);
    }
    sippmh_free_refer_to(referto);
}

void
ccsip_handle_process_in_call_options_request (ccsipCCB_t *ccb,
                                              sipSMEvent_t *event)
{
    const char     *fname = "ccsip_handle_process_in_call_options_request";
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    sipMessage_t   *request;
    sipMethod_t     method = sipMethodInvalid;

    CCSIP_DEBUG_STATE(DEB_F_PREFIX"Processing within-dialog OPTIONS request\n",
        DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));

    
    request = event->u.pSipMessage;

    ccb->featuretype = CC_FEATURE_NONE;

    
    sipGetRequestMethod(request, &method);
    if (sip_sm_request_check_and_store(ccb, request, method, TRUE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       request_check_reason_code,
                                       request_check_reason_phrase, NULL);
        free_sip_message(request);
        return;
    }
    
    
    
    ccb->last_request = NULL;
    sip_cc_options(ccb->gsm_id, ccb->dn_line, event->u.pSipMessage);
}

void
ccsip_handle_ev_cc_answer_options_request (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "ccsip_handle_ev_cc_answer_options_request";
    cc_options_sdp_ack_t *options_sdp_ack;

    options_sdp_ack = (cc_options_sdp_ack_t *) event->u.cc_msg;

    if (!ccb) {
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Processing OPTIONS (out of dialog) "
                          "request(GSM has responded)\n", DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
        (void) sipSPIsendNonActiveOptionResponse((sipMessage_t *) options_sdp_ack->pMessage,
                                                 &options_sdp_ack->msg_body);

        
        
        free_sip_message((sipMessage_t *)options_sdp_ack->pMessage);
        options_sdp_ack->pMessage = NULL;
    } else {
        
        
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Processing OPTIONS (in dialog) "
                          "request(GSM has responded)\n", DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));

        
        ccsip_save_local_msg_body(ccb, &options_sdp_ack->msg_body);
        (void) sipSPISendOptionResponse(ccb, event->u.pSipMessage);
    }
}

void
ccsip_handle_ev_cc_answer_audit_request (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    cc_audit_sdp_ack_t *audit_ack;


    audit_ack = (cc_audit_sdp_ack_t *) event->u.cc_msg;
    


    ccsip_save_local_msg_body(ccb, &audit_ack->msg_body);
    sipSPISendInviteResponse200(ccb);
}

int
sip_sm_process_event (sipSMEvent_t *pEvent)
{
    const char     *fname = "sip_sm_process_event";
    ccsipCCB_t     *ccb;
    sipSMAction_t   event_handler = H_INVALID_EVENT;

    ccb = pEvent->ccb;
    if (!ccb) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"ccb is null. Unable to process event "
                          "<%d>\n", fname, pEvent->type);
        return (-1);
    }

    
    UNBIND_UDP_ICMP_HANDLER(ccb->udpId);

    



    if ((int) (ccb->index) > TEL_CCB_END) {
        
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"illegal line number: %d\n",
                          fname, ccb->index);
        return (-1);
    }

    if (!sip_config_check_line(ccb->dn_line)) {
        
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"illegal directory number: %d\n",
                          fname, ccb->dn_line);
        return (-1);
    }

    if ((event_handler = get_handler_index(ccb->state, pEvent->type))
            != H_INVALID_EVENT) {
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Processing SM event: %d: --0x%08lx--%21s: %s <- %s\n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_EVT, ccb->dn_line, ccb->gsm_id, fname),
						  ccb->index, EVENT_ACTION_SM(event_handler),
                          "", sip_util_state2string(ccb->state),
                          sip_util_event2string(pEvent->type));

        EVENT_ACTION_SM(event_handler) (ccb, pEvent);
    } else {
        
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"illegal state/event pair: "
                          "(%d <-- %d)\n", fname, ccb->state, pEvent->type);
        return (-1);
    }

    return (0);
}











void
sip_extract_kfactor_stats (cc_msg_t *pCCMsg, ccsipCCB_t *ccb)
{
    if (pCCMsg->msg.release.msg_id == CC_MSG_RELEASE) {
        ccb->kfactor_ptr = &pCCMsg->msg.release.kfactor;
    } else if (pCCMsg->msg.release_complete.msg_id == CC_MSG_RELEASE_COMPLETE) {
        ccb->kfactor_ptr = &pCCMsg->msg.release_complete.kfactor;
    } else if ((pCCMsg->msg.feature.msg_id == CC_MSG_FEATURE) ||
               (pCCMsg->msg.feature.msg_id == CC_MSG_FEATURE_ACK)) {
        if (pCCMsg->msg.feature.feature_id == CC_FEATURE_HOLD) {
            ccb->kfactor_ptr = &pCCMsg->msg.feature.data.hold.kfactor;
        } else if (pCCMsg->msg.feature.feature_id == CC_FEATURE_MEDIA) {
            ccb->kfactor_ptr = &pCCMsg->msg.feature.data.resume.kfactor;
        } else {
            ccb->kfactor_ptr = NULL;
        }
    } else {
        ccb->kfactor_ptr = NULL;
    }
}
int
sip_sm_process_cc_event (cprBuffer_t buf)
{
    const char     *fname = "sip_sm_process_cc_event";
    sipSMEvent_t    sip_sm_event;
    line_t          idx = 0;
    sipSMAction_t   event_handler = H_INVALID_EVENT;
    cc_msg_t       *pCCMsg = (cc_msg_t *) buf;

    memset(&sip_sm_event, 0, sizeof(sipSMEvent_t));

    sip_sm_event.u.cc_msg = pCCMsg;
    sip_sm_event.type = sip_util_ccevent2sipccevent(pCCMsg->msg.setup.msg_id);
    if (pCCMsg->msg.setup.msg_id == CC_MSG_SETUP) {
        sip_sm_event.ccb = sip_sm_get_ccb_next_available(&idx);
        if (!sip_sm_event.ccb) {
            sip_cc_release(pCCMsg->msg.setup.call_id, pCCMsg->msg.setup.line,
                           CC_CAUSE_ERROR, NULL);
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"No free lines available\n",
                DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
            cc_free_msg_data(sip_sm_event.u.cc_msg);
            cpr_free(pCCMsg);
            return SIP_OK;
        }
    } else if (pCCMsg->msg.setup.msg_id == CC_MSG_OPTIONS_ACK) {
        



        sip_sm_event.ccb = sip_sm_get_ccb_by_gsm_id(pCCMsg->msg.setup.call_id);
        ccsip_handle_ev_cc_answer_options_request(sip_sm_event.ccb, &sip_sm_event);
        cc_free_msg_data(sip_sm_event.u.cc_msg);
        cpr_free(pCCMsg);
        return (SIP_OK);
    } else if (ccsip_handle_cc_select_event(&sip_sm_event)) {
        


        cc_free_msg_data(sip_sm_event.u.cc_msg);
        cpr_free(pCCMsg);
        return SIP_OK;
    } else if (ccsip_handle_cc_b2bjoin_event(&sip_sm_event)) {
        


        cc_free_msg_data(sip_sm_event.u.cc_msg);
        cpr_free(pCCMsg);
        return SIP_OK;
    } else if (ccsip_handle_cc_hook_event(&sip_sm_event) == TRUE) {
        


        cc_free_msg_data(sip_sm_event.u.cc_msg);
        cpr_free(pCCMsg);
        return SIP_OK;
    } else {
        sip_sm_event.ccb = sip_sm_get_ccb_by_gsm_id(pCCMsg->msg.setup.call_id);
        if (!sip_sm_event.ccb) {
            
            
            if (pCCMsg->msg.setup.msg_id == CC_MSG_FEATURE) {
                
                
                
                sip_cc_release(pCCMsg->msg.setup.call_id, pCCMsg->msg.setup.line,
                               CC_CAUSE_ERROR, NULL);
            } else if (pCCMsg->msg.setup.msg_id == CC_MSG_RELEASE) {
                sip_cc_release_complete(pCCMsg->msg.setup.call_id,
                                        pCCMsg->msg.setup.line, CC_CAUSE_ERROR);
            } else {
                sip_cc_release(pCCMsg->msg.setup.call_id, pCCMsg->msg.setup.line,
                               CC_CAUSE_ERROR, NULL);
            }
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"No ccb with matching gsm_id = <%d>\n",
                DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname),
                             pCCMsg->msg.setup.call_id);
            cc_free_msg_data(sip_sm_event.u.cc_msg);
            cpr_free(pCCMsg);
            return SIP_OK;
        }
    }

    if (pCCMsg->msg.setup.msg_id == CC_MSG_AUDIT_ACK) {
        



        ccsip_handle_ev_cc_answer_audit_request(sip_sm_event.ccb, &sip_sm_event);
        cc_free_msg_data(sip_sm_event.u.cc_msg);
        cpr_free(pCCMsg);
        return (SIP_OK);
    }

    
    if (sip_sm_event.ccb->state == SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING) {
        
        line_t line_index = (line_t) -1;

        sip_sm_event.ccb = sip_sm_get_ccb_next_available(&line_index);
        if (!sip_sm_event.ccb) {
            if (pCCMsg->msg.setup.msg_id == CC_MSG_RELEASE) {
                sip_cc_release_complete(pCCMsg->msg.setup.call_id,
                                        pCCMsg->msg.setup.line, CC_CAUSE_ERROR);
            } else {
                sip_cc_release(pCCMsg->msg.setup.call_id, pCCMsg->msg.setup.line,
                               CC_CAUSE_ERROR, NULL);
            }
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sip_sm_get_ccb_next_available()"
                              " returned null.\n", fname);
            cc_free_msg_data(sip_sm_event.u.cc_msg);
            cpr_free(pCCMsg);
            return SIP_OK;
        }
        
        sip_sm_event.ccb->dn_line = 1;
    }

    sip_extract_kfactor_stats(pCCMsg, sip_sm_event.ccb);
    event_handler = get_handler_index(sip_sm_event.ccb->state, sip_sm_event.type);
    if (event_handler != H_INVALID_EVENT) {

        DEF_DEBUG(DEB_L_C_F_PREFIX"Processing CC event: %-6d: SM: %s <- %s\n",
                        DEB_L_C_F_PREFIX_ARGS(SIP_EVT, sip_sm_event.ccb->dn_line,
                        sip_sm_event.ccb->gsm_id, fname),
                        (long)buf,
                        sip_util_state2string(sip_sm_event.ccb->state),
                        sip_util_event2string(sip_sm_event.type));

        EVENT_ACTION_SM(event_handler)(sip_sm_event.ccb, &sip_sm_event);
    } else {
        
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"illegal state/event pair: (%d <-- %d)\n",
                          fname, sip_sm_event.ccb->state, sip_sm_event.type);
        cc_free_msg_data(sip_sm_event.u.cc_msg);
        cpr_free(pCCMsg);
        return SIP_ERROR;
    }
    sip_sm_event.ccb->kfactor_ptr = NULL;
    cc_free_msg_data(sip_sm_event.u.cc_msg);
    cpr_free(pCCMsg);
    return SIP_OK;
}

void
ccsip_handle_send_blind_notify (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "send_blind_notify";
    cc_features_t   feature_type;

    feature_type = event->u.cc_msg->msg.feature.feature_id;
    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      cc_feature_name(feature_type));

    if (CC_FEATURE_NOTIFY == feature_type) {
        if (event->u.cc_msg->msg.feature.data.notify.final == TRUE) {
            ccb->flags |= FINAL_NOTIFY;
        }
        (void) sipSPISendNotify(ccb, event->u.cc_msg->msg.feature.data.notify.cause_code);
        ccb->xfer_status = event->u.cc_msg->msg.feature.data.notify.cause_code;
    } else {
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type, NULL,
                           CC_CAUSE_ERROR);
    }
}

void
sip_sm_call_cleanup (ccsipCCB_t *ccb)
{
    const char     *fname = "sip_sm_call_cleanup";
    int             i = 0;

    if (ccb == NULL) {
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Null CCB passed into function.\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
        return;
    }

    




    if (ccb->dn_line == 0) {
        return;
    }

    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                      ccb->index, ccb->dn_line, fname, "Cleaning up the call");

    



    if (ccb->dup_flags & DUP_CCB) {
        free_duped(ccb);
    }

    
    (void) sip_platform_localexpires_timer_stop(ccb->index);

    



    if ((int) (ccb->index) <= TEL_CCB_END) {
        (void) sip_platform_supervision_disconnect_timer_stop(ccb->index);
        submanager_update_ccb_addr(ccb);
    }

    
    UNBIND_UDP_ICMP_HANDLER(ccb->udpId);

    
    if (ccb->SRVhandle != NULL) {
        dnsFreeSrvHandle(ccb->SRVhandle);
        ccb->SRVhandle = NULL;
    }

    
    if (ccb->ObpSRVhandle != NULL) {
        dnsFreeSrvHandle(ccb->ObpSRVhandle);
        ccb->ObpSRVhandle = NULL;
    }

    


    if ((int) (ccb->index) <= TEL_CCB_END) {
        (void) sip_platform_expires_timer_stop(ccb->index);
        (void) sip_platform_register_expires_timer_stop(ccb->index);
    
        memcpy(gCallHistory[ccb->index].last_call_id, ccb->sipCallID,
            MAX_SIP_CALL_ID);
    }

    
    cc_free_msg_body_parts(&ccb->local_msg_body);

    if (ccb->contact_info) {
        sippmh_free_contact(ccb->contact_info);
        ccb->contact_info = NULL;
    }
    if (ccb->record_route_info) {
        sippmh_free_record_route(ccb->record_route_info);
        ccb->record_route_info = NULL;
    }

    
    if (ccb->last_request) {
        free_sip_message(ccb->last_request);
        ccb->last_request = NULL;
    }

    
    for (i = 0; i < MAX_DIVERSION_HEADERS; i++) {
        if (ccb->diversion[i]) {
            cpr_free(ccb->diversion[i]);
            ccb->diversion[i] = NULL;
        }
    }

    sippmh_free_diversion_info(ccb->div_info);
    ccb->div_info = NULL;
    ccb->call_type = CC_CALL_NONE;

    
    if (ccb->redirect_info) {
        sippmh_free_contact(ccb->redirect_info->sipContact);
        cpr_free(ccb->redirect_info);
        ccb->redirect_info = NULL;
    }

    ccb->best_rpid = NULL;
    sippmh_free_remote_party_id_info(ccb->rpid_info);
    ccb->rpid_info = NULL;


    ccb->authen.cnonce[0] = '\0';
    if (ccb->authen.authorization != NULL) {
        cpr_free(ccb->authen.authorization);
        ccb->authen.authorization = NULL;
    }
    if (ccb->refer_proxy_auth != NULL) {
        cpr_free(ccb->refer_proxy_auth);
        ccb->refer_proxy_auth = NULL;
    }
#ifdef SIP_ACC_CONT
    if (ccb->refer_acc_cont != NULL) {
        cpr_free(ccb->refer_acc_cont);
        ccb->refer_acc_cont = NULL;
    }
#endif

    if (ccb->authen.sip_authen != NULL) {
        sippmh_free_authen(ccb->authen.sip_authen);
        ccb->authen.sip_authen = NULL;
    }

    ccb->hold_initiated = FALSE;
    ccb->wastransferred = FALSE;
    ccb->blindtransferred = FALSE;
    ccb->gsm_id = CC_NO_CALL_ID;
    ccb->con_call_id = CC_NO_CALL_ID;
    ccb->blind_xfer_call_id = CC_NO_CALL_ID;
    ccb->xfer_status = 0;

    if (ccb->old_session_id != NULL) {
        cpr_free(ccb->old_session_id);
    }
    if (ccb->old_version_id != NULL) {
        cpr_free(ccb->old_version_id);
    }
    ccb->old_session_id = NULL;
    ccb->old_version_id = NULL;

    ccb->displayCalledNumber = TRUE;
    ccb->displayCallingNumber = TRUE;

    

    strlib_free(ccb->calledDisplayedName);
    strlib_free(ccb->callingNumber);
	strlib_free(ccb->altCallingNumber);
    strlib_free(ccb->callingDisplayName);
    strlib_free(ccb->calledNumber);
    strlib_free(ccb->ReqURIOriginal);
    strlib_free(ccb->sip_from);
    strlib_free(ccb->sip_to);
    strlib_free(ccb->sip_to_tag);
    strlib_free(ccb->sip_from_tag);
    strlib_free(ccb->sip_contact);
    strlib_free(ccb->sip_reqby);
    strlib_free(ccb->sip_require);
    strlib_free(ccb->sip_unsupported);
    strlib_free(ccb->referto);
    strlib_free(ccb->sip_referTo);
    strlib_free(ccb->sip_referredBy);
    strlib_free(ccb->sipxfercallid);
    strlib_free(ccb->sip_remote_party_id);

    if (ccb->in_call_info) {
        ccsip_free_call_info_header(ccb->in_call_info);
        ccb->in_call_info = NULL;
    }
    if (ccb->out_call_info) {
        ccsip_free_call_info_header(ccb->out_call_info);
        ccb->out_call_info = NULL;
    }
    if (ccb->join_info) {
        sippmh_free_join_info(ccb->join_info);
        ccb->join_info = NULL;
    }
    if (ccb->feature_data) {
        cpr_free(ccb->feature_data);
        ccb->feature_data = NULL;
    }

    for (i = 0; i < MAX_REQ_OUTSTANDING; i++) {
        ccb->sent_request[i].cseq_number = CCSIP_START_CSEQ;
        ccb->sent_request[i].cseq_method = sipMethodInvalid;
        strlib_free(ccb->sent_request[i].u.sip_via_branch);
        strlib_free(ccb->sent_request[i].sip_via_sentby);
        ccb->recv_request[i].cseq_number = CCSIP_START_CSEQ;
        ccb->recv_request[i].cseq_method = sipMethodInvalid;
        strlib_free(ccb->recv_request[i].u.sip_via_header);
        strlib_free(ccb->recv_request[i].sip_via_sentby);
    }

    if (ccb->index >= REG_CCB_START) {
        



        (void) sip_sm_ccb_init(ccb, ccb->index, ccb->dn_line,
                               SIP_REG_STATE_IDLE);
        return;
    }

    if (sip_platform_msg_timer_outstanding_get(ccb->index)) {
        sip_sm_change_state(ccb, SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING);
        (void) sip_sm_ccb_init(ccb, ccb->index, ccb->dn_line,
                               SIP_REG_STATE_IDLE );
    } else {
        sip_sm_change_state(ccb, SIP_STATE_IDLE);
        (void) sip_sm_ccb_init(ccb, ccb->index, 1, SIP_REG_STATE_IDLE);
    }

}


int
sip_sm_ccb_init (ccsipCCB_t *ccb, line_t ccb_index, int DN,
                 sipRegSMStateType_t initial_state)
{
    int nat_enable = 0;
    uint8_t i;

    



    if ((int) ccb_index <= TEL_CCB_END) {
        memset(ccb, 0, sizeof(ccsipCCB_t));
    }

    ccb->dup_flags = DUP_NO_FLAGS;
    ccb->mother_ccb = NULL;
    sip_reg_sm_change_state(ccb, initial_state); 
    ccb->index = ccb_index;
    ccb->hold_initiated = FALSE;
    ccb->wastransferred = FALSE;
    ccb->blindtransferred = FALSE;
    ccb->callingNumber = strlib_empty();
    ccb->calledNumberFirstDigitDialed = FALSE;
    ccb->calledNumber = strlib_empty();
    ccb->altCallingNumber = strlib_empty();
    ccb->calledDisplayedName = strlib_empty();
    ccb->callingDisplayName = strlib_empty();
    ccb->displayCalledNumber = TRUE;
    ccb->displayCallingNumber = TRUE;
    ccb->calledNumberLen = 0;
    ccb->ReqURIOriginal = strlib_empty();
    ccb->sip_to_tag = strlib_empty();
    ccb->sip_from_tag = strlib_empty();
    ccb->sip_contact = strlib_empty();
    ccb->sip_reqby = strlib_empty();
    ccb->sip_require = strlib_empty();
    ccb->sip_unsupported = strlib_empty();
    ccb->sip_remote_party_id = strlib_empty();
    ccb->flags = 0;
    ccb->avt.payload_type = RTP_NONE;
    ccb->alert_info = ALERTING_NONE;
    ccb->alerting_ring = VCM_INSIDE_RING;
    ccb->wait_for_ack = FALSE;
    ccb->send_delayed_bye = FALSE;
    ccb->retx_flag = FALSE;
    ccb->early_transfer = FALSE;
    ccb->redirect_info = NULL;
    ccb->proxySelection = SIP_PROXY_DEFAULT;
    ccb->outBoundProxyAddr = ip_addr_invalid;
    ccb->outBoundProxyPort = 0;
    ccb->oa_state = OA_IDLE;
    ccb->call_type = CC_CALL_NONE;
    ccb->cc_cfg_table_entry = NULL;
    ccb->first_pass_3xx = TRUE;

    
    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        sip_config_get_net_device_ipaddr(&(ccb->src_addr));
    } else {
        sip_config_get_nat_ipaddr(&(ccb->src_addr));
    }

    config_get_value(CFGID_VOIP_CONTROL_PORT, &ccb->local_port,
                     sizeof(ccb->local_port));

    if ((int) ccb_index <= TEL_CCB_END) {
        ccb->type = SIP_CALL_CCB;
        ccb->dn_line = (line_t) DN;
        sipTransportGetServerIPAddr(&(ccb->dest_sip_addr), ccb->dn_line);
        ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
        ccb->sipCallID[0] = '\0';
    } else if ((ccb_index >= REG_CCB_START) && (ccb_index <= REG_CCB_END)) {
        ccb->type = SIP_REG_CCB;
        ccb->dn_line = ccb_index - MAX_TEL_LINES + 1;
        sip_regmgr_set_cc_info(ccb_index, ccb->dn_line, &ccb->cc_type,
                               (void *)&ccb->cc_cfg_table_entry);
        if (ccb->cc_type == CC_CCM) {
            



            sipTransportGetServerIPAddr(&(ccb->dest_sip_addr), ccb->dn_line);
            ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
        } else {
            


            ccb->dest_sip_addr = ip_addr_invalid;
            ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
        }
    } else if (ccb_index == REG_BACKUP_CCB) {
        ccb->type = SIP_REG_CCB;
        ccb->dn_line = REG_BACKUP_DN;
        sip_regmgr_set_cc_info(ccb_index, ccb->dn_line, &ccb->cc_type,
                               &ccb->cc_cfg_table_entry);
        if (ccb->cc_type != CC_CCM) {
            


            sipTransportGetBkupServerAddress(&ccb->dest_sip_addr, ccb->dn_line, ccb->reg.proxy);
            ccb->dest_sip_port = sipTransportGetBkupServerPort(ccb->dn_line);
        }
    } else if ((ccb_index >= REG_FALLBACK_CCB_START) &&
               (ccb_index <= REG_FALLBACK_CCB_END)) {
        





        ccb->type = SIP_REG_CCB;
        ccb->dn_line = REG_BACKUP_DN;
    }
    




    ccb->local_port = sipTransportGetListenPort(ccb->dn_line, ccb);
    
    memset(&ccb->local_msg_body, 0, sizeof(cc_msgbody_info_t));
    
    
    ccb->old_session_id = NULL;
    ccb->old_version_id = NULL;

    
    ccb->ReqURI[0] = '\0';
    ccb->sip_from  = strlib_empty();   
    ccb->sip_to    = strlib_empty();   

    

    ccb->sip_referTo = strlib_empty();
    ccb->sip_referredBy = strlib_empty();
    ccb->referto = strlib_empty();
    ccb->sipxfercallid = strlib_empty();
    ccb->featuretype = CC_FEATURE_NONE;
    ccb->join_info = NULL;
    ccb->feature_data = NULL;

    for (i = 0; i < MAX_REQ_OUTSTANDING; i++) {
        ccb->sent_request[i].cseq_number = CCSIP_START_CSEQ;
        ccb->sent_request[i].cseq_method = sipMethodInvalid;
        ccb->sent_request[i].u.sip_via_branch = strlib_empty();
        ccb->sent_request[i].sip_via_sentby = strlib_empty();
        ccb->recv_request[i].cseq_number = CCSIP_START_CSEQ;
        ccb->recv_request[i].cseq_method = sipMethodInvalid;
        ccb->recv_request[i].u.sip_via_header = strlib_empty();
        ccb->recv_request[i].sip_via_sentby = strlib_empty();
    }

    ccb->last_recv_request_cseq        = 0;
    ccb->last_recv_request_cseq_method = sipMethodInvalid;

    if (ccb_index < REG_CCB_START) {
        
        ccb->last_used_cseq = CCSIP_START_CSEQ;
    } else {
        
        
        if (ccb->last_used_cseq == 0) {
            ccb->last_used_cseq = CCSIP_START_CSEQ;
        }
    }

    ccb->last_request = NULL;

    ccb->xfr_inprogress = SIP_SM_NO_XFR;


    ccb->gsm_id = CC_NO_CALL_ID;
    ccb->con_call_id = CC_NO_CALL_ID;
    ccb->blind_xfer_call_id = CC_NO_CALL_ID;
    ccb->xfer_status = 0;

    
    config_get_value(CFGID_DTMF_AVT_PAYLOAD, &ccb->avt.payload_type,
                     sizeof(ccb->avt.payload_type));

    
    ccb->reg.registered = 0;
    ccb->reg.tmr_expire = 0;
    ccb->reg.act_time = 0;
    ccb->reg.rereg_pending = 0;

    
    ccb->authen.retries_401_407 = 0;
    ccb->authen.cred_type = 0;
    ccb->authen.authorization = NULL;
    ccb->authen.status_code = 0;
    ccb->authen.nc_count = 0;
    ccb->authen.new_flag = FALSE;
    ccb->in_call_info = NULL;
    ccb->out_call_info = NULL;
    ccb->udpId = NULL;
    ccb->callref = 0;

    return (0);
}



const char *
sip_util_state2string (sipSMStateType_t state)
{
    switch (state) {
    case SIP_STATE_NONE:
        return ("SIP_STATE_NONE");

    case SIP_STATE_IDLE:
        return ("SIP_STATE_IDLE");

    case SIP_STATE_SENT_INVITE:
        return ("SIP_STATE_SENT_INVITE");

    case SIP_STATE_SENT_INVITE_CONNECTED:
        return ("SIP_STATE_SENT_INVITE_CONNECTED");

    case SIP_STATE_RECV_INVITE:
        return ("SIP_STATE_RECV_INVITE");

    case SIP_STATE_RECV_INVITE_PROCEEDING:
        return ("SIP_STATE_RECV_INVITE_PROCEEDING");

    case SIP_STATE_RECV_INVITE_ALERTING:
        return ("SIP_STATE_RECV_INVITE_ALERTING");

    case SIP_STATE_RECV_INVITE_CONNECTED:
        return ("SIP_STATE_RECV_INVITE_CONNECTED");

    case SIP_STATE_ACTIVE:
        return ("SIP_STATE_ACTIVE");

    case SIP_STATE_RECV_MIDCALL_INVITE_CCFEATUREACK_PENDING:
        return ("SIP_STATE_RECV_MIDCALL_INVITE_CCFEATUREACK_PENDING");

    case SIP_STATE_RECV_MIDCALL_INVITE_SIPACK_PENDING:
        return ("SIP_STATE_RECV_MIDCALL_INVITE_SIPACK_PENDING");

    case SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING:
        return ("SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING");

    case SIP_STATE_RELEASE:
        return ("SIP_STATE_RELEASE");

    case SIP_STATE_BLIND_XFER_PENDING:
        return ("SIP_STATE_BLIND_XFER_PENDING");

    case SIP_STATE_SENT_OOD_REFER:
        return ("SIP_STATE_SENT_OOD_REFER");

    case SIP_STATE_RECV_UPDATEMEDIA_CCFEATUREACK_PENDING:
        return ("SIP_STATE_RECV_UPDATEMEDIA_CCFEATUREACK_PENDING");

    case SIP_STATE_SENT_MIDCALL_INVITE:
        return ("SIP_STATE_SENT_MIDCALL_INVITE");

    default:
        return ("UNKNOWN STATE");
    }
}


const char *
sip_util_event2string (sipSMEventType_t event)
{
    switch (event) {
    case E_SIP_INVITE:
        return ("E_SIP_INVITE");
    case E_SIP_ACK:
        return ("E_SIP_ACK");
    case E_SIP_BYE:
        return ("E_SIP_BYE");
    case E_SIP_CANCEL:
        return ("E_SIP_CANCEL");
    case E_SIP_1xx:
        return ("E_SIP_1xx");
    case E_SIP_2xx:
        return ("E_SIP_2xx");
    case E_SIP_3xx:
        return ("E_SIP_3xx");
    case E_SIP_FAILURE_RESPONSE:
        return ("E_SIP_FAILURE_RESPONSE");
    case E_SIP_REFER:
        return ("E_SIP_REFER");
    case E_CC_SETUP:
        return ("E_CC_SETUP");
    case E_CC_SETUP_ACK:
        return ("E_CC_SETUP_ACK");
    case E_CC_PROCEEDING:
        return ("E_CC_PROCEEDING");
    case E_CC_ALERTING:
        return ("E_CC_ALERTING");
    case E_CC_CONNECTED:
        return ("E_CC_CONNECTED");
    case E_CC_CONNECTED_ACK:
        return ("E_CC_CONNECTED_ACK");
    case E_CC_RELEASE:
        return ("E_CC_RELEASE");
    case E_CC_RELEASE_COMPLETE:
        return ("E_CC_RELEASE_COMPLETE");
    case E_CC_FEATURE:
        return ("E_CC_FEATURE");
    case E_CC_FEATURE_ACK:
        return ("E_CC_FEATURE_ACK");
    case E_CC_CAPABILITIES:
        return ("E_CC_CAPABILITIES");
    case E_CC_CAPABILITIES_ACK:
        return ("E_CC_CAPABILITIES_ACK");
    case E_CC_SUBSCRIBE:
        return ("E_CC_SUBSCRIBE");
    case E_CC_INFO:
        return ("E_CC_INFO");
    case E_SIP_INV_EXPIRES_TIMER:
        return ("E_SIP_INV_EXPIRES_TIMER");
    case E_SIP_INV_LOCALEXPIRES_TIMER:
        return ("E_SIP_INV_LOCALEXPIRES_TIMER");
    case E_SIP_SUPERVISION_DISCONNECT_TIMER:
        return ("E_SIP_SUPERVISION_DISCONNECT_TIMER");
    case E_SIP_TIMER:
        return ("E_SIP_TIMER");
    case E_SIP_OPTIONS:
        return ("E_SIP_OPTIONS");
    case E_SIP_UPDATE:
        return ("E_SIP_UPDATE");
    case E_SIP_UPDATE_RESPONSE:
        return ("E_SIP_UPDATE_RESPONSE");
    case E_SIP_GLARE_AVOIDANCE_TIMER:
        return ("E_SIP_GLARE_AVOIDANCE_TIMER");
    case E_SIP_ICMP_UNREACHABLE:
        return "E_SIP_ICMP_UNREACHABLE";
    default:
        return ("UNKNOWN EVENT");
    }
}


sipSMEventType_t
sip_util_ccevent2sipccevent (cc_msgs_t cc_msg_id)
{
    switch (cc_msg_id) {
    case CC_MSG_SETUP:
        return (E_CC_SETUP);
    case CC_MSG_SETUP_ACK:
        return (E_CC_SETUP_ACK);
    case CC_MSG_PROCEEDING:
        return (E_CC_PROCEEDING);
    case CC_MSG_ALERTING:
        return (E_CC_ALERTING);
    case CC_MSG_CONNECTED:
        return (E_CC_CONNECTED);
    case CC_MSG_CONNECTED_ACK:
        return (E_CC_CONNECTED_ACK);
    case CC_MSG_RELEASE:
        return (E_CC_RELEASE);
    case CC_MSG_RELEASE_COMPLETE:
        return (E_CC_RELEASE_COMPLETE);
    case CC_MSG_FEATURE:
        return (E_CC_FEATURE);
    case CC_MSG_FEATURE_ACK:
        return (E_CC_FEATURE_ACK);
    case CC_MSG_INFO:
        return (E_CC_INFO);
    default:
        return ((sipSMEventType_t) (-1));
    }
}


void
sip_create_new_sip_call_id (char *sipCallID, uint8_t *mac_address, char *pSrcAddrStr)
{
    static uint16_t count = 1;

    count++;

    if (sipCallID == NULL) {
        return;
    }
    snprintf(sipCallID, MAX_SIP_CALL_ID, "%.4x%.4x-%.4x%.4x-%.8x-%.8x@%s",
             mac_address[0] * 256 + mac_address[1],
             mac_address[2] * 256 + mac_address[3],
             mac_address[4] * 256 + mac_address[5],
             count, (unsigned int)cpr_rand(), (unsigned int)cpr_rand(), pSrcAddrStr);
}

void
sip_util_get_new_call_id (ccsipCCB_t *ccb)
{
    const char     *fname = "sip_util_get_new_call_id";
    uint8_t         mac_address[MAC_ADDRESS_LENGTH];
    char            pSrcAddrStr[MAX_IPADDR_STR_LEN];
    char           *temp_call_id;

    memset(pSrcAddrStr, 0, MAX_IPADDR_STR_LEN);

    
    if (!ccb) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args Check: ccb is null\n", fname);
        return;
    }

    
    if ((ccb->type == SIP_REG_CCB) && (ccb->sipCallID[0] != 0)) {
        return;
    }

    


    if (ccb->type != SIP_REG_CCB) {
        temp_call_id = ccsip_find_preallocated_sip_call_id(ccb->dn_line);
        if (temp_call_id != NULL) {
            sstrncpy(ccb->sipCallID, temp_call_id, MAX_SIP_CALL_ID);
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"using pre allocated call ID\n",
                DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
            ccsip_free_preallocated_sip_call_id(ccb->dn_line);
            return;
        }
    }
    ipaddr2dotted(pSrcAddrStr, &ccb->src_addr);

    platform_get_wired_mac_address(mac_address);

    sip_create_new_sip_call_id(ccb->sipCallID, mac_address, pSrcAddrStr);
}


void
sip_util_make_tag (char *pTagBuf)
{
    const char     *fname = "sip_util_make_tag";
    uint8_t         mac_address[MAC_ADDRESS_LENGTH];
    static uint16_t count = 1;

    if (!pTagBuf) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args Check: pTagBuf is null\n", fname);
        return;
    }

    platform_get_wired_mac_address(mac_address);
    count++;

    snprintf(pTagBuf, MAX_SIP_URL_LENGTH, "%.4x%.4x%.4x%.4x%.8x-%.8x",
             mac_address[0] * 256 + mac_address[1],
             mac_address[2] * 256 + mac_address[3],
             mac_address[4] * 256 + mac_address[5],
             count, (unsigned int)cpr_rand(), (unsigned int)cpr_rand());
}

int
sip_sm_init (void)
{
    line_t          i;
    const char     *fname = "sip_sm_init";
    int            sdpmode = 0;

	config_get_value(CFGID_SDPMODE, &sdpmode, sizeof(sdpmode));

	if (!sdpmode) {

        if (ccsip_register_init() == SIP_ERROR) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"registration initialization failed\n", fname);
            return SIP_ERROR;
        }

        if (ccsip_info_package_handler_init() == SIP_ERROR) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"info package initialization failed\n", fname);
            return SIP_ERROR;
        }

        


        if (sip_platform_timers_init() == SIP_ERROR) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"timer initialization failed\n", fname);
            return SIP_ERROR;
        }

        if (sipTransportInit() != SIP_OK) {
            return SIP_ERROR;
        }

        DEF_DEBUG(DEB_F_PREFIX"Disabling mass reg state", DEB_F_PREFIX_ARGS(SIP_REG, fname));
        for (i = 0; i < MAX_CCBS; i++) {
            if (i == 0 || i == (MAX_CCBS-1)) {
                g_disable_mass_reg_debug_print = FALSE;
            } else {
                g_disable_mass_reg_debug_print = TRUE;
            }
            sip_sm_call_cleanup(&(gGlobInfo.ccbs[i]));
            if (sip_sm_ccb_init(&(gGlobInfo.ccbs[i]), i, 1, SIP_REG_STATE_IDLE) < 0) {
                return SIP_ERROR;
            }
        }
        g_disable_mass_reg_debug_print = FALSE;

        
        sip_platform_msg_timers_init();

        
        if (sip_subsManager_init() != SIP_OK) {
            return SIP_ERROR;
        }

    }

    
    if (!sip_sdp_init()) {
        
        return (SIP_ERROR);
    }

    return SIP_OK;
}

void
ccsip_handle_sip_shutdown ()
{
    const char     *fname = "handle_sip_shutdown";
    ccsipCCB_t     *ccb = NULL;
    line_t          i;

    for (i = TEL_CCB_START; i <= TEL_CCB_END; i++) {
        ccb = sip_sm_get_ccb_by_index(i);
        if (ccb) {
            switch (ccb->state) {
            case SIP_STATE_RECV_INVITE:
            case SIP_STATE_RECV_INVITE_PROCEEDING:
            case SIP_STATE_RECV_INVITE_ALERTING:
                CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Received invite: %d: STATE: %s\n",
                                  DEB_L_C_F_PREFIX_ARGS(SIP_STATE, ccb->dn_line, ccb->gsm_id, fname),
								  ccb->index, sip_util_state2string(ccb->state));
                sipSPISendInviteResponse(ccb, SIP_SERV_ERR_INTERNAL,
                                         SIP_SERV_ERR_INTERNAL_PHRASE, 0, NULL,
                                         FALSE  , FALSE );
                sip_cc_release_complete(ccb->gsm_id, ccb->dn_line,
                                        CC_CAUSE_FACILITY_REJECTED);
                ccb->wait_for_ack = FALSE;
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                sip_sm_call_cleanup(ccb);
                break;

            case SIP_STATE_RECV_INVITE_CONNECTED:
            case SIP_STATE_ACTIVE:
            default:
                CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Clearing %d STATE: %s\n",
                                  DEB_L_C_F_PREFIX_ARGS(SIP_STATE, ccb->dn_line, ccb->gsm_id, fname),
								  ccb->index, sip_util_state2string(ccb->state));
                sipSPISendBye(ccb, NULL, NULL);
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_call_cleanup(ccb);
                break;

            case SIP_STATE_SENT_INVITE:
                CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Sent invite: Clearing %d STATE: %s\n",
                                 DEB_L_C_F_PREFIX_ARGS(SIP_STATE,  ccb->dn_line, ccb->gsm_id, fname),
								 ccb->index, sip_util_state2string(ccb->state));
                sipSPISendCancel(ccb);
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                sip_cc_release_complete(ccb->gsm_id, ccb->dn_line,
                                        CC_CAUSE_FACILITY_REJECTED);
                sip_sm_call_cleanup(ccb);
                break;

            case SIP_STATE_SENT_INVITE_CONNECTED:
                CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Sent invite connected: Clearing %d STATE: %s\n",
                                  DEB_L_C_F_PREFIX_ARGS(SIP_STATE, ccb->dn_line, ccb->gsm_id, fname),
								  ccb->index, sip_util_state2string(ccb->state));
                if (sipSPISendAck(ccb, NULL) == FALSE) {
                    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                      fname, "sipSPISendAck");
                }
                sipSPISendBye(ccb, NULL, NULL);
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_call_cleanup(ccb);
                break;

            case SIP_STATE_RELEASE:
                CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"Release: Clearing %d STATE: %s\n",
                                  DEB_L_C_F_PREFIX_ARGS(SIP_STATE, ccb->dn_line, ccb->gsm_id, fname),
								  ccb->index, sip_util_state2string(ccb->state));
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                sip_cc_release_complete(ccb->gsm_id, ccb->dn_line,
                                        CC_CAUSE_FACILITY_REJECTED);
                sip_sm_call_cleanup(ccb);
                break;

            case SIP_STATE_IDLE:
                break;
            }

        }
    }
}

void
sip_shutdown (void)
{

    DEF_DEBUG(DEB_F_PREFIX"SIP Shutting down...\n", DEB_F_PREFIX_ARGS(SIP_TASK, "sip_shutdown"));

    
    if (sip.taskInited == FALSE) {
        return;
    }

    sip.taskInited = FALSE;
    DEF_DEBUG(DEB_F_PREFIX" sip.taskInited is set to false\n", DEB_F_PREFIX_ARGS(SIP_TASK, "sip_shutdown"));


    if ((PHNGetState() == STATE_CONNECTED) ||
        (PHNGetState() == STATE_DONE_LOADING) ||
        (PHNGetState() == STATE_CFG_UPDATE)) {

        
        ccsip_handle_sip_shutdown();

        
        sip_regmgr_shutdown();

        
        sip_platform_timers_shutdown();

        
        (void) sip_subsManager_shut();
        
        publish_reset();

        
        sipTransportShutdown();
        ccsip_remove_wlan_classifiers();
    }

    ccsip_info_package_handler_shutdown();
}
































void
sip_restart (void)
{
    const char       *fname = "sip_restart";

    DEF_DEBUG(DEB_F_PREFIX"In sip_restart\n", DEB_F_PREFIX_ARGS(SIP_CTRL, fname));
    if (sip_sm_init() < 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sip_sm_init failed\n", fname);
        return;
    }
    sip_platform_init();
    sip.taskInited = TRUE;
    DEF_DEBUG(DEB_F_PREFIX"sip.taskInited is set to true \n", DEB_F_PREFIX_ARGS(SIP_CTRL, fname));
    sip_mode_quiet = FALSE;
    sip_reg_all_failed = FALSE;
    ccsip_remove_wlan_classifiers();

    
    cc_fail_fallback_gsm(CC_SRC_SIP, CC_RSP_COMPLETE, CC_REG_FAILOVER_RSP);
}

void
sip_shutdown_phase2 (int action)
{
    DEF_DEBUG(DEB_F_PREFIX"(%d)\n",
                     DEB_F_PREFIX_ARGS(SIP_CTRL, "sip_shutdown_phase2"), action);
    sip.taskInited = TRUE; 
    DEF_DEBUG(DEB_F_PREFIX"sip.taskInited is set to true\n", DEB_F_PREFIX_ARGS(SIP_CTRL, "sip_shutdown_phase2"));
    sip_shutdown();
    if (action == SIP_EXTERNAL || action == SIP_STOP) {
        shutdownCCAck(action);
    } else if (action == SIP_INTERNAL) {
        
        sip_restart();
    }
}

void
sip_shutdown_phase1 (int action, int reason)
{
    DEF_DEBUG(DEB_F_PREFIX"In sip_shutdown_phase1 (%d)\n",
                     DEB_F_PREFIX_ARGS(SIP_CTRL, "sip_shutdown_phase1"), action);
    if (sip_reg_all_failed) {
        
        sip_shutdown_phase2(action);
    } else {
        
        ccsip_register_cancel(TRUE, TRUE);
        
        (void) sip_platform_unregistration_timer_start(2000, (boolean) action);
    }
}

ccsipCCB_t *
sip_sm_get_ccb_by_ccm_id_and_index (int ccmid, line_t idx)
{
    static const char fname[] = "sip_sm_get_ccb_by_ccm_id_and_index";
    fallback_ccb_t *fallback_ccb;
    ccsipCCB_t * ccb = NULL;
    CCM_ID ccm_id = ccmid;

    if (ccm_id >= MAX_CCM) {
        CCSIP_DEBUG_ERROR(DEB_F_PREFIX"invalid ccm_id=%d "
            "ccb_index=%d\n",DEB_F_PREFIX_ARGS(SIP_BRANCH, fname),
            ccm_id , idx);
        return ccb;
    }

    if ((int) idx < MAX_CCBS) {
        ccb = &(gGlobInfo.ccbs[idx]);
    }
    


    if (ccb == NULL) {
        fallback_ccb = sip_regmgr_get_fallback_ccb_by_index(idx);
        if (fallback_ccb != NULL) {
            ccb = fallback_ccb->ccb;
        }
    }
    if (ccb != NULL) {
        if ((ccb->cc_cfg_table_entry == NULL) ||
            (((ti_config_table_t *)(ccb->cc_cfg_table_entry))->ti_specific.ti_ccm.ccm_id != ccm_id)) {
            






            DEF_DEBUG(DEB_F_PREFIX"ccb index has moved or cfg_table not initialized for the cucm=%s. "
                "index=%d ccb=%d. Throwing away the msg.\n",DEB_F_PREFIX_ARGS(SIP_BRANCH, fname),
                CCM_ID_PRINT(ccm_id), idx, ccb);
            ccb = NULL;
        }
    }
    if (ccb == NULL) {
        CCSIP_DEBUG_ERROR(DEB_F_PREFIX"Could not find ccb ccb_index=%d",
             DEB_F_PREFIX_ARGS(SIP_BRANCH, fname), idx);
    }
    return ccb;
}

ccsipCCB_t *
sip_sm_get_ccb_by_index (line_t idx)
{
    fallback_ccb_t *fallback_ccb;

    if ((int) idx < MAX_CCBS) {
        return &(gGlobInfo.ccbs[idx]);
    }
    


    fallback_ccb = sip_regmgr_get_fallback_ccb_by_index(idx);
    if (fallback_ccb != NULL) {
        return (fallback_ccb->ccb);
    }
    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_LINE_NUMBER_INVALID), "sip_sm_get_ccb_by_index",
                      idx);
    return NULL;
}


ccsipCCB_t *
sip_sm_get_ccb_by_callid (const char *callid)
{
    line_t i;

    if (callid[0] == '\0') {
        
        return (NULL);
    }
    for (i = 0; i < MAX_CCBS; i++) {
        if (strcmp(callid, gGlobInfo.ccbs[i].sipCallID) == 0) {
            return &(gGlobInfo.ccbs[i]);
        }
    }

    return NULL;
}

callid_t
sip_sm_get_blind_xfereror_ccb_by_gsm_id (callid_t gsm_id)
{
    uint16_t i;

    for (i = 0; i < MAX_CCBS; i++) {
        if (gsm_id == gGlobInfo.ccbs[i].blind_xfer_call_id) {
            return gGlobInfo.ccbs[i].gsm_id;
        }
    }

    return CC_NO_CALL_ID;
}


ccsipCCB_t *
sip_sm_get_ccb_by_target_call_id (callid_t con_id)
{
    uint16_t i;

    for (i = 0; i < MAX_CCBS; i++) {
        if (con_id == gGlobInfo.ccbs[i].gsm_id) {
            return &(gGlobInfo.ccbs[i]);
        }
    }

    return NULL;
}

ccsipCCB_t *
sip_sm_get_target_call_by_gsm_id (callid_t gsm_id)
{
    uint16_t i;

    for (i = 0; i < MAX_CCBS; i++) {
        if (gsm_id == gGlobInfo.ccbs[i].con_call_id) {
            return &(gGlobInfo.ccbs[i]);
        }
    }

    return NULL;
}

ccsipCCB_t *
sip_sm_get_target_call_by_con_call_id (callid_t con_call_id)
{
    uint16_t i;

    for (i = 0; i < MAX_CCBS; i++) {
        if (con_call_id == gGlobInfo.ccbs[i].gsm_id) {
            return &(gGlobInfo.ccbs[i]);
        }
    }

    return NULL;
}

ccsipCCB_t *
sip_sm_get_ccb_next_available (line_t *line_number)
{
    line_t i;

    for (i = TEL_CCB_START; i <= TEL_CCB_END; i++) {
        if (gGlobInfo.ccbs[i].state == SIP_STATE_IDLE) {
            *line_number = i;
            break;
        }
    }

    if (i > TEL_CCB_END) {
        return NULL;
    }
    return &(gGlobInfo.ccbs[i]);
}












ccsipCCB_t *
sip_sm_get_ccb_by_gsm_id (callid_t gsm_id)
{
    line_t i;
    ccsipCCB_t *dupCCB = NULL;

    if ( gsm_id == CC_NO_CALL_ID )
        return NULL;

    for (i = 0; i < MAX_CCBS; i++) {
        if (gGlobInfo.ccbs[i].gsm_id == gsm_id) {
             if ( gGlobInfo.ccbs[i].dup_flags & DUP_CCB ) {
                 dupCCB = &(gGlobInfo.ccbs[i]);
             } else {
                 return &(gGlobInfo.ccbs[i]);
             }
        }
    }

    return dupCCB;
}



















static boolean
sip_sm_ccb_match_branch_cseq (ccsipCCB_t *ccb,
                              sipCseq_t  *sipCseq,
                              sipVia_t   *via_this)
{
    const char       *fname = "sip_sm_ccb_match_branch_cseq";
    int16_t          trx_index = -1;
    sipTransaction_t *trx = NULL;

    trx_index = get_method_request_trx_index(ccb, sipCseq->method,
                                             TRUE);
    if (trx_index != -1) {
        
        trx = &(ccb->sent_request[trx_index]);
        if ((trx->cseq_number == sipCseq->number) &&
            (trx->u.sip_via_branch[0] != '\0') &&
            (via_this->branch_param != NULL) &&
            (strncmp(trx->u.sip_via_branch, via_this->branch_param,
			         VIA_BRANCH_LENGTH) == 0)) {
            CCSIP_DEBUG_STATE(DEB_F_PREFIX"Matched branch_id & CSeq\n", DEB_F_PREFIX_ARGS(SIP_BRANCH, fname));
            return (TRUE);
        } else {
            CCSIP_DEBUG_ERROR(SIP_L_C_F_PREFIX"Mismatched CSeq or"
                              " Via's branch parameter in response:"
                              "ccb=0x%x,%d, cseq(trx,msg)=(%d,%d),"
                              "branch(trx,msg)=(%s,%s)\n",
                               ccb->dn_line, ccb->gsm_id, fname, ccb,
                               ccb->index, trx->cseq_number, sipCseq->number,
                               trx->u.sip_via_branch,
                               via_this->branch_param);
            return (FALSE);
        }
    }

    CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Method index not found\n", fname);
    return (FALSE);
}




uint16_t
sip_sm_determine_ccb (const char *callid,
                      sipCseq_t * sipCseq,
                      sipMessage_t *pSipMessage,
                      boolean is_request,
                      ccsipCCB_t **ccb_ret)
{

    const char     *fname = "sip_sm_determine_ccb";
    const char     *to = NULL;
    sipLocation_t  *to_loc = NULL;
    line_t          i;
    ccsipCCB_t     *ccb = NULL;
    sipReqLine_t   *requestURI = NULL;
    genUrl_t       *genUrl = NULL;
    sipUrl_t       *sipUriUrl = NULL;
    char           *pUser = NULL;
    char            reqURI[MAX_SIP_URL_LENGTH];
    int16_t         trx_index = -1;
    sipTransaction_t *trx = NULL;
    sipVia_t       *via_this = NULL;
    sipVia_t       *via_last = NULL;
    const char     *pViaHeaderStr = NULL;
    boolean        match = FALSE;

    *ccb_ret = NULL;

    
    
    to = sippmh_get_cached_header_val(pSipMessage, TO);
    if (to) {
        to_loc = sippmh_parse_from_or_to((char *)to, TRUE);
        if (to_loc) {
            if (to_loc->tag) {
                for (i = 0; i < MAX_CCBS; i++) {
                    if (strcmp(callid, gGlobInfo.ccbs[i].sipCallID) == 0) {
                        ccb = &(gGlobInfo.ccbs[i]);
                        if (ccb->sip_to_tag[0] != '\0') {
                            if (strcmp(to_loc->tag, ccb->sip_to_tag) == 0) {
                                *ccb_ret = ccb;
                                CCSIP_DEBUG_STATE(DEB_F_PREFIX"Matched to_tag\n",
                                    DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
                                break;
                            } else if (strcmp(to_loc->tag, ccb->sip_from_tag) == 0) {
                                *ccb_ret = ccb;
                                CCSIP_DEBUG_STATE(DEB_F_PREFIX"Matched from_tag\n",
                                    DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
                                break;
                            }
                        }
                    }
                }
            }
            sippmh_free_location(to_loc);
        }
    }

    
    pViaHeaderStr = sippmh_get_cached_header_val(pSipMessage, VIA);
    if (pViaHeaderStr) {
        via_this = sippmh_parse_via(pViaHeaderStr);
    }
    if (!pViaHeaderStr || !via_this) {
        return (SIP_CLI_ERR_BAD_REQ);
    }

    
    
    if ((*ccb_ret == NULL) && is_request) {
        reqURI[0] = '\0';
        requestURI = sippmh_get_request_line(pSipMessage);
        if (requestURI) {
            if (requestURI->url) {
                genUrl = sippmh_parse_url(requestURI->url, TRUE);
                if (genUrl) {
                    if (genUrl->schema == URL_TYPE_SIP) {
                        sipUriUrl = genUrl->u.sipUrl;
                        if (sipUriUrl) {
                            pUser = sippmh_parse_user(sipUriUrl->user);
                            if (pUser) {
                                sstrncpy(reqURI, pUser, sizeof(reqURI));
                                cpr_free(pUser);
                            } else {
                                sstrncpy(reqURI, sipUriUrl->user, sizeof(reqURI));
                            }
                        }
                    }
                    sippmh_genurl_free(genUrl);
                }
            }
            SIPPMH_FREE_REQUEST_LINE(requestURI);
        }

        for (i = 0; i < MAX_CCBS; i++) {
            if (strcmp(callid, gGlobInfo.ccbs[i].sipCallID) == 0) {
                ccb = &(gGlobInfo.ccbs[i]);
                if (ccb->ReqURI[0] != '\0') {
                    if (strcmp(ccb->ReqURI, reqURI) == 0) {
                        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Matched reqURI\n",
                            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
                        *ccb_ret = ccb;
                        break;
                    }
                }
            }
        }
        if (*ccb_ret == NULL) {
            for (i = 0; i < MAX_CCBS; i++) {
                if (strcmp(callid, gGlobInfo.ccbs[i].sipCallID) == 0) {
                    ccb = &(gGlobInfo.ccbs[i]);
                    if ((sipCseq->method == sipMethodInvite) &&
                        (ccb->state < SIP_STATE_ACTIVE)) {
                        
                        
                        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Matched Call-id - not active.\n",
                            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
                        *ccb_ret = ccb;
                        break;
                    }
                    if (((sipCseq->method == sipMethodCancel) &&
                        (ccb->state < SIP_STATE_ACTIVE)) ||
                        ((sipCseq->method == sipMethodAck) &&
                        (ccb->state == SIP_STATE_RELEASE))) {
                        
                        
                        
                        
                        
                        
                        
                        trx_index = get_method_request_trx_index(ccb,
                                                                 sipMethodInvite,
                                                                 FALSE);

                        if (trx_index != -1) {
                            const char *toString;
                            const char *fromString;

                            toString = sippmh_get_cached_header_val(pSipMessage, TO);
                            fromString = sippmh_get_cached_header_val(pSipMessage, FROM);

                            trx = &(ccb->recv_request[trx_index]);

                            if (trx->u.sip_via_header[0] != '\0') {
                                pViaHeaderStr = (char *) (trx->u.sip_via_header);
                                via_last = sippmh_parse_via(pViaHeaderStr);
                            }

                            if (fromString && toString) {
                                if ((strcmp(ccb->sip_from, fromString) == 0) &&
                                    (strncmp(ccb->sip_to, toString, strlen(toString)) == 0) &&
                                    (trx->cseq_number == sipCseq->number) &&
                                    (via_last && (via_last->branch_param != NULL)) &&
                                    (via_this->branch_param != NULL) &&
                                    (strcmp(via_last->branch_param, via_this->branch_param) == 0)) {
                                    CCSIP_DEBUG_STATE(DEB_F_PREFIX"Matched branch_id & CSeq for CANCEL/ACK\n",
                                                      DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
                                    *ccb_ret = ccb;
                                    sippmh_free_via(via_last);
                                    via_last = NULL;
                                    break;
                                }
                            }
                            if (via_last) {
                                sippmh_free_via(via_last);
                                via_last = NULL;
                            }
                        }
                    }
                }
            }
        }
    }

    
    
    
    
    if ((*ccb_ret != NULL) && is_request) {
        ccb = *ccb_ret;
        trx_index = get_method_request_trx_index(ccb, sipCseq->method, FALSE);
        if (trx_index != -1) {
            if (ccb->recv_request[trx_index].u.sip_via_header[0] != '\0') {
                pViaHeaderStr = (char *) (ccb->recv_request[trx_index].u.sip_via_header);
                via_last = sippmh_parse_via(pViaHeaderStr);
            }
            if (via_last) {
                if (sipCseq->number == ccb->recv_request[trx_index].cseq_number) {
                    if (via_this->branch_param && via_last->branch_param) {
                        if (strcmp(via_this->branch_param,
                                   via_last->branch_param)) {
                            
                            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Found Merged Request\n", fname);
                            sippmh_free_via(via_this);
                            sippmh_free_via(via_last);
                            return (SIP_CLI_ERR_LOOP_DETECT);
                        }
                    }
                }
                sippmh_free_via(via_last);
            }
        }

    }
    
    
    
    if ((*ccb_ret == NULL) && !is_request) {

        for (i = 0; i < MAX_CCBS; i++) {
            if (strcmp(callid, gGlobInfo.ccbs[i].sipCallID) == 0) {
                ccb = &(gGlobInfo.ccbs[i]);
                match = sip_sm_ccb_match_branch_cseq(ccb, sipCseq,
                                                     via_this);
                sippmh_free_via(via_this);
                if (match) {
                     *ccb_ret = ccb;
                     return (0);
                 } else {
                     return (SIP_CLI_ERR_NOT_ACCEPT);
                 }
            }
        }
        




        sip_regmgr_find_fallback_ccb_by_callid(callid, ccb_ret);
    }

    



    if ((*ccb_ret != NULL) && !is_request) {
    	match = sip_sm_ccb_match_branch_cseq(*ccb_ret, sipCseq,
                                             via_this);
        sippmh_free_via(via_this);
    	if (match) {
            return (0);
        } else {
            return (SIP_CLI_ERR_NOT_ACCEPT);
        }
    }

    sippmh_free_via(via_this);
    return (0);
}

void
ccsip_handle_default (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_default";

    CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d No action -> %s\n",
                      DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                      ccb->index, sip_util_state2string(ccb->state));
}


void
ccsip_handle_default_sip_message (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char   *fname = "default_sip_message";
    sipMessage_t *msg = NULL;
    int16_t       trx_index = -1;

    msg = event->u.pSipMessage;

    if (event->type == E_SIP_ACK) {
        
        
        clean_method_request_trx(ccb, sipMethodInvite, FALSE);
    } else if (event->type == E_SIP_INVITE) {
        
        
        trx_index = get_method_request_trx_index(ccb, sipMethodInvite, FALSE);
        if (trx_index != -1) {
            (void) sipSPISendErrorResponse(msg, SIP_SERV_ERR_INTERNAL,
                                           SIP_SERV_ERR_INTERNAL_PHRASE,
                                           SIP_WARN_PROCESSING_PREVIOUS_REQUEST,
                                           "Earlier INVITE being processed",
                                           NULL);
        }
    } else if (event->type == E_SIP_UPDATE) {
        
        
        trx_index = get_method_request_trx_index(ccb, sipMethodUpdate, FALSE);
        if (trx_index != -1) {
            (void) sipSPISendErrorResponse(msg, SIP_SERV_ERR_INTERNAL,
                                           SIP_SERV_ERR_INTERNAL_PHRASE,
                                           SIP_WARN_PROCESSING_PREVIOUS_REQUEST,
                                           "Earlier UPDATE being processed",
                                           NULL);
        }

    } else if (event->type == E_SIP_CANCEL) {
        (void) sipSPISendErrorResponse(msg, SIP_CLI_ERR_CALLEG,
                                       SIP_CLI_ERR_CALLEG_PHRASE, 0, NULL, ccb);
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: Sent 481 (CANCEL) %s\n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                          ccb->index, sip_util_state2string(ccb->state));
    }

    
    if (msg) {
        free_sip_message(msg);
    }

    CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: No action -> %s\n",
                      DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                      ccb->index, sip_util_state2string(ccb->state));
}















void
ccsip_handle_default_ev_cc_feature (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "ccsip_handle_default_ev_cc_feature";
    cc_features_t   feature_type;

    feature_type = event->u.cc_msg->msg.feature.feature_id;

    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                      ccb->index, ccb->dn_line, fname);
    




    sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type, NULL,
                       CC_CAUSE_ERROR);
}















void
ccsip_handle_default_recvreq_ack_pending_ev_cc_feature (ccsipCCB_t *ccb,
                                                        sipSMEvent_t *event)
{
    const char *fname =
        "ccsip_handle_default_recvreq_ack_pending_ev_cc_feature";
    cc_features_t feature_type;

    feature_type = event->u.cc_msg->msg.feature_ack.feature_id;

    switch (feature_type) {
    case CC_FEATURE_RESUME:
    case CC_FEATURE_HOLD:
    case CC_FEATURE_MEDIA:
        




        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type,
                           NULL, CC_CAUSE_REQUEST_PENDING);
        break;

    case CC_FEATURE_SELECT:
    case CC_FEATURE_CANCEL:
        break;
    default:
        
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type,
                           NULL, CC_CAUSE_ERROR);
        break;
    }
}













boolean
sip_is_releasing (ccsipCCB_t *ccb)
{
    if (ccb != NULL) {
        
        if (ccb->state == SIP_STATE_RELEASE) {
            return (TRUE);
        }
    }
    return (FALSE);
}

void
ccsip_handle_default_sip_response (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "default_sip_response";
    sipMessage_t   *response;
    int             response_code = 0;

    
    response = event->u.pSipMessage;

    
    if (sipGetResponseCode(response, &response_code) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseCode");
        free_sip_message(response);
        return;
    }

    
    if ((!sip_sm_is_invite_response(response)) || (response_code < 200)) {
        free_sip_message(response);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED), ccb->index,
                          ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        return;
    }

    if (sipSPISendAck(ccb, response) == FALSE) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipSPISendAck");
    }
    
    free_sip_message(response);
}








void
ccsip_restart_reTx_timer (ccsipCCB_t *ccb, sipMethod_t messageType)
{
    const char     *fname = "ccsip_restart_reTx_timer";
    uint32_t        time_t1 = 0;
    uint32_t        time_t2 = 0;
    uint32_t        timeout = 0;

    
    config_get_value(CFGID_TIMER_T1, &time_t1, sizeof(time_t1));
    timeout = time_t1 * (1 << ccb->retx_counter);
    
    if (messageType != sipMethodInvite) {
        config_get_value(CFGID_TIMER_T2, &time_t2, sizeof(time_t2));
        if (timeout > time_t2) {
            timeout = time_t2;
        }
    }
    CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: Restarting timer (%d msec)"
                      " (msg is %s)\n",
					  DEB_L_C_F_PREFIX_ARGS(SIP_TIMER, ccb->dn_line, ccb->gsm_id, fname),
					  ccb->index, timeout, sipGetMethodString(messageType));

    ccb->retx_flag = TRUE;
    if (sip_platform_msg_timer_start(timeout, (void *)((long)ccb->index), ccb->index,
                                     sipPlatformUISMTimers[ccb->index].message_buffer,
                                     sipPlatformUISMTimers[ccb->index].message_buffer_len,
                                     sipPlatformUISMTimers[ccb->index].message_type,
                                     &(sipPlatformUISMTimers[ccb->index].ipaddr),
                                     sipPlatformUISMTimers[ccb->index].port,
                                     FALSE) != SIP_OK) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname,
                          "sip_platform_msg_timer_start()");
        ccb->retx_flag = FALSE;
        return;
    }
}













void
ccsip_handle_obp_error (ccsipCCB_t *ccb, sipMethod_t messageType, cpr_ip_addr_t *data)
{
    const char *fname = "ccsip_handle_obp_error";
    boolean     resend = FALSE;
    uint32_t    max_retx = 0;

    char        obp_address[MAX_IPADDR_STR_LEN];

    config_get_string(CFGID_OUTBOUND_PROXY, obp_address, sizeof(obp_address));
    
    if (util_compare_ip(data, &ccb->outBoundProxyAddr)) {
        
        ccb->outBoundProxyPort = 0;
        ccb->retx_counter = 0;
        if (str2ip(obp_address, &ccb->outBoundProxyAddr) != 0) {
            resend = TRUE;
        }

        
    } else {
        if (messageType == sipMethodInvite) {
            config_get_value(CFGID_SIP_INVITE_RETX, &max_retx, sizeof(max_retx));
            if (max_retx > MAX_INVITE_RETRY_ATTEMPTS) {
                max_retx = MAX_INVITE_RETRY_ATTEMPTS;
            }
        } else {
            config_get_value(CFGID_SIP_RETX, &max_retx, sizeof(max_retx));
            if (max_retx > MAX_NON_INVITE_RETRY_ATTEMPTS) {
                max_retx = MAX_NON_INVITE_RETRY_ATTEMPTS;
            }
        }

        
        if (ccb->retx_counter >= max_retx) {
            ccb->outBoundProxyPort = 0;
            ccb->retx_counter = 0;
            if (str2ip(obp_address, &ccb->outBoundProxyAddr) != 0) {
                
                resend = TRUE;
            }

            
        } else {
            resend = TRUE;
        }
    }

    if (resend) {
        if (sipSPISendLastMessage(ccb) == TRUE) {
            ccb->retx_counter++;
            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d:  Resent message: #%d\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_MSG_SEND, ccb->dn_line, ccb->gsm_id,  fname),
                              ccb->index, ccb->retx_counter);
            ccsip_restart_reTx_timer(ccb, messageType);
            if (ccb->state == SIP_STATE_RELEASE) {
                (void) sip_platform_supervision_disconnect_timer_stop(ccb->index);
            }
        } else {
            sip_platform_msg_timer_outstanding_set(ccb->index, FALSE);
            if ((ccb->state == SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING) ||
                (ccb->state == SIP_STATE_RELEASE)) {
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                sip_sm_call_cleanup(ccb);
            } else {
                sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
                sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            }
        }
    } else {
        sip_platform_msg_timer_outstanding_set(ccb->index, FALSE);
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
    }
}

int
ccsip_pick_a_proxy (ccsipCCB_t *ccb)
{
    uint32_t     max_retx = 0;
    sipMethod_t  retxMessageType = sipPlatformUISMTimers[ccb->index].message_type;
    char         addr[MAX_IPADDR_STR_LEN];
    const char  *fname = "ccsip_pick_a_proxy";

    memset(addr, 0, sizeof(addr));

    


    if (retxMessageType == sipMethodInvite) {
        config_get_value(CFGID_SIP_INVITE_RETX, &max_retx, sizeof(max_retx));
        if (max_retx > MAX_INVITE_RETRY_ATTEMPTS) {
            max_retx = MAX_INVITE_RETRY_ATTEMPTS;
        }
        if (gGlobInfo.backup_active) {
            if (ccb->proxySelection != SIP_PROXY_BACKUP) {
                




                if (ccb->retx_counter >= 3) {
                    ccb->retx_counter = max_retx;
                }
            }
        }
        



        if ((ccb->retx_counter >= max_retx) &&
            (ccb->proxySelection != SIP_PROXY_BACKUP) &&
            (ccb->proxySelection != SIP_PROXY_DO_NOT_CHANGE_MIDCALL)) {
            dns_error_code = DNS_ERR_HOST_UNAVAIL;
            
            sipTransportGetPrimServerAddress(ccb->dn_line, addr);
            if (str2ip(addr, &ccb->dest_sip_addr) != 0) {
                dns_error_code = sip_dns_gethostbysrv(addr, &ccb->dest_sip_addr,
                                                      (uint16_t *)&ccb->dest_sip_port,
                                                      &ccb->SRVhandle, TRUE);
                if (dns_error_code == DNS_OK) {
                    util_ntohl(&(ccb->dest_sip_addr), &(ccb->dest_sip_addr));
                    


                    (void) sip_platform_msg_timer_update_destination(ccb->index,
                                                                     &(ccb->dest_sip_addr),
                                                                     (uint16_t) ccb->dest_sip_port);
                    


                    ccb->retx_counter = 0;

                }
            }
            



            if (dns_error_code != DNS_OK) {
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"Unable to reach proxy, attempting backup.\n",
                    DEB_F_PREFIX_ARGS(SIP_PROXY, fname));
                if (ccsip_attempt_backup_proxy(ccb)) {

                    ccb->first_backup = TRUE;
                    



                    clean_method_request_trx(ccb, sipMethodInvite, TRUE);

                    





                    if (ccb->contact_info) {
                        sippmh_free_contact(ccb->contact_info);
                        ccb->contact_info = NULL;
                    }
                    
                    if (ccb->record_route_info) {
                        sippmh_free_record_route(ccb->record_route_info);
                        ccb->record_route_info = NULL;
                    }

                    







                    if ((sipSPISendInvite(ccb,
                                          ccb->wastransferred ? SIP_INVITE_TYPE_TRANSFER :
                                          SIP_INVITE_TYPE_NORMAL, TRUE) != TRUE)) {
                        sip_sm_call_cleanup(ccb);
                        return FALSE;
                    }

                    


                    ccb->retx_counter = 0;

                    
                } else {
                    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Attempt to reach backup proxy"
                                     " failed. Message will be broadcast.\n",
                                     DEB_F_PREFIX_ARGS(SIP_PROXY, fname));
                    return 1;
                }
            }
        }
    } else {
        config_get_value(CFGID_SIP_RETX, &max_retx, sizeof(max_retx));
        if (max_retx > MAX_NON_INVITE_RETRY_ATTEMPTS) {
            max_retx = MAX_NON_INVITE_RETRY_ATTEMPTS;
        }
    }
    return max_retx;
}














void
ccsip_handle_icmp_unreachable (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_icmp_unreachable";

    










    if (ccb->sip_to_tag[0] != '\0') {
        
        

        CCSIP_DEBUG_TASK(DEB_F_PREFIX"ICMP received within a dialog.\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
        













        ccb->wait_for_ack = FALSE;
        sip_cc_release_complete(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL);
        sip_sm_call_cleanup(ccb);
    } else {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"ICMP received outside of a dialog.\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
        
        ccsip_handle_default_sip_timer(ccb, event);
    }
}














void
ccsip_handle_default_sip_timer (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_default_sip_timer";
    uint32_t    max_retx = 0;
    sipMethod_t retxMessageType = sipPlatformUISMTimers[ccb->index].message_type;
    char        addr[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t out_ip;

    CPR_IP_ADDR_INIT(out_ip);

    












    if ((retxMessageType != sipMethodRefer) &&
        ((ccb->state == SIP_STATE_ACTIVE))) {
        return;
    }

    





    if (ccb->retx_flag == FALSE) {
        if (ccb->state == SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING) {
            sip_platform_msg_timer_outstanding_set(ccb->index, FALSE);
            sip_sm_change_state(ccb, SIP_STATE_IDLE);
            sip_sm_call_cleanup(ccb);
        }
        return;
    }

    
    util_ntohl(&out_ip, &event->u.UsrInfo);
    if ((util_compare_ip(&out_ip, &(ccb->outBoundProxyAddr))) ||
        ((event->u.UsrInfo.type == CPR_IP_ADDR_INVALID) &&
            util_check_if_ip_valid(&(ccb->outBoundProxyAddr)))) {
        ccsip_handle_obp_error(ccb, retxMessageType, &(event->u.UsrInfo));
        return;
    }

    
    max_retx = ccsip_pick_a_proxy(ccb);

    
    ccb->retx_counter++;

    



    if ((ccb->proxySelection == SIP_PROXY_BACKUP) && (ccb->first_backup)) {
        ccb->first_backup = FALSE;
        return;
    }

    
    if (ccb->retx_counter <= max_retx) {
        if (sipSPISendLastMessage(ccb) == TRUE) {
            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d:Resent message: #%d\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_MSG_SEND, ccb->dn_line, ccb->gsm_id, fname),
                              ccb->index, ccb->retx_counter);
        }
        if (ccb->state == SIP_STATE_RELEASE) {
            (void) sip_platform_supervision_disconnect_timer_stop(ccb->index);
        }
        ccsip_restart_reTx_timer(ccb, retxMessageType);

        
    } else {
        ccb->retx_counter = 0;
        sip_platform_msg_timer_outstanding_set(ccb->index, FALSE);

        
        if (ccb->state == SIP_STATE_SENT_INVITE) {
            if (ccb->redirect_info != NULL) {
                sip_redirect(ccb, NULL, SIP_CLI_ERR_REQ_TIMEOUT);
                return;
            }
        }
        



        sipTransportGetPrimServerAddress(ccb->dn_line, addr);
        if (str2ip(addr, &ccb->dest_sip_addr) != 0) {
            dns_error_code = sipTransportGetServerAddrPort(addr,
                                                           &ccb->dest_sip_addr,
                                                           (uint16_t *)&ccb->dest_sip_port,
                                                           &ccb->SRVhandle,
                                                           TRUE);
        } else {
            
            dns_error_code = DNS_ERR_HOST_UNAVAIL;
        }
        if (dns_error_code == 0) {
             util_ntohl(&(ccb->dest_sip_addr), &(ccb->dest_sip_addr));
            
            (void) sip_platform_msg_timer_update_destination(ccb->index,
                                                             &(ccb->dest_sip_addr),
                                                             (uint16_t)ccb->dest_sip_port);
            if (sipSPISendLastMessage(ccb) == TRUE) {
                CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: Resent message: #%d\n",
                                  DEB_L_C_F_PREFIX_ARGS(SIP_MSG_SEND, ccb->dn_line, ccb->gsm_id, fname),
                                  ccb->index, ccb->retx_counter);
            }
            ccsip_restart_reTx_timer(ccb, retxMessageType);
            if (ccb->state == SIP_STATE_RELEASE) {
                (void) sip_platform_supervision_disconnect_timer_stop(ccb->index);
            }
        } else {
            if ((ccb->state == SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING) ||
                (ccb->state == SIP_STATE_RELEASE)) {
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                sip_sm_call_cleanup(ccb);
                return;
            } else if (retxMessageType == sipMethodRefer &&
                       (ccb->featuretype == CC_FEATURE_B2BCONF ||
                        ccb->featuretype == CC_FEATURE_SELECT ||
                        ccb->featuretype == CC_FEATURE_B2B_JOIN ||
                        ccb->featuretype == CC_FEATURE_CANCEL)) {
                sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, ccb->featuretype,
                                   NULL, CC_CAUSE_ERROR);
                return;
            } else {
                handle_error_for_state(ccb, SIP_CLI_ERR_REQ_TIMEOUT);
                return;
            }
        }
    }

    
    ccsip_restart_reTx_timer(ccb, retxMessageType);
}


void
sip_decrement_backup_active_count (ccsipCCB_t *ccb)
{
    



    if ((gGlobInfo.backup_active) && (ccb->proxySelection != SIP_PROXY_BACKUP))
        gGlobInfo.backup_active -= 1;
}

boolean
sip_sm_is_previous_call_id (const char *pCallID, line_t *pPreviousCallIndex)
{
    line_t i;

    for (i = TEL_CCB_START; i <= TEL_CCB_END; i++) {
        if (strcmp(gCallHistory[i].last_call_id, pCallID) == 0) {
            *pPreviousCallIndex = i;
            return TRUE;
        }
    }

    return FALSE;
}

void
sip_sm_200and300_update (ccsipCCB_t *ccb, sipMessage_t *response, int response_code)
{
    const char     *fname = "sip_sm_200and300_update";
    const char     *to;
    const char     *from;
    const char     *contact;
    const char     *record_route = NULL;
    sipLocation_t  *to_loc = NULL;

    to = sippmh_get_cached_header_val(response, TO);
    from = sippmh_get_cached_header_val(response, FROM);
    contact = sippmh_get_cached_header_val(response, CONTACT);

    if (ccb->state < SIP_STATE_ACTIVE) {
        
        record_route = sippmh_get_cached_header_val(response, RECORD_ROUTE);
    }
    


    if (ccb->state < SIP_STATE_ACTIVE) {
        if (to) {
            to_loc = sippmh_parse_from_or_to((char *) to, TRUE);
            if (to_loc) {
                if (to_loc->tag) {
                    ccb->sip_to_tag = strlib_update(ccb->sip_to_tag,
                                                    sip_sm_purify_tag(to_loc->tag));
                    if ( ccb->callref == 0 ) {
                        ccb->callref = get_callref(ccb->sip_to_tag);
                    }
                } else {
                    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                                      ccb->index, ccb->dn_line, fname,
                                      "TO header:missing \"tag=\" param");
                }
                CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: Recorded to_tag=<%s>\n",
                                  DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                                  ccb->index, ccb->sip_to_tag);
                sippmh_free_location(to_loc);
            }
        }
    }

    


    if (response_code == SIP_STATUS_SUCCESS) {
        if (ccb->flags & INCOMING) {
            ccb->sip_to = strlib_update(ccb->sip_to, from);
            if (to) {
                ccb->sip_from = strlib_update(ccb->sip_from, to);
            }
        } else {
            if (to) {
                ccb->sip_to = strlib_update(ccb->sip_to, to);
            }
            ccb->sip_from = strlib_update(ccb->sip_from, from);
        }
    }

    if (response_code == SIP_STATUS_SUCCESS) {
        if (contact) {
            if (ccb->contact_info) {
                sippmh_free_contact(ccb->contact_info);
            }
            ccb->contact_info = sippmh_parse_contact(contact);
        }
    }
    if (record_route) {
        if (ccb->record_route_info) {
            sippmh_free_record_route(ccb->record_route_info);
        }
        ccb->record_route_info = sippmh_parse_record_route(record_route);
    }
}


char *
sip_sm_purify_tag (char *tag)
{
    char *p;

    p = tag;
    while (((*p == ';') || (*p == ' ') || (*p == '\t')) && (*p != '\0')) {
        p++;
    }

    return p;
}


boolean
sip_sm_is_invite_response (sipMessage_t *response)
{
    const char *cseq;
    sipCseq_t  *sipCseq;

    if (response == NULL) {
        return FALSE;
    }

    cseq = sippmh_get_cached_header_val(response, CSEQ);
    sipCseq = sippmh_parse_cseq(cseq);
    if (!sipCseq) {
        return FALSE;
    }

    if (sipCseq->method == sipMethodInvite) {
        cpr_free(sipCseq);
        return TRUE;
    }
    cpr_free(sipCseq);
    return FALSE;
}

boolean
sip_sm_is_bye_or_cancel_response (sipMessage_t *response)
{
    const char *cseq;
    sipCseq_t  *sipCseq;

    if (response == NULL) {
        return FALSE;
    }

    cseq = sippmh_get_cached_header_val(response, CSEQ);
    sipCseq = sippmh_parse_cseq(cseq);
    if (!sipCseq) {
        return FALSE;
    }

    if ((sipCseq->method == sipMethodBye) ||
        (sipCseq->method == sipMethodCancel)) {
        cpr_free(sipCseq);
        return TRUE;
    }
    cpr_free(sipCseq);
    return FALSE;
}



void
sip_sm_dequote_string (char *str, int max_size)
{
    char *p;

    
    p = str;
    while (((*p == '\"') || (*p == ' ') || (*p == '\t')) && (*p != '\0')) {
        p++;
    }

    
    sstrncpy(str, p, max_size);

    
    
    
    p = str;
    while ((*p != '\"') && (*p != '\0')) {
        p++;
    }
    *p = '\0';
}


void
sip_sm_check_retx_timers (ccsipCCB_t *ccb, sipMessage_t *canceller_message)
{
    const char     *fname = "sip_sm_check_retx_timers";
    uint32_t        canceller_cseq;
    sipMethod_t     canceller_cseq_method;
    const char     *canceller_callid;

    sipMessage_t   *retx_message = NULL;
    uint32_t        retx_message_buf_length = 0;
    uint32_t        retx_cseq = 0;
    sipMethod_t     retx_cseq_method = sipMethodInvalid;
    const char     *retx_callid = NULL;

    const char     *cseq;
    sipCseq_t      *sipCseq;
    int             response_code = -1;
    const char     *conn_type = NULL;

    if (!ccb) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "CCB is NULL");
        return;
    }

    if (ccb->index >= MAX_CCBS) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_LINE_NUMBER_INVALID),
                          fname, ccb->index);
        return;
    }

    





    conn_type = sipTransportGetTransportType(1, TRUE, ccb);
    if ((!cpr_strcasecmp(conn_type, "TCP") || !cpr_strcasecmp(conn_type, "TLS")) &&
        0 == sipPlatformUISMTimers[ccb->index].message_buffer_len) {
        

        return;
    }


    


    cseq = sippmh_get_cached_header_val(canceller_message, CSEQ);
    sipCseq = sippmh_parse_cseq(cseq);
    if (sipCseq) {
        canceller_cseq = sipCseq->number;
        canceller_cseq_method = sipCseq->method;
        cpr_free(sipCseq);
    } else {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          "sippmh_parse_cseq()");
        return;
    }

    canceller_callid = sippmh_get_cached_header_val(canceller_message, CALLID);

    


    if (!sippmh_is_request(canceller_message)) {
        if (sipGetResponseCode(canceller_message, &response_code) < 0) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                              ccb->dn_line, fname, "sipGetResponseCode()");
            return;
        }
    }

    


    retx_message = sippmh_message_create();
    if (!retx_message) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname, "sippmh_message_create()");
        return;
    }
    retx_message_buf_length = sipPlatformUISMTimers[ccb->index].message_buffer_len;
    if (sippmh_process_network_message(retx_message,
                                       sipPlatformUISMTimers[ccb->index].message_buffer,
                                       &retx_message_buf_length) == STATUS_FAILURE) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname,
                          "sippmh_process_network_message()");
        free_sip_message(retx_message);
        return;
    }
    if (!sippmh_is_message_complete(retx_message)) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname, "sippmh_is_message_complete()");
        free_sip_message(retx_message);
        return;
    }
    cseq = sippmh_get_cached_header_val(retx_message, CSEQ);
    sipCseq = sippmh_parse_cseq(cseq);
    if (sipCseq) {
        retx_cseq = sipCseq->number;
        retx_cseq_method = sipCseq->method;
        cpr_free(sipCseq);
    } else {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          "sippmh_parse_cseq()");
        free_sip_message(retx_message);
        return;
    }

    retx_callid = sippmh_get_cached_header_val(retx_message, CALLID);

    



    if ((canceller_cseq == retx_cseq) &&
        ((canceller_cseq_method == retx_cseq_method) ||
         ((canceller_cseq_method == sipMethodAck) &&
          (retx_cseq_method == sipMethodInvite))) &&
          (strcmp(canceller_callid, retx_callid) == 0)) {
        sip_platform_msg_timer_stop(ccb->index);
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: Stopping reTx timer.\n"
                          "(callid=%s, cseq=%u, cseq_method=%s)\n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_TIMER, ccb->dn_line, ccb->gsm_id, fname),
                          ccb->index, retx_callid, retx_cseq,
                          sipGetMethodString(retx_cseq_method));

        UNBIND_UDP_ICMP_HANDLER(ccb->udpId);

        if (ccb->state == SIP_STATE_IDLE_MSG_TIMER_OUTSTANDING) {
            if ((response_code == 401) || (response_code == 407) ||
                ((response_code < 200) &&
                (!sippmh_is_request(canceller_message)))) {
                CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED), ccb->index,
                                  ccb->dn_line, fname,
                                  sip_util_state2string(ccb->state));
            } else {
                sip_sm_change_state(ccb, SIP_STATE_IDLE);
                sip_sm_call_cleanup(ccb);
            }
        }
    } else {
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d:CSeq mismatch:\n(Rx: callid=%s,"
                          " cseq=%u, cseq_method=%s),\n(reTx: callid=%s,"
                          " cseq=%u, cseq_method=%s)\n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
						  ccb->index,
                          canceller_callid, canceller_cseq,
                          sipGetMethodString(canceller_cseq_method),
                          retx_callid, retx_cseq,
                          sipGetMethodString(retx_cseq_method));
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                          ccb->index, ccb->dn_line, fname,
                          "Not stopping retx timer");
    }

    free_sip_message(retx_message);
}


static int
sip_sm_request_check_and_store (ccsipCCB_t *ccb, sipMessage_t *request,
                                sipMethod_t request_method, boolean midcall,
                                uint16_t *request_check_reason_code,
                                char *request_check_reason_phrase,
                                boolean store_invite)
{
    const char     *fname = "sip_sm_request_check_and_store";
    const char     *request_cseq = NULL;
    sipCseq_t      *request_cseq_structure = NULL;
    uint32_t        request_cseq_number = 0;
    sipMethod_t     request_cseq_method = sipMethodInvalid;
    const char     *callID = NULL;
    int             content_length = 0;
    boolean         request_uri_error = FALSE;
    sipReqLine_t   *requestURI = NULL;
    sipLocation_t  *uri_loc = NULL;
    const char     *sip_from = NULL;
    const char     *sip_to = NULL;
    sipLocation_t  *to_loc = NULL;
    sipLocation_t  *from_loc = NULL;
    const char     *pViaHeaderStr = NULL;
    int16_t         trx_index = -1;
    sipVia_t       *via = NULL;


    
    if (!request_check_reason_phrase) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Input parameter request_check_reason_phrase is NULL\n",
                          fname);
        return (-1);
    }

    


    callID = sippmh_get_cached_header_val(request, CALLID);
    if (!callID) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Unable to obtain request's "
                          "Call-ID header.\n", fname);
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                SIP_CLI_ERR_BAD_REQ_CALLID_ABSENT,
                SIP_WARNING_LENGTH);
        return (-1);
    }

    


    request_cseq = sippmh_get_cached_header_val(request, CSEQ);
    if (!request_cseq) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Unable to obtain request's CSeq "
                          "header.\n", fname);
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                SIP_CLI_ERR_BAD_REQ_VIA_OR_CSEQ,
                SIP_WARNING_LENGTH);
        return (-1);
    }
    request_cseq_structure = sippmh_parse_cseq(request_cseq);
    if (!request_cseq_structure) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Unable to parse request's CSeq "
                          "header.\n", fname);
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                SIP_CLI_ERR_BAD_REQ_VIA_OR_CSEQ,
                SIP_WARNING_LENGTH);
        return (-1);
    }
    request_cseq_number = request_cseq_structure->number;
    request_cseq_method = request_cseq_structure->method;
    cpr_free(request_cseq_structure);

    


    requestURI = sippmh_get_request_line(request);
    if (requestURI) {
        if (requestURI->url) {
            uri_loc = sippmh_parse_from_or_to(requestURI->url, TRUE);
            if (uri_loc) {
                if (uri_loc->genUrl->schema != URL_TYPE_SIP) {
                    request_uri_error = TRUE;
                }
                sippmh_free_location(uri_loc);
            } else {
                request_uri_error = TRUE;
            }
        } else {
            request_uri_error = TRUE;
        }
        SIPPMH_FREE_REQUEST_LINE(requestURI);
    } else {
        request_uri_error = TRUE;
    }
    if (request_uri_error) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Invalid Request URI"
                          "failed.\n", fname);
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                SIP_CLI_ERR_BAD_REQ_REQLINE_ERROR,
                SIP_WARNING_LENGTH);
        return (-1);
    }

    


    sip_from = sippmh_get_cached_header_val(request, FROM);
    from_loc = sippmh_parse_from_or_to((char *)sip_from, TRUE);
    if (!from_loc) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, get_debug_string(DEBUG_FUNCTIONNAME_SIPPMH_PARSE_FROM));
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                SIP_CLI_ERR_BAD_REQ_FROMURL_ERROR,
                SIP_WARNING_LENGTH);
        return (-1);
    }
    sippmh_free_location(from_loc);

    


    sip_to = sippmh_get_cached_header_val(request, TO);
    to_loc = sippmh_parse_from_or_to((char *)sip_to, TRUE);
    if (!to_loc) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, get_debug_string(DEBUG_FUNCTIONNAME_SIPPMH_PARSE_TO));
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                SIP_CLI_ERR_BAD_REQ_ToURL_ERROR,
                SIP_WARNING_LENGTH);
        return (-1);
    }
    sippmh_free_location(to_loc);

    


    pViaHeaderStr = sippmh_get_cached_header_val(request, VIA);
    if (pViaHeaderStr) {
        via = sippmh_parse_via(pViaHeaderStr);
    }
    if (!pViaHeaderStr || !via) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), NULL,
                          NULL, fname, "sippmh_parse_via");
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                SIP_CLI_ERR_BAD_REQ_VIA_OR_CSEQ,
                SIP_WARNING_LENGTH);
        return (-1);

    }
    sippmh_free_via(via);

    


    switch (request_method) {
        
    case sipMethodInvite:

        content_length = sippmh_get_content_length(request);

        if (request->raw_body) {

            if ((size_t) content_length != strlen(request->raw_body)) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"\n Mismatched Content length and "
                                  "Actual message body length:content length=%d\n"
                                  "and message as %s\n and strlen of messagebody = %d\n",
                                  fname, content_length, request->raw_body,
                                  strlen(request->raw_body));
                *request_check_reason_code = SIP_WARN_MISC;
                sstrncpy(request_check_reason_phrase,
                        SIP_CLI_ERR_BAD_REQ_CONTENT_LENGTH_ERROR,
                        SIP_WARNING_LENGTH);
                return (-1);
            }
        } else {
            if ((size_t) content_length != 0) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"\n Mismatched Content length and "
                                  "Actual message body length:content length=%d\n"
                                  "and message is empty and strlen of messagebody = 0\n",
                                  fname, content_length);
                *request_check_reason_code = SIP_WARN_MISC;
                sstrncpy(request_check_reason_phrase,
                        SIP_CLI_ERR_BAD_REQ_CONTENT_LENGTH_ERROR,
                        SIP_WARNING_LENGTH);
                return (-1);
            }
        }
        if (midcall &&
            ((ccb->last_recv_request_cseq_method == sipMethodInvite) ||
             (ccb->last_recv_request_cseq_method == sipMethodAck))) {
            if (request_cseq_number <= ccb->last_recv_invite_cseq) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error: CSeq (%d) is not greater "
                                  "than previous INVITE (%d).\n", fname,
                                  request_cseq_number,
                                  ccb->last_recv_invite_cseq);
                *request_check_reason_code = SIP_WARN_MISC;
                sstrncpy(request_check_reason_phrase,
                        SIP_CLI_ERR_BAD_REQ_CSEQ_FIELD,
                        SIP_WARNING_LENGTH);
                return (-1);
            }
        }
        ccb->last_recv_invite_cseq = request_cseq_number;
        break;

        
    case sipMethodBye:
        if (ccb->flags & INCOMING) {
            if (request_cseq_number <= ccb->last_recv_invite_cseq) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Error: CSeq (%d) is not greater "
                                  "than original INVITE (%d).\n", fname,
                                  request_cseq_number,
                                  ccb->last_recv_invite_cseq);
                *request_check_reason_code = SIP_WARN_MISC;
                sstrncpy(request_check_reason_phrase,
                        SIP_CLI_ERR_BAD_REQ_CSEQ_FIELD,
                        SIP_WARNING_LENGTH);
                return (-1);
            }
        }
        break;

        
    case sipMethodCancel:
    case sipMethodAck:
        if (request_cseq_number != ccb->last_recv_invite_cseq) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"CSeq (%d) is not the same as "
                              "original INVITE (%d).\n", fname,
                              request_cseq_number, ccb->last_recv_request_cseq);
            *request_check_reason_code = SIP_WARN_MISC;
            sstrncpy(request_check_reason_phrase,
                    SIP_CLI_ERR_BAD_REQ_CSEQ_FIELD,
                    SIP_WARNING_LENGTH);
            return (-1);
        }
        break;

    default:
        break;
    }

    
    trx_index = get_next_request_trx_index(ccb, FALSE);
    if (trx_index < 0) {
        
        *request_check_reason_code = SIP_WARN_MISC;
        sstrncpy(request_check_reason_phrase,
                "Too many Transactions",
                SIP_WARNING_LENGTH);
        return (-2);
    }
    




    if (!store_invite) {
        if (ccb->last_request) {
            free_sip_message(ccb->last_request);
            ccb->last_request = NULL;
        }
    }
    
    ccb->last_request = request;

    ccb->last_recv_request_cseq = request_cseq_number;
    ccb->last_recv_request_cseq_method = request_cseq_method;
    ccb->recv_request[trx_index].cseq_number = request_cseq_number;
    ccb->recv_request[trx_index].cseq_method = request_cseq_method;
    pViaHeaderStr = sippmh_get_cached_header_val(request, VIA);
    if (pViaHeaderStr) {
        ccb->recv_request[trx_index].u.sip_via_header =
            strlib_update(ccb->recv_request[trx_index].u.sip_via_header,
                          pViaHeaderStr);
    }

    return (0);
}












void
sip_sm_update_to_from_on_callsetup_finalresponse (ccsipCCB_t *ccb,
                                                  sipMessage_t *response)
{
    const char     *fname = "sip_sm_update_to_from_on_callsetup_finalresponse";
    const char     *to;
    const char     *from;
    sipLocation_t  *to_loc = NULL;

    to = sippmh_get_cached_header_val(response, TO);
    from = sippmh_get_cached_header_val(response, FROM);

    


    if (to) {
        to_loc = sippmh_parse_from_or_to((char *)to, TRUE);
        if (to_loc) {
            if (to_loc->tag) {
                ccb->sip_to_tag = strlib_update(ccb->sip_to_tag, sip_sm_purify_tag(to_loc->tag));
            } else {
                CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                                  ccb->index, ccb->dn_line, fname,
                                  "TO header missing \"tag=\" param");
                
            }
            CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d: Recorded to_tag=<%s>\n",
                              DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
                              ccb->index, ccb->sip_to_tag);
            sippmh_free_location(to_loc);
        }
    }

    


    if (to) {
        ccb->sip_to = strlib_update(ccb->sip_to, to);
    }
    ccb->sip_from = strlib_update(ccb->sip_from, from);
}












void
sip_sm_update_contact_recordroute (ccsipCCB_t *ccb, sipMessage_t *response,
                                   int response_code, boolean midcall)
{
    const char *contact;
    const char *record_route;

    contact = sippmh_get_cached_header_val(response, CONTACT);
    record_route = sippmh_get_cached_header_val(response, RECORD_ROUTE);

    if (response_code == SIP_STATUS_SUCCESS) {
        if (contact) {
            if (ccb->contact_info) {
                sippmh_free_contact(ccb->contact_info);
            }
            ccb->contact_info = sippmh_parse_contact(contact);
        }
    }
    if (record_route && (!midcall)) {
        if (ccb->record_route_info) {
            sippmh_free_record_route(ccb->record_route_info);
        }
        ccb->record_route_info = sippmh_parse_record_route(record_route);
    }
}





void
ccsip_handle_active_ev_cc_feature (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "active_ev_cc_feature";
    cc_features_t   feature_type;

    feature_type = event->u.cc_msg->msg.feature.feature_id;
    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      cc_feature_name(feature_type));

    switch (feature_type) {
    case CC_FEATURE_HOLD:
        ccb->hold_initiated = TRUE;
        ccb->featuretype = CC_FEATURE_HOLD;
        ccsip_handle_active_ev_cc_feature_hold(ccb, event);
        break;
    case CC_FEATURE_MEDIA:
        ccb->featuretype = CC_FEATURE_MEDIA;
        ccsip_handle_active_ev_cc_feature_resume_or_media(ccb, event);
        break;
    case CC_FEATURE_RESUME:
        ccb->featuretype = CC_FEATURE_RESUME;
        ccsip_handle_active_ev_cc_feature_resume_or_media(ccb, event);
        break;
    case CC_FEATURE_BLIND_XFER:
    case CC_FEATURE_XFER:
        ccsip_handle_active_ev_cc_feature_xfer(ccb, event);
        break;
    case CC_FEATURE_NOTIFY:
        if (event->u.cc_msg->msg.feature.data.notify.final == TRUE) {
            ccb->flags |= FINAL_NOTIFY;
        }
        if (CC_CAUSE_OK != event->u.cc_msg->msg.feature.data.notify.cause) {
            
            (void) sipSPISendNotify(ccb, event->u.cc_msg->msg.feature.data.notify.cause_code);
            ccb->xfer_status = event->u.cc_msg->msg.feature.data.notify.cause_code;
        } else {
            (void) sipSPISendNotify(ccb, SIP_SUCCESS_SETUP); 
            ccb->xfer_status = SIP_SUCCESS_SETUP;
        }
        break;
    case CC_FEATURE_B2BCONF:
    case CC_FEATURE_SELECT:
    case CC_FEATURE_B2B_JOIN:
    case CC_FEATURE_CANCEL:
        break;
    default:
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type, NULL,
                           CC_CAUSE_ERROR);
        break;
    }
}













void
ccsip_handle_active_ev_cc_feature_hold (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    cc_msgbody_info_t *msg_body;

    
    ccsip_store_call_info(&event->u.cc_msg->msg.feature.data.hold.call_info, ccb);
    if (event->u.cc_msg->msg.feature.data_valid) {
        
        msg_body = &event->u.cc_msg->msg.feature.data.hold.msg_body;
        ccsip_save_local_msg_body(ccb, msg_body);
    }

    sip_sm_change_state(ccb, SIP_STATE_SENT_MIDCALL_INVITE);
    if (send_resume_or_hold_request(ccb, TRUE) == FALSE) {
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
    }
}














void
ccsip_handle_active_ev_cc_feature_resume_or_media (ccsipCCB_t *ccb,
                                                   sipSMEvent_t *event)
{
    cc_msgbody_info_t *msg_body;

    if (event->u.cc_msg->msg.feature.data_valid) {
        
        msg_body = &event->u.cc_msg->msg.feature.data.resume.msg_body;
        ccsip_save_local_msg_body(ccb, msg_body);
    }

    
    ccsip_store_call_info(&event->u.cc_msg->msg.feature.data.resume.call_info, ccb);
    sip_sm_change_state(ccb, SIP_STATE_SENT_MIDCALL_INVITE);
    if (send_resume_or_hold_request(ccb, FALSE) == FALSE) {
        sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
    }
}

void
ccsip_handle_active_ev_cc_feature_xfer (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "active_ev_cc_feature_xfer";
    char            referto[MAX_SIP_URL_LENGTH];
    char           *domainloc = NULL;
    char           *pTransferNumberString;
    cc_feature_data_t data;
    cc_xfer_methods_t method;
    char            addr[MAX_IPADDR_STR_LEN];
    ccsipCCB_t     *xfer_ccb = NULL;
    int             n = 0;
    static char     dialtranslate[MAX_SIP_URL_LENGTH];
    char           *pReferToStr = referto;
    sipRefEnum_e   refto_type = SIP_REF_XFER;

    memset(addr, 0, MAX_IPADDR_STR_LEN);
    method = event->u.cc_msg->msg.feature.data.xfer.method;

    if (CC_FEATURE_BLIND_XFER == event->u.cc_msg->msg.feature.feature_id) {
        ccb->featuretype = CC_FEATURE_BLIND_XFER;
        ccb->con_call_id = CC_NO_CALL_ID;
    } else if (CC_FEATURE_XFER == event->u.cc_msg->msg.feature.feature_id) {
        ccb->featuretype = CC_FEATURE_XFER;
        ccb->con_call_id = event->u.cc_msg->msg.feature.data.xfer.target_call_id;
        xfer_ccb = sip_sm_get_ccb_by_target_call_id(ccb->con_call_id);
    }


    pTransferNumberString = event->u.cc_msg->msg.feature.data.xfer.dialstring;
    (void) MatchDialTemplate(pTransferNumberString, ccb->dn_line, CAST_N &n,
                             dialtranslate, sizeof(dialtranslate),
                             (RouteMode *) &(ccb->routeMode), NULL);
    
    domainloc = strchr(dialtranslate, '@');
    if (domainloc == NULL) {
        (void) sippmh_convertURLCharToEscChar(dialtranslate,
                                              strlen(dialtranslate),
                                              pReferToStr, MAX_SIP_URL_LENGTH, TRUE);
    } else {
        (void) sippmh_convertURLCharToEscChar(dialtranslate,
                                              domainloc - dialtranslate,
                                              pReferToStr, MAX_SIP_URL_LENGTH, TRUE);
        
        sstrncat(pReferToStr, domainloc, MAX_SIP_URL_LENGTH - strlen(pReferToStr));
    }
    
    sstrncpy(dialtranslate, referto, MAX_SIP_URL_LENGTH);
    pTransferNumberString = dialtranslate;


    ccb->authen.cred_type = 0;
    

    
    domainloc = strchr(referto, '@');
    if (domainloc == NULL) {
        char *semi = NULL;
        size_t len;

        
        semi = strchr(pTransferNumberString, ';');

        if (semi) {
            sstrncpy(referto, "<sip:", MAX_SIP_URL_LENGTH);
            len = semi - pTransferNumberString;
        } else {
            sstrncpy(referto, "sip:", MAX_SIP_URL_LENGTH);
            len = MAX_SIP_URL_LENGTH - sizeof("sip:");
        }

        
        sstrncat(referto, pTransferNumberString, len);

        domainloc = referto + strlen(referto);
        if ((domainloc - referto) < (MAX_SIP_URL_LENGTH - 1)) {
            




            *domainloc++ = '@';
            xfer_ccb = sip_sm_get_ccb_by_target_call_id(ccb->con_call_id);
            if ((xfer_ccb != NULL) &&
                util_check_if_ip_valid(&(xfer_ccb->dest_sip_addr)) &&
                (ccb->featuretype == CC_FEATURE_XFER)) {
                



                if ((xfer_ccb->routeMode == RouteEmergency) ||
                    (xfer_ccb->proxySelection == SIP_PROXY_BACKUP)) {
                    ipaddr2dotted(addr, &xfer_ccb->dest_sip_addr);
                } else {
                    sipTransportGetPrimServerAddress(xfer_ccb->dn_line, addr);
                }
            } else {
                







                if ((ccb->routeMode == RouteEmergency) ||
                    (ccb->proxySelection == SIP_PROXY_BACKUP)) {
                    ipaddr2dotted(addr, &ccb->dest_sip_addr);
                } else {
                    sipTransportGetPrimServerAddress(ccb->dn_line, addr);
                }
            }

            sstrncpy(domainloc, addr,
                     MAX_SIP_URL_LENGTH - (domainloc - referto - 1));
        }

        
        if (semi) {
            domainloc = referto + strlen(referto);
            sstrncpy(domainloc, semi,
                     MAX_SIP_URL_LENGTH - (domainloc - referto - 1));
        }

        if (semi) {
            sstrncat(domainloc, ">", MAX_SIP_URL_LENGTH - strlen(referto));
        }
    }
    


    if (method == CC_XFER_METHOD_DIRXFR) {
        refto_type = SIP_REF_DIR_XFER;
    }

    if (CC_XFER_METHOD_REFER == method ||
        method == CC_XFER_METHOD_DIRXFR) {
        
        ccsipCCB_t *xfer_refer_ccb;

        ccb->sip_referTo = strlib_update(ccb->sip_referTo, referto);
        xfer_refer_ccb = sip_sm_get_target_call_by_con_call_id(ccb->con_call_id);

        if ((xfer_refer_ccb != NULL) &&  (xfer_ccb != NULL) && (xfer_ccb->sip_referTo[0] != '\0')) {
            ccb->sip_referTo = strlib_update(ccb->sip_referTo,
                                             xfer_refer_ccb->sip_referTo);
        }

        if (ccb->sip_referTo) {
            if (sipSPISendRefer(ccb, (char *)ccb->sip_referTo, refto_type) != TRUE) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                                  ccb->index, ccb->dn_line, fname,
                                  "sipSPISendRefer Failed");
                return;
            }
        } else {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "ccb->sipreferTo is NULL");
            return;
        }

    } else if (CC_XFER_METHOD_BYE == method) {
        cc_features_t   feature;

        ccb->referto = strlib_update(ccb->referto, referto);
        data.xfer.cause = CC_CAUSE_XFER_LOCAL;
        feature = fsmxfr_type_to_feature(fsmxfr_get_xfr_type(ccb->gsm_id));
        data.xfer.method = CC_XFER_METHOD_BYE; 
        data.xfer.dialstring[0] = '\0';
        data.xfer.target_call_id = CC_NO_CALL_ID;
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature, &data,
                           CC_CAUSE_NORMAL);
    } else {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          "Unspecified Method of Transfer");

    }

    
    ADD_TO_ARP_CACHE(ccb->dest_sip_addr);
}

void
ccsip_handle_active_ev_cc_feature_ack (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "ccsip_handle_active_ev_cc_feature_ack";
    ccsipCCB_t     *other_ccb = NULL;
    cc_features_t   feature_type;

    feature_type = event->u.cc_msg->msg.feature.feature_id;
    switch (feature_type) {
    case CC_FEATURE_XFER:
    case CC_FEATURE_BLIND_XFER:
        if (CC_XFER_METHOD_REFER == event->u.cc_msg->msg.feature.data.xfer.method) {
            if (CC_CAUSE_ERROR == event->u.cc_msg->msg.feature.data.xfer.cause) {
                (void) sipSPISendErrorResponse(ccb->last_request,
                                               SIP_SERV_ERR_UNAVAIL,
                                               SIP_SERV_ERR_UNAVAIL_PHRASE,
                                               0, NULL, ccb);
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Got CC_CAUSE_ERROR"
                                  "from GSM \n", fname);
                return;
            }
            if (CC_NO_CALL_ID != event->u.cc_msg->msg.feature.data.xfer.target_call_id) {
                ccb->con_call_id = event->u.cc_msg->msg.feature.data.xfer.target_call_id;
                if (feature_type == CC_FEATURE_BLIND_XFER) {
                    ccb->blind_xfer_call_id = event->u.cc_msg->msg.feature.data.xfer.target_call_id;
                }
                if (!sipSPISendReferResponse202(ccb)) {
                    (void) sipSPISendErrorResponse(ccb->last_request,
                                                   SIP_SERV_ERR_UNAVAIL,
                                                   SIP_SERV_ERR_UNAVAIL_PHRASE,
                                                   0, NULL, ccb);
                    CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sipSPISendReferResponse202"
                                      " failed, sending 503\n", fname);
                    return;
                } else {
                    
                    if (!sipSPISendNotify(ccb, SIP_1XX_TRYING)) {
                        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sipSPISendNotify"
                                          " failed, sending 100\n", fname);
                    }
                    ccb->xfer_status = 100;

                    if (feature_type == CC_FEATURE_BLIND_XFER) {
                        



                        sip_cc_release(ccb->gsm_id, ccb->dn_line,
                                       CC_CAUSE_NORMAL, NULL);
                        sip_sm_change_state(ccb, SIP_STATE_RELEASE);
                    }
                }
            }
        } else if (CC_XFER_METHOD_BYE == event->u.cc_msg->msg.feature.data.xfer.method) {
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_NORMAL, NULL);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
        }
        break;
    case CC_FEATURE_NOTIFY:
        
        other_ccb = sip_sm_get_target_call_by_con_call_id(ccb->con_call_id);
        if ((other_ccb != NULL) && (other_ccb->early_transfer)) {
            other_ccb->early_transfer = FALSE;
            sipSPISendCancel(other_ccb);
            sip_cc_release_complete(other_ccb->gsm_id, other_ccb->dn_line,
                                    CC_CAUSE_NORMAL);
            sip_sm_change_state(other_ccb, SIP_STATE_RELEASE);
        } else {
            
            
            
            (void) sipSPISendNotifyResponse(ccb,
                                            event->u.cc_msg->msg.feature_ack.cause);
        }
        break;

    default:
        break;
    }
}

void
ccsip_handle_active_ev_sip_invite (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "active_ev_sip_invite";
    sipMessage_t   *request;

    
    const char     *contact = NULL;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    cc_feature_data_t data;
    sipsdp_status_t sdp_status;
    boolean         apply_ringout = FALSE;

    
    request = event->u.pSipMessage;

    
    
    if (get_method_request_trx_index(ccb, sipMethodInvite, TRUE) > -1) {
        CCSIP_DEBUG_STATE(DEB_L_C_F_PREFIX"%d Glare condition detected \n",
                          DEB_L_C_F_PREFIX_ARGS(SIP_CALL_STATUS, ccb->dn_line, ccb->gsm_id, fname),
						  ccb->index);
        
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_REQ_PENDING,
                                       SIP_CLI_ERR_REQ_PENDING_PHRASE,
                                       0, NULL, NULL);
        free_sip_message(request);
        return;
    }

    
    if (sip_sm_request_check_and_store(ccb, request, sipMethodInvite, TRUE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       request_check_reason_code,
                                       request_check_reason_phrase, NULL);
        free_sip_message(request);
        return;
    }


    
    contact = sippmh_get_cached_header_val(request, CONTACT);
    if (contact) {
        if (ccb->contact_info) {
            sippmh_free_contact(ccb->contact_info);
        }
        ccb->contact_info = sippmh_parse_contact(contact);
    }

    


    sdp_status = sip_util_extract_sdp(ccb, request);

    switch (sdp_status) {
    case SIP_SDP_SESSION_AUDIT:
        ccb->oa_state = OA_OFFER_RECEIVED;
        if (((ccb->state == SIP_STATE_SENT_MIDCALL_INVITE) &&
             (ccb->hold_initiated)) || (ccb->state == SIP_STATE_ACTIVE)) {
            







            if (ccb->in_call_info &&
                ccb->in_call_info->data.call_info_feat_data.feature_flag & CC_UI_STATE) {
                apply_ringout =
                    (ccb->in_call_info->data.call_info_feat_data.ui_state ==
                     CC_UI_STATE_RINGOUT ? TRUE : FALSE);
            }
            
            ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);
            sip_cc_audit(ccb->gsm_id, ccb->dn_line, apply_ringout);
            return;
        }
        

    case SIP_SDP_SUCCESS:
        




        ccb->oa_state = OA_OFFER_RECEIVED;
        



        ccsip_update_callinfo(ccb, request, TRUE, TRUE, TRUE);
        
        sip_cc_mv_msg_body_to_cc_msg(&data.resume.msg_body, request);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_MEDIA, &data);
        sip_sm_change_state(ccb,
                      SIP_STATE_RECV_MIDCALL_INVITE_CCFEATUREACK_PENDING);
        break;

    case SIP_SDP_ERROR:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC, "Invalid SDP", ccb);
        return;

    case SIP_SDP_DNS_FAIL:
        sipSPISendInviteResponse(ccb, SIP_SERV_ERR_INTERNAL,
                                 SIP_SERV_ERR_INTERNAL_PHRASE,
                                 SIP_WARN_MISC,
                                 "DNS lookup failed for media destination",
                                 FALSE, FALSE);
        return;

    case SIP_SDP_NO_MEDIA:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       "No acceptable media line in SDP", ccb);
        return;

    case SIP_SDP_NOT_PRESENT:
       

    default:
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Waiting for SDP in ACK\n",
            DEB_F_PREFIX_ARGS(SIP_SDP, fname));
        



        ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_MEDIA, NULL);
        sip_sm_change_state(ccb,
                            SIP_STATE_RECV_MIDCALL_INVITE_CCFEATUREACK_PENDING);
        break;
    }

    sipSPISendInviteResponse100(ccb, FALSE);
}


void
ccsip_handle_active_2xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "Active_2xx";
    sipMessage_t   *response;
    int             response_code = 0;

    
    response = event->u.pSipMessage;
    if (sipGetResponseCode(response, &response_code) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseCode");
        free_sip_message(response);
        return;
    }
    if (response_code == SIP_ACCEPTED) {
        ccsip_handle_accept_2xx(ccb, event);
        return;
    }

    if (sipSPISendAck(ccb, response) != TRUE) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipSPISendAck");
    }
    
    ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);

    free_sip_message(response);
}


















void ccsip_handle_sentinvite_midcall_ev_cc_feature (ccsipCCB_t *ccb,
                                                    sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_sentinvite_midcall_ev_cc_feature";
    cc_features_t feature_type;

    feature_type = event->u.cc_msg->msg.feature_ack.feature_id;

    switch (feature_type) {
    case CC_FEATURE_RESUME:
    case CC_FEATURE_HOLD:
    case CC_FEATURE_MEDIA:
        




        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type,
                           NULL, CC_CAUSE_REQUEST_PENDING);
        break;

    default:
        
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type,
                           NULL, CC_CAUSE_ERROR);
        break;
    }
}













void ccsip_handle_sentinvite_midcall_ev_sip_2xx (ccsipCCB_t *ccb,
                                                 sipSMEvent_t *event)
{
    const char   *fname = "ccsip_handle_sentinvite_midcall_ev_sip_2xx";
    sipMessage_t *response;
    cc_feature_data_t data;
    sipsdp_status_t sdp_status;

    response = event->u.pSipMessage;
    if (!sip_sm_is_invite_response(response)) {
        free_sip_message(response);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        return;
    }

    
    (void) sip_platform_expires_timer_stop(ccb->index);

    sip_sm_200and300_update(ccb, response, SIP_STATUS_SUCCESS);

    
    ccb->authen.cred_type = 0;

    



    if (sipSPISendAck(ccb, response) == FALSE) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipSPISendAck");
    }

    
    ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);

    
    sdp_status = sip_util_extract_sdp(ccb, response);

    switch (sdp_status) {
    case SIP_SDP_SUCCESS:
    case SIP_SDP_SESSION_AUDIT:
    case SIP_SDP_NOT_PRESENT:
        ccb->oa_state = OA_IDLE;
        
        switch (ccb->featuretype) {
        case CC_FEATURE_HOLD:
            sip_cc_mv_msg_body_to_cc_msg(&data.hold.msg_body, response);
            sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, ccb->featuretype,
                               &data, CC_CAUSE_NORMAL);
            break;
        case CC_FEATURE_RESUME:
        case CC_FEATURE_MEDIA:
            sip_cc_mv_msg_body_to_cc_msg(&data.resume.msg_body, response);
            sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, ccb->featuretype,
                               &data, CC_CAUSE_NORMAL);
            break;
        default:
            


            CCSIP_DEBUG_ERROR(DEB_L_C_F_PREFIX"%d: unexpected feature %d\n",
                              ccb->dn_line, ccb->gsm_id, fname,
                              ccb->index, ccb->featuretype);
            sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, ccb->featuretype,
                               NULL, CC_CAUSE_ERROR);
            break;
        }
        break;

    case SIP_SDP_DNS_FAIL:
    case SIP_SDP_NO_MEDIA:
    case SIP_SDP_ERROR:
    default:
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, ccb->featuretype, NULL,
                           CC_CAUSE_ERROR);
        break;
    }

    free_sip_message(response);
    sip_sm_change_state(ccb, SIP_STATE_ACTIVE);
}

void
ccsip_handle_accept_2xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    cc_feature_data_t data;
    cc_features_t   feature;
    sipMessage_t   *response;
    int             response_code = 0;
    const char     *cseq = NULL;
    sipCseq_t      *sipCseq = NULL;
    char           *fname = "ccsip_handle_accept_2xx";
    sipMethod_t     response_method;

    response = event->u.pSipMessage;
    if (sipGetResponseCode(response, &response_code) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseCode");
        free_sip_message(response);
        return;
    }

    
    ccsip_update_callinfo(ccb, response, TRUE, FALSE, FALSE);

    
    
    
    
    
    
    
    
    
    cseq = sippmh_get_cached_header_val(response, CSEQ);
    if (!cseq) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sippmh_get_cached_header_val(CSEQ)");
        free_sip_message(response);
        return;
    }
    sipCseq = sippmh_parse_cseq(cseq);
    if (!sipCseq) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sippmh_parse_cseq()");
        free_sip_message(response);
        return;
    }
    response_method = sipCseq->method;
    cpr_free(sipCseq);

    if ((response_code == SIP_SUCCESS_SETUP) &&
        (response_method == sipMethodNotify)) {
        clean_method_request_trx(ccb, sipMethodNotify, TRUE);
        free_sip_message(response);
        return;
    }

    if (response_code != SIP_ACCEPTED) {
        if (sipSPISendAck(ccb, response) != TRUE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipSPISendAck");
            free_sip_message(response);
            return;
        }
    }
    switch (ccb->featuretype) {
    case CC_FEATURE_B2BCONF:
    case CC_FEATURE_SELECT:
    case CC_FEATURE_B2B_JOIN:
    case CC_FEATURE_CANCEL:
        ccb->authen.cred_type = 0;
        feature = ccb->featuretype;
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature, NULL,
                           CC_CAUSE_OK);
        clean_method_request_trx(ccb, sipMethodRefer, TRUE);
        free_sip_message(response);
        return;
    default:
        break;
    }

    data.xfer.cause = CC_CAUSE_XFER_LOCAL;
    data.xfer.method = CC_XFER_METHOD_REFER;
    data.xfer.dialstring[0] = '\0';
    data.xfer.target_call_id = ccb->gsm_id;
    feature = fsmxfr_type_to_feature(fsmxfr_get_xfr_type(ccb->gsm_id));

    sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature, &data, CC_CAUSE_OK);
    clean_method_request_trx(ccb, sipMethodRefer, TRUE);
    free_sip_message(response);
}




void
ccsip_handle_recvmidcallinvite_ccfeatureackpending_ev_cc_feature_ack (
                                           ccsipCCB_t *ccb,
                                           sipSMEvent_t *event)
{
    cc_features_t feature_type;
    cc_msgbody_info_t *msg_body;

    feature_type = event->u.cc_msg->msg.feature_ack.feature_id;

    switch (feature_type) {
    case CC_FEATURE_HOLD:
        if (event->u.cc_msg->msg.feature_ack.data_valid) {
            
            msg_body = &event->u.cc_msg->msg.feature_ack.data.hold.msg_body;
            ccsip_save_local_msg_body(ccb, msg_body);
        }

        sipSPISendInviteResponse200(ccb); 
        sip_sm_change_state(ccb,
                            SIP_STATE_RECV_MIDCALL_INVITE_SIPACK_PENDING);
        break;

    case CC_FEATURE_RESUME:
        
    case CC_FEATURE_MEDIA:
        
        if (event->u.cc_msg->msg.feature_ack.data_valid) {
            msg_body = &event->u.cc_msg->msg.feature_ack.data.resume.msg_body;
            ccsip_save_local_msg_body(ccb, msg_body);
        }
        if (event->u.cc_msg->msg.feature_ack.cause ==  CC_CAUSE_PAYLOAD_MISMATCH ||
            event->u.cc_msg->msg.feature_ack.cause ==  CC_CAUSE_NO_MEDIA ||
            event->u.cc_msg->msg.feature_ack.cause ==  CC_CAUSE_ERROR) {
            




            ccb->oa_state = OA_IDLE;
            sipSPISendInviteResponse(ccb, SIP_CLI_ERR_NOT_ACCEPT_HERE,
                                     SIP_CLI_ERR_NOT_ACCEPT_HERE_PHRASE,
                                     SIP_WARN_MISC,
                                     "SDP Not Acceptable",
                                     FALSE,  TRUE );
        } else {
            sipSPISendInviteResponse200(ccb);
        }
        sip_sm_change_state(ccb, SIP_STATE_RECV_MIDCALL_INVITE_SIPACK_PENDING);
        break;

    default:
        break;
    }
}





void
ccsip_handle_recvmidcallinvite_sipackpending_ev_sip_ack (ccsipCCB_t *ccb,
                                                         sipSMEvent_t *event)
{
    const char     *fname =
                 "ccsip_handle_recvmidcallinvite_sipackpending_ev_sip_ack";
    sipMessage_t   *request;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    cc_feature_data_t data;
    sipsdp_status_t sdp_status;

    
    request = event->u.pSipMessage;

    
    if (sip_sm_request_check_and_store(ccb, request, sipMethodAck, FALSE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                          ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        free_sip_message(request);
        return;
    }

    


    sdp_status = sip_util_extract_sdp(ccb, request);

    switch (sdp_status) {
    case SIP_SDP_SUCCESS:
    case SIP_SDP_SESSION_AUDIT:
        if (ccb->oa_state != OA_OFFER_SENT) {
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                              ccb->index, ccb->dn_line, fname,
                              "Unexpected SDP in ACK, releasing call");
            sipSPISendBye(ccb, NULL, NULL);
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            clean_method_request_trx(ccb, sipMethodAck, FALSE);
            clean_method_request_trx(ccb, sipMethodInvite, FALSE);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        }

        




        ccb->oa_state = OA_IDLE;

        



        ccsip_update_callinfo(ccb, request, TRUE, TRUE, TRUE);

        
        sip_cc_mv_msg_body_to_cc_msg(&data.resume.msg_body, request);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_MEDIA, &data);
        break;

    case SIP_SDP_ERROR:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC, "Invalid SDP", ccb);
        clean_method_request_trx(ccb, sipMethodAck, FALSE);
        clean_method_request_trx(ccb, sipMethodInvite, FALSE);
        return;

    case SIP_SDP_DNS_FAIL:
        sipSPISendInviteResponse(ccb, SIP_SERV_ERR_INTERNAL,
                                 SIP_SERV_ERR_INTERNAL_PHRASE,
                                 SIP_WARN_MISC,
                                 "DNS lookup failed for media destination",
                                 FALSE, FALSE);
        break;

    case SIP_SDP_NO_MEDIA:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       "No acceptable media line in SDP", ccb);
        clean_method_request_trx(ccb, sipMethodAck, FALSE);
        clean_method_request_trx(ccb, sipMethodInvite, FALSE);
        return;

    case SIP_SDP_NOT_PRESENT:
        if (ccb->oa_state == OA_OFFER_SENT) {
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                              ccb->index, ccb->dn_line, fname,
                              "No answer SDP in ACK, releasing call");
            sipSPISendBye(ccb, NULL, NULL);
            sip_cc_release(ccb->gsm_id, ccb->dn_line, CC_CAUSE_ERROR, NULL);
            clean_method_request_trx(ccb, sipMethodAck, FALSE);
            clean_method_request_trx(ccb, sipMethodInvite, FALSE);
            sip_sm_change_state(ccb, SIP_STATE_RELEASE);
            return;
        } else {
            
            ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);
        }
        break;

    default:
        
        ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);
        break;

    }

    


    sip_sm_change_state(ccb, SIP_STATE_ACTIVE);
    clean_method_request_trx(ccb, sipMethodAck, FALSE);
    clean_method_request_trx(ccb, sipMethodInvite, FALSE);
}


void
ccsip_handle_transienthold_ev_cc_feature (ccsipCCB_t *ccb, int feature_type,
                                          sipSMStateType_t final_resume_state,
                                          sipSMEvent_t *event)
{
    const char *fname = "transienthold_ev_cc_feature";
    cc_msgbody_info_t *msg_body;

    switch (feature_type) {
    case CC_FEATURE_RESUME:
        if (event->u.cc_msg->msg.feature.data_valid) {
            msg_body = &event->u.cc_msg->msg.feature.data.resume.msg_body;
            ccsip_save_local_msg_body(ccb, msg_body);
        }

        ccb->hold_initiated = FALSE;
        ccb->featuretype = CC_FEATURE_RESUME;
        sip_sm_change_state(ccb, final_resume_state);
        break;

    default:
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type, NULL,
                           CC_CAUSE_ERROR);
        break;
    }
}


int
strcasecmp_ignorewhitespace (const char *cs, const char *ct)
{
    const char *p;
    const char *q;

    if (cpr_strcasecmp(cs, ct) == 0) {
        return (0);
    }

    p = cs;
    q = ct;

    
    while (((*p == ' ') || (*p == '\t')) && (*p != '\0')) {
        p++;
    }
    while (((*q == ' ') || (*q == '\t')) && (*q != '\0')) {
        q++;
    }

    
    while ((*p != ' ') && (*p != '\t') && (*p != '\0') &&
           (*q != ' ') && (*q != '\t') && (*q != '\0')) {
        if (toupper(*p) != toupper(*q)) {
            return (-1);
        }
        p++;
        q++;
    }

    
    while (*p != '\0') {
        if ((*p != ' ') && (*p != '\t')) {
            return (-1);
        }
        p++;
    }
    while (*q != '\0') {
        if ((*q != ' ') && (*q != '\t')) {
            return (-1);
        }
        q++;
    }

    return (0);
}


void
sip_sm_util_normalize_name (ccsipCCB_t *ccb, char *dialString)
{
    char           *outputString;
    uint32_t        usernameLength = 0;
    const char     *hostnameString = NULL;
    uint32_t        hostnameLength = 0;
    char            proxy_ipaddr_str[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t   proxy_ipaddr;
    char           *extraString = NULL;
    char           *parse_token;
    uint32_t        extraLength = 0;
    uint32_t        n = 0;
    static char     dialtranslate[MAX_SIP_URL_LENGTH];
    char            dest_sip_addr_str[MAX_IPADDR_STR_LEN];
    char            addr[MAX_IPADDR_STR_LEN];
    int             port;
    int             dialStringLength;

    CPR_IP_ADDR_INIT(proxy_ipaddr);
    


    dialStringLength = strlen(dialString);
    memcpy((void *)ccb->calledDisplayedName, dialString, dialStringLength);
    


    ccb->routeMode = RouteDefault;
    (void) MatchDialTemplate(ccb->calledDisplayedName, ccb->dn_line, CAST_N &n, dialtranslate,
                             sizeof(dialtranslate),
                             (RouteMode *) &(ccb->routeMode), NULL);
    dialString = dialtranslate;
    dialStringLength = strlen(dialString);
    


    if ((dialStringLength > 0) && (dialString[0] == '<')) {
        dialString++;
        dialStringLength--;
    }
    


    if ((dialStringLength > 4) &&
        (tolower(dialString[0]) == 's') &&
        (tolower(dialString[1]) == 'i') &&
        (tolower(dialString[2]) == 'p') &&
        (tolower(dialString[3]) == ':')) {
        dialStringLength -= 4;
        dialString += 4;
    }

    


    parse_token = strpbrk(dialString, "@;>"); 
    if (parse_token == NULL) { 
        usernameLength = dialStringLength;
    } else {
        usernameLength = parse_token - dialString; 
        if (parse_token[0] == '@') { 
            extraString = strpbrk(parse_token, ";>"); 
            if (extraString != NULL) { 
                extraLength = dialStringLength - (extraString - dialString);
            }
            




            if (ccb->proxySelection != SIP_PROXY_BACKUP) {
                hostnameString = parse_token + 1;
                
                hostnameLength = dialStringLength - usernameLength - extraLength - 1;
            }
        } else { 
            extraLength = dialStringLength - usernameLength;
            extraString = parse_token;
        }
    }

    









    if (hostnameLength == 0) {
        



        switch (ccb->routeMode) {
        case RouteEmergency:
                



                if (ccb->proxySelection != SIP_PROXY_BACKUP) {
                    
                    sipTransportGetEmerServerAddress(ccb->dn_line, proxy_ipaddr_str);
                    hostnameString = &proxy_ipaddr_str[0];
                    hostnameLength = strlen(hostnameString);
                    if (hostnameLength) {
                        if (!str2ip((const char *) proxy_ipaddr_str, &proxy_ipaddr)) {
                            
                            util_ntohl(&(ccb->dest_sip_addr), &(proxy_ipaddr));

                            



                            port = sipTransportGetEmerServerPort(ccb->dn_line);
                            if (port) {
                                ccb->dest_sip_port = port;
                            } else {
                                ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
                            }
                            break;
                        }
                    }
                }
                
             
        default:
            





            if (ccb->proxySelection == SIP_PROXY_BACKUP) {
                ipaddr2dotted(dest_sip_addr_str, &ccb->dest_sip_addr);
                hostnameString = dest_sip_addr_str;
                hostnameLength = strlen(hostnameString);
            } else {
                sipTransportGetPrimServerAddress(ccb->dn_line, addr);
                hostnameString = addr;
                hostnameLength = strlen(hostnameString);
                sipTransportGetServerIPAddr(&(ccb->dest_sip_addr), ccb->dn_line);
                ccb->dest_sip_port = sipTransportGetPrimServerPort(ccb->dn_line);
            }
        }
    }

    


    outputString = (char *) ccb->calledNumber;
    sstrncpy(outputString, "<sip:", MAX_SIP_URL_LENGTH);
    outputString += 5;
    if (usernameLength) {
        outputString += sippmh_convertURLCharToEscChar(dialString,
                                                       usernameLength,
                                                       outputString,
                                                       (MAX_SIP_URL_LENGTH * 2) - 5, FALSE);
        *outputString++ = '@';
    }
    if (hostnameLength) {
        memcpy(outputString, hostnameString, hostnameLength);
        outputString += hostnameLength;
    }
    if (extraLength) {
        memcpy(outputString, extraString, extraLength);
        


        outputString[extraLength] = 0;
        


        if (strchr(outputString, '>') == NULL) {
            outputString[extraLength++] = '>';
        }
        outputString += extraLength;
    } else {
        *outputString++ = '>';
    }
    


    *outputString = 0;
    ccb->calledNumberLen = (uint16_t) (outputString - ccb->calledNumber);
}


void
get_sip_error_string (char *errortext, int response)
{

    if (NULL == errortext)
        return;
    switch (response) {
    case SIP_SUCCESS_SETUP:
        sstrncpy(errortext, SIP_SUCCESS_SETUP_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_ACCEPTED:
        sstrncpy(errortext, SIP_ACCEPTED_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_1XX_TRYING:
        sstrncpy(errortext, SIP_1XX_TRYING_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_RED_MULT_CHOICES:
        sstrncpy(errortext, SIP_RED_MULT_CHOICES_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_RED_MOVED_PERM:
        sstrncpy(errortext, SIP_RED_MOVED_PERM_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_RED_MOVED_TEMP:
        sstrncpy(errortext, SIP_RED_MOVED_TEMP_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_RED_SEE_OTHER:
        sstrncpy(errortext, SIP_RED_SEE_OTHER_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_RED_USE_PROXY:
        sstrncpy(errortext, SIP_RED_USE_PROXY_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_RED_ALT_SERVICE:
        sstrncpy(errortext, SIP_RED_ALT_SERVICE_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_BAD_REQ:  
        sstrncpy(errortext, SIP_CLI_ERR_BAD_REQ_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_UNAUTH:   
        sstrncpy(errortext, SIP_CLI_ERR_UNAUTH_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_PAY_REQD: 
        sstrncpy(errortext, SIP_CLI_ERR_PAY_REQD_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_FORBIDDEN:    
        sstrncpy(errortext, SIP_CLI_ERR_FORBIDDEN_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_NOT_FOUND:    
        sstrncpy(errortext, SIP_CLI_ERR_NOT_FOUND_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_NOT_ALLOWED:  
        sstrncpy(errortext, SIP_CLI_ERR_NOT_ALLOWED_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_NOT_ACCEPT:   
        sstrncpy(errortext, SIP_CLI_ERR_NOT_ACCEPT_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_PROXY_REQD:   
        sstrncpy(errortext, SIP_CLI_ERR_PROXY_REQD_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_REQ_TIMEOUT:  
        sstrncpy(errortext, SIP_CLI_ERR_REQ_TIMEOUT_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_CONFLICT: 
        sstrncpy(errortext, SIP_CLI_ERR_CONFLICT_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_GONE:     
        sstrncpy(errortext, SIP_CLI_ERR_GONE_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_LEN_REQD: 
        sstrncpy(errortext, SIP_CLI_ERR_LEN_REQD_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_LARGE_MSG:    
        sstrncpy(errortext, SIP_CLI_ERR_LARGE_MSG_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_LARGE_URI:    
        sstrncpy(errortext, SIP_CLI_ERR_LARGE_URI_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_MEDIA:    
        sstrncpy(errortext, SIP_CLI_ERR_MEDIA_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_EXTENSION:    
        sstrncpy(errortext, SIP_CLI_ERR_EXTENSION_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED:    
        sstrncpy(errortext, SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_NOT_AVAIL:    
        sstrncpy(errortext, SIP_CLI_ERR_NOT_AVAIL_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_CALLEG:   
        sstrncpy(errortext, SIP_CLI_ERR_CALLEG_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_LOOP_DETECT:  
        sstrncpy(errortext, SIP_CLI_ERR_LOOP_DETECT_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_MANY_HOPS:    
        sstrncpy(errortext, SIP_CLI_ERR_MANY_HOPS_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_ADDRESS:  
        sstrncpy(errortext, SIP_CLI_ERR_ADDRESS_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_AMBIGUOUS:    
        sstrncpy(errortext, SIP_CLI_ERR_AMBIGUOUS_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_BUSY_HERE:    
        sstrncpy(errortext, SIP_CLI_ERR_BUSY_HERE_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_REQ_CANCEL:   
        sstrncpy(errortext, SIP_CLI_ERR_REQ_CANCEL_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_CLI_ERR_BAD_EVENT:    
        sstrncpy(errortext, SIP_CLI_ERR_BAD_EVENT_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_SERV_ERR_INTERNAL:    
        sstrncpy(errortext, SIP_SERV_ERR_INTERNAL_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_SERV_ERR_NOT_IMPLEM:  
        sstrncpy(errortext, SIP_SERV_ERR_NOT_IMPLEM_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_SERV_ERR_BAD_GW:  
        sstrncpy(errortext, SIP_SERV_ERR_BAD_GW_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_SERV_ERR_UNAVAIL: 
        sstrncpy(errortext, SIP_SERV_ERR_UNAVAIL_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_SERV_ERR_GW_TIMEOUT:  
        sstrncpy(errortext, SIP_SERV_ERR_GW_TIMEOUT_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_SERV_ERR_SIP_VER: 
        sstrncpy(errortext, SIP_SERV_ERR_SIP_VER_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_SERV_ERR_PRECOND_FAILED:  
        sstrncpy(errortext, SIP_SERV_ERR_PRECOND_FAILED_PHRASE,
                 MAX_SIP_URL_LENGTH);
        break;
    case SIP_FAIL_BUSY:        
        sstrncpy(errortext, SIP_FAIL_BUSY_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_FAIL_DECLINE:     
        sstrncpy(errortext, SIP_FAIL_DECLINE_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_FAIL_NOT_EXIST:   
        sstrncpy(errortext, SIP_FAIL_NOT_EXIST_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    case SIP_FAIL_NOT_ACCEPT:  
        sstrncpy(errortext, SIP_FAIL_NOT_ACCEPT_PHRASE, MAX_SIP_URL_LENGTH);
        break;
    default:
        sstrncpy(errortext, SIP_STATUS_PHRASE_NONE, MAX_SIP_URL_LENGTH);
        break;
    }
}






void
ccsip_handle_early_ev_sip_update (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "early_ev_sip_update";
    sipMessage_t   *request = NULL;
    unsigned short  request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    sipMethod_t     method = sipMethodInvalid;
    sipsdp_status_t sdp_status;
    boolean         display_valid = TRUE;


    
    request = event->u.pSipMessage;

    sipGetRequestMethod(request, &method);

    
    if (get_method_request_trx_index(ccb, method, FALSE) > -1) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Received UPDATE while processing an old one!\n",
                          fname);
        (void) sipSPISendErrorResponse(request, SIP_SERV_ERR_INTERNAL,
                                       SIP_SERV_ERR_INTERNAL_PHRASE,
                                       SIP_WARN_PROCESSING_PREVIOUS_REQUEST,
                                       NULL, NULL);
        free_sip_message(request);
        return;
    }

    
    if (get_method_request_trx_index(ccb, method, TRUE) > -1) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Received UPDATE while old one outstanding!\n",
                          fname);
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_REQ_PENDING,
                                       SIP_CLI_ERR_REQ_PENDING_PHRASE, 0, NULL, NULL);
        free_sip_message(request);
        return;
    }

    memset(request_check_reason_phrase, 0, SIP_WARNING_LENGTH);

    
    if (sip_sm_request_check_and_store(ccb, request, method, TRUE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       request_check_reason_code,
                                       request_check_reason_phrase, NULL);
        free_sip_message(request);
        return;
    }

      


    display_valid = ccsip_check_display_validity(ccb, request);
    if (!display_valid) {
         CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Rejecting UPDATE with callerid blocked.Anonymous Callback configured!\n",
                          fname);
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED,
                                       SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED_PHRASE,
                                       SIP_WARN_PROCESSING_PREVIOUS_REQUEST,
                                       NULL, NULL);
        return;
    }

    
    

    
    
    if (!(ccb->flags & INCOMING)) {
        const char *from;

        from = sippmh_get_cached_header_val(request, FROM);
        ccb->sip_to = strlib_update(ccb->sip_to, from);
    }

    
    sdp_status = sip_util_extract_sdp(ccb, request);
    switch (sdp_status) {
    case SIP_SDP_SUCCESS:
        












        if (ccb->oa_state == OA_OFFER_SENT) {
            (void) sipSPISendUpdateResponse(ccb, FALSE, CC_CAUSE_REQUEST_PENDING, FALSE);
        } else {
            



            (void) sipSPISendUpdateResponse(ccb, FALSE, CC_CAUSE_NO_RESOURCE, FALSE);
        }
        return;

    case SIP_SDP_ERROR:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC, "Invalid SDP", ccb);
        return;

    case SIP_SDP_DNS_FAIL:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_SERV_ERR_INTERNAL,
                                       SIP_SERV_ERR_INTERNAL_PHRASE,
                                       SIP_WARN_MISC,
                                       "DNS lookup failed for media destination", ccb);
        return;

    case SIP_SDP_NO_MEDIA:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       "No acceptable media line in SDP", ccb);
        return;

    case SIP_SDP_NOT_PRESENT:
    default:
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX":Update received without SDP\n", fname);
        break;
    }

    
    ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);

    







    (void) sipSPISendUpdateResponse(ccb, FALSE, CC_CAUSE_OK, FALSE);
}






void
ccsip_handle_early_ev_sip_update_response (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
}






void
ccsip_handle_early_ev_cc_feature (ccsipCCB_t *ccb, sipSMEvent_t *event)
{

    const char     *fname = "early_ev_cc_feature";
    cc_features_t   feature_type;
    cc_msgbody_info_t *msg_body;

    feature_type = event->u.cc_msg->msg.feature.feature_id;
    if (feature_type == CC_FEATURE_UPDATE) {
        if (event->u.cc_msg->msg.feature.data_valid) {
            msg_body = &event->u.cc_msg->msg.feature.data.update.msg_body;
            ccsip_save_local_msg_body(ccb, msg_body);
        }
        (void) sipSPISendUpdate(ccb);
    } else if (feature_type == CC_FEATURE_SELECT) {
    } else {
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        sip_cc_feature_ack(ccb->gsm_id, ccb->dn_line, feature_type, NULL,
                           CC_CAUSE_ERROR);
    }
}







void
ccsip_handle_early_ev_cc_feature_ack (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "early_ev_cc_feature_ack";
    cc_features_t   feature_type;
    cc_msgbody_info_t *msg_body;

    feature_type = event->u.cc_msg->msg.feature_ack.feature_id;

    switch (feature_type) {
    case CC_FEATURE_UPDATE:
        if (event->u.cc_msg->msg.feature.data_valid) {
            msg_body = &event->u.cc_msg->msg.feature.data.update.msg_body;
            ccsip_save_local_msg_body(ccb, msg_body);
        }
        (void) sipSPISendUpdateResponse(ccb, event->u.cc_msg->msg.feature.data_valid,
                                        event->u.cc_msg->msg.feature_ack.cause,
                                        FALSE);
        break;

    default:
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_FEATURE_UNSUPPORTED),
                          ccb->index, ccb->dn_line, fname);
        CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_STATE_UNCHANGED),
                          ccb->index, ccb->dn_line, fname,
                          sip_util_state2string(ccb->state));
        break;
    }
}















void
ccsip_handle_active_ev_sip_update (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "active_ev_sip_update";
    sipMessage_t   *request;
    const char     *require = NULL;
    const char     *contact = NULL;
    uint16_t        request_check_reason_code = 0;
    char            request_check_reason_phrase[SIP_WARNING_LENGTH];
    cc_feature_data_t data;
    sipsdp_status_t sdp_status;
    boolean         display_valid = TRUE;

    
    request = event->u.pSipMessage;

    
    if (get_method_request_trx_index(ccb, sipMethodUpdate, FALSE) > -1) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Received UPDATE while processing an old one!\n",
                          fname);
        (void) sipSPISendErrorResponse(request, SIP_SERV_ERR_INTERNAL,
                                       SIP_SERV_ERR_INTERNAL_PHRASE,
                                       SIP_WARN_PROCESSING_PREVIOUS_REQUEST,
                                       NULL, NULL);
        free_sip_message(request);
        return;
    }

    
    if (sip_sm_request_check_and_store(ccb, request, sipMethodUpdate, TRUE,
                                       &request_check_reason_code,
                                       request_check_reason_phrase, FALSE) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname,
                          get_debug_string(DEBUG_FUNCTIONNAME_SIP_SM_REQUEST_CHECK_AND_STORE));
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       request_check_reason_code,
                                       request_check_reason_phrase, NULL);
        free_sip_message(request);
        return;
    }

    


    display_valid = ccsip_check_display_validity(ccb, request);
    if (!display_valid) {
         CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Rejecting UPDATE with callerid blocked.Anonymous Callback configured!\n",
                          fname);
        (void) sipSPISendErrorResponse(request, SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED,
                                       SIP_CLI_ERR_ANONYMITY_NOT_ALLOWED_PHRASE,
                                       SIP_WARN_PROCESSING_PREVIOUS_REQUEST,
                                       NULL, NULL);
        return;
    }

    
    require = sippmh_get_cached_header_val(request, REQUIRE);
    if (require) {
        ccb->sip_require = strlib_update(ccb->sip_require, require);
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Unsupported Require Header in UPDATE\n",
                          DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
        sipSPISendInviteResponse(ccb, SIP_CLI_ERR_EXTENSION,
                                 SIP_CLI_ERR_EXTENSION_PHRASE,
                                 0, NULL, FALSE,  TRUE );
        return;
    }

    
    contact = sippmh_get_cached_header_val(request, CONTACT);
    if (contact) {
        if (ccb->contact_info) {
            sippmh_free_contact(ccb->contact_info);
        }
        ccb->contact_info = sippmh_parse_contact(contact);
    }



    


    sdp_status = sip_util_extract_sdp(ccb, request);

    switch (sdp_status) {
    case SIP_SDP_SESSION_AUDIT:
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Received Session Audit SDP in UPDATE\n",
            DEB_F_PREFIX_ARGS(SIP_SDP, fname));
        

    case SIP_SDP_SUCCESS:
        








        if (ccb->oa_state != OA_IDLE) {
            cc_causes_t cause;

            cause = (ccb->oa_state == OA_OFFER_SENT ?
                    CC_CAUSE_REQUEST_PENDING : CC_CAUSE_NO_RESOURCE);
            (void) sipSPISendUpdateResponse(ccb, FALSE, cause, FALSE);
            return;
        }
        


        if (sdp_status == SIP_SDP_SESSION_AUDIT) {
            
            ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);
        } else {
            
            ccsip_update_callinfo(ccb, request, TRUE, TRUE, TRUE);
        }

        




        ccb->oa_state = OA_OFFER_RECEIVED;
        
        sip_cc_mv_msg_body_to_cc_msg(&data.resume.msg_body, request);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_MEDIA, &data);
        sip_sm_change_state(ccb,
                            SIP_STATE_RECV_UPDATEMEDIA_CCFEATUREACK_PENDING);
        break;

    case SIP_SDP_ERROR:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC, "Invalid SDP", ccb);
        return;

    case SIP_SDP_DNS_FAIL:
        sipSPISendInviteResponse(ccb, SIP_SERV_ERR_INTERNAL,
                                 SIP_SERV_ERR_INTERNAL_PHRASE,
                                 SIP_WARN_MISC,
                                 "DNS lookup failed for media destination",
                                 FALSE, FALSE);
        return;

    case SIP_SDP_NO_MEDIA:
        (void) sipSPISendErrorResponse(ccb->last_request, SIP_CLI_ERR_BAD_REQ,
                                       SIP_CLI_ERR_BAD_REQ_PHRASE,
                                       SIP_WARN_MISC,
                                       "No acceptable media line in SDP", ccb);
        return;

    case SIP_SDP_NOT_PRESENT:
    default:
        
        ccsip_update_callinfo(ccb, request, TRUE, TRUE, FALSE);
        sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_UPDATE, NULL);
        (void) sipSPISendUpdateResponse(ccb, FALSE, CC_CAUSE_OK, FALSE);
        break;
    }

}








void
ccsip_handle_recvupdatemedia_ccfeatureackpending_ev_cc_feature_ack (
                                                           ccsipCCB_t *ccb,
                                                           sipSMEvent_t *event)
{
    cc_features_t   feature_type;
    cc_causes_t     cause;
    cc_msgbody_info_t *msg_body;

    feature_type = event->u.cc_msg->msg.feature_ack.feature_id;
    cause = event->u.cc_msg->msg.feature_ack.cause;

    switch (feature_type) {
    case CC_FEATURE_HOLD:
        if (cause == CC_CAUSE_NORMAL) {
            cause = CC_CAUSE_OK;
        }
        if (event->u.cc_msg->msg.feature_ack.data_valid) {
            msg_body = &event->u.cc_msg->msg.feature_ack.data.hold.msg_body;
            ccsip_save_local_msg_body(ccb, msg_body);
        }
        (void) sipSPISendUpdateResponse(ccb,
                                        event->u.cc_msg->msg.feature.data_valid,
                                        cause, FALSE);
        sip_sm_change_state(ccb, SIP_STATE_ACTIVE);
        break;

    case CC_FEATURE_RESUME:
    case CC_FEATURE_MEDIA:
        if (cause == CC_CAUSE_NORMAL || cause == CC_CAUSE_NO_RESUME) {
            cause = CC_CAUSE_OK;
        }

        
        if (event->u.cc_msg->msg.feature_ack.data_valid) {
            msg_body = &event->u.cc_msg->msg.feature_ack.data.resume.msg_body;
            ccsip_save_local_msg_body(ccb, msg_body);
        }

        (void) sipSPISendUpdateResponse(ccb,
                                        event->u.cc_msg->msg.feature.data_valid,
                                        cause, FALSE);
        sip_sm_change_state(ccb, SIP_STATE_ACTIVE);
        break;

    default:
        break;
    }
}







void
ccsip_handle_timer_glare_avoidance (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "timer_glare_avoidance";

    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY), ccb->index,
                      ccb->dn_line, fname, "Resending message");

    
    if (ccb->state == SIP_STATE_IDLE ||
        ccb->state == SIP_STATE_RELEASE) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"LINE %d CCB no longer used - message not sent!\n",
                          fname, ccb->index);
        return;
    }

    
    
    
    (void) sipSPISendInviteMidCall(ccb, FALSE);
}













sipServiceControl_t *
ccsip_get_notify_service_control (sipMessage_t *pSipMessage)
{
    const char     *fname = "ccsip_get_notify_service_control";
    sipServiceControl_t *scp;
    line_t          i;
    ccsipCCB_t     *ccb = NULL;
    boolean         param_match = FALSE;

    
    if (pSipMessage->mesg_body[0].msgBody == NULL) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY with no body\n", DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
        if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                    SIP_CLI_ERR_BAD_REQ_PHRASE,
                                    SIP_WARN_MISC,
                                    SIP_CLI_ERR_BAD_REQ_NO_BODY,
                                    NULL) != TRUE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                              fname, SIP_CLI_ERR_BAD_REQ);
        }
        return NULL;
    }
    if (pSipMessage->mesg_body[0].msgContentTypeValue != SIP_CONTENT_TYPE_TEXT_PLAIN_VALUE) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY with unknown body type\n",
            DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
        if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                    SIP_CLI_ERR_BAD_REQ_PHRASE,
                                    SIP_WARN_MISC,
                                    SIP_CLI_ERR_BAD_REQ_BAD_BODY_ENCODING,
                                    NULL) != TRUE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                              fname, SIP_CLI_ERR_BAD_REQ);
        }
        return NULL;
    }
    
    scp = sippmh_parse_service_control_body(pSipMessage->mesg_body[0].msgBody,
                                pSipMessage->mesg_body[0].msgLength);

    if (scp == NULL) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY but couldn't parse body\n",
            DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
        if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                    SIP_CLI_ERR_BAD_REQ_PHRASE,
                                    SIP_WARN_MISC,
                                    SIP_CLI_ERR_BAD_REQ_BAD_BODY_ENCODING,
                                    NULL) != TRUE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                              fname, SIP_CLI_ERR_BAD_REQ);
        }
        return NULL;
    }
    
    if (scp->registerCallID == NULL) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY but no mandatory params\n",
            DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
        if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                    SIP_CLI_ERR_BAD_REQ_PHRASE,
                                    0, NULL, NULL) != TRUE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                              fname, SIP_CLI_ERR_BAD_REQ);
        }
        sippmh_free_service_control_info(scp);
        return NULL;
    }


    if (scp->action == SERVICE_CONTROL_ACTION_CHECK_VERSION) {
        
        boolean all_provided;

        all_provided = ((scp->configVersionStamp != NULL) &&
                        (scp->dialplanVersionStamp != NULL) &&
                        (scp->softkeyVersionStamp != NULL)) ? TRUE : FALSE;
        if (!all_provided) {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY but no mandatory params\n",
                DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
            if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                        SIP_CLI_ERR_BAD_REQ_PHRASE,
                                        0, NULL, NULL) != TRUE) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                                  fname, SIP_CLI_ERR_BAD_REQ);
            }
            sippmh_free_service_control_info(scp);
            return NULL;
        }
    }

    if (scp->action == SERVICE_CONTROL_ACTION_APPLY_CONFIG) {
        
        boolean all_provided;

        





        all_provided = ((scp->configVersionStamp != NULL) &&
                        (scp->dialplanVersionStamp != NULL) &&
                        (scp->softkeyVersionStamp != NULL) &&
                        (scp->cucm_result != NULL) &&
                        (scp->firmwareLoadId != NULL) &&
                        (scp->loadServer != NULL) &&
                        (scp->logServer != NULL)) ? TRUE : FALSE;
        if (!all_provided)
        {
            CCSIP_DEBUG_TASK(SIP_F_PREFIX "Incorrect message format or missing "
                   "param value for [configVer/cucmResult] in"
                   " apply-config NOTIFY\n\n"
                   "configVersionStamp=%s \ndialplanVersionStamp=%s"
                   "\nsoftkeyVersionStamp=%s \ncucmResult=%s "
                   "\nloadId=%s \ninactiveLoadId=%s \nloadServer=%s \nlogServer=%s "
                   "\nppid=%s\n\n", fname,
                   (scp->configVersionStamp != NULL) ? scp->configVersionStamp
                                                     : "",
                   (scp->dialplanVersionStamp != NULL) ?
                               scp->dialplanVersionStamp:"",
                   (scp->softkeyVersionStamp != NULL) ?
                               scp->softkeyVersionStamp : "",
                   scp->cucm_result != NULL ? scp->cucm_result : "",
                   (scp->firmwareLoadId != NULL) ? scp->firmwareLoadId : "",
                   (scp->firmwareInactiveLoadId != NULL) ? scp->firmwareInactiveLoadId : "",
                   (scp->loadServer != NULL) ? scp->loadServer : "",
                   (scp->logServer != NULL) ? scp->logServer : "",
                   scp->ppid == TRUE? "True": "False");

            if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                        SIP_CLI_ERR_BAD_REQ_PHRASE,
                                        0, NULL, NULL) != TRUE) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                                  fname, SIP_CLI_ERR_BAD_REQ);
            }
            sippmh_free_service_control_info(scp);
            return NULL;
        }
        



        if (scp->firmwareInactiveLoadId == NULL) {
            scp->firmwareInactiveLoadId = cpr_calloc(1, 2);
        }
    }


    
    
    for (i = REG_CCB_START; i <= TEL_CCB_END + 1; i++) {
        ccb = sip_sm_get_ccb_by_index(i);
        if (ccb) {
            if (strcmp(scp->registerCallID, ccb->sipCallID) == 0) {
                param_match = TRUE;
                break;
            }
        }
    }

    
    if (param_match && ccb != NULL) {
        if (sip_regmgr_get_cc_mode(ccb->dn_line) != REG_MODE_CCM) {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY in non CCM mode\n",
                DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
            if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                        SIP_CLI_ERR_BAD_REQ_PHRASE,
                                        0, NULL, NULL) != TRUE) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                                  fname, SIP_CLI_ERR_BAD_REQ);
            }
            sippmh_free_service_control_info(scp);
            return NULL;
        }
    } else {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY, callid doesn't match\n",
            DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
        if (sipSPISendErrorResponse(pSipMessage, SIP_CLI_ERR_BAD_REQ,
                                    SIP_CLI_ERR_BAD_REQ_PHRASE,
                                    0, NULL, NULL) != TRUE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                              fname, SIP_CLI_ERR_BAD_REQ);
        }
        sippmh_free_service_control_info(scp);
        return NULL;
    }

    
    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Received NOTIFY, callid matches\n",
        DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
    return scp;
}

void
ccsip_handle_unsolicited_notify (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char     *fname = "ccsip_handle_unsolicited_notify";
    sipMessage_t   *request;
    sipServiceControl_t *scp;

    
    request = event->u.pSipMessage;

    scp = ccsip_get_notify_service_control(request);
    if (scp != NULL) {
        if (scp->action == SERVICE_CONTROL_ACTION_CALL_PRESERVATION) {
            if (ccb->state == SIP_STATE_ACTIVE) {
                sip_cc_feature(ccb->gsm_id, ccb->dn_line, CC_FEATURE_CALL_PRESERVATION, NULL);
            } else {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"SIP state %s ignoring call preservation request\n",
                                  fname, sip_util_state2string(ccb->state));
            }
            if (sipSPISendErrorResponse(request, 200, SIP_SUCCESS_SETUP_PHRASE,
                                        0, NULL, NULL) != TRUE) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                                  fname, SIP_SUCCESS_SETUP);


            }
        } else {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"Unsupported unsolicited notify event\n",
                DEB_F_PREFIX_ARGS(SIP_NOTIFY, fname));
            if (sipSPISendErrorResponse(request, SIP_CLI_ERR_BAD_REQ,
                                        SIP_CLI_ERR_BAD_REQ_PHRASE,
                                        0, NULL, NULL) != TRUE) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_SPI_SEND_ERROR),
                                  fname, SIP_CLI_ERR_BAD_REQ);
            }
        }
        sippmh_free_service_control_info(scp);
    }
}








void free_duped (ccsipCCB_t *dup_ccb)
{

}





























ccsipCCB_t *
create_dupCCB (ccsipCCB_t *origCCB, int dup_flags)
{
    ccsipCCB_t *dupCCB;
    line_t child_line;
    const char *fname = "dupCCB()";
    const char *outOfDialogPrefix = "OutOfDialog--";

    dupCCB = sip_sm_get_ccb_next_available(&child_line); 

    if (dupCCB == NULL) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sip_sm_get_ccb_next_available()"
            " returned null.\n", fname);
        return NULL;
    }

    dupCCB->dup_flags = dup_flags|DUP_CCB;
    dupCCB->mother_ccb = (void *)origCCB;

    dupCCB->flags = origCCB->flags;
    sstrncpy(dupCCB->sipCallID, origCCB->sipCallID, MAX_SIP_CALL_ID);
    dupCCB->gsm_id = origCCB->gsm_id;
    dupCCB->con_call_id = origCCB->con_call_id;
    dupCCB->blind_xfer_call_id = origCCB->blind_xfer_call_id;

    dupCCB->state = origCCB->state;
    dupCCB->index = origCCB->index;
    dupCCB->dn_line = origCCB->dn_line;
    dupCCB->retx_counter = 0;
    dupCCB->type = origCCB->type;

    dupCCB->proxySelection = origCCB->proxySelection;
    dupCCB->outBoundProxyAddr = origCCB->outBoundProxyAddr;
    dupCCB->outBoundProxyPort = origCCB->outBoundProxyPort;

    if (!(dup_flags & DUP_CCB_REINIT_DNS)) {
        dupCCB->SRVhandle = NULL;
        dupCCB->ObpSRVhandle = NULL;
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Reiniting DNS fields in duped ccb\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
    } else {
        
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Not reiniting DNS fields in duped ccb\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
    }

    dupCCB->routeMode = origCCB->routeMode;
    dupCCB->udpId = origCCB->udpId;

    dupCCB->contact_info = NULL;

    if (dup_flags & DUP_CCB_NEW_CALLID) {
        dupCCB->record_route_info = NULL;
        dupCCB->sipCallID[0] = '\0';
        sip_util_get_new_call_id(dupCCB);
        sstrncpy(dupCCB->sipCallID, outOfDialogPrefix,
                sizeof(dupCCB->sipCallID));
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Using new Call-ID for OutofDialog ccb\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
    } else {
        dupCCB->record_route_info =
            sippmh_copy_record_route(origCCB->record_route_info);
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Copied mother CCB's route set.\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
    }

    dupCCB->calledDisplayedName = strlib_update(dupCCB->calledDisplayedName,
                                                origCCB->calledDisplayedName);
    dupCCB->callingNumber = strlib_update(dupCCB->callingNumber,
                                          origCCB->callingNumber);
    dupCCB->callingDisplayName = strlib_update(dupCCB->callingDisplayName,
                                               origCCB->callingDisplayName);
    dupCCB->calledNumber = strlib_update(dupCCB->calledNumber,
                                         origCCB->calledNumber);
    dupCCB->calledNumberLen = origCCB->calledNumberLen;
    dupCCB->calledNumberFirstDigitDialed = origCCB->calledNumberFirstDigitDialed;

    dupCCB->src_addr = origCCB->src_addr;
    dupCCB->dest_sip_addr = origCCB->dest_sip_addr;
    dupCCB->local_port = origCCB->local_port;
    dupCCB->dest_sip_port = origCCB->dest_sip_port;
    


    


    sstrncpy(dupCCB->ReqURI, origCCB->ReqURI, sizeof(dupCCB->ReqURI));
    dupCCB->ReqURIOriginal = strlib_update(dupCCB->ReqURIOriginal,
                                           origCCB->ReqURIOriginal);

    dupCCB->sip_to = strlib_update(dupCCB->sip_to, origCCB->sip_to);
    if (dup_flags & DUP_CCB_NEW_CALLID) {
        char *str, *str2;
        str = strlib_open(dupCCB->sip_to, 0);

        if (str == NULL) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"strlib_open returned NULL while trying"
                              " to process To.", fname);
            return NULL;
        }

        str2 = strstr(str,";tag");
        if (str2 != NULL) {
            *str2 = '\0';
        }
        dupCCB->sip_to = strlib_close(str);
    }

    dupCCB->sip_from = strlib_update(dupCCB->sip_from, origCCB->sip_from);
    if (dup_flags & DUP_CCB_NEW_CALLID) {
        char *str, *str2;
        str = strlib_open(dupCCB->sip_from, 0);

        if (str == NULL) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"strlib_open returned NULL while trying"
                              " to process From.", fname);
            return NULL;
        }

        str2 = strstr(str,";tag");
        if (str2 != NULL) {
            *str2 = '\0';
        }
        dupCCB->sip_from = strlib_close(str);
    }

    {
        char tag[MAX_SIP_URL_LENGTH];
        sip_util_make_tag(tag);

        if (dupCCB->flags & INCOMING) {
            dupCCB->sip_to = strlib_append(dupCCB->sip_to, ";tag=");
            dupCCB->sip_to = strlib_append(dupCCB->sip_to, tag);
            dupCCB->sip_to_tag = strlib_update(dupCCB->sip_to_tag, tag);
        } else {
            dupCCB->sip_from = strlib_append(dupCCB->sip_from, ";tag=");
            dupCCB->sip_from = strlib_append(dupCCB->sip_from, tag);
            dupCCB->sip_from_tag = strlib_update(dupCCB->sip_from_tag, tag);
        }
    }

    dupCCB->sip_contact = strlib_update(dupCCB->sip_contact,
                                        origCCB->sip_contact);

    dupCCB->featuretype = origCCB->featuretype;

    if (dup_flags & DUP_CCB_STOLEN_FEAT_DATA) {
        dupCCB->feature_data = origCCB->feature_data;
        dupCCB->dup_flags |= DUP_CCB_STOLEN_FEAT_DATA;
        origCCB->feature_data = NULL;
        origCCB->dup_flags |= DUP_CCB_STOLEN_FEAT_DATA;
        CCSIP_DEBUG_STATE(DEB_F_PREFIX"Stealing feature_data pointer from mother ccb\n",
                DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
    }

    return dupCCB;
}

void
ccsip_handle_sent_ood_refer_ev_sip_1xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_sent_ood_refer_ev_sip_1xx";

    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      sip_util_event2string(event->type));

    ccsip_handle_sentinvite_ev_sip_1xx(ccb, event);
}

void
ccsip_handle_sent_ood_refer_ev_sip_2xx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_sent_ood_refer_ev_sip_2xx";

    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      sip_util_event2string(event->type));

    ccsip_handle_accept_2xx(ccb, event);
    sip_sm_call_cleanup(ccb);
}

void
ccsip_handle_sent_ood_refer_ev_sip_fxx (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    const char *fname = "ccsip_handle_sent_ood_refer_ev_sip_fxx";
    sipMessage_t   *response;
    sipRespLine_t  *respLine = NULL;
    uint16_t        status_code = 0;
    sipMethod_t     method = sipMethodInvalid;

    CCSIP_DEBUG_STATE(get_debug_string(DEBUG_FUNCTION_ENTRY),
                      ccb->index, ccb->dn_line, fname,
                      sip_util_state2string(ccb->state),
                      sip_util_event2string(event->type));

    response = event->u.pSipMessage;

    if (sipGetResponseMethod(response, &method) < 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "sipGetResponseMethod");
        free_sip_message(response);
        return;
    }

    
    respLine = sippmh_get_response_line(response);
    if (respLine) {
        status_code = respLine->status_code;
        
        SIPPMH_FREE_RESPONSE_LINE(respLine);
    }

    ccsip_handle_sentinvite_ev_sip_fxx(ccb, event);

    if ((status_code != SIP_CLI_ERR_UNAUTH) &&
        (status_code != SIP_CLI_ERR_PROXY_REQD)) {
        sip_sm_call_cleanup(ccb);
    }
}

void
ccsip_handle_ev_cc_info (ccsipCCB_t *ccb, sipSMEvent_t *event)
{
    sipSPISendInfo(ccb,
                   (const char *)event->u.cc_msg->msg.info.info_package,
                   (const char *)event->u.cc_msg->msg.info.content_type,
                   (const char *)event->u.cc_msg->msg.info.message_body);
}


















static boolean
ccsip_set_replace_info (ccsipCCB_t *ccb, cc_setup_t * setup)
{
    cc_call_info_t *call_info = &setup->call_info;

    if (call_info->type != CC_FEAT_REPLACE) {
        
        return (FALSE);
    }

    if (call_info->data.replace.remote_call_id != CC_NO_CALL_ID) {
        
        strlib_free(ccb->sipxfercallid);
        ccb->sipxfercallid = strlib_empty();
    }
    return (FALSE);
}















static boolean
ccsip_handle_cc_select_event (sipSMEvent_t *sip_sm_event)
{
    cc_feature_t *msg;

    

    msg = &(sip_sm_event->u.cc_msg->msg.feature);
    if (!((msg->msg_id == CC_MSG_FEATURE) &&
          (msg->feature_id == CC_FEATURE_SELECT))) {
        
        return (FALSE);
    }

    return FALSE;
}















static boolean
ccsip_handle_cc_b2bjoin_event (sipSMEvent_t *sip_sm_event)
{
    cc_feature_t *msg;

    

    msg = &(sip_sm_event->u.cc_msg->msg.feature);
    if (!((msg->msg_id == CC_MSG_FEATURE) &&
          (msg->feature_id == CC_FEATURE_B2B_JOIN))) {
        
        return (FALSE);
    }

    return FALSE;
}
















static void
ccsip_set_join_info (ccsipCCB_t *ccb, cc_setup_t * setup)
{
    const char     *fname = "ccsip_set_join_info";
    cc_call_info_t *call_info = &setup->call_info;

    if (((call_info->type != CC_FEAT_BARGE) &&
         (call_info->type != CC_FEAT_CBARGE)) ||
        (call_info->data.join.join_call_id == CC_NO_CALL_ID)) {
        
        return;
    }

    ccb->join_info = (sipJoinInfo_t *) cpr_calloc(1, sizeof(sipJoinInfo_t));

    if (!ccb->join_info) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb->index, ccb->dn_line, fname, "malloc(join_info)");
        return;
    }
}


















static boolean
ccsip_get_join_info (ccsipCCB_t *ccb, sipMessage_t *request)
{
    const char *fname = "ccsip_get_join_info";
    const char *joinhdr = NULL;

    
    joinhdr = sippmh_get_header_val(request, SIP_HEADER_JOIN, NULL);
    if (joinhdr) {

        ccsipCCB_t *join_ccb = NULL;

		
		int ff_check = 0;
		int ft_check = 0;
		int tf_check = 0;
		int tt_check = 0;

        

        if (ccb->join_info) {
            sippmh_free_join_info(ccb->join_info);
        }
        ccb->join_info = sippmh_parse_join_header(joinhdr);

        if (ccb->join_info) {
            join_ccb = sip_sm_get_ccb_by_callid(ccb->join_info->call_id);
        }
        if (join_ccb == NULL) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              0, 0, fname,
                              "Attempted Join call does not exist");
            return (FALSE);
        }

        if (!ccb->in_call_info) {
            ccb->in_call_info = (cc_call_info_t *)
                cpr_calloc(1, sizeof(cc_call_info_t));
        }
        if (!ccb->in_call_info) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              ccb->index, ccb->dn_line, fname,
                              "malloc(call_info)");
            return (FALSE);
        }

        ccb->in_call_info->type = CC_FEAT_MONITOR;

        if (join_ccb->state != SIP_STATE_ACTIVE) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                              join_ccb->index,
                              join_ccb->dn_line, fname,
                              "Attempted Join call is not active or held, but OK to continue.");
            
            
            
        }

		ff_check = cpr_strcasecmp(join_ccb->sip_from_tag, ccb->join_info->from_tag);
		ft_check = cpr_strcasecmp(join_ccb->sip_from_tag, ccb->join_info->to_tag);
		tt_check = cpr_strcasecmp(join_ccb->sip_to_tag, ccb->join_info->to_tag);
		tf_check = cpr_strcasecmp(join_ccb->sip_to_tag, ccb->join_info->from_tag);

		
		
		
		
		if (  !( (ff_check==0 && tt_check==0) || (ft_check==0 && tf_check==0) ) ) {
			CCSIP_DEBUG_ERROR(
                          get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          join_ccb->index,
                          join_ccb->dn_line, fname,
                          "Join Header's From/To tag does not match any ccb's From/To tag");
        	return (FALSE);
		}
		ccb->in_call_info->data.join.join_call_id = join_ccb->gsm_id;
    }
    return (TRUE);
}











char *
getPreallocatedSipCallID (line_t dn_line)
{
    static const char *fname = "getPreallocatedSipCallID";
    uint8_t  mac_address[MAC_ADDRESS_LENGTH];
    char     pSrcAddrStr[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t src_addr;
    int      nat_enable = 0;

    CPR_IP_ADDR_INIT(src_addr);

    if ((dn_line > MAX_REG_LINES) || (dn_line < 1)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"dn_line=%d is greater than %d or less than 1",
                          fname, dn_line, MAX_REG_LINES);
        return NULL;
    }

    


    if (preAllocatedSipCallID[dn_line - 1] != NULL) {
        return (preAllocatedSipCallID[dn_line - 1]);
    }
    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        sip_config_get_net_device_ipaddr(&src_addr);
    } else {
        sip_config_get_nat_ipaddr(&src_addr);
    }

    platform_get_wired_mac_address(mac_address);
    ipaddr2dotted(pSrcAddrStr, &src_addr);

    preAllocatedSipCallID[dn_line - 1] = (char *) cpr_malloc(MAX_SIP_CALL_ID);

    if (preAllocatedSipCallID[dn_line - 1] != NULL) {
        sip_create_new_sip_call_id(preAllocatedSipCallID[dn_line - 1],
                                   mac_address, pSrcAddrStr);
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"malloc failed", fname);
    }

    return (preAllocatedSipCallID[dn_line - 1]);
}











char *
getPreallocatedSipLocalTag (line_t dn_line)
{
    static const char *fname = "getPreallocatedSipLocalTag";

    if ((dn_line > MAX_REG_LINES) || (dn_line < 1)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"dn_line=%d. The valid  range is 1 to %d\n",
                          fname, dn_line, MAX_REG_LINES);
        return NULL;
    }
    if (preAllocatedTag[dn_line - 1] == NULL) {
        preAllocatedTag[dn_line - 1] = (char *) cpr_malloc(MAX_SIP_TAG_LENGTH);
        if (preAllocatedTag[dn_line - 1] == NULL) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"malloc failed\n", fname);
        } else {
            sip_util_make_tag(preAllocatedTag[dn_line - 1]);
        }
    }

    return (preAllocatedTag[dn_line - 1]);
}











char *
ccsip_find_preallocated_sip_local_tag (line_t dn_line)
{
    static const char *fname = "ccsip_find_preallocated_sip_local_tag";

    if ((dn_line > MAX_REG_LINES) || (dn_line < 1)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"dn_line=%d. The valid  range is 1 to %d\n",
                          fname, dn_line, MAX_REG_LINES);
        return NULL;
    }

    return (preAllocatedTag[dn_line - 1]);
}











void
ccsip_free_preallocated_sip_local_tag (line_t dn_line)
{
    static const char *fname = "ccsip_free_preallocated_sip_local_tag";

    if ((dn_line > MAX_REG_LINES) || (dn_line < 1)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"dn_line=%d. The valid  range is 1 to %d\n",
                          fname, dn_line, MAX_REG_LINES);
        return;
    }

    cpr_free(preAllocatedTag[dn_line - 1]);
    preAllocatedTag[dn_line - 1] = NULL;
}











static char *
ccsip_find_preallocated_sip_call_id (line_t dn_line)
{
    static const char *fname = "ccsip_find_preallocated_sip_call_id";

    if ((dn_line > MAX_REG_LINES) || (dn_line < 1)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"dn_line=%d is greater than %d or less than 1\n",
                          fname, dn_line, MAX_REG_LINES);
        return NULL;
    }
    return (preAllocatedSipCallID[dn_line - 1]);
}











static void
ccsip_free_preallocated_sip_call_id (line_t dn_line)
{
    static const char *fname = "ccsip_free_preallocated_sip_call_id";

    if ((dn_line > MAX_REG_LINES) || (dn_line < 1)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"dn_line=%d is greater than %d or less than 1\n",
                          fname, dn_line, MAX_REG_LINES);
        return;
    }
    cpr_free(preAllocatedSipCallID[dn_line - 1]);
    preAllocatedSipCallID[dn_line - 1] = NULL;
}















static boolean
ccsip_handle_cc_hook_event (sipSMEvent_t *sip_sm_event)
{
    static const char *fname = "ccsip_handle_cc_hook_event";
    line_t         line_number = 0;
    char          *sip_call_id = NULL;
    char          *sip_local_tag;
    cc_msg_t      *pCCMsg;
    ccsipCCB_t    *ccb;
    cc_msgs_t      event;

    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Entering with event %d", DEB_F_PREFIX_ARGS(SIP_EVT, fname),
                     sip_sm_event->u.cc_msg->msg.setup.msg_id);

    pCCMsg = sip_sm_event->u.cc_msg;
    event = pCCMsg->msg.setup.msg_id;

    if ((event != CC_MSG_OFFHOOK) && (event != CC_MSG_ONHOOK)) {
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Exiting because event is not hook state\n",
                         DEB_F_PREFIX_ARGS(SIP_EVT, fname));
        return FALSE;
    }

    if (event == CC_MSG_OFFHOOK) {
        line_number = pCCMsg->msg.offhook.line;
    } else {
        line_number = pCCMsg->msg.onhook.line;
    }
    if (sip_regmgr_get_cc_mode(line_number) != REG_MODE_CCM) {
        
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"Exiting Not CCM mode\n",
            DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
        return TRUE;
    }

    


    if (event == CC_MSG_OFFHOOK) {
        ccb = sip_sm_get_ccb_by_gsm_id(pCCMsg->msg.offhook.call_id);
        line_number = pCCMsg->msg.offhook.line;
    } else {
        ccb = sip_sm_get_ccb_by_gsm_id(pCCMsg->msg.onhook.call_id);
        line_number = pCCMsg->msg.onhook.line;
    }
    if (ccb == NULL) {
        





        sip_call_id = getPreallocatedSipCallID(line_number);
        sip_local_tag = getPreallocatedSipLocalTag(line_number);
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"preallocated callid: %s & local-tag: %s\n",
                         DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname), sip_call_id, sip_local_tag);
    } else {
        sip_call_id = ccb->sipCallID;
        sip_local_tag = (char *) (ccb->sip_from_tag);
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"callid: %s & local-tag: %s\n",
                         DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname), sip_call_id, sip_local_tag);
    }

    



    if ((ccb == NULL) && (event == CC_MSG_ONHOOK)) {
        ccsip_free_preallocated_sip_call_id(line_number);
        ccsip_free_preallocated_sip_local_tag(line_number);
    }
    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Exiting after sending hook state "
                     "notification\n",
                     DEB_F_PREFIX_ARGS(SIP_CALL_STATUS, fname));
    return TRUE;
}
