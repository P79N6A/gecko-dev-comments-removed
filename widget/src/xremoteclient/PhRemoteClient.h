





































#include "nsIXRemoteClient.h"

class XRemoteClient : public nsIXRemoteClient
{
 public:
  XRemoteClient();
  virtual ~XRemoteClient();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIXREMOTECLIENT

 private:
	PRBool mInitialized;
};
