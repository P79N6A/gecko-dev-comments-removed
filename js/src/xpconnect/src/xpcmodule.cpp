







































#define XPCONNECT_MODULE
#include "xpcprivate.h"

nsresult
xpcModuleCtor()
{
    nsXPConnect::InitStatics();
    nsXPCException::InitStatics();
    XPCWrappedNativeScope::InitStatics();
    XPCPerThreadData::InitStatics();

#ifdef XPC_IDISPATCH_SUPPORT
    XPCIDispatchExtension::InitStatics();
#endif

    return NS_OK;
}

void
xpcModuleDtor()
{
    
    nsXPConnect::ReleaseXPConnectSingleton();
    xpc_DestroyJSxIDClassObjects();
#ifdef XPC_IDISPATCH_SUPPORT
    nsDispatchSupport::FreeSingleton();
    XPCIDispatchClassInfo::FreeSingleton();
#endif
}
