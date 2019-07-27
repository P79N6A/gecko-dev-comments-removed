



#include "timecard.h"
#include "cpr_stdio.h"
#include "ccapi_call.h"
#include "sessionHash.h"
#include "CCProvider.h"
#include "cc_call_feature.h"
#include "cc_info.h"
#include "lsm.h"
#include "prot_configmgr.h"
#include "ccapi_call_info.h"
#include "util_string.h"






cc_callinfo_ref_t CCAPI_Call_getCallInfo(cc_call_handle_t handle) {
   unsigned int session_id = ccpro_get_sessionId_by_callid(GET_CALL_ID(handle));
   cc_callinfo_ref_t snapshot=NULL;
   session_data_t * data;

   if ( session_id != 0 ) {
      data = findhash(session_id);
      if ( data != NULL ) {
        snapshot = getDeepCopyOfSessionData(data);
        if (snapshot == NULL) {
            return NULL;
        }
        snapshot->ref_count = 1;
      }
   }
   return snapshot;
}





void CCAPI_Call_retainCallInfo(cc_callinfo_ref_t ref) {
    if (ref != NULL ) {
        ref->ref_count++;
    }
}





void CCAPI_Call_releaseCallInfo(cc_callinfo_ref_t ref) {
    if (ref != NULL ) {
	DEF_DEBUG(DEB_F_PREFIX"ref=%p: count=%d",
           DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_releaseCallInfo"), ref, ref->ref_count);
	ref->ref_count--;
	if ( ref->ref_count == 0 ) {
            cleanSessionData(ref);
            cpr_free(ref);
	}
    }
}






cc_lineid_t CCAPI_Call_getLine(cc_call_handle_t call_handle){
	static const char *fname="CCAPI_Call_getLine";

	if ( call_handle != 0 ) {
    	cc_lineid_t lineid = GET_LINE_ID(call_handle);
    	CCAPP_DEBUG(DEB_F_PREFIX"returned %u", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), lineid);
    	return lineid;
	}
	return 0;
}









cc_return_t CCAPI_Call_originateCall(cc_call_handle_t handle, cc_sdp_direction_t video_pref, cc_string_t digits){
	return CC_CallFeature_dial(handle, video_pref, digits);
}







cc_return_t CCAPI_Call_sendDigit(cc_call_handle_t handle, cc_digit_t digit){
   return CC_CallFeature_sendDigit(handle, digit);
}






cc_return_t CCAPI_Call_backspace(cc_call_handle_t handle){
   return CC_CallFeature_backSpace(handle);
}







cc_return_t CCAPI_Call_answerCall(cc_call_handle_t handle, cc_sdp_direction_t video_pref) {
  return CC_CallFeature_answerCall(handle, video_pref);
}







cc_return_t CCAPI_Call_redial(cc_call_handle_t handle, cc_sdp_direction_t video_pref){
  return CC_CallFeature_redial(handle, video_pref);
}






cc_return_t CCAPI_Call_initiateCallForwardAll(cc_call_handle_t handle){
  return CC_CallFeature_callForwardAll(handle);
}





cc_return_t CCAPI_Call_hold(cc_call_handle_t handle, cc_hold_reason_t reason){
  return CC_CallFeature_holdCall(handle, reason);
}







cc_return_t CCAPI_Call_resume(cc_call_handle_t handle, cc_sdp_direction_t video_pref) {
  return CC_CallFeature_resume(handle, video_pref);
}






