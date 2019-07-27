







#include <stdarg.h>
#include "timecard.h"
#include "cpr.h"
#include "cpr_in.h"
#include "phone.h"
#include "time2.h"
#include "debug.h"
#include "phone_debug.h"
#include "dialplan.h"
#include "ccapi.h"
#include "cfgfile_utils.h"
#include "prot_configmgr.h"
#include "dns_utils.h"
#include "uiapi.h"
#include "lsm.h"
#include "fsm.h"
#include "CCProvider.h"
#include "ccSession.h"
#include "platform_api.h"
#include "vcm.h"
#include "ccapp_task.h"
#include "peer_connection_types.h"











#define CCAPP_F_PREFIX "CCAPP : %s : " // requires 1 arg: __FUNCTION__

#define STRLIB_CREATE(str)  (str)?strlib_malloc((str), strlen((str))):strlib_empty()

#define DEFAULT_DISPLAY_NOTIFY_TIMEOUT   0
#define DEFAULT_DISPLAY_NOTIFY_PRIORITY  0


















extern int Basic_Get_Line_By_Name(char *name);
extern char *Basic_is_phone_forwarded(line_t line);
extern void dp_int_dial_immediate(line_t line, callid_t call_id,
                                  boolean collect_more, char *digit_str,
                                  char *g_call_id,
                                  monitor_mode_t monitor_mode);
extern void dp_int_init_dialing_data(line_t line, callid_t call_id);
extern callid_t dp_get_dialing_call_id(void);
extern void dp_int_update_key_string(line_t line, callid_t call_id,
                                     char *digits);
extern void platform_set_time(int32_t gmt_time);
extern session_id_t createSessionId(line_t line, callid_t call);



















void
ui_call_state (call_events event, line_t nLine, callid_t nCallID, cc_causes_t cause)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"event=%d", DEB_L_C_F_PREFIX_ARGS(UI_API, nLine, nCallID, __FUNCTION__),
              event);

    if (nCallID == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(nLine, nCallID);
    msg.eventID = CALL_STATE;
    msg.update.ccSessionUpd.data.state_data.state = event;
    msg.update.ccSessionUpd.data.state_data.line_id = nLine;
    msg.update.ccSessionUpd.data.state_data.cause = cause;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_STATE(%d) msg", __FUNCTION__, event);
    }
}












void
ui_new_call (call_events event, line_t nLine, callid_t nCallID,
             int call_attr, uint16_t call_instance_id, boolean dialed_digits)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"state=%d attr=%d call_instance=%d, dialed_digits=%s",
              DEB_L_C_F_PREFIX_ARGS(UI_API, nLine, nCallID, __FUNCTION__), event, call_attr, call_instance_id, (dialed_digits)? "true" : "false");

    if (nCallID == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID= createSessionId(nLine, nCallID);

    msg.eventID = CALL_NEWCALL;
    msg.update.ccSessionUpd.data.state_data.state = event;
    msg.update.ccSessionUpd.data.state_data.attr = call_attr;
    msg.update.ccSessionUpd.data.state_data.inst = call_instance_id;
    msg.update.ccSessionUpd.data.state_data.line_id = nLine;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_STATE(%d) msg", __FUNCTION__, event);
    }

    return;
}














void
ui_set_call_attr (line_t line_id, callid_t call_id, call_attr_t attr)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"attr=%d", DEB_L_C_F_PREFIX_ARGS(UI_API, line_id, call_id, __FUNCTION__), attr);

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line_id, call_id);
    msg.eventID = CALL_ATTR;
    msg.update.ccSessionUpd.data.state_data.attr = attr;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_ATTR(%d) msg", __FUNCTION__, attr);
    }
}










void
ui_update_callref (line_t line, callid_t call_id, unsigned int callref)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"callref = %d", DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__), callref);

    if ( callref == 0 ) return;

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = CALL_CALLREF;
    msg.update.ccSessionUpd.data.callref = callref;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_REF() msg", __FUNCTION__);
    }

    return;
}











