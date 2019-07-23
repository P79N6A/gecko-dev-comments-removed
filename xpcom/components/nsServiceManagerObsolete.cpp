




































#include "nsIServiceManager.h"
#include "nsIServiceManagerObsolete.h"
#include "nsComponentManager.h"
#include "nsIModule.h"

extern PRBool gXPCOMShuttingDown;



nsresult
nsServiceManager::GetGlobalServiceManager(nsIServiceManager* *result)
{
    if (gXPCOMShuttingDown)
        return NS_ERROR_UNEXPECTED;
    
    if (nsComponentManagerImpl::gComponentManager == nsnull)
        return NS_ERROR_UNEXPECTED;
        
    
    
    *result =  (nsIServiceManager*) NS_STATIC_CAST(nsIServiceManagerObsolete*, 
                                                   nsComponentManagerImpl::gComponentManager);
    return NS_OK;
}

nsresult
nsServiceManager::ShutdownGlobalServiceManager(nsIServiceManager* *result)
{
    NS_NOTREACHED("nsServiceManager::ShutdownGlobalServiceManager is deprecated");
    return NS_OK;
}

nsresult
nsServiceManager::GetService(const nsCID& aClass, const nsIID& aIID,
                             nsISupports* *result,
                             nsIShutdownListener* shutdownListener)
{
    NS_NOTREACHED("nsServiceManager::GetService is deprecated");

    if (nsComponentManagerImpl::gComponentManager == nsnull)
        return NS_ERROR_UNEXPECTED;
    
    return nsComponentManagerImpl::gComponentManager->GetService(aClass, aIID, (void**)result);
}

nsresult
nsServiceManager::ReleaseService(const nsCID& aClass, nsISupports* service,
                                 nsIShutdownListener* shutdownListener)
{
    NS_NOTREACHED("nsServiceManager::ReleaseService is deprecated");

    NS_IF_RELEASE(service);
    return NS_OK;
}

nsresult
nsServiceManager::RegisterService(const nsCID& aClass, nsISupports* aService)
{
    NS_NOTREACHED("nsServiceManager::RegisterService is deprecated");
    
    if (nsComponentManagerImpl::gComponentManager == nsnull)
        return NS_ERROR_UNEXPECTED;
    
    return nsComponentManagerImpl::gComponentManager->RegisterService(aClass, aService);
}

nsresult
nsServiceManager::UnregisterService(const nsCID& aClass)
{
    NS_NOTREACHED("nsServiceManager::UnregisterService is deprecated");

    if (nsComponentManagerImpl::gComponentManager == nsnull)
        return NS_ERROR_UNEXPECTED;
    
    return nsComponentManagerImpl::gComponentManager->UnregisterService(aClass);
}




nsresult
nsServiceManager::GetService(const char* aContractID, const nsIID& aIID,
                             nsISupports* *result,
                             nsIShutdownListener* shutdownListener)
{
    NS_NOTREACHED("nsServiceManager::GetService is deprecated");
    
    if (nsComponentManagerImpl::gComponentManager == nsnull)
        return NS_ERROR_UNEXPECTED;

    return nsComponentManagerImpl::gComponentManager->GetServiceByContractID(aContractID, aIID, (void**)result);
}

nsresult
nsServiceManager::ReleaseService(const char* aContractID, nsISupports* service,
                                 nsIShutdownListener* shutdownListener)
{
    NS_NOTREACHED("nsServiceManager::ReleaseService is deprecated");

    NS_RELEASE(service);
    return NS_OK;
}

nsresult
nsServiceManager::RegisterService(const char* aContractID, nsISupports* aService)
{
    NS_NOTREACHED("nsServiceManager::RegisterService is deprecated");
    
    if (nsComponentManagerImpl::gComponentManager == nsnull)
        return NS_ERROR_UNEXPECTED;
    
    return nsComponentManagerImpl::gComponentManager->RegisterService(aContractID, aService);
}

nsresult
nsServiceManager::UnregisterService(const char* aContractID)
{
    NS_NOTREACHED("nsServiceManager::UnregisterService is deprecated");

    
    
    
    if (gXPCOMShuttingDown)
        return NS_OK;
    
    if (nsComponentManagerImpl::gComponentManager == nsnull)
        return NS_ERROR_UNEXPECTED;

    return nsComponentManagerImpl::gComponentManager->UnregisterService(aContractID);
}

