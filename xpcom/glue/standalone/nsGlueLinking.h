




































#ifndef nsGlueLinking_h__
#define nsGlueLinking_h__

#include "nsXPCOMPrivate.h"

#define XPCOM_DEPENDENT_LIBS_LIST "dependentlibs.list"

NS_HIDDEN_(GetFrozenFunctionsFunc)
XPCOMGlueLoad(const char *xpcomFile);

NS_HIDDEN_(void)
XPCOMGlueUnload();

typedef void (*DependentLibsCallback)(const char *aDependentLib);

NS_HIDDEN_(void)
XPCOMGlueLoadDependentLibs(const char *xpcomDir, DependentLibsCallback cb);

#endif 
