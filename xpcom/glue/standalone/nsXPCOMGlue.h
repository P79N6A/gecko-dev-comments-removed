





#ifndef nsXPCOMGlue_h__
#define nsXPCOMGlue_h__

#include "nscore.h"

#ifdef XPCOM_GLUE








extern "C" NS_HIDDEN_(void) XPCOMGlueEnablePreload();





extern "C" NS_HIDDEN_(nsresult) XPCOMGlueStartup(const char* aXPCOMFile);

typedef void (*NSFuncPtr)();

struct nsDynamicFunctionLoad
{
  const char* functionName;
  NSFuncPtr* function;
};









extern "C" NS_HIDDEN_(nsresult)
XPCOMGlueLoadXULFunctions(const nsDynamicFunctionLoad* aSymbols);

#endif 
#endif 
