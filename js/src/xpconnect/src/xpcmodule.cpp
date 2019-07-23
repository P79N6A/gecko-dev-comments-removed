









































#ifdef XPCONNECT_STANDALONE
#define NO_SUBSCRIPT_LOADER
#endif

#include "xpcmodule.h"

nsresult
xpcModuleCtor(nsIModule* self)
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
xpcModuleDtor(nsIModule*)
{
    
    nsXPConnect::ReleaseXPConnectSingleton();
    xpc_DestroyJSxIDClassObjects();
#ifdef XPC_IDISPATCH_SUPPORT
    nsDispatchSupport::FreeSingleton();
    XPCIDispatchClassInfo::FreeSingleton();
#endif
}

#ifdef XPCONNECT_STANDALONE



XPCONNECT_FACTORIES

static const nsModuleComponentInfo components[] = {
  XPCONNECT_COMPONENTS
};

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(xpconnect, components, xpcModuleCtor, xpcModuleDtor)
#endif
