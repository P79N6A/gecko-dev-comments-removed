



#ifndef _CC_CALL_LISTENER_H_
#define _CC_CALL_LISTENER_H_

#include "cc_constants.h"


















void CC_CallListener_callCreated(cc_call_handle_t call_handle,
		cc_call_state_t call_state,
		cc_cause_t cc_cause,
		cc_call_attr_t cc_attr);













void CC_CallListener_callStateChanged(cc_call_handle_t call_handle,
		cc_call_state_t call_state,
		cc_call_attr_t call_attr,
		cc_cause_t call_cause,
		cc_call_instance_t call_instance);







void CC_CallListener_callAttributeChanged(cc_call_handle_t call_handle,
		cc_call_attr_t call_attr);







void CC_CallListener_callSecurityChanged(cc_call_handle_t call_handle,
		cc_call_security_t call_security);








void CC_CallListener_xferCnfCancelled(cc_call_handle_t call_handle,
		cc_call_handle_t other_call_handle);



















void CC_CallListener_callInfoChanged(cc_call_handle_t call_handle,
		cc_string_t clg_name, cc_string_t clg_number,
		cc_string_t cld_name, cc_string_t cld_number,
		cc_string_t alt_clg_number,
		cc_string_t orig_name, cc_string_t orig_number,
		cc_string_t redir_name, cc_string_t redir_number,
		cc_call_security_t call_security,
		cc_call_policy_t call_policy,
		cc_call_instance_t call_instance,
		cc_call_type_t call_type);









void CC_CallListener_callGCIDChanged(cc_call_handle_t call_handle, char* gcid);









void CC_CallListener_callPlacedInfoChanged(cc_call_handle_t call_handle,
		cc_string_t cld_name,
		cc_string_t cld_number);










void CC_CallListener_callStatusChanged(cc_call_handle_t call_handle, cc_string_t status, int timeout, char priority);







void CC_CallListener_backSpaceEnabled(cc_call_handle_t call_handle, cc_boolean state);







void CC_CallListener_callSelected(cc_call_handle_t call_handle,
		cc_call_selection_t call_selection);







void CC_CallListener_logDispositionUpdated(cc_call_handle_t call_handle,
		cc_log_disposition_t log_disp);






void CC_CallListener_lastDigitDeleted(cc_call_handle_t call_handle);








void CC_CallListener_callPreserved(cc_call_handle_t call_handle);







void CC_CallListener_videoAvailable(cc_call_handle_t call_handle, cc_int32_t state);







void CC_CallListener_remoteVideoOffered(cc_call_handle_t call_handle, cc_sdp_direction_t direction);




#endif 
