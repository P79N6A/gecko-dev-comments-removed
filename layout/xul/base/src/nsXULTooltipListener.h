




































#ifndef nsXULTooltipListener_h__
#define nsXULTooltipListener_h__

#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMXULListener.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#ifdef MOZ_XUL
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#endif

class nsXULTooltipListener : public nsIDOMMouseListener,
                             public nsIDOMMouseMotionListener,
                             public nsIDOMKeyListener,
                             public nsIDOMXULListener
{
public:

  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

  
  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent) { return NS_OK; };
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);

  
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent) { return NS_OK; };
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent) { return NS_OK; };

  
  NS_IMETHOD PopupShowing(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD PopupShown(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD PopupHiding(nsIDOMEvent* aEvent);
  NS_IMETHOD PopupHidden(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD Close(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD Command(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD Broadcast(nsIDOMEvent* aEvent) { return NS_OK; };
  NS_IMETHOD CommandUpdate(nsIDOMEvent* aEvent) { return NS_OK; };

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  nsresult AddTooltipSupport(nsIContent* aNode);
  nsresult RemoveTooltipSupport(nsIContent* aNode);
  static nsXULTooltipListener* GetInstance() {
    if (!mInstance)
      NS_IF_ADDREF(mInstance = new nsXULTooltipListener());
    return mInstance;
  }
  static void ReleaseInstance() { NS_IF_RELEASE(mInstance); }

protected:

  nsXULTooltipListener();
  ~nsXULTooltipListener();

  
  static int sTooltipPrefChanged (const char* aPref, void* aData);
  static PRBool sShowTooltips;
  static PRUint32 sTooltipListenerCount;

  void KillTooltipTimer();
  void CreateAutoHideTimer();

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

  nsCOMPtr<nsIContent> mSourceNode;
  nsCOMPtr<nsIDOMNode> mTargetNode;
  nsCOMPtr<nsIContent> mCurrentTooltip;

  
  nsCOMPtr<nsITimer> mTooltipTimer;
  static void sTooltipCallback (nsITimer* aTimer, void* aListener);
  PRInt32 mMouseClientX, mMouseClientY;

  
  nsCOMPtr<nsITimer> mAutoHideTimer;
  static void sAutoHideCallback (nsITimer* aTimer, void* aListener);
  
  
  enum {
    kTooltipAutoHideTime = 5000,       
    kTooltipShowTime = 500             
  };

#ifdef MOZ_XUL
  
  PRBool mIsSourceTree;
  PRBool mNeedTitletip;
  PRInt32 mLastTreeRow;
  nsCOMPtr<nsITreeColumn> mLastTreeCol;
#endif
};

#endif 
