




































#ifndef nsModule_h__
#define nsModule_h__

#include "nsIModule.h"
#include "nsIFile.h"
#include "nsIComponentManager.h"
#include "nsXPCOM.h"


#define NS_GET_MODULE_SYMBOL "NSGetModule"

extern "C" NS_EXPORT nsresult PR_CALLBACK 
NSGetModule(nsIComponentManager *aCompMgr,
            nsIFile* location,
            nsIModule** return_cobj);

#endif 
