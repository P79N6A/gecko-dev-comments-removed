




































#ifndef NSQTNETWORKLINKSERVICE_H_
#define NSQTNETWORKLINKSERVICE_H_

#include "nsINetworkLinkService.h"
#include "nsIObserver.h"

class nsQtNetworkLinkService: public nsINetworkLinkService,
                              public nsIObserver
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETWORKLINKSERVICE
  NS_DECL_NSIOBSERVER

  nsQtNetworkLinkService();
  virtual ~nsQtNetworkLinkService();

  nsresult Init();
  nsresult Shutdown();

};

#endif 
