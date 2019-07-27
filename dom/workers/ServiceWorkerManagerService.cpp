





#include "ServiceWorkerManagerService.h"
#include "ServiceWorkerManagerParent.h"
#include "ServiceWorkerRegistrar.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/unused.h"

namespace mozilla {

using namespace ipc;

namespace dom {
namespace workers {

namespace {

ServiceWorkerManagerService* sInstance = nullptr;

} 

ServiceWorkerManagerService::ServiceWorkerManagerService()
{
  AssertIsOnBackgroundThread();

  
  MOZ_ASSERT(!sInstance);
  sInstance = this;
}

ServiceWorkerManagerService::~ServiceWorkerManagerService()
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(sInstance == this);
  MOZ_ASSERT(mAgents.Count() == 0);

  sInstance = nullptr;
}

 already_AddRefed<ServiceWorkerManagerService>
ServiceWorkerManagerService::Get()
{
  AssertIsOnBackgroundThread();

  nsRefPtr<ServiceWorkerManagerService> instance = sInstance;
  return instance.forget();
}

 already_AddRefed<ServiceWorkerManagerService>
ServiceWorkerManagerService::GetOrCreate()
{
  AssertIsOnBackgroundThread();

  nsRefPtr<ServiceWorkerManagerService> instance = sInstance;
  if (!instance) {
    instance = new ServiceWorkerManagerService();
  }
  return instance.forget();
}

void
ServiceWorkerManagerService::RegisterActor(ServiceWorkerManagerParent* aParent)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(!mAgents.Contains(aParent));

  mAgents.PutEntry(aParent);
}

void
ServiceWorkerManagerService::UnregisterActor(ServiceWorkerManagerParent* aParent)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(mAgents.Contains(aParent));

  mAgents.RemoveEntry(aParent);
}

namespace {

struct MOZ_STACK_CLASS UnregisterData final
{
  UnregisterData(const PrincipalInfo& aPrincipalInfo,
                 const nsAString& aScope,
                 uint64_t aParentID)
    : mPrincipalInfo(aPrincipalInfo)
    , mScope(aScope)
    , mParentID(aParentID)
#ifdef DEBUG
    , mParentFound(false)
#endif
  {
    MOZ_COUNT_CTOR(UnregisterData);
  }

  ~UnregisterData()
  {
    MOZ_COUNT_DTOR(UnregisterData);
  }

  const PrincipalInfo mPrincipalInfo;
  const nsString mScope;
  const uint64_t mParentID;
#ifdef DEBUG
  bool mParentFound;
#endif
};

PLDHashOperator
UnregisterEnumerator(nsPtrHashKey<ServiceWorkerManagerParent>* aKey, void* aPtr)
{
  AssertIsOnBackgroundThread();

  auto* data = static_cast<UnregisterData*>(aPtr);
  ServiceWorkerManagerParent* parent = aKey->GetKey();
  MOZ_ASSERT(parent);

  if (parent->ID() != data->mParentID) {
    unused << parent->SendNotifyUnregister(data->mPrincipalInfo, data->mScope);
#ifdef DEBUG
  } else {
    data->mParentFound = true;
#endif
  }

  return PL_DHASH_NEXT;
}

struct MOZ_STACK_CLASS RemoveAllData final
{
  explicit RemoveAllData(uint64_t aParentID)
    : mParentID(aParentID)
#ifdef DEBUG
    , mParentFound(false)
#endif
  {
    MOZ_COUNT_CTOR(RemoveAllData);
  }

  ~RemoveAllData()
  {
    MOZ_COUNT_DTOR(RemoveAllData);
  }

  const uint64_t mParentID;
#ifdef DEBUG
  bool mParentFound;
#endif
};

PLDHashOperator
RemoveAllEnumerator(nsPtrHashKey<ServiceWorkerManagerParent>* aKey, void* aPtr)
{
  AssertIsOnBackgroundThread();

  auto* data = static_cast<RemoveAllData*>(aPtr);
  ServiceWorkerManagerParent* parent = aKey->GetKey();
  MOZ_ASSERT(parent);

  if (parent->ID() != data->mParentID) {
    unused << parent->SendNotifyRemoveAll();
#ifdef DEBUG
  } else {
    data->mParentFound = true;
#endif
  }

  return PL_DHASH_NEXT;
}

struct MOZ_STACK_CLASS RemoveData final
{
  RemoveData(const nsACString& aHost,
             uint64_t aParentID)
    : mHost(aHost)
    , mParentID(aParentID)
#ifdef DEBUG
    , mParentFound(false)
#endif
  {
    MOZ_COUNT_CTOR(RemoveData);
  }

