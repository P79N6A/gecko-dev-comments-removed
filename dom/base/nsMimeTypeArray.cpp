






































#include "nsMimeTypeArray.h"
#include "nsContentUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMPluginArray.h"
#include "nsIDOMPlugin.h"
#include "nsDOMClassInfoID.h"
#include "nsIMIMEService.h"
#include "nsIMIMEInfo.h"
#include "nsIFile.h"


nsMimeTypeArray::nsMimeTypeArray(nsIDOMNavigator* navigator)
  : mNavigator(navigator),
    mPluginMimeTypeCount(0),
    mInited(PR_FALSE)
{
}

nsMimeTypeArray::~nsMimeTypeArray()
{
  Clear();
}


DOMCI_DATA(MimeTypeArray, nsMimeTypeArray)


NS_INTERFACE_MAP_BEGIN(nsMimeTypeArray)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMimeTypeArray)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MimeTypeArray)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsMimeTypeArray)
NS_IMPL_RELEASE(nsMimeTypeArray)


NS_IMETHODIMP
nsMimeTypeArray::GetLength(PRUint32* aLength)
{
  if (!mInited) {
    nsresult rv = GetMimeTypes();
    if (rv != NS_OK)
      return rv;
  }

  NS_ASSERTION(mPluginMimeTypeCount <= (PRUint32)mMimeTypeArray.Count(),
               "The number of total mimetypes should be equal to or higher "
               "than the number of plugin mimetypes.");
 
  *aLength = mPluginMimeTypeCount;
  return NS_OK;
}

nsIDOMMimeType*
nsMimeTypeArray::GetItemAt(PRUint32 aIndex, nsresult *aResult)
{
  if (!mInited) {
    *aResult = GetMimeTypes();
    if (*aResult != NS_OK)
      return nsnull;
  }

  NS_ASSERTION(mPluginMimeTypeCount <= (PRUint32)mMimeTypeArray.Count(),
               "The number of total mimetypes should be equal to or higher "
               "than the number of plugin mimetypes.");

  if (aIndex >= mPluginMimeTypeCount) {
    *aResult = NS_ERROR_FAILURE;

    return nsnull;
  }

  *aResult = NS_OK;

  return mMimeTypeArray[aIndex];
}

NS_IMETHODIMP
nsMimeTypeArray::Item(PRUint32 aIndex, nsIDOMMimeType** aReturn)
{
  nsresult rv;

  NS_IF_ADDREF(*aReturn = GetItemAt(aIndex, &rv));

  return rv;
}

