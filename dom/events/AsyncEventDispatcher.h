




#ifndef mozilla_AsyncEventDispatcher_h_
#define mozilla_AsyncEventDispatcher_h_

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsINode.h"
#include "nsString.h"
#include "nsThreadUtils.h"

namespace mozilla {







 
class AsyncEventDispatcher : public nsRunnable
{
public:
  AsyncEventDispatcher(nsINode* aEventNode, const nsAString& aEventType,
                       bool aBubbles, bool aDispatchChromeOnly)
    : mEventNode(aEventNode)
    , mEventType(aEventType)
    , mBubbles(aBubbles)
    , mDispatchChromeOnly(aDispatchChromeOnly)
  {
  }

  AsyncEventDispatcher(nsINode* aEventNode, nsIDOMEvent* aEvent)
    : mEventNode(aEventNode)
    , mEvent(aEvent)
    , mDispatchChromeOnly(false)
  {
  }

  AsyncEventDispatcher(nsINode* aEventNode, WidgetEvent& aEvent);

  NS_IMETHOD Run() MOZ_OVERRIDE;
  nsresult PostDOMEvent();
  void RunDOMEventWhenSafe();

  nsCOMPtr<nsINode>     mEventNode;
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsString              mEventType;
  bool                  mBubbles;
  bool                  mDispatchChromeOnly;
};

class LoadBlockingAsyncEventDispatcher MOZ_FINAL : public AsyncEventDispatcher
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
