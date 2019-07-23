




































#ifndef nsIEventListener_h__
#define nsIEventListener_h__

#include "nsISupports.h"

class nsGUIEvent;







#define NS_IEVENTLISTENER_IID \
{ 0xc83f6b80, 0xd7ce, 0x11d2, { 0x83, 0x60, 0xc4, 0xc8, 0x94, 0xc4, 0x91, 0x7c } }

class nsIEventListener : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEVENTLISTENER_IID)
  
 





  virtual nsEventStatus ProcessEvent(const nsGUIEvent & anEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEventListener, NS_IEVENTLISTENER_IID)

#endif 
