





#ifndef mozilla_MediaChild_h
#define mozilla_MediaChild_h

#include "mozilla/dom/ContentChild.h"
#include "mozilla/media/PMediaChild.h"
#include "mozilla/media/PMediaParent.h"
#include "MediaUtils.h"

namespace mozilla {
namespace media {










already_AddRefed<Pledge<nsCString>>
GetOriginKey(const nsCString& aOrigin, bool aPrivateBrowsing);

void
SanitizeOriginKeys(const uint64_t& aSinceWhen);

class Child : public PMediaChild
{
public:
  static Child* Get();

  Child();

  bool RecvGetOriginKeyResponse(const uint32_t& aRequestId, const nsCString& aKey) override;

  void ActorDestroy(ActorDestroyReason aWhy) override;
  virtual ~Child();
private:

  bool mActorDestroyed;
};

PMediaChild* AllocPMediaChild();
bool DeallocPMediaChild(PMediaChild *aActor);

} 
} 

#endif  
