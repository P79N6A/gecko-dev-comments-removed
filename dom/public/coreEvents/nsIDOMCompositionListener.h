




































#ifndef nsIDOMCompositionListener_h__
#define nsIDOMCompositionListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMCOMPOSITIONLISTENER_IID	\
{ 0x47f158c0, 0xc534, 0x43a1, \
{ 0x84, 0x15, 0x8b, 0x17, 0x70, 0x6e, 0x2f, 0xbc } }


class nsIDOMCompositionListener : public nsIDOMEventListener {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMCOMPOSITIONLISTENER_IID)

  NS_IMETHOD HandleStartComposition(nsIDOMEvent* aCompositionEvent) = 0;
  NS_IMETHOD HandleEndComposition(nsIDOMEvent* aCompositionEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMCompositionListener,
                              NS_IDOMCOMPOSITIONLISTENER_IID)

#endif 
