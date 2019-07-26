




#ifndef nsAsyncDOMEvent_h___
#define nsAsyncDOMEvent_h___

#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsINode.h"
#include "nsIDOMEvent.h"
#include "nsString.h"
#include "nsIDocument.h"







 
class nsAsyncDOMEvent : public nsRunnable {
public:
  nsAsyncDOMEvent(nsINode *aEventNode, const nsAString& aEventType,
                  bool aBubbles, bool aDispatchChromeOnly)
    : mEventNode(aEventNode), mEventType(aEventType),
      mBubbles(aBubbles),
      mDispatchChromeOnly(aDispatchChromeOnly)
  { }

  nsAsyncDOMEvent(nsINode *aEventNode, nsIDOMEvent *aEvent)
    : mEventNode(aEventNode), mEvent(aEvent), mDispatchChromeOnly(false)
  { }

  nsAsyncDOMEvent(nsINode *aEventNode, nsEvent &aEvent);

  NS_IMETHOD Run();
  nsresult PostDOMEvent();
  void RunDOMEventWhenSafe();

  nsCOMPtr<nsINode>     mEventNode;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsString              mEventType;
  bool                  mBubbles;
  bool                  mDispatchChromeOnly;
};

class nsLoadBlockingAsyncDOMEvent : public nsAsyncDOMEvent {
public:
  nsLoadBlockingAsyncDOMEvent(nsINode *aEventNode, const nsAString& aEventType,
                              bool aBubbles, bool aDispatchChromeOnly)
    : nsAsyncDOMEvent(aEventNode, aEventType, aBubbles, aDispatchChromeOnly),
      mBlockedDoc(aEventNode->OwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }

  nsLoadBlockingAsyncDOMEvent(nsINode *aEventNode, nsIDOMEvent *aEvent)
    : nsAsyncDOMEvent(aEventNode, aEvent),
      mBlockedDoc(aEventNode->OwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }
  
  ~nsLoadBlockingAsyncDOMEvent();

  nsCOMPtr<nsIDocument> mBlockedDoc;
};

#endif
