






































#include "nsXPInstallManager.h"
#include "nsInstallTrigger.h"
#include "nsIDOMInstallTriggerGlobal.h"

#include "nscore.h"
#include "nsAutoPtr.h"
#include "netCore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPermissionManager.h"
#include "nsIDocShell.h"
#include "nsNetUtil.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIPrincipal.h"
#include "nsIObserverService.h"
#include "nsIPropertyBag2.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsIContentHandler.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsXPIInstallInfo.h"


nsInstallTrigger::nsInstallTrigger()
{
    mScriptObject   = nsnull;
}

nsInstallTrigger::~nsInstallTrigger()
{
}


NS_IMPL_THREADSAFE_ISUPPORTS3 (nsInstallTrigger,
                              nsIScriptObjectOwner,
                              nsIDOMInstallTriggerGlobal,
                              nsIContentHandler)


NS_IMETHODIMP
nsInstallTrigger::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
    NS_PRECONDITION(nsnull != aScriptObject, "null arg");
    nsresult res = NS_OK;

    if (nsnull == mScriptObject)
    {
        res = NS_NewScriptInstallTriggerGlobal(aContext,
                                               (nsIDOMInstallTriggerGlobal*)this,
                                               aContext->GetGlobalObject(),
                                               &mScriptObject);
    }

    *aScriptObject = mScriptObject;
    return res;
}

NS_IMETHODIMP
nsInstallTrigger::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}




NS_IMETHODIMP
nsInstallTrigger::HandleContent(const char * aContentType,
                                nsIInterfaceRequestor* aWindowContext,
                                nsIRequest* aRequest)
{
    nsresult rv = NS_OK;
    if (!aRequest)
        return NS_ERROR_NULL_POINTER;

    if (nsCRT::strcasecmp(aContentType, "application/x-xpinstall") != 0)
    {
        
        return NS_ERROR_WONT_HANDLE_CONTENT;
    }

    
    nsCOMPtr<nsIURI> uri;
    nsCAutoString    urispec;
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
    if (channel)
    {
        rv = channel->GetURI(getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv) && uri)
            rv = uri->GetSpec(urispec);
    }
    if (NS_FAILED(rv))
        return rv;
    if (urispec.IsEmpty())
        return NS_ERROR_ILLEGAL_VALUE;


    
    NS_NAMED_LITERAL_STRING(referrerProperty, "docshell.internalReferrer");
    bool useReferrer = false;
    nsCOMPtr<nsIURI> referringURI;
    nsCOMPtr<nsIPropertyBag2> channelprops(do_QueryInterface(channel));

    if (channelprops)
    {
        
        
        
        
        
        
        
        
        
        
        
        rv = channelprops->GetPropertyAsInterface(referrerProperty,
                                                  NS_GET_IID(nsIURI),
                                                  getter_AddRefs(referringURI));
        if (NS_SUCCEEDED(rv))
            useReferrer = true;
    }

    
    
    aRequest->Cancel(NS_BINDING_ABORTED);


    
    nsCOMPtr<nsIScriptGlobalObjectOwner> globalObjectOwner =
                                         do_QueryInterface(aWindowContext);
    nsIScriptGlobalObject* globalObject =
      globalObjectOwner ? globalObjectOwner->GetScriptGlobalObject() : nsnull;
    if ( !globalObject )
        return NS_ERROR_INVALID_ARG;


    nsCOMPtr<nsIURI> checkuri;

    if ( useReferrer )
    {
        
        
        
        
        
        
        

        checkuri = referringURI;
    }
    else
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        checkuri = uri;
    }

    nsAutoPtr<nsXPITriggerInfo> trigger(new nsXPITriggerInfo());
    nsAutoPtr<nsXPITriggerItem> item(new nsXPITriggerItem(0, NS_ConvertUTF8toUTF16(urispec).get(),
                                                          nsnull));
    if (trigger && item)
    {
        
        trigger->Add(item.forget());
        nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(globalObject);
        nsCOMPtr<nsIXPIInstallInfo> installInfo =
                              new nsXPIInstallInfo(win, checkuri, trigger, 0);
        if (installInfo)
        {
            
            trigger.forget();
            if (AllowInstall(checkuri))
            {
                return StartInstall(installInfo, nsnull);
            }
            else
            {
                nsCOMPtr<nsIObserverService> os =
                  mozilla::services::GetObserverService();
                if (os)
                    os->NotifyObservers(installInfo,
                                        "xpinstall-install-blocked",
                                        nsnull);
                return NS_ERROR_ABORT;
            }
        }
    }
    return NS_ERROR_OUT_OF_MEMORY;
}







