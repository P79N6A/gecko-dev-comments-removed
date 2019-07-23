




































#ifndef nsIMdbFactoryFactory_h__
#define nsIMdbFactoryFactory_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"

class nsIMdbFactory;


#define NS_IMDBFACTORYFACTORY_IID          \
{ 0xd5440650, 0x2807, 0x11d3, { 0x8d, 0x75, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0x17}}



class nsIMdbFactoryFactory : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBFACTORYFACTORY_IID)
	NS_IMETHOD GetMdbFactory(nsIMdbFactory **aFactory) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbFactoryFactory, NS_IMDBFACTORYFACTORY_IID)

#endif
