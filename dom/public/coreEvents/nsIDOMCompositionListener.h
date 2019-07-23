




































#ifndef nsIDOMCompositionListener_h__
#define nsIDOMCompositionListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMCOMPOSITIONLISTENER_IID	\
{ 0xf14b6491, 0xe95b, 0x11d2, \
{ 0x9e, 0x85, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b } }


class nsIDOMCompositionListener : public nsIDOMEventListener {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMCOMPOSITIONLISTENER_IID)

  NS_IMETHOD HandleStartComposition(nsIDOMEvent* aCompositionEvent) = 0;
  NS_IMETHOD HandleEndComposition(nsIDOMEvent* aCompositionEvent) = 0;
  NS_IMETHOD HandleQueryComposition(nsIDOMEvent* aCompositionEvent) = 0;
  NS_IMETHOD HandleQueryReconversion(nsIDOMEvent* aCompositionEvent) = 0;
  NS_IMETHOD HandleQueryCaretRect(nsIDOMEvent* aCompositionEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMCompositionListener,
                              NS_IDOMCOMPOSITIONLISTENER_IID)

#endif 
