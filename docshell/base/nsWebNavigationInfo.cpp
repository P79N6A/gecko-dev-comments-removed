




































#include "nsWebNavigationInfo.h"
#include "nsIWebNavigation.h"
#include "nsString.h"
#include "nsServiceManagerUtils.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIPluginManager.h"

NS_IMPL_ISUPPORTS1(nsWebNavigationInfo, nsIWebNavigationInfo)

#define CONTENT_DLF_CONTRACT "@mozilla.org/content/document-loader-factory;1"
#define PLUGIN_DLF_CONTRACT \
    "@mozilla.org/content/plugin/document-loader-factory;1"

nsresult
nsWebNavigationInfo::Init()
{
  nsresult rv;
  mCategoryManager = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mImgLoader = do_GetService("@mozilla.org/image/loader;1", &rv);

  return rv;
}

NS_IMETHODIMP
nsWebNavigationInfo::IsTypeSupported(const nsACString& aType,
                                     nsIWebNavigation* aWebNav,
                                     PRUint32* aIsTypeSupported)
{
  NS_PRECONDITION(aIsTypeSupported, "null out param?");

  
  
  
  
  
  
  *aIsTypeSupported = nsIWebNavigationInfo::UNSUPPORTED;

  const nsCString& flatType = PromiseFlatCString(aType);
  nsresult rv = IsTypeSupportedInternal(flatType, aIsTypeSupported);
  NS_ENSURE_SUCCESS(rv, rv);

  if (*aIsTypeSupported) {
    return rv;
  }
  
  
  nsCOMPtr<nsIPluginManager> pluginManager =
    do_GetService("@mozilla.org/plugin/manager;1");
  if (pluginManager) {
    
    
    rv = pluginManager->ReloadPlugins(PR_FALSE);
    if (NS_SUCCEEDED(rv)) {
      
      
      
      
      return IsTypeSupportedInternal(flatType, aIsTypeSupported);
    }
  }

  return NS_OK;
}

nsresult
nsWebNavigationInfo::IsTypeSupportedInternal(const nsCString& aType,
                                             PRUint32* aIsSupported)
{
  NS_PRECONDITION(mCategoryManager, "Must have category manager");
  NS_PRECONDITION(aIsSupported, "Null out param?");

  nsXPIDLCString value;
  nsresult rv = mCategoryManager->GetCategoryEntry("Gecko-Content-Viewers",
                                                   aType.get(),
                                                   getter_Copies(value));

  
  
  

  if (NS_FAILED(rv) && rv != NS_ERROR_NOT_AVAILABLE)
    return rv;

  
  
  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory;
  if (!value.IsEmpty()) {
    docLoaderFactory = do_GetService(value.get());
  }

  
  if (!docLoaderFactory) {
    *aIsSupported = nsIWebNavigationInfo::UNSUPPORTED;
  }
  else if (value.EqualsLiteral(CONTENT_DLF_CONTRACT)) {
    PRBool isImage = PR_FALSE;
    mImgLoader->SupportImageWithMimeType(aType.get(), &isImage);
    if (isImage) {
      *aIsSupported = nsIWebNavigationInfo::IMAGE;
    }
    else {
      *aIsSupported = nsIWebNavigationInfo::OTHER;
    }
  }
  else if (value.EqualsLiteral(PLUGIN_DLF_CONTRACT)) {
    *aIsSupported = nsIWebNavigationInfo::PLUGIN;
  }
  else {
    *aIsSupported = nsIWebNavigationInfo::OTHER;
  }
  
  return NS_OK;
}
