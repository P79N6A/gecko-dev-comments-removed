





#include "DOMCursor.h"
#include "nsIDOMClassInfo.h"
#include "nsError.h"
#include "mozilla/dom/DOMCursorBinding.h"

DOMCI_DATA(DOMCursor, mozilla::dom::DOMCursor)

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DOMCursor,
                                                  DOMRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCallback)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DOMCursor,
                                                DOMRequest)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCallback)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DOMCursor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMCursor)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMCursor)
NS_INTERFACE_MAP_END_INHERITING(DOMRequest)

NS_IMPL_ADDREF_INHERITED(DOMCursor, DOMRequest)
NS_IMPL_RELEASE_INHERITED(DOMCursor, DOMRequest)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(DOMCursor, DOMRequest)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

DOMCursor::DOMCursor(nsIDOMWindow* aWindow, nsICursorContinueCallback* aCallback)
  : DOMRequest(aWindow)
  , mCallback(aCallback)
  , mFinished(false)
{
}

void
DOMCursor::Reset()
{
  MOZ_ASSERT(!mFinished);

  
  if (mRooted) {
    UnrootResultVal();
  }
  mDone = false;
}

void
DOMCursor::FireDone()
{
  Reset();
  mFinished = true;
  FireSuccess(JSVAL_VOID);
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
DOMCursor::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return DOMCursorBinding::Wrap(aCx, aScope, this);
}

} 
} 
