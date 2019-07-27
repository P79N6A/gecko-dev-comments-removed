



#include "CSFLog.h"

#include "CC_Common.h"

#include "CC_SIPCCLine.h"
#include "CC_SIPCCCall.h"
#include "CC_SIPCCLineInfo.h"

extern "C"
{
#include "ccapi_line.h"
#include "ccapi_line_listener.h"
}

using namespace std;
using namespace CSF;

CSF_IMPLEMENT_WRAP(CC_SIPCCLine, cc_lineid_t);

cc_lineid_t CC_SIPCCLine::getID()
{
    return lineId;
}

CC_LineInfoPtr CC_SIPCCLine::getLineInfo ()
{
    cc_lineinfo_ref_t lineInfoRef = CCAPI_Line_getLineInfo(lineId);
    CC_LineInfoPtr lineInfoPtr = CC_SIPCCLineInfo::wrap(lineInfoRef).get();

    
    
    

    CCAPI_Line_releaseLineInfo(lineInfoRef);

    
    
    
    

    return lineInfoPtr;
}

CC_CallPtr CC_SIPCCLine::createCall ()
{
    cc_call_handle_t callHandle = CCAPI_Line_CreateCall(lineId);

    return CC_SIPCCCall::wrap(callHandle).get();
}


