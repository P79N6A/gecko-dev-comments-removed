




































#ifndef nsGlueLinking_h__
#define nsGlueLinking_h__

#include "nsXPCOMPrivate.h"

#define XPCOM_DEPENDENT_LIBS_LIST "dependentlibs.list"

NS_HIDDEN_(nsresult)
XPCOMGlueLoad(const char *xpcomFile, GetFrozenFunctionsFunc *func NS_OUTPARAM);

NS_HIDDEN_(void)
XPCOMGlueUnload();

typedef void (*DependentLibsCallback)(const char *aDependentLib);

NS_HIDDEN_(void)
XPCOMGlueLoadDependentLibs(const char *xpcomDir, DependentLibsCallback cb);

#endif 
