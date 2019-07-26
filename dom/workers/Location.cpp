




#include "Location.h"

#include "mozilla/dom/WorkerLocationBinding.h"

BEGIN_WORKERS_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WorkerLocation)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WorkerLocation, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WorkerLocation, Release)

 already_AddRefed<WorkerLocation>
WorkerLocation::Create(JSContext* aCx, JS::Handle<JSObject*> aGlobal,
                       WorkerPrivate::LocationInfo& aInfo)
{
  nsRefPtr<WorkerLocation> location =
    new WorkerLocation(NS_ConvertUTF8toUTF16(aInfo.mHref),
                       NS_ConvertUTF8toUTF16(aInfo.mProtocol),
                       NS_ConvertUTF8toUTF16(aInfo.mHost),
                       NS_ConvertUTF8toUTF16(aInfo.mHostname),
                       NS_ConvertUTF8toUTF16(aInfo.mPort),
                       NS_ConvertUTF8toUTF16(aInfo.mPathname),
                       NS_ConvertUTF8toUTF16(aInfo.mSearch),
                       NS_ConvertUTF8toUTF16(aInfo.mHash));

  return location.forget();
}

JSObject*
WorkerLocation::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return WorkerLocationBinding_workers::Wrap(aCx, aScope, this);
}

END_WORKERS_NAMESPACE