static void updatePermissions( const char* aPref,
                               PRUint32 aPermission,
                               nsIPermissionManager* aPermissionManager,
                               nsIPrefBranch*        aPrefBranch)
{
    NS_PRECONDITION(aPref && aPermissionManager && aPrefBranch, "Null arguments!");

    nsXPIDLCString hostlist;
    nsresult rv = aPrefBranch->GetCharPref( aPref, getter_Copies(hostlist));
    if (NS_SUCCEEDED(rv) && !hostlist.IsEmpty())
    {
        nsCAutoString host;
        PRInt32 start=0, match=0;
        nsresult rv;
        nsCOMPtr<nsIURI> uri;

        do {
            match = hostlist.FindChar(',', start);

            host = Substring(hostlist, start, match-start);
            host.CompressWhitespace();
            host.Insert("http://", 0);

            rv = NS_NewURI(getter_AddRefs(uri), host);
            if (NS_SUCCEEDED(rv))
            {
                aPermissionManager->Add( uri, XPI_PERMISSION, aPermission, 
                                         nsIPermissionManager::EXPIRE_NEVER, 0 );
            }
            start = match+1;
        } while ( match > 0 );

        
        aPrefBranch->SetCharPref( aPref, "");
    }
}




bool
nsInstallTrigger::AllowInstall(nsIURI* aLaunchURI)
{
    
    bool xpiEnabled = false;
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if ( !prefBranch)
    {
        return true; 
    }

    prefBranch->GetBoolPref( XPINSTALL_ENABLE_PREF, &xpiEnabled);
    if ( !xpiEnabled )
    {
        
        return false;
    }


    
    nsCOMPtr<nsIPermissionManager> permissionMgr =
                            do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

    if ( permissionMgr && aLaunchURI )
    {
        bool isChrome = false;
        bool isFile = false;
        aLaunchURI->SchemeIs( "chrome", &isChrome );
        aLaunchURI->SchemeIs( "file", &isFile );

        
        if ( !isChrome && !isFile )
        {
            
            updatePermissions( XPINSTALL_WHITELIST_ADD,
                               nsIPermissionManager::ALLOW_ACTION,
                               permissionMgr, prefBranch );
            updatePermissions( XPINSTALL_WHITELIST_ADD_36,
                               nsIPermissionManager::ALLOW_ACTION,
                               permissionMgr, prefBranch );
            updatePermissions( XPINSTALL_BLACKLIST_ADD,
                               nsIPermissionManager::DENY_ACTION,
                               permissionMgr, prefBranch );

            bool requireWhitelist = true;
            prefBranch->GetBoolPref( XPINSTALL_WHITELIST_REQUIRED, &requireWhitelist );

            PRUint32 permission = nsIPermissionManager::UNKNOWN_ACTION;
            permissionMgr->TestPermission( aLaunchURI, XPI_PERMISSION, &permission );

            if ( permission == nsIPermissionManager::DENY_ACTION )
            {
                xpiEnabled = false;
            }
            else if ( requireWhitelist &&
                      permission != nsIPermissionManager::ALLOW_ACTION )
            {
                xpiEnabled = false;
            }
        }
    }

    return xpiEnabled;
}


NS_IMETHODIMP
nsInstallTrigger::GetOriginatingURI(nsIScriptGlobalObject* aGlobalObject, nsIURI * *aUri)
{
    NS_ENSURE_ARG_POINTER(aGlobalObject);

    *aUri = nsnull;

    
    nsCOMPtr<nsIDOMDocument> domdoc;
    nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(aGlobalObject));
    if ( window )
    {
        window->GetDocument(getter_AddRefs(domdoc));
        nsCOMPtr<nsIDocument> doc(do_QueryInterface(domdoc));
        if ( doc )
            NS_ADDREF(*aUri = doc->GetDocumentURI());
    }
    return NS_OK;
}

NS_IMETHODIMP
nsInstallTrigger::UpdateEnabled(nsIScriptGlobalObject* aGlobalObject, bool aUseWhitelist, bool* aReturn)
{
    nsCOMPtr<nsIURI> uri;
    nsresult rv = GetOriginatingURI(aGlobalObject, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    return UpdateEnabled(uri, aUseWhitelist, aReturn);
}

NS_IMETHODIMP
nsInstallTrigger::UpdateEnabled(nsIURI* aURI, bool aUseWhitelist, bool* aReturn)
{
    
    *aReturn = false;

    if (!aUseWhitelist)
    {
        
        nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
        if (prefBranch)
            prefBranch->GetBoolPref( XPINSTALL_ENABLE_PREF, aReturn);
    }
    else if (aURI)
    {
        *aReturn = AllowInstall(aURI);
    }

    return NS_OK;
}


NS_IMETHODIMP
nsInstallTrigger::StartInstall(nsIXPIInstallInfo* aInstallInfo, bool* aReturn)
{
    if (aReturn)
        *aReturn = false;

    nsXPInstallManager *mgr = new nsXPInstallManager();
    if (mgr)
    {
        nsresult rv = mgr->InitManagerWithInstallInfo(aInstallInfo);
        if (NS_SUCCEEDED(rv) && aReturn)
            *aReturn = true;
        return rv;
    }
    else
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
}
