




































#ifndef nsPLDOMEvent_h___
#define nsPLDOMEvent_h___

#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsINode.h"
#include "nsIDOMEvent.h"
#include "nsString.h"
#include "nsIDocument.h"











 
class nsPLDOMEvent : public nsRunnable {
public:
  nsPLDOMEvent(nsINode *aEventNode, const nsAString& aEventType,
               bool aBubbles, bool aDispatchChromeOnly)
    : mEventNode(aEventNode), mEventType(aEventType),
      mBubbles(aBubbles),
      mDispatchChromeOnly(aDispatchChromeOnly)
  { }

  nsPLDOMEvent(nsINode *aEventNode, nsIDOMEvent *aEvent)
    : mEventNode(aEventNode), mEvent(aEvent), mDispatchChromeOnly(false)
  { }

  nsPLDOMEvent(nsINode *aEventNode, nsEvent &aEvent);

  NS_IMETHOD Run();
  nsresult PostDOMEvent();
  void RunDOMEventWhenSafe();

  nsCOMPtr<nsINode>     mEventNode;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsString              mEventType;
  bool                  mBubbles;
  bool                  mDispatchChromeOnly;
};

class nsLoadBlockingPLDOMEvent : public nsPLDOMEvent {
public:
  nsLoadBlockingPLDOMEvent(nsINode *aEventNode, const nsAString& aEventType,
                           bool aBubbles, bool aDispatchChromeOnly)
    : nsPLDOMEvent(aEventNode, aEventType, aBubbles, aDispatchChromeOnly),
      mBlockedDoc(aEventNode->OwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }

  nsLoadBlockingPLDOMEvent(nsINode *aEventNode, nsIDOMEvent *aEvent)
    : nsPLDOMEvent(aEventNode, aEvent),
      mBlockedDoc(aEventNode->OwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }
  
  ~nsLoadBlockingPLDOMEvent();

  nsCOMPtr<nsIDocument> mBlockedDoc;
};

#endif
