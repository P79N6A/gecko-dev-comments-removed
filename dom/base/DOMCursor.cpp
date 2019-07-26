





#include "DOMCursor.h"
#include "mozilla/dom/DOMCursorBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(DOMCursor, DOMRequest,
                                     mCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DOMCursor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMCursor)
NS_INTERFACE_MAP_END_INHERITING(DOMRequest)

NS_IMPL_ADDREF_INHERITED(DOMCursor, DOMRequest)
NS_IMPL_RELEASE_INHERITED(DOMCursor, DOMRequest)

DOMCursor::DOMCursor(nsPIDOMWindow* aWindow, nsICursorContinueCallback* aCallback)
  : DOMRequest(aWindow)
  , mCallback(aCallback)
  , mFinished(false)
{
}

void
DOMCursor::Reset()
{
  MOZ_ASSERT(!mFinished);

  
  mResult = JSVAL_VOID;
  mDone = false;
}

void
DOMCursor::FireDone()
{
  Reset();
  mFinished = true;
  FireSuccess(JS::UndefinedHandleValue);
}

NS_IMETHODIMP
DOMCursor::GetDone(bool *aDone)
{
  *aDone = Done();
  return NS_OK;
}

NS_IMETHODIMP
DOMCursor::Continue()
{
  ErrorResult rv;
  Continue(rv);
  return rv.ErrorCode();
}

void
DOMCursor::Continue(ErrorResult& aRv)
{
  MOZ_ASSERT(mCallback, "If you're creating your own cursor class with no callback, you should override Continue()");

  
  if (mResult == JSVAL_VOID) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  Reset();
  mCallback->HandleContinue();
}

 JSObject*
DOMCursor::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DOMCursorBinding::Wrap(aCx, this);
}

} 
} 
