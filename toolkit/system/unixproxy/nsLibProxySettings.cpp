




#include "nsISystemProxySettings.h"
#include "mozilla/ModuleUtils.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"
#include "nspr.h"

extern "C" {
#include <proxy.h>
}

class nsUnixSystemProxySettings : public nsISystemProxySettings {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMPROXYSETTINGS

  nsUnixSystemProxySettings() { mProxyFactory = nullptr; }
  nsresult Init();

private:
  ~nsUnixSystemProxySettings() {
    if (mProxyFactory) 
      px_proxy_factory_free(mProxyFactory); 
  }

  pxProxyFactory *mProxyFactory;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUnixSystemProxySettings, nsISystemProxySettings)

NS_IMETHODIMP
nsUnixSystemProxySettings::GetMainThreadOnly(bool *aMainThreadOnly)
{
  *aMainThreadOnly = false;
  return NS_OK;
}

nsresult
nsUnixSystemProxySettings::Init()
{
  return NS_OK;
}

nsresult
nsUnixSystemProxySettings::GetPACURI(nsACString& aResult)
{
  
  aResult.Truncate();
  return NS_OK;
}

nsresult
nsUnixSystemProxySettings::GetProxyForURI(const nsACString & aSpec,
                                          const nsACString & aScheme,
                                          const nsACString & aHost,
                                          const int32_t      aPort,
                                          nsACString & aResult)
{
  nsresult rv;

  if (!mProxyFactory) {
    mProxyFactory = px_proxy_factory_new();
  }
  NS_ENSURE_TRUE(mProxyFactory, NS_ERROR_NOT_AVAILABLE);

  char **proxyArray = nullptr;
  proxyArray = px_proxy_factory_get_proxies(mProxyFactory,
                                            PromiseFlatCString(aSpec).get());
  NS_ENSURE_TRUE(proxyArray, NS_ERROR_NOT_AVAILABLE);

  
  
  
  
  
  
  
  

  int c = 0;
  while (proxyArray[c] != NULL) {
    if (!aResult.IsEmpty()) {
      aResult.AppendLiteral("; ");
    }

    
    
    char *colon = strchr (proxyArray[c], ':');
    uint32_t schemelen = colon ? colon - proxyArray[c] : 0;
    if (schemelen < 1) {
      c++;
      continue;
    }

    if (schemelen == 6 && !strncasecmp(proxyArray[c], "direct", 6)) {
      aResult.AppendLiteral("DIRECT");
    }
    else {
      aResult.AppendLiteral("PROXY ");
      aResult.Append(proxyArray[c]);
    }

    c++;
  }

  PR_Free(proxyArray);
  return NS_OK;
}

#define NS_UNIXSYSTEMPROXYSERVICE_CID\
     { 0x0fa3158c, 0xd5a7, 0x43de, \
       {0x91, 0x81, 0xa2, 0x85, 0xe7, 0x4c, 0xf1, 0xd4 } }

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUnixSystemProxySettings, Init)
NS_DEFINE_NAMED_CID(NS_UNIXSYSTEMPROXYSERVICE_CID);

static const mozilla::Module::CIDEntry kUnixProxyCIDs[] = {
  { &kNS_UNIXSYSTEMPROXYSERVICE_CID, false, NULL, nsUnixSystemProxySettingsConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kUnixProxyContracts[] = {
  { NS_SYSTEMPROXYSETTINGS_CONTRACTID, &kNS_UNIXSYSTEMPROXYSERVICE_CID },
  { NULL }
};

static const mozilla::Module kUnixProxyModule = {
  mozilla::Module::kVersion,
  kUnixProxyCIDs,
  kUnixProxyContracts
};
        
NSMODULE_DEFN(nsUnixProxyModule) = &kUnixProxyModule;

