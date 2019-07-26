





#include "FutureCallback.h"
#include "mozilla/dom/Future.h"
#include "mozilla/dom/FutureResolver.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(FutureCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FutureCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FutureCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(FutureCallback)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(FutureCallback)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

FutureCallback::FutureCallback()
{
  MOZ_COUNT_CTOR(FutureCallback);
}

FutureCallback::~FutureCallback()
{
  MOZ_COUNT_DTOR(FutureCallback);
}



NS_IMPL_CYCLE_COLLECTION_INHERITED_1(ResolveFutureCallback,
                                     FutureCallback,
                                     mResolver)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ResolveFutureCallback)
NS_INTERFACE_MAP_END_INHERITING(FutureCallback)

NS_IMPL_ADDREF_INHERITED(ResolveFutureCallback, FutureCallback)
NS_IMPL_RELEASE_INHERITED(ResolveFutureCallback, FutureCallback)

ResolveFutureCallback::ResolveFutureCallback(FutureResolver* aResolver)
  : mResolver(aResolver)
{
  MOZ_ASSERT(aResolver);
  MOZ_COUNT_CTOR(ResolveFutureCallback);
}

ResolveFutureCallback::~ResolveFutureCallback()
{
  MOZ_COUNT_DTOR(ResolveFutureCallback);
}

void
ResolveFutureCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  
  AutoJSContext cx;
  
  Maybe<JSAutoCompartment> ac;
  if (aValue.WasPassed() && aValue.Value().isObject()) {
    JS::Rooted<JSObject*> rooted(cx, &aValue.Value().toObject());
    ac.construct(cx, rooted);
  }

  mResolver->Resolve(cx, aValue, FutureResolver::SyncTask);
}



NS_IMPL_CYCLE_COLLECTION_INHERITED_1(RejectFutureCallback,
                                     FutureCallback,
                                     mResolver)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(RejectFutureCallback)
NS_INTERFACE_MAP_END_INHERITING(FutureCallback)

NS_IMPL_ADDREF_INHERITED(RejectFutureCallback, FutureCallback)
NS_IMPL_RELEASE_INHERITED(RejectFutureCallback, FutureCallback)

RejectFutureCallback::RejectFutureCallback(FutureResolver* aResolver)
  : mResolver(aResolver)
{
  MOZ_ASSERT(aResolver);
  MOZ_COUNT_CTOR(RejectFutureCallback);
}

RejectFutureCallback::~RejectFutureCallback()
{
  MOZ_COUNT_DTOR(RejectFutureCallback);
}

void
RejectFutureCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  
  AutoJSContext cx;
  
  Maybe<JSAutoCompartment> ac;
  if (aValue.WasPassed() && aValue.Value().isObject()) {
    JS::Rooted<JSObject*> rooted(cx, &aValue.Value().toObject());
    ac.construct(cx, rooted);
  }

  mResolver->Reject(cx, aValue, FutureResolver::SyncTask);
}



NS_IMPL_CYCLE_COLLECTION_INHERITED_2(WrapperFutureCallback,
                                     FutureCallback,
                                     mNextResolver, mCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(WrapperFutureCallback)
NS_INTERFACE_MAP_END_INHERITING(FutureCallback)

NS_IMPL_ADDREF_INHERITED(WrapperFutureCallback, FutureCallback)
NS_IMPL_RELEASE_INHERITED(WrapperFutureCallback, FutureCallback)

WrapperFutureCallback::WrapperFutureCallback(FutureResolver* aNextResolver,
                                             AnyCallback* aCallback)
  : mNextResolver(aNextResolver)
  , mCallback(aCallback)
{
  MOZ_ASSERT(aNextResolver);
  MOZ_COUNT_CTOR(WrapperFutureCallback);
}

WrapperFutureCallback::~WrapperFutureCallback()
{
  MOZ_COUNT_DTOR(WrapperFutureCallback);
}

void
WrapperFutureCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  AutoJSContext cx;
  
  Maybe<JSAutoCompartment> ac;
  if (aValue.WasPassed() && aValue.Value().isObject()) {
    JS::Rooted<JSObject*> rooted(cx, &aValue.Value().toObject());
    ac.construct(cx, rooted);
  }

  ErrorResult rv;

  
  
  Optional<JS::Handle<JS::Value> > value(cx,
    mCallback->Call(mNextResolver->GetParentObject(), aValue, rv,
                    CallbackObject::eRethrowExceptions));

  rv.WouldReportJSException();

  if (rv.Failed() && rv.IsJSException()) {
    Optional<JS::Handle<JS::Value> > value(cx);
    rv.StealJSException(cx, &value.Value());
    mNextResolver->Reject(cx, value, FutureResolver::SyncTask);
    return;
  }

  
  
  mNextResolver->Resolve(cx, value, FutureResolver::SyncTask);
}



NS_IMPL_CYCLE_COLLECTION_INHERITED_2(SimpleWrapperFutureCallback,
                                     FutureCallback,
                                     mFuture, mCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(SimpleWrapperFutureCallback)
NS_INTERFACE_MAP_END_INHERITING(FutureCallback)

NS_IMPL_ADDREF_INHERITED(SimpleWrapperFutureCallback, FutureCallback)
NS_IMPL_RELEASE_INHERITED(SimpleWrapperFutureCallback, FutureCallback)

SimpleWrapperFutureCallback::SimpleWrapperFutureCallback(Future* aFuture,
                                                         AnyCallback* aCallback)
  : mFuture(aFuture)
  , mCallback(aCallback)
{
  MOZ_ASSERT(aFuture);
  MOZ_COUNT_CTOR(SimpleWrapperFutureCallback);
}

SimpleWrapperFutureCallback::~SimpleWrapperFutureCallback()
{
  MOZ_COUNT_DTOR(SimpleWrapperFutureCallback);
}

void
SimpleWrapperFutureCallback::Call(const Optional<JS::Handle<JS::Value> >& aValue)
{
  ErrorResult rv;
  mCallback->Call(mFuture, aValue, rv);
}

 FutureCallback*
FutureCallback::Factory(FutureResolver* aNextResolver,
                        AnyCallback* aCallback, Task aTask)
{
  MOZ_ASSERT(aNextResolver);

  
  
  if (aCallback) {
    return new WrapperFutureCallback(aNextResolver, aCallback);
  }

  if (aTask == Resolve) {
    return new ResolveFutureCallback(aNextResolver);
  }

  if (aTask == Reject) {
    return new RejectFutureCallback(aNextResolver);
  }

  MOZ_ASSERT(false, "This should not happen");
  return nullptr;
}

} 
} 
