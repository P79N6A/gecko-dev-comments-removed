




































#ifndef nsMenuBarListener_h__
#define nsMenuBarListener_h__

#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"


#ifdef KeyPress
#undef KeyPress
#endif

class nsMenuBarFrame;
class nsIDOMKeyEvent;



class nsMenuBarListener : public nsIDOMEventListener
{
public:
  

  nsMenuBarListener(nsMenuBarFrame* aMenuBar);
  

  virtual ~nsMenuBarListener();
   
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  
  nsresult KeyUp(nsIDOMEvent* aMouseEvent);
  nsresult KeyDown(nsIDOMEvent* aMouseEvent);
  nsresult KeyPress(nsIDOMEvent* aMouseEvent);
  nsresult Blur(nsIDOMEvent* aEvent);
  nsresult MouseDown(nsIDOMEvent* aMouseEvent);

  static nsresult GetMenuAccessKey(PRInt32* aAccessKey);
  
  NS_DECL_ISUPPORTS

  static bool IsAccessKeyPressed(nsIDOMKeyEvent* event);

protected:
  static void InitAccessKey();

  static PRUint32 GetModifiers(nsIDOMKeyEvent* event);

  
  
  void ToggleMenuActiveState();

  nsMenuBarFrame* mMenuBarFrame; 
  
  bool mAccessKeyDown;
  
  bool mAccessKeyDownCanceled;
  static bool mAccessKeyFocuses; 
  static PRInt32 mAccessKey;     
  static PRUint32 mAccessKeyMask;
};


#endif
