



#ifndef _CCAPI_CALL_H_
#define _CCAPI_CALL_H_

#if defined(__cplusplus) && __cplusplus >= 201103L
typedef struct Timecard Timecard;
#else
#include "timecard.h"
#endif

#include "ccapi_types.h"







cc_callinfo_ref_t CCAPI_Call_getCallInfo(cc_call_handle_t handle);






void CCAPI_Call_releaseCallInfo(cc_callinfo_ref_t ref);








void CCAPI_Call_retainCallInfo(cc_callinfo_ref_t ref);






cc_lineid_t CCAPI_Call_getLine(cc_call_handle_t call_handle);








cc_return_t CCAPI_Call_originateCall(cc_call_handle_t handle, cc_sdp_direction_t video_pref, cc_string_t digits);








cc_return_t CCAPI_Call_sendDigit(cc_call_handle_t handle, cc_digit_t digit);






cc_return_t CCAPI_Call_backspace(cc_call_handle_t handle);







cc_return_t CCAPI_Call_answerCall(cc_call_handle_t handle, cc_sdp_direction_t video_pref);







cc_return_t CCAPI_Call_redial(cc_call_handle_t handle, cc_sdp_direction_t video_pref);






cc_return_t CCAPI_Call_initiateCallForwardAll(cc_call_handle_t handle);






cc_return_t CCAPI_Call_hold(cc_call_handle_t handle, cc_hold_reason_t reason);







cc_return_t CCAPI_Call_resume(cc_call_handle_t handle, cc_sdp_direction_t video_pref) ;






cc_return_t CCAPI_Call_endConsultativeCall(cc_call_handle_t handle);






cc_return_t CCAPI_Call_endCall(cc_call_handle_t handle);







cc_return_t CCAPI_Call_conferenceStart(cc_call_handle_t handle, cc_sdp_direction_t video_pref);








cc_return_t CCAPI_Call_conferenceComplete(cc_call_handle_t handle, cc_call_handle_t phandle,
                          cc_sdp_direction_t video_pref);







cc_return_t CCAPI_Call_transferStart(cc_call_handle_t handle, cc_sdp_direction_t video_pref);








cc_return_t CCAPI_Call_transferComplete(cc_call_handle_t handle, cc_call_handle_t phandle,
                          cc_sdp_direction_t video_pref);






cc_return_t CCAPI_Call_cancelTransferOrConferenceFeature(cc_call_handle_t handle);







cc_return_t CCAPI_Call_directTransfer(cc_call_handle_t handle, cc_call_handle_t target);







cc_return_t CCAPI_Call_joinAcrossLine(cc_call_handle_t handle, cc_call_handle_t target);








cc_return_t CCAPI_Call_blfCallPickup(cc_call_handle_t handle, cc_sdp_direction_t video_pref, cc_string_t speed);






cc_return_t CCAPI_Call_select(cc_call_handle_t handle);







cc_return_t CCAPI_Call_updateVideoMediaCap(cc_call_handle_t handle, cc_sdp_direction_t video_pref);









cc_return_t CCAPI_Call_sendInfo(cc_call_handle_t handle, cc_string_t infopackage, cc_string_t infotype, cc_string_t infobody);








cc_return_t CCAPI_Call_setAudioMute(cc_call_handle_t handle, cc_boolean val);








cc_return_t CCAPI_Call_setVideoMute(cc_call_handle_t handle, cc_boolean val);


#endif 
