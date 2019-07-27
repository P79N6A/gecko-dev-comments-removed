





#include "mozilla/dom/cache/CacheStorageParent.h"

#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/cache/ActorUtils.h"
#include "mozilla/dom/cache/CacheOpParent.h"
#include "mozilla/dom/cache/ManagerId.h"
#include "mozilla/ipc/PBackgroundParent.h"

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::ipc::PBackgroundParent;
using mozilla::ipc::PrincipalInfo;


PCacheStorageParent*
AllocPCacheStorageParent(PBackgroundParent* aManagingActor,
                         Namespace aNamespace,
                         const mozilla::ipc::PrincipalInfo& aPrincipalInfo)
{
  return new CacheStorageParent(aManagingActor, aNamespace, aPrincipalInfo);
}


void
DeallocPCacheStorageParent(PCacheStorageParent* aActor)
{
  delete aActor;
}

CacheStorageParent::CacheStorageParent(PBackgroundParent* aManagingActor,
                                       Namespace aNamespace,
                                       const PrincipalInfo& aPrincipalInfo)
  : mNamespace(aNamespace)
  , mVerifiedStatus(NS_OK)
{
  MOZ_COUNT_CTOR(cache::CacheStorageParent);
  MOZ_ASSERT(aManagingActor);

  
  mVerifier = PrincipalVerifier::CreateAndDispatch(this, aManagingActor,
                                                   aPrincipalInfo);
  MOZ_ASSERT(mVerifier);
}

CacheStorageParent::~CacheStorageParent()
{
  MOZ_COUNT_DTOR(cache::CacheStorageParent);
  MOZ_ASSERT(!mVerifier);
}

void
CacheStorageParent::ActorDestroy(ActorDestroyReason aReason)
{
  if (mVerifier) {
    mVerifier->RemoveListener(this);
    mVerifier = nullptr;
  }
}

PCacheOpParent*
CacheStorageParent::AllocPCacheOpParent(const CacheOpArgs& aOpArgs)
{
  if (aOpArgs.type() != CacheOpArgs::TStorageMatchArgs &&
      aOpArgs.type() != CacheOpArgs::TStorageHasArgs &&
      aOpArgs.type() != CacheOpArgs::TStorageOpenArgs &&
      aOpArgs.type() != CacheOpArgs::TStorageDeleteArgs &&
      aOpArgs.type() != CacheOpArgs::TStorageKeysArgs)
  {
    MOZ_CRASH("Invalid operation sent to CacheStorage actor!");
  }

  return new CacheOpParent(Manager(), mNamespace, aOpArgs);
}

bool
CacheStorageParent::DeallocPCacheOpParent(PCacheOpParent* aActor)
{
  delete aActor;
  return true;
}

bool
CacheStorageParent::RecvPCacheOpConstructor(PCacheOpParent* aActor,
                                            const CacheOpArgs& aOpArgs)
{
  auto actor = static_cast<CacheOpParent*>(aActor);

  if (mVerifier) {
    MOZ_ASSERT(!mManagerId);
    actor->WaitForVerification(mVerifier);
    return true;
  }

  if (NS_FAILED(mVerifiedStatus)) {
    unused << CacheOpParent::Send__delete__(actor, ErrorResult(mVerifiedStatus),
                                            void_t());
    return true;
  }

  MOZ_ASSERT(mManagerId);
  actor->Execute(mManagerId);
  return true;
}

bool
CacheStorageParent::RecvTeardown()
{
  if (!Send__delete__(this)) {
    
    NS_WARNING("CacheStorage failed to delete actor.");
  }
  return true;
}

void
CacheStorageParent::OnPrincipalVerified(nsresult aRv, ManagerId* aManagerId)
{
  MOZ_ASSERT(mVerifier);
  MOZ_ASSERT(!mManagerId);
  MOZ_ASSERT(NS_SUCCEEDED(mVerifiedStatus));

  if (NS_WARN_IF(NS_FAILED(aRv))) {
    mVerifiedStatus = aRv;
  }

  mManagerId = aManagerId;
  mVerifier->RemoveListener(this);
  mVerifier = nullptr;
}

} 
} 
} 
