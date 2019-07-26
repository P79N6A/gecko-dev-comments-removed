




#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/Event.h" 
#include "mozilla/dom/EventTarget.h"
#include "nsContentUtils.h"
#include "nsIDOMEvent.h"

namespace mozilla {

using namespace dom;





AsyncEventDispatcher::AsyncEventDispatcher(EventTarget* aTarget,
                                           WidgetEvent& aEvent)
  : mTarget(aTarget)
  , mDispatchChromeOnly(false)
{
  MOZ_ASSERT(mTarget);
  EventDispatcher::CreateEvent(aTarget, nullptr, &aEvent, EmptyString(),
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

      nsCOMPtr<nsINode> node = do_QueryInterface(mTarget);
      MOZ_ASSERT(node, "ChromeOnly dispatch supported with Node targets only!");
      nsPIDOMWindow* window = node->OwnerDoc()->GetWindow();
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
      bool defaultActionEnabled; 
      mTarget->DispatchEvent(mEvent, &defaultActionEnabled);
    }
  } else {
    if (mDispatchChromeOnly) {
      nsCOMPtr<nsINode> node = do_QueryInterface(mTarget);
      MOZ_ASSERT(node, "ChromeOnly dispatch supported with Node targets only!");
      nsContentUtils::DispatchChromeEvent(node->OwnerDoc(), node, mEventType,
                                          mBubbles, false);
    } else {
      nsCOMPtr<nsIDOMEvent> event;
      NS_NewDOMEvent(getter_AddRefs(event), mTarget, nullptr, nullptr);
      nsresult rv = event->InitEvent(mEventType, mBubbles, false);
      NS_ENSURE_SUCCESS(rv, rv);
      event->SetTrusted(true);
      bool dummy;
      mTarget->DispatchEvent(event, &dummy);
    }
  }

  return NS_OK;
}

nsresult
AsyncEventDispatcher::PostDOMEvent()
{
  nsRefPtr<AsyncEventDispatcher> ensureDeletionWhenFailing = this;
  return NS_DispatchToCurrentThread(this);
}

void
AsyncEventDispatcher::RunDOMEventWhenSafe()
{
  nsRefPtr<AsyncEventDispatcher> ensureDeletionWhenFailing = this;
  nsContentUtils::AddScriptRunner(this);
}





LoadBlockingAsyncEventDispatcher::~LoadBlockingAsyncEventDispatcher()
{
  if (mBlockedDoc) {
    mBlockedDoc->UnblockOnload(true);
  }
}

} 
