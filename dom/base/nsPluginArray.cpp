





































#include "nsPluginArray.h"
#include "nsMimeTypeArray.h"
#include "nsGlobalWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMMimeType.h"
#include "nsIPluginHost.h"
#include "nsIDocShell.h"
#include "nsIWebNavigation.h"
#include "nsDOMClassInfo.h"
#include "nsPluginError.h"
#include "nsContentUtils.h"

nsPluginArray::nsPluginArray(nsNavigator* navigator,
                             nsIDocShell *aDocShell)
{
  nsresult rv;
  mNavigator = navigator; 
  mPluginHost = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &rv);
  mPluginCount = 0;
  mPluginArray = nsnull;
  mDocShell = aDocShell;
}

nsPluginArray::~nsPluginArray()
{
  if (mPluginArray != nsnull) {
    for (PRUint32 i = 0; i < mPluginCount; i++) {
      NS_IF_RELEASE(mPluginArray[i]);
    }
    delete[] mPluginArray;
  }
}


NS_INTERFACE_MAP_BEGIN(nsPluginArray)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMPluginArray)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPluginArray)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(PluginArray)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPluginArray)
NS_IMPL_RELEASE(nsPluginArray)

NS_IMETHODIMP
nsPluginArray::GetLength(PRUint32* aLength)
{
  if (AllowPlugins() && mPluginHost)
    return mPluginHost->GetPluginCount(aLength);
  
  *aLength = 0;
  return NS_OK;
}

PRBool
nsPluginArray::AllowPlugins()
{
  PRBool allowPlugins = PR_FALSE;
  if (mDocShell)
    if (NS_FAILED(mDocShell->GetAllowPlugins(&allowPlugins)))
      allowPlugins = PR_FALSE;

  return allowPlugins;
}

nsIDOMPlugin*
nsPluginArray::GetItemAt(PRUint32 aIndex, nsresult* aResult)
{
  *aResult = NS_OK;

  if (!AllowPlugins())
    return nsnull;

  if (mPluginArray == nsnull) {
    *aResult = GetPlugins();
    if (*aResult != NS_OK)
      return nsnull;
  }

  return aIndex < mPluginCount ? mPluginArray[aIndex] : nsnull;
}

NS_IMETHODIMP
nsPluginArray::Item(PRUint32 aIndex, nsIDOMPlugin** aReturn)
{
  nsresult rv;

  NS_IF_ADDREF(*aReturn = GetItemAt(aIndex, &rv));

  return rv;
}

nsIDOMPlugin*
nsPluginArray::GetNamedItem(const nsAString& aName, nsresult* aResult)
{
  *aResult = NS_OK;

  if (!AllowPlugins())
    return nsnull;

  if (mPluginArray == nsnull) {
    *aResult = GetPlugins();
    if (*aResult != NS_OK)
      return nsnull;
  }

  for (PRUint32 i = 0; i < mPluginCount; i++) {
    nsAutoString pluginName;
    nsIDOMPlugin* plugin = mPluginArray[i];
    if (plugin->GetName(pluginName) == NS_OK && pluginName.Equals(aName)) {
      return plugin;
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsPluginArray::NamedItem(const nsAString& aName, nsIDOMPlugin** aReturn)
{
  NS_PRECONDITION(nsnull != aReturn, "null arg");

  nsresult rv;

  NS_IF_ADDREF(*aReturn = GetNamedItem(aName, &rv));

  return rv;
}

nsresult
nsPluginArray::GetPluginHost(nsIPluginHost** aPluginHost)
{
  NS_ENSURE_ARG_POINTER(aPluginHost);

  nsresult rv = NS_OK;

  if (!mPluginHost) {
    mPluginHost = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &rv);

    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  *aPluginHost = mPluginHost;
  NS_IF_ADDREF(*aPluginHost);

  return rv;
}

void
nsPluginArray::SetDocShell(nsIDocShell* aDocShell)
{
  mDocShell = aDocShell;
}

NS_IMETHODIMP
nsPluginArray::Refresh(PRBool aReloadDocuments)
{
  PRTime then = PR_Now();
  nsresult res = NS_OK;
  if (!AllowPlugins())
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;

  if (!mPluginHost) {
    mPluginHost = do_GetService(MOZ_PLUGIN_HOST_CONTRACTID, &res);
  }

  printf("1XXXXX  %lld\n\n\n", (PR_Now() - then)/1000);

  if(NS_FAILED(res)) {
    return res;
  }

  printf("2XXXXX  %lld\n\n\n", (PR_Now() - then)/1000);

  
  
  PRBool pluginsNotChanged = PR_FALSE;
  if(mPluginHost)
    pluginsNotChanged = (NS_ERROR_PLUGINS_PLUGINSNOTCHANGED == mPluginHost->ReloadPlugins(aReloadDocuments));

  printf("3XXXXX  %lld\n\n\n", (PR_Now() - then)/1000);

  
  
  if(pluginsNotChanged)
    return res;

  printf("4XXXXX  %lld\n\n\n", (PR_Now() - then)/1000);

  nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mDocShell);

  if (mPluginArray != nsnull) {
    for (PRUint32 i = 0; i < mPluginCount; i++) 
      NS_IF_RELEASE(mPluginArray[i]);

    delete[] mPluginArray;
  }

  mPluginCount = 0;
  mPluginArray = nsnull;

  printf("5XXXXX  %lld\n\n\n", (PR_Now() - then)/1000);

  if (mNavigator)
    mNavigator->RefreshMIMEArray();
  
  if (aReloadDocuments && webNav)
    webNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);

  printf("6XXXXX  %lld\n\n\n", (PR_Now() - then)/1000);
  return res;
}

nsresult
nsPluginArray::GetPlugins()
{
  nsresult rv = GetLength(&mPluginCount);
  if (NS_SUCCEEDED(rv)) {
    mPluginArray = new nsIDOMPlugin*[mPluginCount];
    if (!mPluginArray)
      return NS_ERROR_OUT_OF_MEMORY;

    if (!mPluginCount)
      return NS_OK;

    rv = mPluginHost->GetPlugins(mPluginCount, mPluginArray);
    if (NS_SUCCEEDED(rv)) {
      
      
      for (PRUint32 i = 0; i < mPluginCount; i++) {
        nsIDOMPlugin* wrapper = new nsPluginElement(mPluginArray[i]);
        NS_IF_ADDREF(wrapper);
        mPluginArray[i] = wrapper;
      }
    } else {
      



      mPluginCount = 0;
    }
  }
  return rv;
}



nsPluginElement::nsPluginElement(nsIDOMPlugin* plugin)
{
  mPlugin = plugin;  
  mMimeTypeCount = 0;
  mMimeTypeArray = nsnull;
}

nsPluginElement::~nsPluginElement()
{
  NS_IF_RELEASE(mPlugin);

  if (mMimeTypeArray != nsnull) {
    for (PRUint32 i = 0; i < mMimeTypeCount; i++)
      NS_IF_RELEASE(mMimeTypeArray[i]);
    delete[] mMimeTypeArray;
  }
}



NS_INTERFACE_MAP_BEGIN(nsPluginElement)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPlugin)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Plugin)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsPluginElement)
NS_IMPL_RELEASE(nsPluginElement)