cc_return_t CCAPI_Call_endConsultativeCall(cc_call_handle_t handle){
  cc_callinfo_ref_t info_handle = CCAPI_Call_getCallInfo(handle);
  cc_call_attr_t attr = CCAPI_CallInfo_getCallAttr(info_handle);
  if (attr != CC_ATTR_CONF_CONSULT &&
    attr != CC_ATTR_XFR_CONSULT &&
    attr != CC_ATTR_LOCAL_CONF_CONSULT &&
    attr != CC_ATTR_LOCAL_XFER_CONSULT) {
    DEF_DEBUG(DEB_F_PREFIX"This method only calls on a consultative call, handle %u",
      DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_endConsultativeCall"), handle);
    return CC_FAILURE;
  }

  return CC_CallFeature_endConsultativeCall(handle);
}






cc_return_t CCAPI_Call_endCall(cc_call_handle_t handle){
  return CC_CallFeature_terminateCall(handle);
}







cc_return_t CCAPI_Call_conferenceStart(cc_call_handle_t handle, cc_sdp_direction_t video_pref){
  return CC_CallFeature_conference(handle, TRUE,
                  CC_EMPTY_CALL_HANDLE, video_pref);
}








cc_return_t CCAPI_Call_conferenceComplete(cc_call_handle_t handle, cc_call_handle_t phandle,
                          cc_sdp_direction_t video_pref){
  return CC_CallFeature_conference(handle, TRUE,
                  phandle, video_pref);

}







cc_return_t CCAPI_Call_transferStart(cc_call_handle_t handle, cc_sdp_direction_t video_pref){
  return CC_CallFeature_transfer(handle, CC_EMPTY_CALL_HANDLE, video_pref);
}








cc_return_t CCAPI_Call_transferComplete(cc_call_handle_t handle, cc_call_handle_t phandle,
                              cc_sdp_direction_t video_pref){
  return CC_CallFeature_transfer(handle, phandle, video_pref);
}






cc_return_t CCAPI_Call_cancelTransferOrConferenceFeature(cc_call_handle_t handle){
  return CC_CallFeature_cancelXfrerCnf(handle);
}







cc_return_t CCAPI_Call_directTransfer(cc_call_handle_t handle, cc_call_handle_t target){
  return CC_CallFeature_directTransfer(handle, target);
}







cc_return_t CCAPI_Call_joinAcrossLine(cc_call_handle_t handle, cc_call_handle_t target){
  return CC_CallFeature_joinAcrossLine(handle, target);
}







cc_return_t CCAPI_Call_blfCallPickup(cc_call_handle_t handle,
                  cc_sdp_direction_t video_pref, cc_string_t speed){
  return CC_CallFeature_blfCallPickup(handle, video_pref, speed);
}






cc_return_t CCAPI_Call_select(cc_call_handle_t handle){
  return CC_CallFeature_select(handle);
}







cc_return_t CCAPI_Call_updateVideoMediaCap (cc_call_handle_t handle, cc_sdp_direction_t video_pref) {
  return CC_CallFeature_updateCallMediaCapability(handle, video_pref);
}









cc_return_t CCAPI_Call_sendInfo (cc_call_handle_t handle, cc_string_t infopackage, cc_string_t infotype, cc_string_t infobody)
{
	CC_Info_sendInfo(handle, infopackage, infotype, infobody);
        return CC_SUCCESS;
}








cc_return_t CCAPI_Call_setAudioMute (cc_call_handle_t handle, cc_boolean val) {
	unsigned int session_id = ccpro_get_sessionId_by_callid(GET_CALL_ID(handle));
        session_data_t * sess_data_p = (session_data_t *)findhash(session_id);
	DEF_DEBUG(DEB_F_PREFIX": val=%d, handle=%d datap=%p",
           DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_setAudioMute"), val, handle, sess_data_p);
	if ( sess_data_p != NULL ) {
		sess_data_p->audio_mute = val;
	}
        return CC_SUCCESS;
}








cc_return_t CCAPI_Call_setVideoMute (cc_call_handle_t handle, cc_boolean val){
	unsigned int session_id = ccpro_get_sessionId_by_callid(GET_CALL_ID(handle));
        session_data_t * sess_data_p = (session_data_t *)findhash(session_id);
	DEF_DEBUG(DEB_F_PREFIX": val=%d, handle=%d datap=%p",
           DEB_F_PREFIX_ARGS(SIP_CC_PROV, "CCAPI_Call_setVideoMute"), val, handle, sess_data_p);
	if ( sess_data_p != NULL ) {
		sess_data_p->video_mute = val;
		lsm_set_video_mute(GET_CALL_ID(handle), val);
	}
        return CC_SUCCESS;
}
