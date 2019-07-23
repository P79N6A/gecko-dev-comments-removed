




































#include "nsIServiceManager.h"
#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "ipcdclient.h"
#include "ipcService.h"
#include "ipcConfig.h"
#include "ipcCID.h"






NS_GENERIC_FACTORY_CONSTRUCTOR(ipcService)


#if 0
NS_METHOD
ipcServiceRegisterProc(nsIComponentManager *aCompMgr,
                       nsIFile *aPath,
                       const char *registryLocation,
                       const char *componentType,
                       const nsModuleComponentInfo *info)
{
    
    
    
    nsCOMPtr<nsICategoryManager> catman(do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
    if (catman) {
        nsXPIDLCString prevEntry;
        catman->AddCategoryEntry(NS_XPCOM_STARTUP_OBSERVER_ID, "ipcService",
                                 IPC_SERVICE_CONTRACTID, PR_TRUE, PR_TRUE,
                                 getter_Copies(prevEntry));
    }
    return NS_OK;
}

NS_METHOD
ipcServiceUnregisterProc(nsIComponentManager *aCompMgr,
                         nsIFile *aPath,
                         const char *registryLocation,
                         const nsModuleComponentInfo *info)
{
    nsCOMPtr<nsICategoryManager> catman(do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
    if (catman)
        catman->DeleteCategoryEntry(NS_XPCOM_STARTUP_OBSERVER_ID, 
                                    IPC_SERVICE_CONTRACTID, PR_TRUE);
    return NS_OK;
}
#endif




#include "ipcLockService.h"
#include "ipcLockCID.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(ipcLockService, Init)

#include "tmTransactionService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(tmTransactionService)

#ifdef BUILD_DCONNECT

#include "ipcDConnectService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(ipcDConnectService, Init)


NS_METHOD
ipcDConnectServiceRegisterProc(nsIComponentManager *aCompMgr,
                               nsIFile *aPath,
                               const char *registryLocation,
                               const char *componentType,
                               const nsModuleComponentInfo *info)
{
    
    
    
    nsCOMPtr<nsICategoryManager> catman(do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
    if (catman) {
        nsXPIDLCString prevEntry;
        catman->AddCategoryEntry(NS_XPCOM_STARTUP_OBSERVER_ID, "ipcDConnectService",
                                 IPC_DCONNECTSERVICE_CONTRACTID, PR_TRUE, PR_TRUE,
                                 getter_Copies(prevEntry));
    }
    return NS_OK;
}

NS_METHOD
ipcDConnectServiceUnregisterProc(nsIComponentManager *aCompMgr,
                                 nsIFile *aPath,
                                 const char *registryLocation,
                                 const nsModuleComponentInfo *info)
{
    nsCOMPtr<nsICategoryManager> catman(do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
    if (catman)
        catman->DeleteCategoryEntry(NS_XPCOM_STARTUP_OBSERVER_ID, 
                                    IPC_DCONNECTSERVICE_CONTRACTID, PR_TRUE);
    return NS_OK;
}

#endif 






static const nsModuleComponentInfo components[] = {
  { IPC_SERVICE_CLASSNAME,
    IPC_SERVICE_CID,
    IPC_SERVICE_CONTRACTID,
    ipcServiceConstructor },
    



  
  
  
  { IPC_LOCKSERVICE_CLASSNAME,
    IPC_LOCKSERVICE_CID,
    IPC_LOCKSERVICE_CONTRACTID,
    ipcLockServiceConstructor },
  { IPC_TRANSACTIONSERVICE_CLASSNAME,
    IPC_TRANSACTIONSERVICE_CID,
    IPC_TRANSACTIONSERVICE_CONTRACTID,
    tmTransactionServiceConstructor },

#ifdef BUILD_DCONNECT
  { IPC_DCONNECTSERVICE_CLASSNAME,
    IPC_DCONNECTSERVICE_CID,
    IPC_DCONNECTSERVICE_CONTRACTID,
    ipcDConnectServiceConstructor,
    ipcDConnectServiceRegisterProc,
    ipcDConnectServiceUnregisterProc },
#endif
};



PR_STATIC_CALLBACK(nsresult)
ipcdclient_init(nsIModule *module)
{
  return IPC_Init();
}

PR_STATIC_CALLBACK(void)
ipcdclient_shutdown(nsIModule *module)
{
  IPC_Shutdown();
}





NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(ipcdclient, components,
                                   ipcdclient_init,
                                   ipcdclient_shutdown)
