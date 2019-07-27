





#include "mozilla/dom/cache/ActorChild.h"

#include "mozilla/dom/cache/Feature.h"

namespace mozilla {
namespace dom {
namespace cache {

void
ActorChild::SetFeature(Feature* aFeature)
{
  
  
  
  if (mFeature) {
    MOZ_ASSERT(mFeature == aFeature);
    return;
  }

  mFeature = aFeature;
  if (mFeature) {
    mFeature->AddActor(this);
  }
}

void
ActorChild::RemoveFeature()
{
  if (mFeature) {
    mFeature->RemoveActor(this);
    mFeature = nullptr;
  }
}

Feature*
ActorChild::GetFeature() const
{
  return mFeature;
}

bool
ActorChild::FeatureNotified() const
{
  return mFeature && mFeature->Notified();
}

ActorChild::~ActorChild()
{
  MOZ_ASSERT(!mFeature);
}

} 
} 
} 
