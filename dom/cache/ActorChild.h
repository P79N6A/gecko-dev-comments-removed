





#ifndef mozilla_dom_cache_ActioChild_h
#define mozilla_dom_cache_ActioChild_h

#include "nsRefPtr.h"

namespace mozilla {
namespace dom {
namespace cache {

class Feature;

class ActorChild
{
public:
  virtual void
  StartDestroy() = 0;

  void
  SetFeature(Feature* aFeature);

  void
  RemoveFeature();

  Feature*
  GetFeature() const;

  bool
  FeatureNotified() const;

protected:
  ~ActorChild();

private:
  nsRefPtr<Feature> mFeature;
};

} 
} 
} 

#endif 
