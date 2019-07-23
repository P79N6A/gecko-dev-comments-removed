



































#include <nsIURIContentListener.h>
#include <nsIInterfaceRequestor.h>
#include <nsCOMPtr.h>

class XRemoteContentListener : public nsIURIContentListener,
                               public nsIInterfaceRequestor
{
 public:
  XRemoteContentListener();
  virtual ~XRemoteContentListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURICONTENTLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

 private:

  nsCOMPtr<nsISupports> mLoadCookie;
};