NS_IMETHODIMP
nsPluginElement::GetDescription(nsAString& aDescription)
{
  return mPlugin->GetDescription(aDescription);
}

NS_IMETHODIMP
nsPluginElement::GetFilename(nsAString& aFilename)
{
  return mPlugin->GetFilename(aFilename);
}

NS_IMETHODIMP
nsPluginElement::GetVersion(nsAString& aVersion)
{
  return mPlugin->GetVersion(aVersion);
}

NS_IMETHODIMP
nsPluginElement::GetName(nsAString& aName)
{
  return mPlugin->GetName(aName);
}

NS_IMETHODIMP
nsPluginElement::GetLength(PRUint32* aLength)
{
  return mPlugin->GetLength(aLength);
}

nsIDOMMimeType*
nsPluginElement::GetItemAt(PRUint32 aIndex, nsresult *aResult)
{
  if (mMimeTypeArray == nsnull) {
    *aResult = GetMimeTypes();
    if (*aResult != NS_OK)
      return nsnull;
  }

  if (aIndex >= mMimeTypeCount) {
    *aResult = NS_ERROR_FAILURE;

    return nsnull;
  }

  *aResult = NS_OK;

  return mMimeTypeArray[aIndex];
}

NS_IMETHODIMP
nsPluginElement::Item(PRUint32 aIndex, nsIDOMMimeType** aReturn)
{
  nsresult rv;

  NS_IF_ADDREF(*aReturn = GetItemAt(aIndex, &rv));

  return rv;
}

nsIDOMMimeType*
nsPluginElement::GetNamedItem(const nsAString& aName, nsresult *aResult)
{
  if (mMimeTypeArray == nsnull) {
    *aResult = GetMimeTypes();
    if (*aResult != NS_OK)
      return nsnull;
  }

  *aResult = NS_OK;
  for (PRUint32 i = 0; i < mMimeTypeCount; i++) {
    nsAutoString type;
    nsIDOMMimeType* mimeType = mMimeTypeArray[i];
    if (mimeType->GetType(type) == NS_OK && type.Equals(aName)) {
      return mimeType;
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsPluginElement::NamedItem(const nsAString& aName, nsIDOMMimeType** aReturn)
{
  nsresult rv;

  NS_IF_ADDREF(*aReturn = GetNamedItem(aName, &rv));

  return rv;
}

nsresult
nsPluginElement::GetMimeTypes()
{
  nsresult rv = mPlugin->GetLength(&mMimeTypeCount);
  if (rv == NS_OK) {
    mMimeTypeArray = new nsIDOMMimeType*[mMimeTypeCount];
    if (mMimeTypeArray == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;
    for (PRUint32 i = 0; i < mMimeTypeCount; i++) {
      nsCOMPtr<nsIDOMMimeType> mimeType;
      rv = mPlugin->Item(i, getter_AddRefs(mimeType));
      if (rv != NS_OK)
        break;
      mimeType = new nsMimeType(this, mimeType);
      NS_IF_ADDREF(mMimeTypeArray[i] = mimeType);
    }
  }
  return rv;
}
