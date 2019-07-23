





































#ifndef nsClipboardPrivacyHandler_h__
#define nsClipboardPrivacyHandler_h__

#include "nsIObserver.h"
#include "nsIPrivateBrowsingService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"

class nsITransferable;








class nsClipboardPrivacyHandler : public nsIObserver,
                                  public nsSupportsWeakReference
{

public:
  nsClipboardPrivacyHandler();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  nsresult PrepareDataForClipboard(nsITransferable * aTransferable);

private:

  PRBool InPrivateBrowsing();

  nsCOMPtr<nsIPrivateBrowsingService> mPBService;

};

#endif 

