



#ifndef _CCAPI_CALL_INFO_H_
#define _CCAPI_CALL_INFO_H_

#if defined(__cplusplus) && __cplusplus >= 201103L
typedef struct Timecard Timecard;
#else
#include "timecard.h"
#endif

#include "ccapi_types.h"
#include "peer_connection_types.h"
#include "fsmdef_states.h"






cc_lineid_t CCAPI_CallInfo_getLine(cc_callinfo_ref_t handle);






cc_call_state_t CCAPI_CallInfo_getCallState(cc_callinfo_ref_t handle);






fsmdef_states_t CCAPI_CallInfo_getFsmState(cc_callinfo_ref_t handle);






cc_call_attr_t CCAPI_CallInfo_getCallAttr(cc_callinfo_ref_t handle);






cc_call_type_t CCAPI_CallInfo_getCallType(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getCalledPartyName(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getCalledPartyNumber(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getCallingPartyName(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getCallingPartyNumber(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getAlternateNumber(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getOriginalCalledPartyName(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getOriginalCalledPartyNumber(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getLastRedirectingPartyName(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getLastRedirectingPartyNumber(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getPlacedCallPartyName(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getPlacedCallPartyNumber(cc_callinfo_ref_t handle);








cc_int32_t CCAPI_CallInfo_getCallInstance(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getStatus(cc_callinfo_ref_t handle);






cc_call_security_t CCAPI_CallInfo_getSecurity(cc_callinfo_ref_t handle);






cc_int32_t CCAPI_CallInfo_getSelectionStatus(cc_callinfo_ref_t handle);







cc_string_t CCAPI_CallInfo_getGCID(cc_callinfo_ref_t handle);






cc_boolean CCAPI_CallInfo_getIsRingOnce(cc_callinfo_ref_t handle);






cc_boolean CCAPI_CallInfo_getRingerState(cc_callinfo_ref_t handle);






int CCAPI_CallInfo_getRingerMode(cc_callinfo_ref_t handle);







cc_int32_t  CCAPI_CallInfo_getOnhookReason(cc_callinfo_ref_t handle);






cc_boolean  CCAPI_CallInfo_getIsConference(cc_callinfo_ref_t handle);








cc_return_t  CCAPI_CallInfo_getStreamStatistics(cc_callinfo_ref_t handle, cc_int32_t stats[], cc_int32_t *count);








cc_boolean  CCAPI_CallInfo_hasCapability(cc_callinfo_ref_t handle, cc_int32_t feat_id);







cc_return_t  CCAPI_CallInfo_getCapabilitySet(cc_callinfo_ref_t handle, cc_int32_t feat_set[]);






cc_boolean  CCAPI_CallInfo_isCallSelected(cc_callinfo_ref_t handle);






cc_string_t  CCAPI_CallInfo_getINFOPack(cc_callinfo_ref_t handle);






cc_string_t  CCAPI_CallInfo_getINFOType(cc_callinfo_ref_t handle);






cc_string_t  CCAPI_CallInfo_getINFOBody(cc_callinfo_ref_t handle);








cc_calllog_ref_t  CCAPI_CallInfo_getCallLogRef(cc_callinfo_ref_t handle);






cc_sdp_direction_t  CCAPI_CallInfo_getVideoDirection(cc_callinfo_ref_t handle);





cc_boolean CCAPI_CallInfo_isAudioMuted(cc_callinfo_ref_t handle);





cc_boolean CCAPI_CallInfo_isVideoMuted(cc_callinfo_ref_t handle);

#endif 
