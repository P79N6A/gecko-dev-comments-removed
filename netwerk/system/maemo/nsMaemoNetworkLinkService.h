
























#ifndef NSMAEMONETWORKLINKSERVICE_H_
#define NSMAEMONETWORKLINKSERVICE_H_

#include "nsINetworkLinkService.h"
#include "nsIObserver.h"

class nsMaemoNetworkLinkService: public nsINetworkLinkService,
                                 public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETWORKLINKSERVICE
  NS_DECL_NSIOBSERVER

  nsMaemoNetworkLinkService();
  virtual ~nsMaemoNetworkLinkService();

  nsresult Init();
  nsresult Shutdown();
};

#endif 
