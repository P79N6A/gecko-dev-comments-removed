




































#ifndef nsIDOMCompositionListener_h__
#define nsIDOMCompositionListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMCOMPOSITIONLISTENER_IID	\
{ 0x93a5a335, 0xaa51, 0x4d32, \
{ 0x97, 0x7d, 0x36, 0x80, 0xb7, 0x72, 0x2a, 0xd5 } }


class nsIDOMCompositionListener : public nsIDOMEventListener {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMCOMPOSITIONLISTENER_IID)

  NS_IMETHOD HandleStartComposition(nsIDOMEvent* aCompositionEvent) = 0;
  NS_IMETHOD HandleEndComposition(nsIDOMEvent* aCompositionEvent) = 0;
  NS_IMETHOD HandleQueryComposition(nsIDOMEvent* aCompositionEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMCompositionListener,
                              NS_IDOMCOMPOSITIONLISTENER_IID)

#endif 
