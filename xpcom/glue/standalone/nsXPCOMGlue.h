





































#ifndef nsXPCOMGlue_h__
#define nsXPCOMGlue_h__

#include "nscore.h"

#ifdef XPCOM_GLUE








extern "C" NS_HIDDEN_(void)
XPCOMGlueEnablePreload();





extern "C" NS_HIDDEN_(nsresult)
XPCOMGlueStartup(const char* xpcomFile);

typedef void (*NSFuncPtr)();

struct nsDynamicFunctionLoad
{
    const char *functionName;
    NSFuncPtr  *function;
};









extern "C" NS_HIDDEN_(nsresult)
XPCOMGlueLoadXULFunctions(const nsDynamicFunctionLoad *symbols);




extern "C" NS_HIDDEN_(nsresult)
XPCOMGlueShutdown();

#endif 
#endif 