nsIDOMMimeType*
nsMimeTypeArray::GetNamedItem(const nsAString& aName, nsresult* aResult)
{
  if (!mInited) {
    *aResult = GetMimeTypes();
    if (*aResult != NS_OK)
      return nsnull;
  }

  NS_ASSERTION(mPluginMimeTypeCount <= (PRUint32)mMimeTypeArray.Count(),
               "The number of total mimetypes should be equal to or higher "
               "than the number of plugin mimetypes.");

  *aResult = NS_OK;

  nsAutoString type;

  for (PRInt32 i = 0; i < mMimeTypeArray.Count(); i++) {
    nsIDOMMimeType *mtype = mMimeTypeArray[i];

    mtype->GetType(type);

    if (type.Equals(aName)) {
      return mtype;
    }
  }

  
  nsCOMPtr<nsIMIMEService> mimeSrv = do_GetService("@mozilla.org/mime;1");
  if (mimeSrv) {
    nsCOMPtr<nsIMIMEInfo> mimeInfo;
    mimeSrv->GetFromTypeAndExtension(NS_ConvertUTF16toUTF8(aName), EmptyCString(),
                                     getter_AddRefs(mimeInfo));
    if (mimeInfo) {
      
      nsHandlerInfoAction action = nsIHandlerInfo::saveToDisk;
      mimeInfo->GetPreferredAction(&action);
      if (action != nsIMIMEInfo::handleInternally) {
        bool hasHelper = false;
        mimeInfo->GetHasDefaultHandler(&hasHelper);
        if (!hasHelper) {
          nsCOMPtr<nsIHandlerApp> helper;
          mimeInfo->GetPreferredApplicationHandler(getter_AddRefs(helper));
          if (!helper) {
            
            
            nsAutoString defaultDescription;
            mimeInfo->GetDefaultDescription(defaultDescription);
            if (defaultDescription.IsEmpty()) {
              
              return nsnull;
            }
          }
        }
      }

      
      nsCOMPtr<nsIDOMMimeType> helper, entry;
      if (!(helper = new nsHelperMimeType(aName)) ||
          !(entry = new nsMimeType(nsnull, helper)) ||
          !mMimeTypeArray.AppendObject(entry)) {
        *aResult = NS_ERROR_OUT_OF_MEMORY;

        return nsnull;
      }

      return entry;
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsMimeTypeArray::NamedItem(const nsAString& aName, nsIDOMMimeType** aReturn)
{
  nsresult rv;

  NS_IF_ADDREF(*aReturn = GetNamedItem(aName, &rv));

  return rv;
}

void  nsMimeTypeArray::Clear()
{
  mInited = PR_FALSE;
  mMimeTypeArray.Clear();
  mPluginMimeTypeCount = 0;
}

nsresult nsMimeTypeArray::Refresh()
{
  Clear();
  return GetMimeTypes();
}

nsresult nsMimeTypeArray::GetMimeTypes()
{
  NS_PRECONDITION(!mInited && mPluginMimeTypeCount==0,
                      "already initialized");

  if (!mNavigator) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsIDOMPluginArray* pluginArray = nsnull;
  nsresult rv = mNavigator->GetPlugins(&pluginArray);
  if (rv == NS_OK) {
    
    
    PRUint32 pluginMimeTypeCount = 0;
    PRUint32 pluginCount = 0;
    rv = pluginArray->GetLength(&pluginCount);
    if (rv == NS_OK) {
      PRUint32 i;
      for (i = 0; i < pluginCount; i++) {
        nsCOMPtr<nsIDOMPlugin> plugin;
        if (NS_SUCCEEDED(pluginArray->Item(i, getter_AddRefs(plugin))) &&
            plugin) {
          PRUint32 mimeTypeCount = 0;
          if (plugin->GetLength(&mimeTypeCount) == NS_OK)
            pluginMimeTypeCount += mimeTypeCount;
        }
      }
      
      if (!mMimeTypeArray.SetCapacity(pluginMimeTypeCount))
        return NS_ERROR_OUT_OF_MEMORY;

      mPluginMimeTypeCount = pluginMimeTypeCount;
      mInited = PR_TRUE;

      PRUint32 k;
      for (k = 0; k < pluginCount; k++) {
        nsCOMPtr<nsIDOMPlugin> plugin;
        if (NS_SUCCEEDED(pluginArray->Item(k, getter_AddRefs(plugin))) &&
            plugin) {
          PRUint32 mimeTypeCount = 0;
          if (plugin->GetLength(&mimeTypeCount) == NS_OK) {
            nsCOMPtr<nsIDOMMimeType> item;
            for (PRUint32 j = 0; j < mimeTypeCount; j++) {
              plugin->Item(j, getter_AddRefs(item));
              mMimeTypeArray.AppendObject(item);
            }
          }
        }
      }
    }
    NS_RELEASE(pluginArray);
  }
  return rv;
}

nsMimeType::nsMimeType(nsIDOMPlugin* aPlugin, nsIDOMMimeType* aMimeType)
{
  mPlugin = aPlugin;
  mMimeType = aMimeType;
}

nsMimeType::~nsMimeType()
{
}


DOMCI_DATA(MimeType, nsMimeType)


NS_INTERFACE_MAP_BEGIN(nsMimeType)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMimeType)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MimeType)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsMimeType)
NS_IMPL_RELEASE(nsMimeType)


NS_IMETHODIMP
nsMimeType::GetDescription(nsAString& aDescription)
{
  return mMimeType->GetDescription(aDescription);
}

NS_IMETHODIMP
nsMimeType::GetEnabledPlugin(nsIDOMPlugin** aEnabledPlugin)
{
  nsAutoString type;
  GetType(type);

  *aEnabledPlugin = mPlugin;

  NS_IF_ADDREF(*aEnabledPlugin);

  return NS_OK;
}

NS_IMETHODIMP
nsMimeType::GetSuffixes(nsAString& aSuffixes)
{
  return mMimeType->GetSuffixes(aSuffixes);
}

NS_IMETHODIMP
nsMimeType::GetType(nsAString& aType)
{
  return mMimeType->GetType(aType);
}


NS_IMPL_ISUPPORTS1(nsHelperMimeType, nsIDOMMimeType)

NS_IMETHODIMP
nsHelperMimeType::GetDescription(nsAString& aDescription)
{
  aDescription.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsHelperMimeType::GetEnabledPlugin(nsIDOMPlugin** aEnabledPlugin)
{
  *aEnabledPlugin = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsHelperMimeType::GetSuffixes(nsAString& aSuffixes)
{
  aSuffixes.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsHelperMimeType::GetType(nsAString& aType)
{
  aType = mType;
  return NS_OK;
}

