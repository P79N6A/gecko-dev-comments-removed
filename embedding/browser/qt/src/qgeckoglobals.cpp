






































#include "qgeckoglobals.h"

#include "qgeckoembed.h"
#include "EmbedWindow.h"
#include "QtPromptService.h"

#include "nsIAppShell.h"
#include <nsIDocShell.h>
#include <nsIWebProgress.h>
#include <nsIWebNavigation.h>
#include <nsIWebBrowser.h>
#include <nsISHistory.h>
#include <nsIWebBrowserChrome.h>
#include "nsIWidget.h"
#include "nsCRT.h"
#include <nsIWindowWatcher.h>
#include <nsILocalFile.h>
#include <nsEmbedAPI.h>
#include <nsWidgetsCID.h>
#include <nsIDOMUIEvent.h>

#include <nsIInterfaceRequestor.h>
#include <nsIComponentManager.h>
#include <nsIFocusController.h>
#include <nsProfileDirServiceProvider.h>
#include <nsIGenericFactory.h>
#include <nsIComponentRegistrar.h>
#include <nsVoidArray.h>
#include <nsIDOMBarProp.h>
#include <nsIDOMWindow.h>

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

PRUint32     QGeckoGlobals::sWidgetCount = 0;
char        *QGeckoGlobals::sCompPath    = nsnull;
nsIAppShell *QGeckoGlobals::sAppShell    = nsnull;
char        *QGeckoGlobals::sProfileDir  = nsnull;
char        *QGeckoGlobals::sProfileName = nsnull;
nsVoidArray *QGeckoGlobals::sWindowList  = nsnull;
nsIDirectoryServiceProvider *QGeckoGlobals::sAppFileLocProvider = nsnull;
nsProfileDirServiceProvider *QGeckoGlobals::sProfileDirServiceProvider = nsnull;

#define NS_PROMPTSERVICE_CID \
 {0x95611356, 0xf583, 0x46f5, {0x81, 0xff, 0x4b, 0x3e, 0x01, 0x62, 0xc6, 0x19}}

NS_GENERIC_FACTORY_CONSTRUCTOR(QtPromptService)

static const nsModuleComponentInfo defaultAppComps[] = {
  {
    "Prompt Service",
    NS_PROMPTSERVICE_CID,
    "@mozilla.org/embedcomp/prompt-service;1",
    QtPromptServiceConstructor
  }
};

void
QGeckoGlobals::pushStartup()
{
    
    sWidgetCount++;

    
    if (sWidgetCount == 1) {
        nsresult rv;
        nsCOMPtr<nsILocalFile> binDir;

        if (sCompPath) {
            rv = NS_NewNativeLocalFile(nsDependentCString(sCompPath), 1, getter_AddRefs(binDir));
            if (NS_FAILED(rv))
                return;
        }

        rv = NS_InitEmbedding(binDir, sAppFileLocProvider);
        if (NS_FAILED(rv))
            return;

        
        if (sAppFileLocProvider) {
            NS_RELEASE(sAppFileLocProvider);
            sAppFileLocProvider = nsnull;
        }

        rv = startupProfile();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Warning: Failed to start up profiles.\n");

        rv = registerAppComponents();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Warning: Failed to register app components.\n");

        

        nsCOMPtr<nsIAppShell> appShell;
        appShell = do_CreateInstance(kAppShellCID);
        if (!appShell) {
            NS_WARNING("Failed to create appshell in QGeckoGlobals::pushStartup!\n");
            return;
        }
        sAppShell = appShell.get();
        NS_ADDREF(sAppShell);
        sAppShell->Create(0, nsnull);
        sAppShell->Spinup();
    }
}

void
QGeckoGlobals::popStartup()
{
    sWidgetCount--;
    if (sWidgetCount == 0) {
        
        shutdownProfile();

        if (sAppShell) {
            
            sAppShell->Spindown();
            NS_RELEASE(sAppShell);
            sAppShell = 0;
        }

        
        NS_TermEmbedding();
    }
}

void
QGeckoGlobals::setCompPath(const char *aPath)
{
    if (sCompPath)
        free(sCompPath);
    if (aPath)
        sCompPath = strdup(aPath);
    else
        sCompPath = nsnull;
}

