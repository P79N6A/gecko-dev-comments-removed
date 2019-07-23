





































#include "nsSoftwareUpdate.h"
#include "nsXPInstallManager.h"
#include "nsInstallTrigger.h"
#include "nsInstallVersion.h"
#include "nsIDOMInstallTriggerGlobal.h"

#include "nscore.h"
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

#include "VerReg.h"

#include "nsIContentHandler.h"
#include "nsIChannel.h"
#include "nsIURI.h"



nsInstallTrigger::nsInstallTrigger()
{
    mScriptObject   = nsnull;

    
    nsCOMPtr<nsISoftwareUpdate> svc (do_GetService(NS_IXPINSTALLCOMPONENT_CONTRACTID));
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
    PRBool useReferrer = PR_FALSE;
    nsCOMPtr<nsIURI> referringURI;
    nsCOMPtr<nsIPropertyBag2> channelprops(do_QueryInterface(channel));

    if (channelprops)
    {
        
        
        
        
        
        
        
        
        
        
        
        rv = channelprops->GetPropertyAsInterface(referrerProperty,
                                                  NS_GET_IID(nsIURI),
                                                  getter_AddRefs(referringURI));
        if (NS_SUCCEEDED(rv))
            useReferrer = PR_TRUE;
    }

    
    
    aRequest->Cancel(NS_BINDING_ABORTED);


    
    nsCOMPtr<nsIScriptGlobalObjectOwner> globalObjectOwner = 
                                         do_QueryInterface(aWindowContext);
    nsIScriptGlobalObject* globalObject =
      globalObjectOwner ? globalObjectOwner->GetScriptGlobalObject() : nsnull;
    if ( !globalObject )
        return NS_ERROR_INVALID_ARG;


    
    
    PRBool enabled = PR_FALSE;
    
    
    
    nsCAutoString host;

    if ( useReferrer )
    {
        
        
        
        
        
        
        

        enabled = AllowInstall( referringURI );
        referringURI->GetHost(host);
    }
    else
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        enabled = AllowInstall( uri );
        uri->GetHost(host);
    }


    if ( enabled )
    {
        rv = StartSoftwareUpdate( globalObject,
                                  NS_ConvertUTF8toUTF16(urispec),
                                  0,
                                  &enabled);
    }
    else
    {
        nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(globalObject));
        nsCOMPtr<nsIObserverService> os(do_GetService("@mozilla.org/observer-service;1"));
        if (os) {
            os->NotifyObservers(win->GetDocShell(),
                                "xpinstall-install-blocked",
                                NS_ConvertUTF8toUTF16(host).get());
        }
        rv = NS_ERROR_ABORT;
    }
    
    return rv;
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
                aPermissionManager->Add( uri, XPI_PERMISSION, aPermission );
            }
            start = match+1;
        } while ( match > 0 );

        
        aPrefBranch->SetCharPref( aPref, "");
    }
}




PRBool
nsInstallTrigger::AllowInstall(nsIURI* aLaunchURI)
{
    
    PRBool xpiEnabled = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if ( !prefBranch)
    {
        return PR_TRUE; 
    }

    prefBranch->GetBoolPref( XPINSTALL_ENABLE_PREF, &xpiEnabled);
    if ( !xpiEnabled )
    {
        
        return PR_FALSE;
    }


    
    nsCOMPtr<nsIPermissionManager> permissionMgr =
                            do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

    if ( permissionMgr && aLaunchURI )
    {
        PRBool isChrome = PR_FALSE;
        PRBool isFile = PR_FALSE;
        aLaunchURI->SchemeIs( "chrome", &isChrome );
        aLaunchURI->SchemeIs( "file", &isFile );

        
        if ( !isChrome && !isFile )
        {
            
            updatePermissions( XPINSTALL_WHITELIST_ADD,
                               nsIPermissionManager::ALLOW_ACTION,
                               permissionMgr, prefBranch );
            updatePermissions( XPINSTALL_WHITELIST_ADD_103,
                               nsIPermissionManager::ALLOW_ACTION,
                               permissionMgr, prefBranch );
            updatePermissions( XPINSTALL_BLACKLIST_ADD,
                               nsIPermissionManager::DENY_ACTION,
                               permissionMgr, prefBranch );

            PRBool requireWhitelist = PR_TRUE;
            prefBranch->GetBoolPref( XPINSTALL_WHITELIST_REQUIRED, &requireWhitelist );

            PRUint32 permission = nsIPermissionManager::UNKNOWN_ACTION;
            permissionMgr->TestPermission( aLaunchURI, XPI_PERMISSION, &permission );

            if ( permission == nsIPermissionManager::DENY_ACTION )
            {
                xpiEnabled = PR_FALSE;
            }
            else if ( requireWhitelist &&
                      permission != nsIPermissionManager::ALLOW_ACTION )
            {
                xpiEnabled = PR_FALSE;
            }
        }
    }

    return xpiEnabled;
}


