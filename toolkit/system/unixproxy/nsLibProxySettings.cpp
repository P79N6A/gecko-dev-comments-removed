




































#include "nsISystemProxySettings.h"
#include "mozilla/ModuleUtils.h"
#include "nsIServiceManager.h"
#include "nsIIOService.h"
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

  nsUnixSystemProxySettings() { mProxyFactory = nsnull; }
  nsresult Init();

private:
  ~nsUnixSystemProxySettings() {
    if (mProxyFactory) 
      px_proxy_factory_free(mProxyFactory); 
  }

  pxProxyFactory *mProxyFactory;
};

NS_IMPL_ISUPPORTS1(nsUnixSystemProxySettings, nsISystemProxySettings)

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
nsUnixSystemProxySettings::GetProxyForURI(nsIURI* aURI, nsACString& aResult)
{
  nsresult rv;

  if (!mProxyFactory) {
    mProxyFactory = px_proxy_factory_new();
  }
  NS_ENSURE_TRUE(mProxyFactory, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIIOService> ios = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  char **proxyArray = nsnull;
  proxyArray = px_proxy_factory_get_proxies(mProxyFactory, (char*)(spec.get()));
  NS_ENSURE_TRUE(proxyArray, NS_ERROR_NOT_AVAILABLE);

  
  
  
  
  
  
  
  int c = 0;
  while (proxyArray[c] != NULL) {
    if (!aResult.IsEmpty()) {
      aResult.AppendLiteral("; ");
    }

    bool isScheme = false;
    nsXPIDLCString schemeString;
    nsXPIDLCString hostPortString;
    nsCOMPtr<nsIURI> proxyURI;

    rv = ios->NewURI(nsDependentCString(proxyArray[c]),
                                        nsnull,
                                        nsnull,
                                        getter_AddRefs(proxyURI));
    if (NS_FAILED(rv)) {
      c++;
      continue;
    }

    proxyURI->GetScheme(schemeString);
    if (NS_SUCCEEDED(proxyURI->SchemeIs("http", &isScheme)) && isScheme) {
      schemeString.AssignLiteral("proxy");
    }
    aResult.Append(schemeString);
    if (NS_SUCCEEDED(proxyURI->SchemeIs("direct", &isScheme)) && !isScheme) {
      
      proxyURI->GetHostPort(hostPortString);
      aResult.AppendLiteral(" ");
      aResult.Append(hostPortString);
    }

    c++;
  }

#ifdef DEBUG
  printf("returned PAC proxy string: %s\n", PromiseFlatCString(aResult).get());
#endif

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