  ~RemoveData()
  {
    MOZ_COUNT_DTOR(RemoveData);
  }

  const nsCString mHost;
  const uint64_t mParentID;
#ifdef DEBUG
  bool mParentFound;
#endif
};

PLDHashOperator
RemoveEnumerator(nsPtrHashKey<ServiceWorkerManagerParent>* aKey, void* aPtr)
{
  AssertIsOnBackgroundThread();

  auto* data = static_cast<RemoveData*>(aPtr);
  ServiceWorkerManagerParent* parent = aKey->GetKey();
  MOZ_ASSERT(parent);

  if (parent->ID() != data->mParentID) {
    unused << parent->SendNotifyRemove(data->mHost);
#ifdef DEBUG
  } else {
    data->mParentFound = true;
#endif
  }

  return PL_DHASH_NEXT;
}

} 

void
ServiceWorkerManagerService::PropagateRegistration(
                                           uint64_t aParentID,
                                           ServiceWorkerRegistrationData& aData)
{
  AssertIsOnBackgroundThread();

  DebugOnly<bool> parentFound = false;
  for (auto iter = mAgents.Iter(); !iter.Done(); iter.Next()) {
    ServiceWorkerManagerParent* parent = iter.Get()->GetKey();
    MOZ_ASSERT(parent);

    if (parent->ID() != aParentID) {
      unused << parent->SendNotifyRegister(aData);
#ifdef DEBUG
    } else {
      parentFound = true;
#endif
    }
  }

#ifdef DEBUG
  MOZ_ASSERT(parentFound);
#endif
}

void
ServiceWorkerManagerService::PropagateSoftUpdate(
                                      uint64_t aParentID,
                                      const OriginAttributes& aOriginAttributes,
                                      const nsAString& aScope)
{
  AssertIsOnBackgroundThread();

  DebugOnly<bool> parentFound = false;
  for (auto iter = mAgents.Iter(); !iter.Done(); iter.Next()) {
    ServiceWorkerManagerParent* parent = iter.Get()->GetKey();
    MOZ_ASSERT(parent);

    if (parent->ID() != aParentID) {
      nsString scope(aScope);
      unused << parent->SendNotifySoftUpdate(aOriginAttributes,
                                             scope);
#ifdef DEBUG
    } else {
      parentFound = true;
#endif
    }
  }

#ifdef DEBUG
  MOZ_ASSERT(parentFound);
#endif
}

void
ServiceWorkerManagerService::PropagateUnregister(
                                            uint64_t aParentID,
                                            const PrincipalInfo& aPrincipalInfo,
                                            const nsAString& aScope)
{
  AssertIsOnBackgroundThread();

  nsRefPtr<dom::ServiceWorkerRegistrar> service =
    dom::ServiceWorkerRegistrar::Get();
  MOZ_ASSERT(service);

  
  
  service->UnregisterServiceWorker(aPrincipalInfo,
                                   NS_ConvertUTF16toUTF8(aScope));

  UnregisterData data(aPrincipalInfo, aScope, aParentID);
  mAgents.EnumerateEntries(UnregisterEnumerator, &data);

#ifdef DEBUG
  MOZ_ASSERT(data.mParentFound);
#endif
}

void
ServiceWorkerManagerService::PropagateRemove(uint64_t aParentID,
                                             const nsACString& aHost)
{
  AssertIsOnBackgroundThread();

  RemoveData data(aHost, aParentID);
  mAgents.EnumerateEntries(RemoveEnumerator, &data);

#ifdef DEBUG
  MOZ_ASSERT(data.mParentFound);
#endif
}

void
ServiceWorkerManagerService::PropagateRemoveAll(uint64_t aParentID)
{
  AssertIsOnBackgroundThread();

  nsRefPtr<dom::ServiceWorkerRegistrar> service =
    dom::ServiceWorkerRegistrar::Get();
  MOZ_ASSERT(service);

  service->RemoveAll();

  RemoveAllData data(aParentID);
  mAgents.EnumerateEntries(RemoveAllEnumerator, &data);

#ifdef DEBUG
  MOZ_ASSERT(data.mParentFound);
#endif
}

} 
} 
} 
