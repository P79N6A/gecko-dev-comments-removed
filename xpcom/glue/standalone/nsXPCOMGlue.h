





































#ifndef nsXPCOMGlue_h__
#define nsXPCOMGlue_h__

#include "nscore.h"

class nsILocalFile;






struct GREVersionRange {
    const char *lower;
    PRBool      lowerInclusive;
    const char *upper;
    PRBool      upperInclusive;
};

struct GREProperty {
    const char *property;
    const char *value;
};























extern "C" NS_COM_GLUE nsresult
GRE_GetGREPathWithProperties(const GREVersionRange *versions,
                             PRUint32 versionsLength,
                             const GREProperty *properties,
                             PRUint32 propertiesLength,
                             char *buffer, PRUint32 buflen);

#ifdef XPCOM_GLUE









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
