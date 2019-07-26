



#ifndef _CCAPI_LINE_H_
#define _CCAPI_LINE_H_

#include "ccapi_types.h"







cc_lineinfo_ref_t CCAPI_Line_getLineInfo(cc_uint32_t line);






cc_call_handle_t CCAPI_Line_CreateCall(cc_lineid_t line);










void CCAPI_Line_retainLineInfo(cc_lineinfo_ref_t ref);






void CCAPI_Line_releaseLineInfo(cc_lineinfo_ref_t ref);

#endif 
