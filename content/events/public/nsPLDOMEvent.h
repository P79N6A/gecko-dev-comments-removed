




































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
               PRBool aBubbles, PRBool aDispatchChromeOnly)
    : mEventNode(aEventNode), mEventType(aEventType),
      mBubbles(aBubbles),
      mDispatchChromeOnly(aDispatchChromeOnly)
  { }

  nsPLDOMEvent(nsINode *aEventNode, nsIDOMEvent *aEvent)
    : mEventNode(aEventNode), mEvent(aEvent), mDispatchChromeOnly(PR_FALSE)
  { }

  nsPLDOMEvent(nsINode *aEventNode, nsEvent &aEvent);

  NS_IMETHOD Run();
  nsresult PostDOMEvent();
  void RunDOMEventWhenSafe();

  nsCOMPtr<nsINode>     mEventNode;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsString              mEventType;
  PRPackedBool          mBubbles;
  PRPackedBool          mDispatchChromeOnly;
};

class nsLoadBlockingPLDOMEvent : public nsPLDOMEvent {
public:
  nsLoadBlockingPLDOMEvent(nsINode *aEventNode, const nsAString& aEventType,
                           PRBool aBubbles, PRBool aDispatchChromeOnly)
    : nsPLDOMEvent(aEventNode, aEventType, aBubbles, aDispatchChromeOnly),
      mBlockedDoc(aEventNode->GetOwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }

  nsLoadBlockingPLDOMEvent(nsINode *aEventNode, nsIDOMEvent *aEvent)
    : nsPLDOMEvent(aEventNode, aEvent),
      mBlockedDoc(aEventNode->GetOwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }
  
  ~nsLoadBlockingPLDOMEvent();

  nsCOMPtr<nsIDocument> mBlockedDoc;
};

#endif
