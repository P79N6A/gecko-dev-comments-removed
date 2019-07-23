





































#ifndef nsIDOMMouseListener_h__
#define nsIDOMMouseListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMMOUSELISTENER_IID \
{ /* ccd7fa30-da37-11d1-bd85-00805f8ae3f4 */ \
0xccd7fa30, 0xda37, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMMouseListener : public nsIDOMEventListener {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMMOUSELISTENER_IID)

  




  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent) = 0;

  




  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) = 0;

  





  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) = 0;

  





  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) = 0;

  




  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) = 0;

  




  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMMouseListener, NS_IDOMMOUSELISTENER_IID)

#endif 
