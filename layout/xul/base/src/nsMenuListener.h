



































#ifndef nsMenuListener_h__
#define nsMenuListener_h__

#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMEventReceiver.h"

class nsIMenuParent;
class nsPresContext;



class nsMenuListener : public nsIDOMKeyListener, public nsIDOMFocusListener, public nsIDOMMouseListener
{
public:
  nsMenuListener(nsIMenuParent* aMenuParent);
  
  virtual ~nsMenuListener();
   
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);
  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
  
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);
  
  NS_DECL_ISUPPORTS

protected:
  nsIMenuParent* mMenuParent;    
};


#endif
