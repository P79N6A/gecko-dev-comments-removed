



































#include "nsCOMPtr.h"
#include "nsIPrivateBrowsingService.h"
#include "nsIObserver.h"

class nsIJSContextStack;

class nsPrivateBrowsingServiceWrapper : public nsIPrivateBrowsingService,
                                        public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRIVATEBROWSINGSERVICE
  NS_DECL_NSIOBSERVER

  nsresult Init();

private:
  nsresult PrepareCall(nsIJSContextStack ** aJSStack);
  void FinishCall(nsIJSContextStack * aJSStack);

  nsCOMPtr<nsIPrivateBrowsingService> mPBService;
};
