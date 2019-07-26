




#include "DOMBindingBase.h"

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
  TraceJSObject(aTrc, "cached wrapper");
}

void
DOMBindingBase::_finalize(JSFreeOp* aFop)
{
  ClearWrapper();
  NS_RELEASE_THIS();
}

JSContext*
DOMBindingBase::GetJSContext() const {
  return mJSContext ? mJSContext : nsContentUtils::GetCurrentJSContext();
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
