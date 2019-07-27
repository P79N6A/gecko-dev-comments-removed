




#if !defined(MediaSystemResourceManagerChild_h_)
#define MediaSystemResourceManagerChild_h_

#include "mozilla/media/PMediaSystemResourceManagerChild.h"
#include "nsISupportsImpl.h"

namespace mozilla {

class MediaSystemResourceManager;

namespace ipc {
class BackgroundChildImpl;
}

namespace media {




class MediaSystemResourceManagerChild final : public PMediaSystemResourceManagerChild
{
public:
  struct ResourceListener {
    


    virtual void resourceReserved() = 0;
    



    virtual void resourceCanceled() = 0;
  };

  MediaSystemResourceManagerChild();
  virtual ~MediaSystemResourceManagerChild();

  void Destroy();

  void SetManager(MediaSystemResourceManager* aManager)
  {
    mManager = aManager;
  }

protected:
  bool RecvResponse(const uint32_t& aId,
                    const bool& aSuccess) override;

private:
  void ActorDestroy(ActorDestroyReason aActorDestroyReason) override;

  bool mDestroyed;
  MediaSystemResourceManager* mManager;

  friend class mozilla::ipc::BackgroundChildImpl;
};

} 
} 

#endif
