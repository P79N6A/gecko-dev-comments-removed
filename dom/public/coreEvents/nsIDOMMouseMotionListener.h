





































#ifndef nsIDOMMouseMotionListener_h__
#define nsIDOMMouseMotionListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMMOUSEMOTIONLISTENER_IID \
{ /* 162b3480-ded6-11d1-bd85-00805f8ae3f4 */ \
0x162b3480, 0xded6, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMMouseMotionListener : public nsIDOMEventListener {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMMOUSEMOTIONLISTENER_IID)
  




  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent) = 0;

  




  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMMouseMotionListener,
                              NS_IDOMMOUSEMOTIONLISTENER_IID)

#endif 
