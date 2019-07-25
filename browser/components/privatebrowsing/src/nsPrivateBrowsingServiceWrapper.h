



#include "nsCOMPtr.h"
#include "nsIPrivateBrowsingService.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"

class nsIJSContextStack;

class nsPrivateBrowsingServiceWrapper MOZ_FINAL : public nsIPrivateBrowsingService,
                                                  public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRIVATEBROWSINGSERVICE
  NS_DECL_NSIOBSERVER

  nsresult Init();

private:
  nsCOMPtr<nsIPrivateBrowsingService> mPBService;
};
