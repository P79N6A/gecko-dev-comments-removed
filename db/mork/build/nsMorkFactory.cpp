





































#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsMorkCID.h"
#include "nsIMdbFactoryFactory.h"
#include "mdb.h"

class nsMorkFactoryService : public nsIMdbFactoryService
{
public:
  nsMorkFactoryService() {};
  
  NS_DECL_ISUPPORTS 

  NS_IMETHOD GetMdbFactory(nsIMdbFactory **aFactory);

protected:
  nsCOMPtr<nsIMdbFactory> mMdbFactory;
};

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMorkFactoryService)

static const nsModuleComponentInfo components[] =
{
    { "Mork Factory Service", 
      NS_MORK_CID, 
      NS_MORK_CONTRACTID,
      nsMorkFactoryServiceConstructor 
    }
};

NS_IMPL_NSGETMODULE(nsMorkModule, components)

NS_IMPL_ISUPPORTS1(nsMorkFactoryService, nsIMdbFactoryService)

NS_IMETHODIMP nsMorkFactoryService::GetMdbFactory(nsIMdbFactory **aFactory)
{
  if (!mMdbFactory)
    mMdbFactory = MakeMdbFactory();
  NS_IF_ADDREF(*aFactory = mMdbFactory);
  return *aFactory ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