void
QGeckoGlobals::setAppComponents(const nsModuleComponentInfo *,
                             int)
{
}

void
QGeckoGlobals::setProfilePath(const char *aDir, const char *aName)
{
    if (sProfileDir) {
        nsMemory::Free(sProfileDir);
        sProfileDir = nsnull;
    }

    if (sProfileName) {
        nsMemory::Free(sProfileName);
        sProfileName = nsnull;
    }

    if (aDir)
        sProfileDir = (char *)nsMemory::Clone(aDir, strlen(aDir) + 1);

    if (aName)
        sProfileName = (char *)nsMemory::Clone(aName, strlen(aName) + 1);
}

void
QGeckoGlobals::setDirectoryServiceProvider(nsIDirectoryServiceProvider
                                        *appFileLocProvider)
{
    if (sAppFileLocProvider)
        NS_RELEASE(sAppFileLocProvider);

    if (appFileLocProvider) {
        sAppFileLocProvider = appFileLocProvider;
        NS_ADDREF(sAppFileLocProvider);
    }
}



int
QGeckoGlobals::startupProfile(void)
{
    
    if (sProfileDir && sProfileName) {
        nsresult rv;
        nsCOMPtr<nsILocalFile> profileDir;
        NS_NewNativeLocalFile(nsDependentCString(sProfileDir), PR_TRUE,
                              getter_AddRefs(profileDir));
        if (!profileDir)
            return NS_ERROR_FAILURE;
        rv = profileDir->AppendNative(nsDependentCString(sProfileName));
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;

        nsCOMPtr<nsProfileDirServiceProvider> locProvider;
        NS_NewProfileDirServiceProvider(PR_TRUE, getter_AddRefs(locProvider));
        if (!locProvider)
            return NS_ERROR_FAILURE;
        rv = locProvider->Register();
        if (NS_FAILED(rv))
            return rv;
        rv = locProvider->SetProfileDir(profileDir);
        if (NS_FAILED(rv))
            return rv;
        
        NS_ADDREF(sProfileDirServiceProvider = locProvider);
    }
    return NS_OK;
}


void
QGeckoGlobals::shutdownProfile(void)
{
    if (sProfileDirServiceProvider) {
        sProfileDirServiceProvider->Shutdown();
        NS_RELEASE(sProfileDirServiceProvider);
        sProfileDirServiceProvider = 0;
    }
}


int
QGeckoGlobals::registerAppComponents()
{
  nsCOMPtr<nsIComponentRegistrar> cr;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(cr));
  NS_ENSURE_SUCCESS(rv, rv);

  int numAppComps = sizeof(defaultAppComps) / sizeof(nsModuleComponentInfo);
  for (int i = 0; i < numAppComps; ++i) {
    nsCOMPtr<nsIGenericFactory> componentFactory;
    rv = NS_NewGenericFactory(getter_AddRefs(componentFactory),
                              &(defaultAppComps[i]));
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to create factory for component");
      continue;  
    }

    rv = cr->RegisterFactory(defaultAppComps[i].mCID, defaultAppComps[i].mDescription,
                             defaultAppComps[i].mContractID, componentFactory);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to register factory for component");
  }

  return rv;
}

void QGeckoGlobals::initializeGlobalObjects()
{
    if (!sWindowList) {
        sWindowList = new nsVoidArray();
    }
}

void QGeckoGlobals::addEngine(QGeckoEmbed *embed)
{
    sWindowList->AppendElement(embed);
}

void QGeckoGlobals::removeEngine(QGeckoEmbed *embed)
{
    sWindowList->RemoveElement(embed);
}

QGeckoEmbed *QGeckoGlobals::findPrivateForBrowser(nsIWebBrowserChrome *aBrowser)
{
    if (!sWindowList)
        return nsnull;

    
    PRInt32 count = sWindowList->Count();
    
    
    
    for (int i = 0; i < count; i++) {
        QGeckoEmbed *tmpPrivate = NS_STATIC_CAST(QGeckoEmbed *,
                                                sWindowList->ElementAt(i));
        
        nsIWebBrowserChrome *chrome = NS_STATIC_CAST(nsIWebBrowserChrome *,
                                                     tmpPrivate->window());
        if (chrome == aBrowser)
            return tmpPrivate;
    }

    return nsnull;
}
