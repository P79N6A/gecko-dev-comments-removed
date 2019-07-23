


































#ifndef nsContentBlocker_h__
#define nsContentBlocker_h__

#include "nsIContentPolicy.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIPermissionManager.h"
#include "nsIPrefBranch2.h"

class nsIPrefBranch;




#define NUMBER_OF_TYPES 8

class nsContentBlocker : public nsIContentPolicy,
                         public nsIObserver,
                         public nsSupportsWeakReference
{
public:

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPOLICY
  NS_DECL_NSIOBSERVER

  nsContentBlocker();
  nsresult Init();

private:
  ~nsContentBlocker() {}

  void PrefChanged(nsIPrefBranch *, const char *);
  nsresult TestPermission(nsIURI *aCurrentURI,
                          nsIURI *aFirstURI,
                          PRInt32 aContentType,
                          PRBool *aPermission,
                          PRBool *aFromPrefs);

  nsCOMPtr<nsIPermissionManager> mPermissionManager;
  nsCOMPtr<nsIPrefBranch2> mPrefBranchInternal;
  PRUint8 mBehaviorPref[NUMBER_OF_TYPES];
};

#define NS_CONTENTBLOCKER_CID \
{ 0x4ca6b67b, 0x5cc7, 0x4e71, \
  { 0xa9, 0x8a, 0x97, 0xaf, 0x1c, 0x13, 0x48, 0x62 } }

#define NS_CONTENTBLOCKER_CONTRACTID "@mozilla.org/permissions/contentblocker;1"

#endif 
