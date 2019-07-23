





































#ifndef nsIDOMDragListener_h__
#define nsIDOMDragListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"







#define NS_IDOMDRAGLISTENER_IID \
{ /* 1A107271-1E26-419A-BCF1-0A4CF7A66B45 */ \
0x1a107271, 0x1e26, 0x419a, \
{0xbc, 0xf1, 0x0a, 0x4c, 0xf7, 0xa6, 0x6b, 0x45} }



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

  
  NS_IMETHOD DragStart(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD DragLeave(nsIDOMEvent* aMouseEvent) { return NS_OK; }
  NS_IMETHOD Drop(nsIDOMEvent* aMouseEvent) { return NS_OK; }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMDragListener, NS_IDOMDRAGLISTENER_IID)

#endif 
