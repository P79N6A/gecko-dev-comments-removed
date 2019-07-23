




































#ifndef nsIMdbFactoryFactory_h__
#define nsIMdbFactoryFactory_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"

class nsIMdbFactory;


#define NS_IMDBFACTORYFACTORY_IID          \
{ 0x2794d0b7, 0xe740, 0x47a4, { 0x91, 0xc0, 0x3e, 0x4f, 0xcb, 0x95, 0xb8, 0x6 } }



class nsIMdbFactoryService : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBFACTORYFACTORY_IID)
  NS_IMETHOD GetMdbFactory(nsIMdbFactory **aFactory) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbFactoryService, NS_IMDBFACTORYFACTORY_IID)

#endif
