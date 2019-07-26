




#include "DOMBindingBase.h"

#include "nsIJSContextStack.h"

#include "jsfriendapi.h"
#include "mozilla/dom/DOMJSClass.h"
#include "nsContentUtils.h"
#include "nsWrapperCacheInlines.h"

using namespace mozilla;
using namespace mozilla::dom;
USING_WORKERS_NAMESPACE

DOMBindingBase::DOMBindingBase(JSContext* aCx)
: mJSContext(aCx)
{
  if (!aCx) {
    AssertIsOnMainThread();
  }
}

DOMBindingBase::~DOMBindingBase()
{
  if (!mJSContext) {
    AssertIsOnMainThread();
  }
}

NS_IMPL_ADDREF(DOMBindingBase)
NS_IMPL_RELEASE(DOMBindingBase)
NS_INTERFACE_MAP_BEGIN(DOMBindingBase)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_INTERFACE_MAP_END

void
DOMBindingBase::_trace(JSTracer* aTrc)
{
  JSObject* obj = GetJSObject();
  if (obj) {
    JS_CallObjectTracer(aTrc, obj, "cached wrapper");
  }
}

void
DOMBindingBase::_finalize(JSFreeOp* aFop)
{
  ClearWrapper();
  NS_RELEASE_THIS();
}

JSContext*
DOMBindingBase::GetJSContextFromContextStack() const
{
  AssertIsOnMainThread();
  MOZ_ASSERT(!mJSContext);

  if (!mContextStack) {
    mContextStack = nsContentUtils::ThreadJSContextStack();
    MOZ_ASSERT(mContextStack);
  }

  JSContext* cx;
  if (NS_FAILED(mContextStack->Peek(&cx))) {
    MOZ_NOT_REACHED("This should never fail!");
  }

  MOZ_ASSERT(cx);
  return cx;
}

#ifdef DEBUG
JSObject*
DOMBindingBase::GetJSObject() const
{
  
  
  MOZ_ASSERT(GetJSObjectFromBits() == GetWrapperPreserveColor());
  return GetJSObjectFromBits();
}

void
DOMBindingBase::SetJSObject(JSObject* aObject)
{
  
  
  SetWrapper(aObject);

  uintptr_t oldWrapperPtrBits = mWrapperPtrBits;

  SetWrapperBits(aObject);

  MOZ_ASSERT(oldWrapperPtrBits == mWrapperPtrBits);
}
#endif
