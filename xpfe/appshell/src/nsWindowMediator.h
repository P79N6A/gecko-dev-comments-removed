




































#ifndef nsWindowMediator_h_
#define nsWindowMediator_h_

#include "nsCOMPtr.h"
#include "nsIWindowMediator.h"
#include "nsISupportsArray.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "nsXPIDLString.h"
#include "nsWeakReference.h"
#include "nsCRT.h"

class nsAppShellWindowEnumerator;
class nsASXULWindowEarlyToLateEnumerator;
class nsASDOMWindowEarlyToLateEnumerator;
class nsASDOMWindowFrontToBackEnumerator;
class nsASXULWindowFrontToBackEnumerator;
class nsASDOMWindowBackToFrontEnumerator;
class nsASXULWindowBackToFrontEnumerator;
struct nsWindowInfo;
struct PRLock;

class nsWindowMediator :
  public nsIWindowMediator,
  public nsIObserver,
  public nsSupportsWeakReference
{
friend class nsAppShellWindowEnumerator;
friend class nsASXULWindowEarlyToLateEnumerator;
friend class nsASDOMWindowEarlyToLateEnumerator;
friend class nsASDOMWindowFrontToBackEnumerator;
friend class nsASXULWindowFrontToBackEnumerator;
friend class nsASDOMWindowBackToFrontEnumerator;
friend class nsASXULWindowBackToFrontEnumerator;

public:
  nsWindowMediator();
  virtual ~nsWindowMediator();

  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINDOWMEDIATOR
  NS_DECL_NSIOBSERVER

private:
  PRInt32 AddEnumerator(nsAppShellWindowEnumerator* inEnumerator);
  PRInt32 RemoveEnumerator(nsAppShellWindowEnumerator* inEnumerator);
  nsWindowInfo *MostRecentWindowInfo(const PRUnichar* inType);

  nsresult      UnregisterWindow(nsWindowInfo *inInfo);
  nsWindowInfo *GetInfoFor(nsIXULWindow *aWindow);
  nsWindowInfo *GetInfoFor(nsIWidget *aWindow);
  void          SortZOrderFrontToBack();
  void          SortZOrderBackToFront();

  nsTArray<nsAppShellWindowEnumerator*> mEnumeratorList;
  nsWindowInfo *mOldestWindow;
  nsWindowInfo *mTopmostWindow;
  PRInt32       mTimeStamp;
  PRBool        mSortingZOrder;
  PRBool        mReady;
  PRLock       *mListLock;

  nsCOMPtr<nsISupportsArray> mListeners;
};

#endif
