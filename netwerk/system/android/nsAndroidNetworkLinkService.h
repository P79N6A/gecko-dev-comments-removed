





































#ifndef NSANDROIDNETWORKLINKSERVICE_H_
#define NSANDROIDNETWORKLINKSERVICE_H_

#include "nsINetworkLinkService.h"

class nsAndroidNetworkLinkService: public nsINetworkLinkService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETWORKLINKSERVICE

  nsAndroidNetworkLinkService();
  virtual ~nsAndroidNetworkLinkService();
};

#endif 
