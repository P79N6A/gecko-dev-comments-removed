




#ifndef nsSocketProviderService_h__
#define nsSocketProviderService_h__

#include "nsISocketProviderService.h"

class nsSocketProviderService : public nsISocketProviderService
{
  virtual ~nsSocketProviderService() {}

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISOCKETPROVIDERSERVICE

  nsSocketProviderService() {}

  static nsresult Create(nsISupports *, REFNSIID aIID, void **aResult);
};

#endif 