void
ui_update_gcid (line_t line, callid_t call_id, char *gcid)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"gcid = %s", DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__), gcid);

    if ( *gcid == '\0' ) return;

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = CALL_GCID;
    sstrncpy(msg.update.ccSessionUpd.data.gcid, gcid, CC_MAX_GCID);

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_GCID() msg", __FUNCTION__);
    }

    return;
}









void
ui_update_video_avail (line_t line, callid_t call_id, int avail)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__));

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = VIDEO_AVAIL;
    msg.update.ccSessionUpd.data.action = avail;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send VIDEO_AVAIL() msg", __FUNCTION__);
    }

    return;
}

void ui_update_media_interface_change(line_t line, callid_t call_id, group_call_event_t event) {
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    if (event != MEDIA_INTERFACE_UPDATE_BEGIN &&
        event != MEDIA_INTERFACE_UPDATE_SUCCESSFUL &&
        event != MEDIA_INTERFACE_UPDATE_FAIL) {
        
        return;
    }

    if (event != MEDIA_INTERFACE_UPDATE_BEGIN) {
        
        g_dock_undock_event = MEDIA_INTERFACE_UPDATE_NOT_REQUIRED;
    }

    DEF_DEBUG(DEB_L_C_F_PREFIX "event=%s", DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__),
        event == MEDIA_INTERFACE_UPDATE_BEGIN ? "MEDIA_INTERFACE_UPDATE_BEGIN" :
        event == MEDIA_INTERFACE_UPDATE_SUCCESSFUL ? "MEDIA_INTERFACE_UPDATE_SUCCESSFUL" :
        event == MEDIA_INTERFACE_UPDATE_FAIL ? "MEDIA_INTERFACE_UPDATE_FAIL" : "unknown");
    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }
    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = event;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS )     {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send media update () msg", __FUNCTION__);
    }
}

void
ui_call_stop_ringer (line_t line, callid_t call_id)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__));

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = RINGER_STATE;
    msg.update.ccSessionUpd.data.ringer.start = FALSE;
    msg.update.ccSessionUpd.data.ringer.mode = VCM_RING_OFF;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send RINGER_STATE() msg", __FUNCTION__);
    }

    return;
}

void
ui_call_start_ringer (vcm_ring_mode_t ringMode, short once, line_t line, callid_t call_id)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__));

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = RINGER_STATE;
    msg.update.ccSessionUpd.data.ringer.start = TRUE;
    msg.update.ccSessionUpd.data.ringer.mode = ringMode;
    msg.update.ccSessionUpd.data.ringer.once = (cc_boolean) once;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send RINGER_STATE() msg", __FUNCTION__);
    }

    return;
}









void
ui_update_video_offered (line_t line, callid_t call_id, int avail)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX, DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__));

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = VIDEO_OFFERED;
    msg.update.ccSessionUpd.data.action = avail;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send VIDEO_OFFERED() msg", __FUNCTION__);
    }

    return;
}




















