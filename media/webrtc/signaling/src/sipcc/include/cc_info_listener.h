






































#ifndef _CC_INFO_LISTENER_H_
#define _CC_INFO_LISTENER_H_

#include "cc_constants.h"










void CC_InfoListener_receivedInfo(cc_call_handle_t call_handle, int pack_id,
		cc_string_t info_package,
		cc_string_t info_type,
		cc_string_t info_body);

#endif 
