




































#include "nsWebNavigationInfo.h"
#include "nsIWebNavigation.h"
#include "nsString.h"
#include "nsServiceManagerUtils.h"
#include "nsIContentUtils.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIPluginHost.h"

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
  
  
  nsCOMPtr<nsIPluginHost> pluginHost =
    do_GetService(MOZ_PLUGIN_HOST_CONTRACTID);
  if (pluginHost) {
    
    
    rv = pluginHost->ReloadPlugins(PR_FALSE);
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
  NS_PRECONDITION(aIsSupported, "Null out param?");


  nsCOMPtr<nsIContentUtils> cutils = do_GetService("@mozilla.org/content/contentutils;1");
  if (!cutils)
      return NS_ERROR_FAILURE;

  nsIContentUtils::ContentViewerType vtype = nsIContentUtils::TYPE_UNSUPPORTED;

  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
    cutils->FindInternalContentViewer(aType.get(), &vtype);
  
  switch (vtype) {
  case nsIContentUtils::TYPE_UNSUPPORTED:
    *aIsSupported = nsIWebNavigationInfo::UNSUPPORTED;
    break;

  case nsIContentUtils::TYPE_PLUGIN:
    *aIsSupported = nsIWebNavigationInfo::PLUGIN;
    break;

  case nsIContentUtils::TYPE_UNKNOWN:
    *aIsSupported = nsIWebNavigationInfo::OTHER;
    break;

  case nsIContentUtils::TYPE_CONTENT:
    PRBool isImage = PR_FALSE;
    mImgLoader->SupportImageWithMimeType(aType.get(), &isImage);
    if (isImage) {
      *aIsSupported = nsIWebNavigationInfo::IMAGE;
    }
    else {
      *aIsSupported = nsIWebNavigationInfo::OTHER;
    }
    break;
  }
  
  return NS_OK;
}