void
ui_call_info (string_t pCallingPartyNameStr,
              string_t pCallingPartyNumberStr,
              string_t pAltCallingPartyNumberStr,
              boolean displayCallingNumber,
              string_t pCalledPartyNameStr,
              string_t pCalledPartyNumberStr,
              boolean displayCalledNumber,
              string_t pOrigCalledNameStr,
              string_t pOrigCalledNumberStr,
              string_t pLastRedirectingNameStr,
              string_t pLastRedirectingNumberStr,
              calltype_t call_type,
              line_t line,
              callid_t call_id,
              uint16_t call_instance_id,
              cc_security_e call_security,
	      cc_policy_e call_policy)
{
    session_update_t msg;
    const char *uiCalledName;
    const char *uiCalledNumber;
    const char *uiCallingName;
    const char *uiCallingNumber;
    int inbound;
    char       lineName[MAX_LINE_NAME_SIZE];
    char       lineNumber[MAX_LINE_NAME_SIZE];

    memset( &msg, 0, sizeof(session_update_t));


    TNP_DEBUG(DEB_L_C_F_PREFIX"call instance=%d callednum=%s calledname=%s clngnum=%s clngname = %s",
        DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__), call_instance_id, pCalledPartyNumberStr,
        pCalledPartyNameStr, pCallingPartyNumberStr, pCallingPartyNameStr);

    TNP_DEBUG(DEB_F_PREFIX"calltype=%d displayClng=%d displayCld=%d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), call_type,
        displayCallingNumber, displayCalledNumber);


    inbound  = (call_type == FSMDEF_CALL_TYPE_INCOMING) || (call_type == FSMDEF_CALL_TYPE_FORWARD);
    uiCalledNumber = pCalledPartyNumberStr;
    uiCalledName = pCalledPartyNameStr;
    uiCallingNumber = pCallingPartyNumberStr;
    uiCallingName = pCallingPartyNameStr;

    config_get_line_string(CFGID_LINE_DISPLAYNAME, lineName, line, sizeof(lineName));
    config_get_line_string(CFGID_LINE_NAME, lineNumber, line, sizeof(lineNumber));

    if (inbound) {
        uiCalledNumber = lineNumber;
        uiCalledName = lineName;
    } else {
        uiCallingNumber = lineNumber;
        uiCallingName = lineName;
    }

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }


    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = CALL_INFORMATION;
    msg.update.ccSessionUpd.data.call_info.clgName = uiCallingName?strlib_malloc(uiCallingName, strlen(uiCallingName)):strlib_empty();

    if ( uiCallingNumber== NULL || (inbound && !displayCallingNumber)) {
        msg.update.ccSessionUpd.data.call_info.clgNumber = strlib_empty();
    } else {
        msg.update.ccSessionUpd.data.call_info.clgNumber = strlib_malloc(uiCallingNumber, strlen(uiCallingNumber));
    }

    if ( pAltCallingPartyNumberStr == NULL ||  (inbound && !displayCallingNumber)) {
        msg.update.ccSessionUpd.data.call_info.altClgNumber = strlib_empty();
    } else {
        msg.update.ccSessionUpd.data.call_info.altClgNumber = strlib_malloc(pAltCallingPartyNumberStr,strlen(pAltCallingPartyNumberStr));
    }

    msg.update.ccSessionUpd.data.call_info.cldName = uiCalledName?strlib_malloc(uiCalledName,strlen(uiCalledName)):strlib_empty();

    if (uiCalledNumber == NULL || (!inbound && !displayCalledNumber)) {
        msg.update.ccSessionUpd.data.call_info.cldNumber = strlib_empty();
    } else {
        msg.update.ccSessionUpd.data.call_info.cldNumber = strlib_malloc(uiCalledNumber,strlen(uiCalledNumber));
    }
    msg.update.ccSessionUpd.data.call_info.origCalledName = pOrigCalledNameStr?strlib_malloc(pOrigCalledNameStr, strlen(pOrigCalledNameStr)):strlib_empty();
    msg.update.ccSessionUpd.data.call_info.origCalledNumber = pOrigCalledNumberStr?strlib_malloc(pOrigCalledNumberStr,strlen(pOrigCalledNumberStr)):strlib_empty();
    msg.update.ccSessionUpd.data.call_info.lastRedirectingName = pLastRedirectingNameStr?strlib_malloc(pLastRedirectingNameStr,strlen(pLastRedirectingNameStr)):strlib_empty();
    msg.update.ccSessionUpd.data.call_info.lastRedirectingNumber = pLastRedirectingNumberStr?strlib_malloc(pLastRedirectingNumberStr, strlen(pLastRedirectingNumberStr)):strlib_empty();
    msg.update.ccSessionUpd.data.call_info.call_type = call_type;
    msg.update.ccSessionUpd.data.call_info.instance_id = call_instance_id;
    msg.update.ccSessionUpd.data.call_info.security = call_security;
    msg.update.ccSessionUpd.data.call_info.policy = call_policy;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_INFO() msg", __FUNCTION__);
    }

    return;
}










void
ui_cc_capability (line_t line, callid_t call_id, string_t recv_info_list)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"recv_info_list:%s",
        DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__),
        recv_info_list);

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = CALL_RECV_INFO_LIST;
    msg.update.ccSessionUpd.data.recv_info_list = strlib_copy(recv_info_list);

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_RECV_INFO_LIST msg", __FUNCTION__);
    }
}












