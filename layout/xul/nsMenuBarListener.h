



#ifndef nsMenuBarListener_h__
#define nsMenuBarListener_h__

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsIDOMEventListener.h"


#ifdef KeyPress
#undef KeyPress
#endif

class nsMenuBarFrame;
class nsIDOMKeyEvent;



class nsMenuBarListener MOZ_FINAL : public nsIDOMEventListener
{
public:
  

  explicit nsMenuBarListener(nsMenuBarFrame* aMenuBar);

  static void InitializeStatics();
   
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) MOZ_OVERRIDE;
  
  nsresult KeyUp(nsIDOMEvent* aMouseEvent);
  nsresult KeyDown(nsIDOMEvent* aMouseEvent);
  nsresult KeyPress(nsIDOMEvent* aMouseEvent);
  nsresult Blur(nsIDOMEvent* aEvent);
  nsresult MouseDown(nsIDOMEvent* aMouseEvent);

  static nsresult GetMenuAccessKey(int32_t* aAccessKey);
  
  NS_DECL_ISUPPORTS

  static bool IsAccessKeyPressed(nsIDOMKeyEvent* event);

protected:
  

  virtual ~nsMenuBarListener();

  static void InitAccessKey();

  static mozilla::Modifiers GetModifiersForAccessKey(nsIDOMKeyEvent* event);

  
  
  void ToggleMenuActiveState();

  nsMenuBarFrame* mMenuBarFrame; 
  
  bool mAccessKeyDown;
  
  bool mAccessKeyDownCanceled;
  static bool mAccessKeyFocuses; 
  static int32_t mAccessKey;     
  static mozilla::Modifiers mAccessKeyMask;
};


#endif
