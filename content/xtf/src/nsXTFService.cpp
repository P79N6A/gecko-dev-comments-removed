





































#include "nsCOMPtr.h"
#include "nsINodeInfo.h"
#include "nsIServiceManager.h"
#include "nsIXTFElement.h"
#include "nsIXTFElementFactory.h"
#include "nsIXTFService.h"
#include "nsInterfaceHashtable.h"
#include "nsString.h"
#include "nsXTFElementWrapper.h"




class nsXTFService : public nsIXTFService
{
protected:
  friend nsresult NS_NewXTFService(nsIXTFService** aResult);
  
  nsXTFService();

public:
  
  NS_DECL_ISUPPORTS

  
  nsresult CreateElement(nsIContent** aResult, nsINodeInfo* aNodeInfo);

private:
  nsInterfaceHashtable<nsUint32HashKey, nsIXTFElementFactory> mFactoryHash;
};




nsXTFService::nsXTFService()
{
  mFactoryHash.Init(); 
}

nsresult
NS_NewXTFService(nsIXTFService** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  nsXTFService* result = new nsXTFService();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsXTFService, nsIXTFService)




nsresult
nsXTFService::CreateElement(nsIContent** aResult, nsINodeInfo* aNodeInfo)
{
  nsCOMPtr<nsIXTFElementFactory> factory;

  
  if (!mFactoryHash.Get(aNodeInfo->NamespaceID(), getter_AddRefs(factory))) {
    
    nsCAutoString xtf_contract_id(NS_XTF_ELEMENT_FACTORY_CONTRACTID_PREFIX);
    nsAutoString uri;
    aNodeInfo->GetNamespaceURI(uri);
    AppendUTF16toUTF8(uri, xtf_contract_id);
#ifdef DEBUG_xtf_verbose
    printf("Testing for XTF factory at %s\n", xtf_contract_id.get());
#endif
    factory = do_GetService(xtf_contract_id.get());
    if (factory) {
#ifdef DEBUG
      printf("We've got an XTF factory: %s \n", xtf_contract_id.get());
#endif
      
      mFactoryHash.Put(aNodeInfo->NamespaceID(), factory);
    }
  }
  if (!factory) return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIXTFElement> elem;
  nsAutoString tagName;
  aNodeInfo->GetName(tagName);
  factory->CreateElement(tagName, getter_AddRefs(elem));
  if (!elem) return NS_ERROR_FAILURE;
  
  
  return NS_NewXTFElementWrapper(elem, aNodeInfo, aResult);
}

