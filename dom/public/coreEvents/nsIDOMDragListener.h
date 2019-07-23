





































#ifndef nsIDOMDragListener_h__
#define nsIDOMDragListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMDRAGLISTENER_IID \
{ /* 6b8b25d0-ded5-11d1-bd85-00805f8ae3f4 */ \
0x6b8b25d0, 0xded5, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMDragListener : public nsIDOMEventListener {

public:

   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMDRAGLISTENER_IID)

  




  NS_IMETHOD DragEnter(nsIDOMEvent* aMouseEvent) = 0;

  




  NS_IMETHOD DragOver(nsIDOMEvent* aMouseEvent) = 0;

  




  NS_IMETHOD DragExit(nsIDOMEvent* aMouseEvent) = 0;

  




  NS_IMETHOD DragDrop(nsIDOMEvent* aMouseEvent) = 0;
  
  




  NS_IMETHOD DragGesture(nsIDOMEvent* aMouseEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMDragListener, NS_IDOMDRAGLISTENER_IID)

#endif 
