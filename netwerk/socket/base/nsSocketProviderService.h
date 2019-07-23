




































#ifndef nsSocketProviderService_h__
#define nsSocketProviderService_h__

#include "nsISocketProviderService.h"

class nsSocketProviderService : public nsISocketProviderService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISOCKETPROVIDERSERVICE

  nsSocketProviderService() {}
  virtual ~nsSocketProviderService() {}

  static NS_METHOD Create(nsISupports *, REFNSIID aIID, void **aResult);
};

#endif 
