



#include "cpr_stdio.h"
#include "ccapi_call.h"
#include "sessionHash.h"
#include "CCProvider.h"
#include "text_strings.h"
#include "phone_debug.h"
#include "peer_connection_types.h"






cc_lineid_t CCAPI_CallInfo_getLine(cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getLine";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %u", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), GET_LINE_ID(CREATE_CALL_HANDLE_FROM_SESSION_ID(data->sess_id)));
     return GET_LINE_ID(CREATE_CALL_HANDLE_FROM_SESSION_ID(data->sess_id));
  }

  return 0;
}






cc_call_state_t CCAPI_CallInfo_getCallState(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCallState";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %02X", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->state);
     return data->state;
  }

  return ONHOOK;
}






fsmdef_states_t CCAPI_CallInfo_getFsmState(cc_callinfo_ref_t handle){
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering",
              DEB_F_PREFIX_ARGS(SIP_CC_PROV, __FUNCTION__));

  if ( data ){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %02X",
                 DEB_F_PREFIX_ARGS(SIP_CC_PROV, __FUNCTION__), data->state);
     return data->fsm_state;
  }

  return FSMDEF_S_IDLE;
}






cc_call_attr_t CCAPI_CallInfo_getCallAttr(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCallAttr";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %02X", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->attr);
     return data->attr;
  }

  return 0;
}






cc_call_type_t CCAPI_CallInfo_getCallType(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCallType";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %02X", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->type);
     return data->type;
  }

  return 0;
}






cc_string_t CCAPI_CallInfo_getCalledPartyName(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCalledPartyName";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->cld_name);
     return data->cld_name;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getCalledPartyNumber(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCalledPartyNumber";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->cld_number);
     return data->cld_number;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getCallingPartyName(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCallingPartyName";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->clg_name);
     return data->clg_name;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getCallingPartyNumber(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCallingPartyNumber";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->clg_number);
     return data->clg_number;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getAlternateNumber(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getAlternateNumber";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->alt_number);
     return data->alt_number;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getOriginalCalledPartyName(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getOriginalCalledPartyName";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->orig_called_name);
     return data->orig_called_name;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getOriginalCalledPartyNumber(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getOriginalCalledPartyNumber";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->orig_called_number);
     return data->orig_called_number;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getLastRedirectingPartyName(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getLastRedirectingPartyName";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->last_redir_name);
     return data->last_redir_name;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getLastRedirectingPartyNumber(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getLastRedirectingPartyNumber";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->last_redir_number);
     return data->last_redir_number;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getPlacedCallPartyName(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getPlacedCallPartyName";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->plcd_name);
     return data->plcd_name;
  }

  return strlib_empty();
}






cc_string_t CCAPI_CallInfo_getPlacedCallPartyNumber(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getPlacedCallPartyNumber";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->plcd_number);
     return data->plcd_number;
  }

  return strlib_empty();
}







cc_int32_t CCAPI_CallInfo_getCallInstance(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getCallInstance";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->inst);
     return data->inst;
  }

  return 0;
}






cc_string_t CCAPI_CallInfo_getStatus(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getStatus";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if (data && data->status){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->status);
     return data->status;
  }

  return strlib_empty();

}






cc_call_security_t CCAPI_CallInfo_getSecurity(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getSecurity";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %02X", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->security);
     return data->security;
  }

  return CC_SECURITY_NONE;
}






cc_boolean CCAPI_CallInfo_getSelectionStatus(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getSelectionStatus";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %02X", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->isSelected);
       return data->isSelected;
   }

   return FALSE;
}






cc_call_policy_t CCAPI_CallInfo_getPolicy(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getPolicy";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %02X", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->policy);
     return data->policy;
  }

  return CC_POLICY_NONE;
}






cc_string_t CCAPI_CallInfo_getGCID(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getGCID";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->gci);
     return data->gci;
  }

  return strlib_empty();
}






cc_boolean CCAPI_CallInfo_getRingerState(cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getRingerState";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->ringer_start);
     return data->ringer_start;
  }

  return FALSE;
}






int CCAPI_CallInfo_getRingerMode(cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getRingerMode";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->ringer_mode);
     return (int)(data->ringer_mode);
  }

  return -1;
}






cc_boolean CCAPI_CallInfo_getIsRingOnce(cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getIsRingOnce";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->ringer_once);
     return (int)(data->ringer_once);
  }

  return TRUE;
}






cc_int32_t  CCAPI_CallInfo_getOnhookReason(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getOnhookReason";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->cause);
     return data->cause;
  }

  return CC_CAUSE_NORMAL;
}






