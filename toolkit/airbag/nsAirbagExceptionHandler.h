




































#ifndef nsAirbagExceptionHandler_h__
#define nsAirbagExceptionHandler_h__

#include "nscore.h"
#include "nsXPCOM.h"
#include "nsStringGlue.h"

nsresult SetAirbagExceptionHandler();
nsresult SetAirbagMinidumpPath(const nsAString* aPath);
nsresult UnsetAirbagExceptionHandler();

#endif 
