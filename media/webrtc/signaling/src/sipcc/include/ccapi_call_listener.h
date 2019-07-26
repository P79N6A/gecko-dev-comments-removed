






































#ifndef _CCAPI_CALL_LISTENER_H_
#define _CCAPI_CALL_LISTENER_H_

#include "ccapi_types.h"











void CCAPI_CallListener_onCallEvent(ccapi_call_event_e event, cc_call_handle_t handle, cc_callinfo_ref_t info, char* sdp);

#endif 
