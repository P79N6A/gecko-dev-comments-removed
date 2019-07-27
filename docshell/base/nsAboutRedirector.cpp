





#include "nsAboutRedirector.h"
#include "nsNetUtil.h"
#include "nsAboutProtocolUtils.h"
#include "mozilla/ArrayUtils.h"
#include "nsDOMString.h"

NS_IMPL_ISUPPORTS(nsAboutRedirector, nsIAboutModule)

struct RedirEntry
{
  const char* id;
  const char* url;
  uint32_t flags;
};











static RedirEntry kRedirMap[] = {
  {
    "", "chrome://global/content/about.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  { "about", "chrome://global/content/aboutAbout.xhtml", 0 },
  {
    "credits", "http://www.mozilla.org/credits/",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT
  },
  {
    "mozilla", "chrome://global/content/mozilla.xhtml",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT
  },
  {
    "plugins", "chrome://global/content/plugins.html",
    nsIAboutModule::URI_MUST_LOAD_IN_CHILD
  },
  { "config", "chrome://global/content/config.xul", 0 },
#ifdef MOZ_CRASHREPORTER
  { "crashes", "chrome://global/content/crashes.xhtml", 0 },
#endif
  {
    "logo", "chrome://branding/content/about.png",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT
  },
  {
    "buildconfig", "chrome://global/content/buildconfig.html",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT
  },
  {
    "license", "chrome://global/content/license.html",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT
  },
  {
    "neterror", "chrome://global/content/netError.xhtml",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
      nsIAboutModule::URI_CAN_LOAD_IN_CHILD |
      nsIAboutModule::ALLOW_SCRIPT |
      nsIAboutModule::HIDE_FROM_ABOUTABOUT
  },
  {
    "memory", "chrome://global/content/aboutMemory.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  {
    "performance", "chrome://global/content/aboutPerformance.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  {
    "addons", "chrome://mozapps/content/extensions/extensions.xul",
    nsIAboutModule::ALLOW_SCRIPT
  },
  {
    "newaddon", "chrome://mozapps/content/extensions/newaddon.xul",
    nsIAboutModule::ALLOW_SCRIPT |
      nsIAboutModule::HIDE_FROM_ABOUTABOUT
  },
  {
    "support", "chrome://global/content/aboutSupport.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  {
    "telemetry", "chrome://global/content/aboutTelemetry.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  {
    "networking", "chrome://global/content/aboutNetworking.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  {
    "webrtc", "chrome://global/content/aboutwebrtc/aboutWebrtc.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  {
    "serviceworkers", "chrome://global/content/aboutServiceWorkers.xhtml",
    nsIAboutModule::ALLOW_SCRIPT
  },
  
  
  {
    "srcdoc", "about:blank",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
      nsIAboutModule::HIDE_FROM_ABOUTABOUT
  }
};
static const int kRedirTotal = mozilla::ArrayLength(kRedirMap);

NS_IMETHODIMP
nsAboutRedirector::NewChannel(nsIURI* aURI,
                              nsILoadInfo* aLoadInfo,
                              nsIChannel** aResult)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ASSERTION(aResult, "must not be null");

  nsAutoCString path;
  nsresult rv = NS_GetAboutModuleName(aURI, path);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (int i = 0; i < kRedirTotal; i++) {
    if (!strcmp(path.get(), kRedirMap[i].id)) {
      nsCOMPtr<nsIChannel> tempChannel;
      nsCOMPtr<nsIURI> tempURI;
      rv = NS_NewURI(getter_AddRefs(tempURI), kRedirMap[i].url);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = NS_NewChannelInternal(getter_AddRefs(tempChannel),
                                 tempURI,
                                 aLoadInfo);
      if (NS_FAILED(rv)) {
        return rv;
      }

      tempChannel->SetOriginalURI(aURI);

      tempChannel.forget(aResult);
      return rv;
    }
  }

  NS_ERROR("nsAboutRedirector called for unknown case");
  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
nsAboutRedirector::GetURIFlags(nsIURI* aURI, uint32_t* aResult)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsAutoCString name;
  nsresult rv = NS_GetAboutModuleName(aURI, name);
  NS_ENSURE_SUCCESS(rv, rv);

  for (int i = 0; i < kRedirTotal; i++) {
    if (name.EqualsASCII(kRedirMap[i].id)) {
      *aResult = kRedirMap[i].flags;
      return NS_OK;
    }
  }

  NS_ERROR("nsAboutRedirector called for unknown case");
  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
nsAboutRedirector::GetIndexedDBOriginPostfix(nsIURI* aURI, nsAString& aResult)
{
  SetDOMStringToNull(aResult);
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsAboutRedirector::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  nsRefPtr<nsAboutRedirector> about = new nsAboutRedirector();
  return about->QueryInterface(aIID, aResult);
}
