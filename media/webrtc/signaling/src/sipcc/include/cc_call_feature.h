





















































































#ifndef _CC_CALL_FEATURE_H_
#define _CC_CALL_FEATURE_H_

#if defined(__cplusplus) && __cplusplus >= 201103L
typedef struct Timecard Timecard;
#else
#include "timecard.h"
#endif

#include "cc_constants.h"













cc_call_handle_t CC_createCall(cc_lineid_t line);







 
cc_return_t CC_CallFeature_originateCall(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref);






cc_return_t CC_CallFeature_terminateCall(cc_call_handle_t call_handle);







cc_return_t CC_CallFeature_answerCall(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref);







cc_return_t CC_CallFeature_sendDigit(cc_call_handle_t call_handle, cc_digit_t cc_digit);






cc_return_t CC_CallFeature_backSpace(cc_call_handle_t call_handle);








cc_return_t CC_CallFeature_dial(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref, const cc_string_t numbers);








cc_return_t CC_CallFeature_speedDial(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref, const cc_string_t speed_dial_number);








cc_return_t CC_CallFeature_blfCallPickup(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref, const cc_string_t speed_dial_number);








cc_return_t CC_CallFeature_redial(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref);





cc_return_t CC_CallFeature_updateCallMediaCapability(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref);






cc_return_t CC_CallFeature_callForwardAll(cc_call_handle_t call_handle);







cc_return_t CC_CallFeature_resume(cc_call_handle_t call_handle, cc_sdp_direction_t video_pref);






cc_return_t CC_CallFeature_endConsultativeCall(cc_call_handle_t call_handle);





























cc_return_t CC_CallFeature_conference(cc_call_handle_t call_handle, cc_boolean is_local,
		cc_call_handle_t parent_call_handle, cc_sdp_direction_t video_pref);











cc_return_t CC_CallFeature_transfer(cc_call_handle_t call_handle, cc_call_handle_t parent_call_handle, cc_sdp_direction_t video_pref);








cc_return_t CC_CallFeature_directTransfer(cc_call_handle_t call_handle, cc_call_handle_t target_call_handle);






cc_return_t CC_CallFeature_joinAcrossLine(cc_call_handle_t call_handle, cc_call_handle_t target_call_handle);










cc_return_t CC_CallFeature_holdCall(cc_call_handle_t call_handle, cc_hold_reason_t reason);


















cc_return_t CC_CallFeature_select(cc_call_handle_t call_handle);







cc_return_t CC_CallFeature_cancelXfrerCnf(cc_call_handle_t call_handle);





void CC_CallFeature_mute(cc_boolean mute);





void CC_CallFeature_speaker(cc_boolean mute);





cc_call_handle_t CC_CallFeature_getConnectedCall();

#endif 
