




#ifndef nsClipboardPrivacyHandler_h__
#define nsClipboardPrivacyHandler_h__

#include "nsIObserver.h"
#include "nsIPrivateBrowsingService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

class nsITransferable;








class nsClipboardPrivacyHandler MOZ_FINAL : public nsIObserver,
                                            public nsSupportsWeakReference
{

public:

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  nsresult Init();
  nsresult PrepareDataForClipboard(nsITransferable * aTransferable);

private:

  bool InPrivateBrowsing();

  nsCOMPtr<nsIPrivateBrowsingService> mPBService;

};

nsresult NS_NewClipboardPrivacyHandler(nsClipboardPrivacyHandler ** aHandler);

#endif 
