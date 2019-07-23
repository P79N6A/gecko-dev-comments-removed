




































#ifndef nsMenuBarListener_h__
#define nsMenuBarListener_h__

#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMEventTarget.h"

class nsMenuBarFrame;
class nsPresContext;
class nsIDOMKeyEvent;



class nsMenuBarListener : public nsIDOMKeyListener, public nsIDOMFocusListener, public nsIDOMMouseListener
{
public:
  

  nsMenuBarListener(nsMenuBarFrame* aMenuBar);
  

  virtual ~nsMenuBarListener();
   
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  
  NS_IMETHOD KeyUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD KeyDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aMouseEvent);
  
  NS_IMETHOD Focus(nsIDOMEvent* aEvent);
  NS_IMETHOD Blur(nsIDOMEvent* aEvent);
  
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

  static nsresult GetMenuAccessKey(PRInt32* aAccessKey);
  
  NS_DECL_ISUPPORTS

  static PRBool IsAccessKeyPressed(nsIDOMKeyEvent* event);

protected:
  static void InitAccessKey();

  static PRUint32 GetModifiers(nsIDOMKeyEvent* event);

  nsMenuBarFrame* mMenuBarFrame; 
  PRBool mAccessKeyDown;         
  static PRBool mAccessKeyFocuses; 
  static PRInt32 mAccessKey;     
  static PRUint32 mAccessKeyMask;
};


#endif
