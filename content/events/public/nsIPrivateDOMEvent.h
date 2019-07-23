




































#ifndef nsIPrivateDOMEvent_h__
#define nsIPrivateDOMEvent_h__

#include "nsISupports.h"

class nsPresContext;




#define NS_IPRIVATEDOMEVENT_IID \
{0x5b3543d3, 0x84ed, 0x4b59, \
{0x95, 0xca, 0xa4, 0x21, 0xf2, 0x59, 0x22, 0x22}}

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
  NS_IMETHOD_(PRBool) IsDispatchStopped() = 0;
  NS_IMETHOD_(nsEvent*) GetInternalNSEvent() = 0;
  NS_IMETHOD_(PRBool) HasOriginalTarget() = 0;
  NS_IMETHOD SetTrusted(PRBool aTrusted) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateDOMEvent, NS_IPRIVATEDOMEVENT_IID)

nsresult
NS_NewDOMEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, nsEvent *aEvent);
nsresult
NS_NewDOMDataContainerEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, nsEvent *aEvent);
nsresult
NS_NewDOMUIEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsGUIEvent *aEvent);
nsresult
NS_NewDOMMouseEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsInputEvent *aEvent);
nsresult
NS_NewDOMMouseScrollEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsInputEvent *aEvent);
nsresult
NS_NewDOMDragEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsDragEvent *aEvent);
nsresult
NS_NewDOMKeyboardEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsKeyEvent *aEvent);
nsresult
NS_NewDOMMutationEvent(nsIDOMEvent** aResult NS_OUTPARAM, nsPresContext* aPresContext, class nsMutationEvent* aEvent);
nsresult
NS_NewDOMPopupBlockedEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, nsEvent* aEvent);
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
nsresult
NS_NewDOMMessageEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsEvent* aEvent);
nsresult
NS_NewDOMProgressEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsEvent* aEvent);
nsresult
NS_NewDOMNotifyPaintEvent(nsIDOMEvent** aResult, nsPresContext* aPresContext, class nsNotifyPaintEvent* aEvent);
nsresult
NS_NewDOMSimpleGestureEvent(nsIDOMEvent** aInstancePtrResult, nsPresContext* aPresContext, class nsSimpleGestureEvent* aEvent);
#endif 
