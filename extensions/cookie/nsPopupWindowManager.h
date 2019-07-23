




































#ifndef nsPopupWindowManager_h__
#define nsPopupWindowManager_h__

#include "nsCOMPtr.h"

#include "nsIObserver.h"
#include "nsIPermissionManager.h"
#include "nsIPopupWindowManager.h"
#include "nsWeakReference.h"

class nsIURI;

class nsPopupWindowManager : public nsIPopupWindowManager,
                             public nsIObserver,
                             public nsSupportsWeakReference {

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPOPUPWINDOWMANAGER
  NS_DECL_NSIOBSERVER

  nsPopupWindowManager();
  virtual ~nsPopupWindowManager();
  nsresult Init();

private:
  PRUint32                       mPolicy;
  nsCOMPtr<nsIPermissionManager> mPermissionManager;
};


#define NS_POPUPWINDOWMANAGER_CID \
 {0x822bcd11, 0x6432, 0x48be, {0x9e, 0x9d, 0x36, 0xf7, 0x80, 0x4b, 0x77, 0x47}}

#endif 
