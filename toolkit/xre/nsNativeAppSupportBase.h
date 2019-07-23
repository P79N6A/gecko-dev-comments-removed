





































#ifndef nsNativeAppSupportBase_h__
#define nsNativeAppSupportBase_h__

#include "nsAppRunner.h"
#include "nsINativeAppSupport.h"






class nsNativeAppSupportBase : public nsINativeAppSupport {
public:
    nsNativeAppSupportBase();
    virtual ~nsNativeAppSupportBase();

    NS_DECL_ISUPPORTS
    NS_DECL_NSINATIVEAPPSUPPORT
};

#endif
