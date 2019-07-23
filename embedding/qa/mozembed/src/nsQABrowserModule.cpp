





































#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"

#include "nsQABrowserCID.h"
#include "nsQABrowserView.h"
#include "nsQABrowserUIGlue.h"
#include "nsQABrowserChrome.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsQABrowserView)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsQABrowserUIGlue)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsQABrowserChrome)





static NS_DEFINE_CID(kBrowserViewCID, NS_QABROWSERVIEW_CID);
static NS_DEFINE_CID(kBrowserUIGlueCID, NS_QABROWSERUIGLUE_CID);
static NS_DEFINE_CID(kBrowserChromeCID, NS_QABROWSERCHROME_CID);

static const nsModuleComponentInfo gQAEmbeddingModuleViewInfo[] =
{
   { "QA_BrowserView Component", NS_QABROWSERVIEW_CID, 
     NS_QABROWSERVIEW_CONTRACTID, nsQABrowserViewConstructor }
};

static const nsModuleComponentInfo gQAEmbeddingModuleUIInfo[] =
{
   { "QA_BrowserUIGlue Component", NS_QABROWSERUIGLUE_CID,
     NS_QABROWSERUIGLUE_CONTRACTID, nsQABrowserUIGlueConstructor }
};

static const nsModuleComponentInfo gQAEmbeddingModuleChromeInfo[] =
{
   { "QA_BrowserChrome Component", NS_QABROWSERCHROME_CID,
     NS_QABROWSERCHROME_CONTRACTID, nsQABrowserChromeConstructor }
};

nsresult
RegisterComponents()
{

  nsresult rv=NS_OK;
  nsIGenericFactory* fact;

  
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  if (NS_FAILED(rv))
    return rv;

  
  rv = NS_NewGenericFactory(&fact, gQAEmbeddingModuleViewInfo);
  rv = registrar->RegisterFactory(kBrowserViewCID,
                                  "QA Embedding BrowserView",
                                  NS_QABROWSERVIEW_CONTRACTID,
                                  fact);
  NS_RELEASE(fact);

  
  rv = NS_NewGenericFactory(&fact, gQAEmbeddingModuleUIInfo);

  rv = registrar->RegisterFactory(kBrowserUIGlueCID,
                                  "QA Embedding BrowserUIGlue",
                                  NS_QABROWSERUIGLUE_CONTRACTID,
                                  fact);
  NS_RELEASE(fact);

  
  rv = NS_NewGenericFactory(&fact, gQAEmbeddingModuleChromeInfo);

  rv = registrar->RegisterFactory(kBrowserChromeCID,
                                  "QA Embedding BrowserChrome",
                                  NS_QABROWSERCHROME_CONTRACTID,
                                  fact);
  NS_RELEASE(fact);
  return rv;

}




#if 0
nsresult
NSRegisterSelf(nsISupports* aServMgr, const char* aPath)
{
  nsresult rv;
  nsCOMPtr<nsIComponentManagerObsolete> compMgr =
           do_GetService(kComponentManagerCID, aServMgr, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->RegisterComponent(kBrowserViewCID,
                                  "QA Embedding BrowserView",
                                  NS_QABROWSERVIEW_CONTRACTID,
                                  aPath, PR_TRUE, PR_TRUE);
  rv = compMgr->RegisterComponent(kBrowserUIGlueCID,
                                  "QA Embedding BrowserUIGlue",
                                  NS_QABROWSERUIGLUE_CONTRACTID,
                                  aPath, PR_TRUE, PR_TRUE);

  return rv;

}

nsresult
NSUnregisterSelf(nsISupports* aServMgr, const char* aPath)
{
  nsresult rv;
  nsCOMPtr<nsIComponentManagerObsolete> compMgr =
         do_GetService(kComponentManagerCID, aServMgr, &rv);
  if (NS_FAILED(rv)) return rv;

  

  rv = compMgr->UnregisterComponent(kBrowserViewCID, aPath);
  rv = compMgr->UnregisterComponent(kBrowserUIGlueCID, aPath);
                            
  return rv;
}
#endif  






#if 0

NS_IMPL_NSGETMODULE(Mozilla_Embedding_Component, gQAEmbeddingModuleInfo)

nsresult
NSGetFactory(nsISupports* aServMgr,
             const nsCID &aClass,
             const char* aClassName,
             const char* aContractID,
             nsIFactory **aFactory)
{
  nsresult rv=NS_OK;
  nsIGenericFactory* fact;
 
    rv = NS_NewGenericFactory(&fact, gQAEmbeddingModuleInfo);
 
 
 

  if (NS_SUCCEEDED(rv))
    *aFactory = fact;

#ifdef DEBUG_radha
  printf("nsQABrowserComponent NSGetFactory!\n");
#endif
  return rv;
}

#endif  
