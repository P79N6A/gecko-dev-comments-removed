





#ifndef mozilla_AsyncEventDispatcher_h_
#define mozilla_AsyncEventDispatcher_h_

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsString.h"
#include "nsThreadUtils.h"

class nsINode;

namespace mozilla {







 
class AsyncEventDispatcher : public nsRunnable
{
public:
  





  AsyncEventDispatcher(nsINode* aTarget, const nsAString& aEventType,
                       bool aBubbles, bool aOnlyChromeDispatch)
    : mTarget(aTarget)
    , mEventType(aEventType)
    , mBubbles(aBubbles)
    , mOnlyChromeDispatch(aOnlyChromeDispatch)
  {
  }

  AsyncEventDispatcher(dom::EventTarget* aTarget, const nsAString& aEventType,
                       bool aBubbles)
    : mTarget(aTarget)
    , mEventType(aEventType)
    , mBubbles(aBubbles)
    , mOnlyChromeDispatch(false)
  {
  }

  AsyncEventDispatcher(dom::EventTarget* aTarget, nsIDOMEvent* aEvent)
    : mTarget(aTarget)
    , mEvent(aEvent)
    , mOnlyChromeDispatch(false)
  {
  }

  AsyncEventDispatcher(dom::EventTarget* aTarget, WidgetEvent& aEvent);

  NS_IMETHOD Run() override;
  nsresult PostDOMEvent();
  void RunDOMEventWhenSafe();

  nsCOMPtr<dom::EventTarget> mTarget;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsString              mEventType;
  bool                  mBubbles;
  bool                  mOnlyChromeDispatch;
};

class LoadBlockingAsyncEventDispatcher final : public AsyncEventDispatcher
{
public:
  LoadBlockingAsyncEventDispatcher(nsINode* aEventNode,
                                   const nsAString& aEventType,
                                   bool aBubbles, bool aDispatchChromeOnly)
    : AsyncEventDispatcher(aEventNode, aEventType,
                           aBubbles, aDispatchChromeOnly)
    , mBlockedDoc(aEventNode->OwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }

  LoadBlockingAsyncEventDispatcher(nsINode* aEventNode, nsIDOMEvent* aEvent)
    : AsyncEventDispatcher(aEventNode, aEvent)
    , mBlockedDoc(aEventNode->OwnerDoc())
  {
    if (mBlockedDoc) {
      mBlockedDoc->BlockOnload();
    }
  }
  
  ~LoadBlockingAsyncEventDispatcher();

private:
  nsCOMPtr<nsIDocument> mBlockedDoc;
};

} 

#endif 
