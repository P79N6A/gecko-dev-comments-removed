





































#ifndef nsIDOMXULListener_h__
#define nsIDOMXULListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"


#define NS_IDOMXULLISTENER_IID \
{ 0x730c841, 0x42f3, 0x11d3, { 0x97, 0xfa, 0x0, 0x40, 0x5, 0x53, 0xee, 0xf0 } }

class nsIDOMXULListener : public nsIDOMEventListener {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMXULLISTENER_IID)

  NS_IMETHOD PopupShowing(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD PopupShown(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD PopupHiding(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD PopupHidden(nsIDOMEvent* aEvent) = 0;

  NS_IMETHOD Close(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Command(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Broadcast(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD CommandUpdate(nsIDOMEvent* aEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMXULListener, NS_IDOMXULLISTENER_IID)

#endif 