void
ui_info_received (line_t line, callid_t call_id, const char *info_package,
                  const char *content_type, const char *message_body)
{
    session_rcvd_info_t msg;

    TNP_DEBUG(DEB_L_C_F_PREFIX"info_package:%s content_type:%s message_body:%s",
        DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__),
        info_package, content_type, message_body);

    msg.sessionID = createSessionId(line, call_id);
    msg.packageID = INFO_PKG_ID_GENERIC_RAW;
    msg.info.generic_raw.info_package = info_package?strlib_malloc(info_package, strlen(info_package)):strlib_empty();
    msg.info.generic_raw.content_type = content_type?strlib_malloc(content_type, strlen(content_type)):strlib_empty();
    msg.info.generic_raw.message_body = message_body?strlib_malloc(message_body, strlen(message_body)):strlib_empty();

    if ( ccappTaskPostMsg(CCAPP_RCVD_INFO, &msg, sizeof(session_rcvd_info_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_INFO_RECEIVED msg", __FUNCTION__);
    }
}












static void
ui_set_call_status_display (string_t status, line_t line, callid_t callID, int timeout, char priority)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"the stat string =%s, timeout= %d, priority=%d", DEB_L_C_F_PREFIX_ARGS(UI_API, line, callID, __FUNCTION__),
              status,
              timeout,
              priority);

    if (callID == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line, callID);
    msg.eventID = CALL_STATUS;
    msg.update.ccSessionUpd.data.status.timeout = timeout;
    msg.update.ccSessionUpd.data.status.priority = priority;
    if ( status ) {
      msg.update.ccSessionUpd.data.status.status = strlib_malloc(status, strlen(status));
    } else {
      msg.update.ccSessionUpd.data.status.status = strlib_empty();
    }

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_STATUS(%s) msg", __FUNCTION__, status);
    }
}












void
ui_set_call_status (string_t status, line_t line, callid_t callID)
{

    TNP_DEBUG(DEB_L_C_F_PREFIX"the stat string =%s", DEB_L_C_F_PREFIX_ARGS(UI_API, line, callID, __FUNCTION__),
              status);

    if (callID == CC_NO_CALL_ID) {
        
        return;
    }

    ui_set_call_status_display(status, line, callID, DEFAULT_DISPLAY_NOTIFY_TIMEOUT, DEFAULT_DISPLAY_NOTIFY_PRIORITY);
}














void
ui_set_notification (line_t line, callid_t call_id, char *promptString, int timeout,
                     boolean notifyProgress, char priority)
{
    feature_update_t msg;

    TNP_DEBUG(DEB_F_PREFIX"line=%d callid=%d str=%s tout=%d notifyProgress=%d pri=%d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__),
              line, call_id, promptString, timeout, notifyProgress, priority);

    if (line > 0 && call_id > 0) {
        ui_set_call_status_display(promptString, line, call_id, timeout, priority);
        return;
    }
    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_NOTIFICATION;
    msg.update.ccFeatUpd.data.notification.timeout = timeout;
    msg.update.ccFeatUpd.data.notification.notifyProgress = notifyProgress;
    msg.update.ccFeatUpd.data.notification.priority = priority;
    if ( promptString != NULL ) {
       msg.update.ccFeatUpd.data.notification.prompt = strlib_malloc(promptString, strlen(promptString));
    } else {
        msg.update.ccFeatUpd.data.notification.prompt = strlib_empty();
    }

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_NOTIFICATION(%s) msg", __FUNCTION__, promptString);
    }
}









void
ui_clear_notification ()
{
    TNP_DEBUG(DEB_F_PREFIX"called..", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__));

    
    ui_set_notification(CC_NO_LINE, CC_NO_CALL_ID, NULL, 0, FALSE, 1);
}









void
ui_change_mwi_lamp (int status)
{
    feature_update_t msg;


    TNP_DEBUG(DEB_F_PREFIX"status=%d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), status);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_FEATURE_MWILAMP;
    msg.update.ccFeatUpd.data.mwi_status.status = status;

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_FEATURE_MWILAMP(%d) msg", __FUNCTION__, status);
    }
}










