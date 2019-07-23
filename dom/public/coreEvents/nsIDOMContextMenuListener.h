





































#ifndef nsIDOMContextMenuListener_h__
#define nsIDOMContextMenuListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMCONTEXTMENULISTENER_IID \
{ /* 162b3480-ded6-11d1-bd85-00805f8ae3f7 */ \
0x162b3480, 0xded6, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf7} }

class nsIDOMContextMenuListener : public nsIDOMEventListener {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMCONTEXTMENULISTENER_IID)
  




  NS_IMETHOD ContextMenu(nsIDOMEvent* aContextMenuEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMContextMenuListener,
                              NS_IDOMCONTEXTMENULISTENER_IID)

#endif 
