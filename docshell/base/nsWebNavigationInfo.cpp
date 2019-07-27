




#include "nsWebNavigationInfo.h"
#include "nsIWebNavigation.h"
#include "nsServiceManagerUtils.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIPluginHost.h"
#include "nsIDocShell.h"
#include "nsContentUtils.h"
#include "imgLoader.h"

NS_IMPL_ISUPPORTS(nsWebNavigationInfo, nsIWebNavigationInfo)

#define CONTENT_DLF_CONTRACT "@mozilla.org/content/document-loader-factory;1"
#define PLUGIN_DLF_CONTRACT \
  "@mozilla.org/content/plugin/document-loader-factory;1"

nsresult
nsWebNavigationInfo::Init()
{
  nsresult rv;
  mCategoryManager = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsWebNavigationInfo::IsTypeSupported(const nsACString& aType,
                                     nsIWebNavigation* aWebNav,
                                     uint32_t* aIsTypeSupported)
{
  NS_PRECONDITION(aIsTypeSupported, "null out param?");

  
  
  

  
  
  *aIsTypeSupported = nsIWebNavigationInfo::UNSUPPORTED;

  const nsCString& flatType = PromiseFlatCString(aType);
  nsresult rv = IsTypeSupportedInternal(flatType, aIsTypeSupported);
  NS_ENSURE_SUCCESS(rv, rv);

  if (*aIsTypeSupported) {
    return rv;
  }

  
  
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aWebNav));
  bool allowed;
  if (docShell &&
      NS_SUCCEEDED(docShell->GetAllowPlugins(&allowed)) && !allowed) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIPluginHost> pluginHost =
    do_GetService(MOZ_PLUGIN_HOST_CONTRACTID);
  if (pluginHost) {
    
    
    rv = pluginHost->ReloadPlugins();
    if (NS_SUCCEEDED(rv)) {
      
      
      
      
      return IsTypeSupportedInternal(flatType, aIsTypeSupported);
    }
  }

  return NS_OK;
}

nsresult
nsWebNavigationInfo::IsTypeSupportedInternal(const nsCString& aType,
                                             uint32_t* aIsSupported)
{
  NS_PRECONDITION(aIsSupported, "Null out param?");

  nsContentUtils::ContentViewerType vtype = nsContentUtils::TYPE_UNSUPPORTED;

  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
    nsContentUtils::FindInternalContentViewer(aType.get(), &vtype);

  switch (vtype) {
    case nsContentUtils::TYPE_UNSUPPORTED:
      *aIsSupported = nsIWebNavigationInfo::UNSUPPORTED;
      break;

    case nsContentUtils::TYPE_PLUGIN:
      *aIsSupported = nsIWebNavigationInfo::PLUGIN;
      break;

    case nsContentUtils::TYPE_UNKNOWN:
      *aIsSupported = nsIWebNavigationInfo::OTHER;
      break;

    case nsContentUtils::TYPE_CONTENT:
      
      
      
      if (imgLoader::SupportImageWithMimeType(aType.get())) {
        *aIsSupported = nsIWebNavigationInfo::IMAGE;
      } else {
        *aIsSupported = nsIWebNavigationInfo::OTHER;
      }
      break;
  }

  return NS_OK;
}