void
ui_set_mwi (line_t line, boolean status, int type, int newCount, int oldCount, int hpNewCount, int hpOldCount)
{
    feature_update_t msg;

    TNP_DEBUG(DEB_F_PREFIX"line=%d count=%d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), line, status);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_FEATURE_MWI;
    msg.update.ccFeatUpd.data.mwi_status.line = line;
    msg.update.ccFeatUpd.data.mwi_status.status = status;
    msg.update.ccFeatUpd.data.mwi_status.type = type;
    msg.update.ccFeatUpd.data.mwi_status.newCount = newCount;
    msg.update.ccFeatUpd.data.mwi_status.oldCount = oldCount;
    msg.update.ccFeatUpd.data.mwi_status.hpNewCount = hpNewCount;
    msg.update.ccFeatUpd.data.mwi_status.hpOldCount = hpOldCount;

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_FEATURE_MWI(%d,%d) msg", __FUNCTION__, line, status);
    }
}













void ui_mnc_reached (line_t line, boolean mnc_reached)
{
    feature_update_t msg;

    DEF_DEBUG(DEB_F_PREFIX"line %d: Max number of calls reached =%d",
            DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__),
            line, mnc_reached);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_FEATURE_MNC_REACHED;
    msg.update.ccFeatUpd.data.line_info.line = line;
    msg.update.ccFeatUpd.data.line_info.info = mnc_reached;

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_FEATURE_MNC_REACHED(%d,%d) msg", __FUNCTION__,
			line, mnc_reached);
    }

}









boolean
ui_line_has_mwi_active (line_t line)
{
    session_mgmt_t msg;

    TNP_DEBUG(DEB_F_PREFIX"line=%d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), line);

    msg.func_id = SESSION_MGMT_LINE_HAS_MWI_ACTIVE;
    msg.data.line_mwi_active.line = line;

    ccappSyncSessionMgmt(&msg);

    return msg.data.line_mwi_active.ret_val;
}











void
ui_update_label_n_speeddial (line_t line, line_t button_no, string_t speed_dial, string_t label)
{
    feature_update_t msg;

    TNP_DEBUG(DEB_F_PREFIX"line=%d speeddial=%s displayname=%s", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), line,
                   speed_dial, label);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_LABEL_N_SPEED;
    msg.update.ccFeatUpd.data.cfg_lbl_n_spd.line = line;
    msg.update.ccFeatUpd.data.cfg_lbl_n_spd.button = (unsigned char) button_no;
    msg.update.ccFeatUpd.data.cfg_lbl_n_spd.speed = strlib_malloc(speed_dial, sizeof(speed_dial));
    msg.update.ccFeatUpd.data.cfg_lbl_n_spd.label = strlib_malloc(label, sizeof(label));

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_LABEL_N_SPEED(%d) msg", __FUNCTION__, button_no);
    }
}









void
ui_set_sip_registration_state (line_t line, boolean registered)
{
    feature_update_t msg;
    int value;

    TNP_DEBUG(DEB_F_PREFIX"%s %d: %s", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__),
                (line==CC_ALL_LINES) ? "ALL LINES":"LINE" ,line,
                        (registered)? "REGISTERED":"UN-REGISTERED");

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_REG_STATE;
    msg.update.ccFeatUpd.data.line_info.line = line;
    msg.update.ccFeatUpd.data.line_info.info = registered ? CC_REGISTERED : CC_UNREGISTERED;
    config_get_value(CFGID_PROXY_REGISTER, &value, sizeof(value));
    if (value == 0) {
        msg.update.ccFeatUpd.data.line_info.info = CC_REGISTERED;
    }

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_STATE(%d, %d) msg", __FUNCTION__, line, registered);
    }
}








void
ui_update_registration_state_all_lines (boolean registered)
{
    DEF_DEBUG(DEB_F_PREFIX"***********ALL LINES %s****************",
                        DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__),
                        (registered)? "REGISTERED":"UN-REGISTERED");

    ui_set_sip_registration_state(CC_ALL_LINES, registered);

}









