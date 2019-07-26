




#ifndef nsXULTooltipListener_h__
#define nsXULTooltipListener_h__

#include "nsIDOMEventListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMElement.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#ifdef MOZ_XUL
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#endif
#include "nsWeakPtr.h"
#include "mozilla/Attributes.h"

class nsIContent;

class nsXULTooltipListener MOZ_FINAL : public nsIDOMEventListener
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
  static void ClearTooltipCache() { mInstance = nullptr; }

protected:

  nsXULTooltipListener();
  ~nsXULTooltipListener();

  
  static bool sShowTooltips;
  static uint32_t sTooltipListenerCount;

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

  
  
  int32_t mMouseScreenX, mMouseScreenY;

  
  enum {
    kTooltipMouseMoveTolerance = 7     
  };

  
  
  
  bool mTooltipShownOnce;

#ifdef MOZ_XUL
  
  bool mIsSourceTree;
  bool mNeedTitletip;
  int32_t mLastTreeRow;
  nsCOMPtr<nsITreeColumn> mLastTreeCol;
#endif
};

#endif 
