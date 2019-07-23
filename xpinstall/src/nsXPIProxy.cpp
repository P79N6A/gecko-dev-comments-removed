









































#include "nsXPIProxy.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIPluginManager.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsIPromptService.h"
#include "nsEmbedCID.h"

nsXPIProxy::nsXPIProxy()
{
}

nsXPIProxy::~nsXPIProxy()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsXPIProxy, nsPIXPIProxy)

NS_IMETHODIMP
nsXPIProxy::RefreshPlugins(PRBool aReloadPages)
{
    NS_DEFINE_CID(pluginManagerCID,NS_PLUGINMANAGER_CID);

    nsCOMPtr<nsIPluginManager> plugins(do_GetService(pluginManagerCID));

    if (!plugins)
        return NS_ERROR_FAILURE;

    return plugins->ReloadPlugins(aReloadPages);
}

NS_IMETHODIMP
nsXPIProxy::NotifyRestartNeeded()
{
    nsCOMPtr<nsIObserverService> obs(do_GetService("@mozilla.org/observer-service;1"));
    if (obs)
        obs->NotifyObservers( nsnull, "xpinstall-restart", nsnull );

    return NS_OK;
}

NS_IMETHODIMP
nsXPIProxy::Alert(const PRUnichar* aTitle, const PRUnichar* aText)
{
    nsCOMPtr<nsIPromptService> dialog(do_GetService(NS_PROMPTSERVICE_CONTRACTID));
    
    if (!dialog)
        return NS_ERROR_FAILURE;

    return dialog->Alert( nsnull, aTitle, aText );
}

NS_IMETHODIMP
nsXPIProxy::ConfirmEx(const PRUnichar* aDialogTitle, const PRUnichar* aText, PRUint32 aButtonFlags, const PRUnichar* aButton0Title, const PRUnichar* aButton1Title, const PRUnichar* aButton2Title, const PRUnichar* aCheckMsg, PRBool* aCheckState, PRInt32* aReturn)
{
    nsCOMPtr<nsIPromptService> dialog(do_GetService(NS_PROMPTSERVICE_CONTRACTID));

    if (!dialog)
        return NS_ERROR_FAILURE;

    return dialog->ConfirmEx( nsnull, aDialogTitle, aText, aButtonFlags, aButton0Title, aButton1Title, aButton2Title, aCheckMsg, aCheckState, aReturn );
}