void
ui_reg_all_failed (void)
{
    feature_update_t msg;

    TNP_DEBUG(DEB_F_PREFIX"***********Registration to all CUCMs failed.***********",
                        DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__));

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = CCAPP_REG_ALL_FAIL;
    msg.update.ccFeatUpd.data.line_info.line = CC_ALL_LINES;
    msg.update.ccFeatUpd.data.line_info.info = FALSE;

    if ( ccappTaskPostMsg(CCAPP_REG_ALL_FAIL, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_STATE() msg", __FUNCTION__);
    }
}










void
ui_set_ccm_conn_status (char * ccm_addr, int status)
{
    feature_update_t msg;

    DEF_DEBUG(DEB_F_PREFIX"***********CUCM %s %s***********",
            DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), ccm_addr,
            ((status == 0) ?"Not connected":((status == 1)?"STAND BY":
            ((status == 2)?"ACTIVE":"UNKNOWN"))));

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_CCM_CONN_STATUS;
    msg.update.ccFeatUpd.data.ccm_conn.addr = ccm_addr?strlib_malloc(ccm_addr, strlen(ccm_addr)):strlib_empty();
    msg.update.ccFeatUpd.data.ccm_conn.status = status;

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_CCM_CONN_STATUS(%d) msg", __FUNCTION__, status);
    }
}









void
ui_set_local_hold (line_t line, callid_t call_id)
{
    
    TNP_DEBUG(DEB_L_C_F_PREFIX"called", DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, "ui_set_local_hold"));
    return;
}











void
ui_cfwd_status (line_t line, boolean cfa, char *cfa_number, boolean lcl_fwd)
{
    feature_update_t msg;

    TNP_DEBUG(DEB_F_PREFIX"line=%d cfa=%d cfa_number=%s lcl_fwd=%d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__),
              line, cfa, cfa_number, lcl_fwd);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_FEATURE_CFWD;
    msg.update.ccFeatUpd.data.cfwd.line = line;
    msg.update.ccFeatUpd.data.cfwd.isFwd = cfa;
    msg.update.ccFeatUpd.data.cfwd.isLocal = lcl_fwd;
    msg.update.ccFeatUpd.data.cfwd.cfa_num = cfa_number?strlib_malloc(cfa_number, strlen(cfa_number)):strlib_empty();

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_FEATURE_CFWD(%d) msg", __FUNCTION__, cfa);
    }
}






char *
ui_get_idle_prompt_string (void)
{
    TNP_DEBUG(DEB_F_PREFIX"called", DEB_F_PREFIX_ARGS(UI_API, "ui_get_idle_prompt_string"));
    return platform_get_phrase_index_str(IDLE_PROMPT);
}









void
ui_set_idle_prompt_string (string_t pString, int prompt)
{
    TNP_DEBUG(DEB_F_PREFIX"Prompt=%d, Prompt string=%s NOP operation", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), prompt, pString);
}











void
ui_update_placed_call_info (line_t line, callid_t call_id, string_t cldName,
                            string_t cldNumber)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"calledName:calledNumber %s:%s",
              DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__), cldName, cldNumber);

    if (call_id == CC_NO_CALL_ID) {
        
        TNP_DEBUG(DEB_F_PREFIX"invalid callid", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__));
        return;
    }
    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = CALL_PLACED_INFO;
	msg.update.ccSessionUpd.data.plcd_info.cldName = strlib_empty();
	msg.update.ccSessionUpd.data.plcd_info.cldNum = strlib_empty();

	if ( cldName) {
      msg.update.ccSessionUpd.data.plcd_info.cldName = strlib_update(
		  msg.update.ccSessionUpd.data.plcd_info.cldName, cldName);
	}
	if ( cldNumber) {
      msg.update.ccSessionUpd.data.plcd_info.cldNum = strlib_update(
		  msg.update.ccSessionUpd.data.plcd_info.cldNum, cldNumber);
	}

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_PLACED_INFO(%s) msg", __FUNCTION__, cldNumber);
    }
}










