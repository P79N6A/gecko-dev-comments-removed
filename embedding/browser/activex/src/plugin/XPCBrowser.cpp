





































#include "StdAfx.h"

#include "npapi.h"

#include "nsCOMPtr.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsStringAPI.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsMemory.h"


#include "XPCBrowser.h"


IEBrowser::IEBrowser()
{
}

IEBrowser::~IEBrowser()
{
}

HRESULT IEBrowser::Init(PluginInstanceData *pData)
{
    mData = pData;
    
    NPN_GetValue(mData->pPluginInstance, NPNVDOMWindow, 
        NS_STATIC_CAST(nsIDOMWindow **,getter_AddRefs(mDOMWindow)));
    if (mDOMWindow)
    {
        mWebNavigation  = do_GetInterface(mDOMWindow);
    }
    return S_OK;
}


nsresult IEBrowser::GetWebNavigation(nsIWebNavigation **aWebNav)
{
    NS_ENSURE_ARG_POINTER(aWebNav);
    *aWebNav = mWebNavigation;
    NS_IF_ADDREF(*aWebNav);
    return (*aWebNav) ? NS_OK : NS_ERROR_FAILURE;
}


nsresult IEBrowser::GetDOMWindow(nsIDOMWindow **aDOMWindow)
{
    NS_ENSURE_ARG_POINTER(aDOMWindow);
    *aDOMWindow = mDOMWindow;
    NS_IF_ADDREF(*aDOMWindow);
    return (*aDOMWindow) ? NS_OK : NS_ERROR_FAILURE;
}


nsresult IEBrowser::GetPrefs(nsIPrefBranch **aPrefBranch)
{
    return CallGetService(NS_PREFSERVICE_CONTRACTID, aPrefBranch);
}


PRBool IEBrowser::BrowserIsValid()
{
    return mWebNavigation ? PR_TRUE : PR_FALSE;
}

