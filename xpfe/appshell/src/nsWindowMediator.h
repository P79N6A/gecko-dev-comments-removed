




































#ifndef __nsWindowMediator_h
#define __nsWindowMediator_h

#include "nsCOMPtr.h"
#include "nsIWindowMediator.h"
#include "nsISupportsArray.h"
#include "nsVoidArray.h"
#include "nsXPIDLString.h"
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

class nsWindowMediator : public nsIWindowMediator
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

  NS_DECL_NSIWINDOWMEDIATOR
  
  
  NS_DECL_ISUPPORTS 

private:
  
  PRInt32 AddEnumerator( nsAppShellWindowEnumerator* inEnumerator );
  PRInt32 RemoveEnumerator( nsAppShellWindowEnumerator* inEnumerator);
  nsWindowInfo *MostRecentWindowInfo(const PRUnichar* inType);

  NS_IMETHOD    UnregisterWindow(nsWindowInfo *inInfo);
  nsWindowInfo *GetInfoFor(nsIXULWindow *aWindow);
  nsWindowInfo *GetInfoFor(nsIWidget *aWindow);
  void          SortZOrderFrontToBack();
  void          SortZOrderBackToFront();

  nsVoidArray   mEnumeratorList;
  nsWindowInfo *mOldestWindow,
               *mTopmostWindow;
  PRInt32       mTimeStamp;
  PRBool        mSortingZOrder;
  PRLock       *mListLock;
  nsCOMPtr<nsISupportsArray> mListeners;

  static PRInt32 gRefCnt;
};

#endif