void
ui_delete_last_digit (line_t line_id, callid_t call_id)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"called", DEB_L_C_F_PREFIX_ARGS(UI_API, line_id, call_id, __FUNCTION__));

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    msg.sessionID = createSessionId(line_id, call_id);
    msg.eventID = CALL_DELETE_LAST_DIGIT;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_DELETE_LAST_DIGIT() msg", __FUNCTION__);
    }
}










void
ui_control_featurekey_bksp (line_t line_id, callid_t call_id, boolean enable)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"enable=%d", DEB_L_C_F_PREFIX_ARGS(UI_API, line_id, call_id, __FUNCTION__),
              enable);

    msg.sessionID = createSessionId(line_id, call_id);
    msg.eventID = CALL_ENABLE_BKSP;
    msg.update.ccSessionUpd.data.action = enable;
    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_ENABLE_BKSP(%d) msg", __FUNCTION__, enable);
    }
}













void
ui_call_selected (line_t line_id, callid_t call_id, int selected)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"selected=%d",
              DEB_L_C_F_PREFIX_ARGS(UI_API, line_id, call_id, __FUNCTION__), selected);

    msg.sessionID = createSessionId(line_id, call_id);
    msg.eventID = CALL_SELECTED;
    msg.update.ccSessionUpd.data.action = selected;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_SELECTED(%d) msg", __FUNCTION__, selected);
    }
}








void ui_BLF_notification (int request_id, cc_blf_state_t blf_state, int app_id)
{
    feature_update_t msg;

    TNP_DEBUG(DEB_F_PREFIX"state=%d app_id=%d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), blf_state, app_id);

    msg.sessionType = SESSIONTYPE_CALLCONTROL;
    msg.featureID = DEVICE_FEATURE_BLF;
    msg.update.ccFeatUpd.data.blf_data.state = blf_state;
    msg.update.ccFeatUpd.data.blf_data.request_id = request_id;
    msg.update.ccFeatUpd.data.blf_data.app_id = app_id;

    if ( ccappTaskPostMsg(CCAPP_FEATURE_UPDATE, &msg, sizeof(feature_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send DEVICE_FEATURE_BLF(state=%d, app_id=%d) msg",
                    __FUNCTION__, blf_state, app_id);
    }
}















void
ui_call_in_preservation (line_t line_id, callid_t call_id)
{
    TNP_DEBUG(DEB_L_C_F_PREFIX"called", DEB_L_C_F_PREFIX_ARGS(UI_API, line_id, call_id, __FUNCTION__));

    

    ui_call_state (evCallPreservation , line_id, call_id, CC_CAUSE_NORMAL);
}




















void
ui_select_feature_key_set (line_t line_id, callid_t call_id, char *set_name,
                           int sk_mask_list[], int len)
{
    int i;
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"called", DEB_L_C_F_PREFIX_ARGS(UI_API, line_id, call_id, __FUNCTION__));

    if (call_id == CC_NO_CALL_ID) {
        
        return;
    }

    if (len <= 0 || len > MAX_SOFT_KEYS) {
        TNP_DEBUG(DEB_F_PREFIX"Incorrect softkey array length passed in : %d", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), len);
        return;
    }

    msg.sessionID = createSessionId(line_id, call_id);
    msg.eventID = CALL_SELECT_FEATURE_SET;

    if ( set_name == NULL ) {
       
       return;
    }

    msg.update.ccSessionUpd.data.feat_set.featSet = set_name?strlib_malloc(set_name, sizeof(set_name)):strlib_empty();
    for (i = 0; i < len; i++) {
      msg.update.ccSessionUpd.data.feat_set.featMask[i] = sk_mask_list[i];
    }

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_SELECT_FEATURE_SET() msg", __FUNCTION__);
    }
}











void
ui_execute_uri (char *uri)
{
    session_mgmt_t msg;

    TNP_DEBUG(DEB_F_PREFIX"uri=%s", DEB_F_PREFIX_ARGS(UI_API, __FUNCTION__), uri);

    msg.func_id = SESSION_MGMT_EXECUTE_URI;
    msg.data.uri.uri = STRLIB_CREATE(uri);

    if ( ccappTaskPostMsg(CCAPP_SESSION_MGMT, &msg, sizeof(session_mgmt_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(DEB_F_PREFIX"failed to send EXECUTE_URI() msg", DEB_F_PREFIX_ARGS(PLAT_API, __FUNCTION__));
    }
}
















void
ui_update_call_security (line_t line, callid_t call_id,
                        cc_security_e call_security)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"security=%d", DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__),
              call_security);

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = CALL_SECURITY;
    msg.update.ccSessionUpd.data.security = call_security;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_SECURITY(%d) msg", __FUNCTION__, call_security);
	}
}




