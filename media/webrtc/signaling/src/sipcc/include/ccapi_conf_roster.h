






































#ifndef _CCAPI_CONF_ROSTER_H_
#define _CCAPI_CONF_ROSTER_H_

#include "cpr_stdio.h"
#include "ccapi_call.h"
#include "CCProvider.h"
#include "phone_debug.h"


typedef string_t cc_participant_ref_t;















void CCAPI_CallInfo_getConfParticipants (cc_callinfo_ref_t handle, cc_participant_ref_t participantHandles[], int* count);







cc_uint16_t CCAPI_CallInfo_getConfParticipantMax (cc_callinfo_ref_t handle);








cc_string_t CCAPI_CallInfo_getConfParticipantName (cc_callinfo_ref_t handle, cc_participant_ref_t participantHandle);







cc_string_t CCAPI_CallInfo_getConfParticipantNumber (cc_callinfo_ref_t handle, cc_participant_ref_t participantHandle);







cc_conf_participant_status_t CCAPI_CallInfo_getConfParticipantStatus (cc_callinfo_ref_t handle, cc_participant_ref_t participantHandleHandle);







cc_call_security_t CCAPI_CallInfo_getConfParticipantSecurity (cc_callinfo_ref_t handle, cc_participant_ref_t participantHandleHandle);







cc_boolean CCAPI_CallInfo_selfHasRemoveConfParticipantCapability (cc_callinfo_ref_t handle);







cc_boolean CCAPI_CallInfo_isConfSelfParticipant (cc_callinfo_ref_t handle, cc_participant_ref_t participantHandle);






cc_participant_ref_t CCAPI_CallInfo_getConfSelfParticipant (cc_callinfo_ref_t handle);


#endif 
