





#ifndef mozilla_dom_cache_Feature_h
#define mozilla_dom_cache_Feature_h

#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "WorkerFeature.h"

namespace mozilla {

namespace workers {
class WorkerPrivate;
}

namespace dom {
namespace cache {

class ActorChild;

class Feature MOZ_FINAL : public workers::WorkerFeature
{
public:
  static already_AddRefed<Feature> Create(workers::WorkerPrivate* aWorkerPrivate);

  void AddActor(ActorChild* aActor);
  void RemoveActor(ActorChild* aActor);

  bool Notified() const;

  
  virtual bool Notify(JSContext* aCx, workers::Status aStatus) MOZ_OVERRIDE;

private:
  explicit Feature(workers::WorkerPrivate *aWorkerPrivate);
  ~Feature();

  workers::WorkerPrivate* mWorkerPrivate;
  nsTArray<ActorChild*> mActorList;
  bool mNotified;

public:
  NS_INLINE_DECL_REFCOUNTING(mozilla::dom::cache::Feature)
};

} 
} 
} 

#endif
