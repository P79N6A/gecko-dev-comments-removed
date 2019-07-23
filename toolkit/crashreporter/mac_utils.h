




































#ifndef toolkit_breakpad_mac_utils_h__
#define toolkit_breakpad_mac_utils_h__

#include "nsStringGlue.h"





bool PassToOSCrashReporter();


void GetObjCExceptionInfo(void* inException, nsACString& outString);

#endif 
