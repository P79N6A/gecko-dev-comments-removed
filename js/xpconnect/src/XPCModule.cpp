





#define XPCONNECT_MODULE
#include "xpcprivate.h"

nsresult
xpcModuleCtor()
{
    nsXPConnect::InitStatics();

    return NS_OK;
}

void
xpcModuleDtor()
{
    
    nsXPConnect::ReleaseXPConnectSingleton();
    xpc_DestroyJSxIDClassObjects();
}
