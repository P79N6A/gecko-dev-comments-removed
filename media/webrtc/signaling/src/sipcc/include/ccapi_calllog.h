






































#include "cpr_stdio.h"
#include "ccapi_call.h"
#include "sessionHash.h"
#include "CCProvider.h"
#include "phone_debug.h"







cc_log_disposition_t CCAPI_CallLog_getCallDisposition(cc_calllog_ref_t handle);






cc_uint32_t CCAPI_CallLog_getStartTime(cc_calllog_ref_t handle);






cc_uint32_t CCAPI_CallLog_getCallDuration(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getFirstLegRemotePartyName(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getLastLegRemotePartyName(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getFirstLegRemotePartyNumber(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getLastLegRemotePartyNumber(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getLocalPartyName(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getLocalPartyNumber(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getFirstLegAltPartyNumber(cc_calllog_ref_t handle);






cc_string_t CCAPI_CallLog_getLastLegAltPartyNumber(cc_calllog_ref_t handle);
