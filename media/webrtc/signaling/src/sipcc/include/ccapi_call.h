



#ifndef _CCAPI_CALL_H_
#define _CCAPI_CALL_H_

#include "ccapi_types.h"







cc_callinfo_ref_t CCAPI_Call_getCallInfo(cc_call_handle_t handle);






void CCAPI_Call_releaseCallInfo(cc_callinfo_ref_t ref);








void CCAPI_Call_retainCallInfo(cc_callinfo_ref_t ref);






cc_lineid_t CCAPI_Call_getLine(cc_call_handle_t call_handle);








cc_return_t CCAPI_Call_originateCall(cc_call_handle_t handle, cc_sdp_direction_t video_pref, cc_string_t digits);


cc_return_t CCAPI_CreateOffer(cc_call_handle_t handle, const cc_media_constraints_t *constraints);

cc_return_t CCAPI_CreateAnswer(cc_call_handle_t handle, const cc_media_constraints_t *constraints);

cc_return_t CCAPI_SetLocalDescription(cc_call_handle_t handle, cc_jsep_action_t action, cc_string_t sdp);

cc_return_t CCAPI_SetRemoteDescription(cc_call_handle_t handle, cc_jsep_action_t action, cc_string_t sdp);

cc_return_t CCAPI_SetPeerConnection(cc_call_handle_t handle, cc_peerconnection_t pc);

cc_return_t CCAPI_AddStream(cc_call_handle_t handle, cc_media_stream_id_t stream_id, cc_media_track_id_t track_id, cc_media_type_t media_type);

cc_return_t CCAPI_RemoveStream(cc_call_handle_t handle, cc_media_stream_id_t stream_id, cc_media_track_id_t track_id, cc_media_type_t media_type);

cc_return_t CCAPI_AddICECandidate(cc_call_handle_t handle, cc_string_t candidate, cc_string_t mid, cc_level_t level);







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
