





#ifndef mozilla_MediaParent_h
#define mozilla_MediaParent_h

#include "MediaChild.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/media/PMediaParent.h"

namespace mozilla {
namespace media {



class ParentSingleton;

class Parent : public PMediaParent
{
  NS_INLINE_DECL_REFCOUNTING(Parent)
public:
  virtual bool RecvGetOriginKey(const int& aRequestId,
                                const nsCString& aOrigin,
                                const bool& aPrivateBrowsing) override;
  virtual bool RecvSanitizeOriginKeys(const uint64_t& aSinceWhen) override;
  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  Parent();
private:
  virtual ~Parent();

  nsRefPtr<ParentSingleton> mSingleton;
  bool mDestroyed;
};

PMediaParent* AllocPMediaParent();
bool DeallocPMediaParent(PMediaParent *aActor);

} 
} 

#endif  