NS_IMETHODIMP
nsInstallTrigger::UpdateEnabled(nsIScriptGlobalObject* aGlobalObject, PRBool aUseWhitelist, PRBool* aReturn)
{
    
    *aReturn = PR_FALSE;

    if (!aUseWhitelist)
    {
        
        nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
        if (prefBranch)
            prefBranch->GetBoolPref( XPINSTALL_ENABLE_PREF, aReturn);
    }
    else
    {
        NS_ENSURE_ARG_POINTER(aGlobalObject);

        
        nsCOMPtr<nsIDOMDocument> domdoc;
        nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(aGlobalObject));
        if ( window )
        {
            window->GetDocument(getter_AddRefs(domdoc));
            nsCOMPtr<nsIDocument> doc(do_QueryInterface(domdoc));
            if ( doc )
            {
                *aReturn = AllowInstall( doc->GetDocumentURI() );
            }
        }
    }

    return NS_OK;
}


NS_IMETHODIMP
nsInstallTrigger::Install(nsIScriptGlobalObject* aGlobalObject, nsXPITriggerInfo* aTrigger, PRBool* aReturn)
{
    NS_ASSERTION(aReturn, "Invalid pointer arg");
    *aReturn = PR_FALSE;

    nsresult rv;
    nsXPInstallManager *mgr = new nsXPInstallManager();
    if (mgr)
    {
        
        rv = mgr->InitManager( aGlobalObject, aTrigger, 0 );
        if (NS_SUCCEEDED(rv))
            *aReturn = PR_TRUE;
    }
    else
    {
        delete aTrigger;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }


    return rv;
}


NS_IMETHODIMP
nsInstallTrigger::InstallChrome(nsIScriptGlobalObject* aGlobalObject, PRUint32 aType, nsXPITriggerItem *aItem, PRBool* aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);
    NS_ENSURE_ARG_POINTER(aItem);
    *aReturn = PR_FALSE;


    
    
    nsresult rv = NS_ERROR_OUT_OF_MEMORY;
    nsXPInstallManager *mgr = new nsXPInstallManager();
    if (mgr)
    {
        nsXPITriggerInfo* trigger = new nsXPITriggerInfo();
        if ( trigger )
        {
            trigger->Add( aItem );

            
            rv = mgr->InitManager( aGlobalObject, trigger, aType );
            *aReturn = PR_TRUE;
        }
        else
        {
            rv = NS_ERROR_OUT_OF_MEMORY;
            delete mgr;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsInstallTrigger::StartSoftwareUpdate(nsIScriptGlobalObject* aGlobalObject, const nsString& aURL, PRInt32 aFlags, PRBool* aReturn)
{
    nsresult rv = NS_ERROR_OUT_OF_MEMORY;
    *aReturn = PR_FALSE;

    
    
    nsXPInstallManager *mgr = new nsXPInstallManager();
    if (mgr)
    {
        nsXPITriggerInfo* trigger = new nsXPITriggerInfo();
        if ( trigger )
        {
            nsXPITriggerItem* item = new nsXPITriggerItem(0,aURL.get(),nsnull);
            if (item)
            {
                trigger->Add( item );
                
                rv = mgr->InitManager(aGlobalObject, trigger, 0 );
                *aReturn = PR_TRUE;
            }
            else
            {
                rv = NS_ERROR_OUT_OF_MEMORY;
                delete trigger;
                delete mgr;
            }
        }
        else
        {
            rv = NS_ERROR_OUT_OF_MEMORY;
            delete mgr;
        }
    }

    return rv;
}


NS_IMETHODIMP
nsInstallTrigger::CompareVersion(const nsString& aRegName, PRInt32 aMajor, PRInt32 aMinor, PRInt32 aRelease, PRInt32 aBuild, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aMajor, aMinor, aRelease, aBuild);

    return CompareVersion(aRegName, &inVersion, aReturn);
}

NS_IMETHODIMP
nsInstallTrigger::CompareVersion(const nsString& aRegName, const nsString& aVersion, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aVersion);

    return CompareVersion(aRegName, &inVersion, aReturn);
}

NS_IMETHODIMP
nsInstallTrigger::CompareVersion(const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32* aReturn)
{
    *aReturn = NOT_FOUND;  

    VERSION              cVersion;
    NS_ConvertUTF16toUTF8 regName(aRegName);
    REGERR               status;
    nsInstallVersion     regNameVersion;

    status = VR_GetVersion( const_cast<char *>(regName.get()), &cVersion );
    if ( status == REGERR_OK )
    {
        
        if ( VR_ValidateComponent( const_cast<char *>(regName.get()) ) != REGERR_NOFILE )
        {
            
            regNameVersion.Init(cVersion.major,
                                cVersion.minor,
                                cVersion.release,
                                cVersion.build);

            regNameVersion.CompareTo( aVersion, aReturn );
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsInstallTrigger::GetVersion(const nsString& component, nsString& version)
{
    VERSION              cVersion;
    NS_ConvertUTF16toUTF8 regName(component);
    REGERR               status;

    status = VR_GetVersion( const_cast<char *>(regName.get()), &cVersion );

    version.Truncate();

    
    
    
    if ( status == REGERR_OK )
    {
        nsInstallVersion regNameVersion;

        regNameVersion.Init(cVersion.major,
                            cVersion.minor,
                            cVersion.release,
                            cVersion.build);

        regNameVersion.ToString(version);
    }

    return NS_OK;
}

