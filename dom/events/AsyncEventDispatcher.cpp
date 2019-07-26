




#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/Event.h" 
#include "mozilla/dom/EventTarget.h"
#include "nsContentUtils.h"
#include "nsIDOMEvent.h"

namespace mozilla {

using namespace dom;





AsyncEventDispatcher::AsyncEventDispatcher(nsINode* aEventNode,
                                           WidgetEvent& aEvent)
  : mEventNode(aEventNode)
  , mDispatchChromeOnly(false)
{
  MOZ_ASSERT(mEventNode);
  EventDispatcher::CreateEvent(aEventNode, nullptr, &aEvent, EmptyString(),
                               getter_AddRefs(mEvent));
  NS_ASSERTION(mEvent, "Should never fail to create an event");
  mEvent->DuplicatePrivateData();
  mEvent->SetTrusted(aEvent.mFlags.mIsTrusted);
}

NS_IMETHODIMP
AsyncEventDispatcher::Run()
{
  if (mEvent) {
    if (mDispatchChromeOnly) {
      MOZ_ASSERT(mEvent->InternalDOMEvent()->IsTrusted());

      nsCOMPtr<nsIDocument> ownerDoc = mEventNode->OwnerDoc();
      nsPIDOMWindow* window = ownerDoc->GetWindow();
      if (!window) {
        return NS_ERROR_INVALID_ARG;
      }

      nsCOMPtr<EventTarget> target = window->GetParentTarget();
      if (!target) {
        return NS_ERROR_INVALID_ARG;
      }
      EventDispatcher::DispatchDOMEvent(target, nullptr, mEvent,
                                        nullptr, nullptr);
    } else {
      nsCOMPtr<EventTarget> target = mEventNode.get();
      bool defaultActionEnabled; 
      target->DispatchEvent(mEvent, &defaultActionEnabled);
    }
  } else {
    nsIDocument* doc = mEventNode->OwnerDoc();
    if (mDispatchChromeOnly) {
      nsContentUtils::DispatchChromeEvent(doc, mEventNode, mEventType,
                                          mBubbles, false);
    } else {
      nsContentUtils::DispatchTrustedEvent(doc, mEventNode, mEventType,
                                           mBubbles, false);
    }
  }

  return NS_OK;
}

nsresult
AsyncEventDispatcher::PostDOMEvent()
{
  return NS_DispatchToCurrentThread(this);
}

void
AsyncEventDispatcher::RunDOMEventWhenSafe()
{
  nsContentUtils::AddScriptRunner(this);
}





LoadBlockingAsyncEventDispatcher::~LoadBlockingAsyncEventDispatcher()
{
  if (mBlockedDoc) {
    mBlockedDoc->UnblockOnload(true);
  }
}

} 
