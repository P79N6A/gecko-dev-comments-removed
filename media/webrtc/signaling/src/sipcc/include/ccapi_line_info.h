






































#ifndef _CCAPI_LINE_INFO_H_
#define _CCAPI_LINE_INFO_H_

#include "ccapi_types.h"






cc_uint32_t CCAPI_lineInfo_getID(cc_lineinfo_ref_t line);







cc_string_t CCAPI_lineInfo_getName(cc_lineinfo_ref_t line);







cc_string_t CCAPI_lineInfo_getLabel(cc_lineinfo_ref_t line);







cc_string_t CCAPI_lineInfo_getNumber(cc_lineinfo_ref_t line);







cc_string_t CCAPI_lineInfo_getExternalNumber(cc_lineinfo_ref_t line);








cc_uint32_t CCAPI_lineInfo_getButton(cc_lineinfo_ref_t line);






cc_line_feature_t CCAPI_lineInfo_getLineType(cc_lineinfo_ref_t line);






cc_boolean CCAPI_lineInfo_getRegState(cc_lineinfo_ref_t line);






cc_boolean CCAPI_lineInfo_isCFWDActive(cc_lineinfo_ref_t line);







cc_string_t CCAPI_lineInfo_getCFWDName(cc_lineinfo_ref_t line);








void CCAPI_LineInfo_getCalls(cc_lineid_t line, cc_call_handle_t handles[], int *count);









void CCAPI_LineInfo_getCallsByState(cc_lineid_t line, cc_call_state_t state, 
                cc_call_handle_t handles[], int *count);






cc_uint32_t CCAPI_lineInfo_getMWIStatus(cc_lineinfo_ref_t line);






cc_uint32_t CCAPI_lineInfo_getMWIType(cc_lineinfo_ref_t line);






cc_uint32_t CCAPI_lineInfo_getMWINewMsgCount(cc_lineinfo_ref_t line);






cc_uint32_t CCAPI_lineInfo_getMWIOldMsgCount(cc_lineinfo_ref_t line);






cc_uint32_t CCAPI_lineInfo_getMWIPrioNewMsgCount(cc_lineinfo_ref_t line);






cc_uint32_t CCAPI_lineInfo_getMWIPrioOldMsgCount(cc_lineinfo_ref_t line);







cc_boolean  CCAPI_LineInfo_hasCapability(cc_lineinfo_ref_t line, cc_int32_t feat_id);







cc_return_t  CCAPI_LineInfo_getCapabilitySet(cc_lineinfo_ref_t line, cc_int32_t feat_set[]);

#endif 
