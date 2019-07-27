





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
  , mOnlyChromeDispatch(false)
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
  nsCOMPtr<nsIDOMEvent> event = mEvent;
  if (!event) {
    NS_NewDOMEvent(getter_AddRefs(event), mTarget, nullptr, nullptr);
    nsresult rv = event->InitEvent(mEventType, mBubbles, false);
    NS_ENSURE_SUCCESS(rv, rv);
    event->SetTrusted(true);
  }
  if (mOnlyChromeDispatch) {
    MOZ_ASSERT(event->InternalDOMEvent()->IsTrusted());
    event->GetInternalNSEvent()->mFlags.mOnlyChromeDispatch = true;
  }
  bool dummy;
  mTarget->DispatchEvent(event, &dummy);
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
