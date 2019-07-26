




#include "Navigator.h"

#include "mozilla/dom/WorkerNavigatorBinding.h"

#include "RuntimeService.h"

BEGIN_WORKERS_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WorkerNavigator)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WorkerNavigator, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WorkerNavigator, Release)

 already_AddRefed<WorkerNavigator>
WorkerNavigator::Create(JSContext* aCx, JS::Handle<JSObject*> aGlobal)
{
  RuntimeService* rts = RuntimeService::GetService();
  MOZ_ASSERT(rts);

  const RuntimeService::NavigatorStrings& strings =
    rts->GetNavigatorStrings();

  nsRefPtr<WorkerNavigator> navigator =
    new WorkerNavigator(strings.mAppName, strings.mAppVersion,
                        strings.mPlatform, strings.mUserAgent);

  return navigator.forget();
}

JSObject*
WorkerNavigator::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return WorkerNavigatorBinding_workers::Wrap(aCx, aScope, this);
}

END_WORKERS_NAMESPACE
