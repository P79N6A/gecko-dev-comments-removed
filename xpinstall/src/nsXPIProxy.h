






































#ifndef nsXPIProxy_h__
#define nsXPIProxy_h__

#include "nscore.h"
#include "nsPIXPIProxy.h"

class nsXPIProxy : public nsPIXPIProxy
{
    public:

        nsXPIProxy();
        virtual ~nsXPIProxy();

        NS_DECL_ISUPPORTS
        NS_DECL_NSPIXPIPROXY
};

#endif
