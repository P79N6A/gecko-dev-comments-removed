




































#ifndef nsIPrivateDOMEvent_h__
#define nsIPrivateDOMEvent_h__

#include "nsISupports.h"

class nsPresContext;




#define NS_IPRIVATEDOMEVENT_IID \
{ /* 25e6626c-8e54-409b-87b5-2beceaac399e */ \
0x25e6626c, 0x8e54, 0x409b, \
{0x87, 0xb5, 0x2b, 0xec, 0xea, 0xac, 0x39, 0x9e} }

class nsIDOMEventTarget;
class nsIDOMEvent;
class nsEvent;
class nsCommandEvent;

class nsIPrivateDOMEvent : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATEDOMEVENT_IID)

  NS_IMETHOD DuplicatePrivateData() = 0;
  NS_IMETHOD SetTarget(nsIDOMEventTarget* aTarget) = 0;
  NS_IMETHOD SetCurrentTarget(nsIDOMEventTarget* aTarget) = 0;
  NS_IMETHOD SetOriginalTarget(nsIDOMEventTarget* aTarget) = 0;
  NS_IMETHOD IsDispatchStopped(PRBool* aIsDispatchPrevented) = 0;
  NS_IMETHOD GetInternalNSEvent(nsEvent** aNSEvent) = 0;
  NS_IMETHOD HasOriginalTarget(PRBool* aResult)=0;
  NS_IMETHOD SetTrusted(PRBool aTrusted)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateDOMEvent, NS_IPRIVATEDOMEVENT_IID)

nsresult
NS_NewDOMEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, nsEvent *aEvent);
nsresult
NS_NewDOMUIEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsGUIEvent *aEvent);
nsresult
NS_NewDOMMouseEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsInputEvent *aEvent);
nsresult
NS_NewDOMKeyboardEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsKeyEvent *aEvent);
nsresult
NS_NewDOMMutationEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsMutationEvent* aEvent);
nsresult
NS_NewDOMPopupBlockedEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsPopupBlockedEvent* aEvent);
nsresult
NS_NewDOMTextEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsTextEvent* aEvent);
nsresult
NS_NewDOMBeforeUnloadEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsBeforePageUnloadEvent* aEvent);
nsresult
NS_NewDOMPageTransitionEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsPageTransitionEvent* aEvent);
#ifdef MOZ_SVG
nsresult
NS_NewDOMSVGEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsEvent* aEvent);
nsresult
NS_NewDOMSVGZoomEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsGUIEvent* aEvent);
#endif 
nsresult
NS_NewDOMXULCommandEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsXULCommandEvent* aEvent);
nsresult
NS_NewDOMCommandEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, nsCommandEvent* aEvent);

#endif 
