





#ifndef mozilla_MediaParent_h
#define mozilla_MediaParent_h

#include "MediaChild.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/media/PMediaParent.h"

namespace mozilla {
namespace media {




class OriginKeyStore;

class NonE10s
{
  typedef mozilla::ipc::IProtocolManager<mozilla::ipc::IProtocol>::ActorDestroyReason
      ActorDestroyReason;
protected:
  virtual bool RecvGetOriginKey(const uint32_t& aRequestId,
                                const nsCString& aOrigin,
                                const bool& aPrivateBrowsing) = 0;
  virtual bool RecvSanitizeOriginKeys(const uint64_t& aSinceWhen) = 0;
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) = 0;
};



template<class Super>
class Parent : public Super
{
  typedef mozilla::ipc::IProtocolManager<mozilla::ipc::IProtocol>::ActorDestroyReason
      ActorDestroyReason;
public:
  static Parent* GetSingleton();

  virtual bool RecvGetOriginKey(const uint32_t& aRequestId,
                                const nsCString& aOrigin,
                                const bool& aPrivateBrowsing) override;
  virtual bool RecvSanitizeOriginKeys(const uint64_t& aSinceWhen) override;
  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  explicit Parent(bool aSameProcess = false);
  virtual ~Parent();
private:

  nsRefPtr<OriginKeyStore> mOriginKeyStore;
  bool mDestroyed;
  bool mSameProcess;

  CoatCheck<Pledge<nsCString>> mOutstandingPledges;
};

PMediaParent* AllocPMediaParent();
bool DeallocPMediaParent(PMediaParent *aActor);

} 
} 

#endif  