void
ui_update_conf_invoked (line_t line, callid_t call_id,
                        boolean invoked)
{
      
}












void
ui_terminate_feature (line_t line, callid_t call_id,
                        callid_t target_call_id)
{
    session_update_t msg;
    memset( &msg, 0, sizeof(session_update_t));

    TNP_DEBUG(DEB_L_C_F_PREFIX"target_call_id=%d", DEB_L_C_F_PREFIX_ARGS(UI_API, line, call_id, __FUNCTION__),
              target_call_id);

    msg.sessionID = createSessionId(line, call_id);
    msg.eventID = CALL_FEATURE_CANCEL;
    if (target_call_id != CC_NO_CALL_ID) {
        msg.update.ccSessionUpd.data.target_sess_id = createSessionId(line, target_call_id);
    } else {
        msg.update.ccSessionUpd.data.target_sess_id = 0;
    }

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR(CCAPP_F_PREFIX"failed to send CALL_FEATURE_CANCEL(%d) msg", __FUNCTION__, target_call_id);
    }
}






void
ui_set_speaker_mode (boolean mode)
{
    return;
}

void
ui_cfwdall_req (unsigned int line)
{
    lsm_clear_cfwd_all_ccm(line);
    return;
}




char *
Basic_is_phone_forwarded (line_t line)
{
    TNP_DEBUG(DEB_F_PREFIX"called for line %d", DEB_F_PREFIX_ARGS(UI_API, "Basic_is_phone_forwarded"), line);
    return ((char *) lsm_is_phone_forwarded(line));
}

void
ui_sip_config_done (void)
{
}





static int map_digit(char k)
{
    switch(k) {
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case '0':
        return 0;
    case '*':
        return 10;
    case'#':
        return 11;
    case 'A':
        return 12;
    case 'B':
        return 13;
    case 'C':
        return 14;
    case 'D':
        return 15;
    default:
        return -1;
    }
}











void
ui_keypad_button (char *digitstr, int direction)
{
    int digit;
    unsigned int i;


    for (i=0; i<strlen(digitstr); i++) {
        digit = map_digit(digitstr[i]);
        if (digit != -1) {
            
            vcmDtmfBurst(digit, direction, 100);
            cprSleep(100+10);
        }
    }

}








void
ui_log_status_msg (char *msg)
{
    ui_set_notification(CC_NO_LINE, CC_NO_CALL_ID, msg, 0, FALSE, 1);
}










void ui_log_disposition (callid_t call_id, int logdisp)
{
    session_update_t msg;
    fsmdef_dcb_t *dcb = fsmdef_get_dcb_by_call_id(call_id);
    memset( &msg, 0, sizeof(session_update_t));


    if (call_id == CC_NO_CALL_ID || dcb == NULL) {
        
        return;
    }

    TNP_DEBUG(DEB_L_C_F_PREFIX"called", DEB_L_C_F_PREFIX_ARGS(UI_API, dcb->line, call_id, __FUNCTION__));

    msg.sessionID = createSessionId(dcb->line, call_id);
    msg.eventID = CALL_LOGDISP;
    msg.update.ccSessionUpd.data.action = logdisp;

    if ( ccappTaskPostMsg(CCAPP_SESSION_UPDATE, &msg, sizeof(session_update_t), CCAPP_CCPROVIER) != CPR_SUCCESS ) {
        CCAPP_ERROR("%s: failed to send CALL_PRESERVATION_ACTIVE(%d) msg", __FUNCTION__, call_id);
    }
}











void
ui_control_feature (line_t line_id, callid_t call_id,
                       int feat_list[], int len, int enable)
{
    
}


