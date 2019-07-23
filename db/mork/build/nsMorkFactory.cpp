





































#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsMorkCID.h"
#include "nsIMdbFactoryFactory.h"
#include "mdb.h"

class nsMorkFactoryFactory : public nsIMdbFactoryFactory
{
public:
  nsMorkFactoryFactory();
  
  NS_DECL_ISUPPORTS 

  NS_IMETHOD GetMdbFactory(nsIMdbFactory **aFactory);

};

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMorkFactoryFactory)

static const nsModuleComponentInfo components[] =
{
    { "Mork Factory", 
      NS_MORK_CID, 
      NS_MORK_CONTRACTID,
      nsMorkFactoryFactoryConstructor 
    }
};

NS_IMPL_NSGETMODULE(nsMorkModule, components)




static nsIMdbFactory *gMDBFactory = nsnull;

NS_IMPL_ADDREF(nsMorkFactoryFactory)
NS_IMPL_RELEASE(nsMorkFactoryFactory)

NS_IMETHODIMP
nsMorkFactoryFactory::QueryInterface(REFNSIID iid, void** result)
{
  if (! result)
    return NS_ERROR_NULL_POINTER;
  
  *result = nsnull;
  if(iid.Equals(NS_GET_IID(nsIMdbFactoryFactory)) ||
    iid.Equals(NS_GET_IID(nsISupports))) {
    *result = NS_STATIC_CAST(nsIMdbFactoryFactory*, this);
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}



nsMorkFactoryFactory::nsMorkFactoryFactory()
{
}

NS_IMETHODIMP nsMorkFactoryFactory::GetMdbFactory(nsIMdbFactory **aFactory)
{
  if (!gMDBFactory)
    gMDBFactory = MakeMdbFactory();
  *aFactory = gMDBFactory;
  NS_IF_ADDREF(gMDBFactory);
  return (gMDBFactory) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