cc_boolean  CCAPI_CallInfo_getIsConference(cc_callinfo_ref_t handle){
  session_data_t *data = (session_data_t *)handle;
  char isConf[32];

  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, __FUNCTION__));

  memset(isConf, 0, sizeof(isConf));

  if(platGetPhraseText(CONFERENCE_LOCALE_CODE, isConf, sizeof(isConf)) == CC_FAILURE){
	  return FALSE;
  }

  if( data != NULL){
      if( (strcasecmp(data->cld_name, isConf) == 0 && strcasecmp(data->cld_number, "") == 0) ||
          (strcasecmp(data->clg_name, isConf) == 0 && strcasecmp(data->clg_number, "") == 0) )
      {
	      return TRUE;
      }
  }

  return FALSE;
}






cc_return_t  CCAPI_CallInfo_getStreamStatistics(cc_callinfo_ref_t handle, cc_int32_t stats[], cc_int32_t *count)
{
  static const char *fname="CCAPI_CallInfo_getStreamStatistics";
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
  CCAPP_DEBUG(DEB_F_PREFIX"returned CC_SUCCESS (default)", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
 
 return CC_SUCCESS;
}








cc_boolean  CCAPI_CallInfo_hasCapability(cc_callinfo_ref_t handle, cc_int32_t feat_id){
  static const char *fname="CCAPI_CallInfo_hasCapability";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"feature id:  %d , value returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),feat_id,  data->allowed_features[feat_id]);
     return data->allowed_features[feat_id];
  }

  return FALSE;
}






cc_boolean  CCAPI_CallInfo_getCapabilitySet(cc_callinfo_ref_t handle, cc_int32_t feat_set[]){
  static const char *fname="CCAPI_CallInfo_getCapabilitySet";
  session_data_t *data = (session_data_t *)handle;
  int feat_id;

  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     for (feat_id = 0; feat_id < CCAPI_CALL_CAP_MAX; feat_id++) {
         feat_set[feat_id] = data->allowed_features[feat_id];
         CCAPP_DEBUG(DEB_F_PREFIX"feature id:  %d , value %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),feat_id,  feat_set[feat_id]);
     }

     CCAPP_DEBUG(DEB_F_PREFIX"returned CC_SUCCESS", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
     return CC_SUCCESS;
  }

  return CC_FAILURE;
}






cc_boolean  CCAPI_CallInfo_isCallSelected(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_isCallSelected";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->isSelected);
     return data->isSelected;
  }

  return FALSE;
}






cc_sdp_direction_t  CCAPI_CallInfo_getVideoDirection(cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_getVideoDirection";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->vid_dir);
     return (data->vid_dir);
  }

  return CC_SDP_DIRECTION_INACTIVE;
}






cc_string_t  CCAPI_CallInfo_getINFOPack (cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getINFOPackage";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->info_package);
     return data->info_package;
  }

  return strlib_empty();
}






cc_string_t  CCAPI_CallInfo_getINFOType (cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getINFOType";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->info_type);
     return data->info_type;
  }

  return strlib_empty();
}






cc_string_t  CCAPI_CallInfo_getINFOBody (cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getINFOBody";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %s", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), data->info_body);
     return data->info_body;
  }

  return strlib_empty();
}








cc_calllog_ref_t  CCAPI_CallInfo_getCallLogRef(cc_callinfo_ref_t handle)
{
  static const char *fname="CCAPI_CallInfo_getCallLogRef";
  session_data_t *data = (session_data_t *)handle;
  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

  if ( data != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"returned %p", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), &data->call_log);
     return &data->call_log;
  }

  return NULL;
}






cc_boolean CCAPI_CallInfo_isAudioMuted (cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_isAudioMuted";
  session_data_t *data = (session_data_t *)handle;
  session_data_t * sess_data_p;

  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
  if ( data != NULL){
      sess_data_p = (session_data_t *)findhash(data->sess_id);
      if ( sess_data_p != NULL ) {
          CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), sess_data_p->audio_mute);
          return sess_data_p->audio_mute;
      }
  }

  return FALSE;
}





cc_boolean CCAPI_CallInfo_isVideoMuted (cc_callinfo_ref_t handle){
  static const char *fname="CCAPI_CallInfo_isVideoMuted";
  session_data_t *data = (session_data_t *)handle;
  session_data_t * sess_data_p;

  CCAPP_DEBUG(DEB_F_PREFIX"Entering", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
  if ( data != NULL){
      sess_data_p = (session_data_t *)findhash(data->sess_id);
      if ( sess_data_p != NULL ) {
          CCAPP_DEBUG(DEB_F_PREFIX"returned %d", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), sess_data_p->video_mute);
          return sess_data_p->video_mute;
      }
  }

  return FALSE;
}


