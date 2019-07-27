





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
public:
  virtual bool RecvGetOriginKey(const nsCString& aOrigin,
                                const bool& aPrivateBrowsing,
                                nsCString* aKey) override;
  virtual bool RecvSanitizeOriginKeys(const uint64_t& aSinceWhen) override;
  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  Parent();
  virtual ~Parent();
private:
  nsRefPtr<ParentSingleton> mSingleton;
};

PMediaParent* CreateParent();

} 
} 

#endif  
