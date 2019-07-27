





#include "mozilla/dom/cache/Feature.h"

#include "mozilla/dom/cache/ActorChild.h"
#include "WorkerPrivate.h"

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::dom::workers::Canceling;
using mozilla::dom::workers::Status;
using mozilla::dom::workers::WorkerPrivate;


already_AddRefed<Feature>
Feature::Create(WorkerPrivate* aWorkerPrivate)
{
  MOZ_ASSERT(aWorkerPrivate);

  nsRefPtr<Feature> feature = new Feature(aWorkerPrivate);

  if (!aWorkerPrivate->AddFeature(aWorkerPrivate->GetJSContext(), feature)) {
    return nullptr;
  }

  return feature.forget();
}

void
Feature::AddActor(ActorChild* aActor)
{
  NS_ASSERT_OWNINGTHREAD(Feature);
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(!mActorList.Contains(aActor));

  mActorList.AppendElement(aActor);

  
  
  
  
  if (mNotified) {
    aActor->StartDestroy();
  }
}

void
Feature::RemoveActor(ActorChild* aActor)
{
  NS_ASSERT_OWNINGTHREAD(Feature);
  MOZ_ASSERT(aActor);

  DebugOnly<bool> removed = mActorList.RemoveElement(aActor);

  MOZ_ASSERT(removed);
  MOZ_ASSERT(!mActorList.Contains(aActor));
}

bool
Feature::Notified() const
{
  return mNotified;
}

bool
Feature::Notify(JSContext* aCx, Status aStatus)
{
  NS_ASSERT_OWNINGTHREAD(Feature);

  if (aStatus < Canceling || mNotified) {
    return true;
  }

  mNotified = true;

  
  
  for (uint32_t i = 0; i < mActorList.Length(); ++i) {
    mActorList[i]->StartDestroy();
  }

  return true;
}

Feature::Feature(WorkerPrivate* aWorkerPrivate)
  : mWorkerPrivate(aWorkerPrivate)
  , mNotified(false)
{
  MOZ_ASSERT(mWorkerPrivate);
}

Feature::~Feature()
{
  NS_ASSERT_OWNINGTHREAD(Feature);
  MOZ_ASSERT(mActorList.IsEmpty());

  mWorkerPrivate->RemoveFeature(mWorkerPrivate->GetJSContext(), this);
}

} 
} 
} 
