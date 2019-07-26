



#include "cc_info.h"
#include "sessionTypes.h"
#include "phone_debug.h"
#include "CCProvider.h"
#include "sessionConstants.h"
#include "ccapp_task.h"









void CC_Info_sendInfo(cc_call_handle_t call_handle,
        string_t info_package,
        string_t info_type,
        string_t info_body) {
    static const char *fname = "CC_Info_sendInfo";
    session_send_info_t send_info;

    CCAPP_DEBUG(DEB_F_PREFIX"entry... call_handle=0x%x\n",
                DEB_F_PREFIX_ARGS(SIP_CC_SES, fname), call_handle);

    send_info.sessionID= (SESSIONTYPE_CALLCONTROL << CC_SID_TYPE_SHIFT) + call_handle;;
    send_info.generic_raw.info_package = strlib_malloc(info_package, strlen(info_package));
    send_info.generic_raw.content_type = strlib_malloc(info_type, strlen(info_type));
    send_info.generic_raw.message_body = strlib_malloc(info_body, strlen(info_body));

    
    
    if (ccappTaskPostMsg(CCAPP_SEND_INFO, &send_info,
                         sizeof(session_send_info_t), CCAPP_CCPROVIER) != CPR_SUCCESS) {
        CCAPP_ERROR(DEB_F_PREFIX"ccappTaskPostMsg failed\n",
                    DEB_F_PREFIX_ARGS(SIP_CC_SES, fname));
    }

}
