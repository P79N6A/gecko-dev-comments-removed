





































#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIAppStartupNotifier.h"
#include "nsIStringBundle.h"

#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"

#include "nsXPCOM.h"
#include "nsEmbedAPI.h"

static nsIServiceManager *sServiceManager = nsnull;
static PRBool             sRegistryInitializedFlag = PR_FALSE;
static PRUint32           sInitCounter = 0;

#define HACK_AROUND_THREADING_ISSUES


#ifdef HACK_AROUND_NONREENTRANT_INITXPCOM

class XPCOMCleanupHack
{
public:
    PRBool mCleanOnExit;

    XPCOMCleanupHack() : mCleanOnExit(PR_FALSE) {}
    ~XPCOMCleanupHack()
    {
        if (mCleanOnExit)
        {
            if (sInitCounter > 0)
            {
                sInitCounter = 1;
                NS_TermEmbedding();
            }
            

        }
    }
};
static PRBool sXPCOMInitializedFlag = PR_FALSE;
static XPCOMCleanupHack sXPCOMCleanupHack;
#endif


NS_METHOD
NS_InitEmbedding(nsILocalFile *mozBinDirectory,
                 nsIDirectoryServiceProvider *appFileLocProvider,
                 nsStaticModuleInfo const *aStaticComponents,
                 PRUint32 aStaticComponentCount)
{
    nsresult rv;

    
    sInitCounter++;
    if (sInitCounter > 1)
        return NS_OK;

    
#ifdef HACK_AROUND_NONREENTRANT_INITXPCOM
    
    if (!sXPCOMInitializedFlag)
#endif
    {
        
        rv = NS_InitXPCOM3(&sServiceManager, mozBinDirectory, appFileLocProvider,
                           aStaticComponents, aStaticComponentCount);
        NS_ENSURE_SUCCESS(rv, rv);
                
#ifdef HACK_AROUND_NONREENTRANT_INITXPCOM
        sXPCOMInitializedFlag = PR_TRUE;
        sXPCOMCleanupHack.mCleanOnExit = PR_TRUE;
#endif
    }
    
    if (!sRegistryInitializedFlag)
    {
#ifdef DEBUG
        nsIComponentRegistrar *registrar;
        rv = sServiceManager->QueryInterface(NS_GET_IID(nsIComponentRegistrar),
                                             (void **) &registrar);
        if (NS_FAILED(rv))
        {
            NS_WARNING("Could not QI to registrar");
            return rv;
        }
        rv = registrar->AutoRegister(nsnull);
        if (NS_FAILED(rv))
        {
            NS_WARNING("Could not AutoRegister");
        }
        else
        {
            
            
            
            
            
            

            if (appFileLocProvider)
            {
                nsIFile *greDir = nsnull;
                PRBool persistent = PR_TRUE;

                appFileLocProvider->GetFile(NS_GRE_DIR, &persistent,
                                            &greDir);
                if (greDir)
                {
                    rv = registrar->AutoRegister(greDir);
                    if (NS_FAILED(rv))
                        NS_WARNING("Could not AutoRegister GRE components");
                    NS_RELEASE(greDir);
                }
            }
        }
        NS_RELEASE(registrar);
        if (NS_FAILED(rv))
            return rv;
#endif
        sRegistryInitializedFlag = PR_TRUE;
    }

    nsIComponentManager *compMgr;
    rv = sServiceManager->QueryInterface(NS_GET_IID(nsIComponentManager),
                                         (void **) &compMgr);
    if (NS_FAILED(rv))
        return rv;

    nsIObserver *startupNotifier;
    rv = compMgr->CreateInstanceByContractID(NS_APPSTARTUPNOTIFIER_CONTRACTID,
                                             NULL,
                                             NS_GET_IID(nsIObserver),
                                             (void **) &startupNotifier);
    NS_RELEASE(compMgr);
    if (NS_FAILED(rv))
        return rv;

	  startupNotifier->Observe(nsnull, APPSTARTUP_TOPIC, nsnull);
    NS_RELEASE(startupNotifier);

#ifdef HACK_AROUND_THREADING_ISSUES
    
    nsIStringBundleService *bundleService;
    rv = sServiceManager->GetServiceByContractID(NS_STRINGBUNDLE_CONTRACTID,
                                                 NS_GET_IID(nsIStringBundleService),
                                                 (void **) &bundleService);
    if (NS_SUCCEEDED(rv))
    {
        nsIStringBundle *stringBundle;
        const char propertyURL[] = "chrome://necko/locale/necko.properties";
        rv = bundleService->CreateBundle(propertyURL, &stringBundle);
        NS_RELEASE(stringBundle);
        NS_RELEASE(bundleService);
    }
#endif

    return NS_OK;
}

NS_METHOD
NS_TermEmbedding()
{
    
    if (sInitCounter > 1)
    {
        sInitCounter--;
        return NS_OK;
    }
    sInitCounter = 0;

    NS_IF_RELEASE(sServiceManager);

    
#ifndef HACK_AROUND_NONREENTRANT_INITXPCOM
    nsresult rv = NS_ShutdownXPCOM(sServiceManager);
    NS_ENSURE_SUCCESS(rv, rv);
#endif

    return NS_OK;
}
