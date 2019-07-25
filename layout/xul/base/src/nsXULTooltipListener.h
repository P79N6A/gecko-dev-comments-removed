




































#ifndef nsXULTooltipListener_h__
#define nsXULTooltipListener_h__

#include "nsIDOMEventListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#ifdef MOZ_XUL
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#endif
#include "nsWeakPtr.h"

class nsXULTooltipListener : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  void MouseOut(nsIDOMEvent* aEvent);
  void MouseMove(nsIDOMEvent* aEvent);

  nsresult AddTooltipSupport(nsIContent* aNode);
  nsresult RemoveTooltipSupport(nsIContent* aNode);
  static nsXULTooltipListener* GetInstance() {
    if (!mInstance)
      mInstance = new nsXULTooltipListener();
    return mInstance;
  }
  static void ClearTooltipCache() { mInstance = nsnull; }

protected:

  nsXULTooltipListener();
  ~nsXULTooltipListener();

  
  static bool sShowTooltips;
  static PRUint32 sTooltipListenerCount;

  void KillTooltipTimer();

#ifdef MOZ_XUL
  void CheckTreeBodyMove(nsIDOMMouseEvent* aMouseEvent);
  nsresult GetSourceTreeBoxObject(nsITreeBoxObject** aBoxObject);
#endif

  nsresult ShowTooltip();
  void LaunchTooltip();
  nsresult HideTooltip();
  nsresult DestroyTooltip();
  
  nsresult FindTooltip(nsIContent* aTarget, nsIContent** aTooltip);
  
  
  nsresult GetTooltipFor(nsIContent* aTarget, nsIContent** aTooltip);

  static nsXULTooltipListener* mInstance;
  static int ToolbarTipsPrefChanged(const char *aPref, void *aClosure);

  nsWeakPtr mSourceNode;
  nsWeakPtr mTargetNode;
  nsWeakPtr mCurrentTooltip;

  
  nsCOMPtr<nsITimer> mTooltipTimer;
  static void sTooltipCallback (nsITimer* aTimer, void* aListener);

  
  
  PRInt32 mMouseScreenX, mMouseScreenY;

  
  enum {
    kTooltipMouseMoveTolerance = 7,    
    kTooltipShowTime = 500             
  };

  
  
  
  bool mTooltipShownOnce;

#ifdef MOZ_XUL
  
  bool mIsSourceTree;
  bool mNeedTitletip;
  PRInt32 mLastTreeRow;
  nsCOMPtr<nsITreeColumn> mLastTreeCol;
#endif
};

#endif 
