





































#ifndef nsIDOMDragListener_h__
#define nsIDOMDragListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"




#define NS_IDOMDRAGLISTENER_IID \
{ /* CD5186C4-228F-4413-AFD9-B65DAA105714 */ \
0xcd5186c4, 0x228f, 0x4413, \
{0xaf, 0xd9, 0xb6, 0x5d, 0xaa, 0x10, 0x57, 0x14} }



class nsIDOMDragListener : public nsIDOMEventListener {

public:

   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMDRAGLISTENER_IID)

  







  NS_IMETHOD DragEnter(nsIDOMEvent* aMouseEvent) = 0;

  







  NS_IMETHOD DragOver(nsIDOMEvent* aMouseEvent) = 0;

  










  NS_IMETHOD DragExit(nsIDOMEvent* aMouseEvent) = 0;

  







  NS_IMETHOD DragDrop(nsIDOMEvent* aMouseEvent) = 0;
  
  







  NS_IMETHOD DragGesture(nsIDOMEvent* aMouseEvent) = 0;

  







  NS_IMETHOD DragEnd(nsIDOMEvent* aMouseEvent) = 0;

  






  NS_IMETHOD Drag(nsIDOMEvent* aMouseEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMDragListener, NS_IDOMDRAGLISTENER_IID)

#endif 
