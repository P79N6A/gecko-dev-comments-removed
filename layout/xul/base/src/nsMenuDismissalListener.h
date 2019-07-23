




































#ifndef nsMenuDismissalListener_h__
#define nsMenuDismissalListener_h__

#include "nsIWidget.h"
#include "nsIDOMMouseListener.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIDOMEventTarget.h"
#include "nsCOMPtr.h"

class nsIMenuParent;








class nsMenuDismissalListener : public nsIDOMMouseListener,
                                public nsIMenuRollup,
                                public nsIRollupListener
{

public:
  friend class nsMenuPopupFrame;

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIROLLUPLISTENER
  NS_DECL_NSIMENUROLLUP

  void EnableListener(PRBool aEnabled);
  void SetCurrentMenuParent(nsIMenuParent* aMenuParent);
  nsIMenuParent* GetCurrentMenuParent();

  static nsMenuDismissalListener* GetInstance();
  static nsMenuDismissalListener* sInstance;
  static void Shutdown();

protected:
  nsMenuDismissalListener();
  ~nsMenuDismissalListener();

  



  void Register();
  
  void Unregister();

  nsIMenuParent* mMenuParent;
  nsCOMPtr<nsIWidget> mWidget;
  PRBool mEnabled;
};


#endif
