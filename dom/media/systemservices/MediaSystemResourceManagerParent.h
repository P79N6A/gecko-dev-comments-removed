




#if !defined(MediaSystemResourceManagerParent_h_)
#define MediaSystemResourceManagerParent_h_

#include "MediaSystemResourceManager.h"
#include "MediaSystemResourceService.h"
#include "MediaSystemResourceTypes.h"
#include "mozilla/media/PMediaSystemResourceManagerParent.h"

namespace mozilla {
namespace media {




class MediaSystemResourceManagerParent final : public PMediaSystemResourceManagerParent
{
public:
  MediaSystemResourceManagerParent();
  virtual ~MediaSystemResourceManagerParent();

protected:
  bool RecvAcquire(const uint32_t& aId,
                   const MediaSystemResourceType& aResourceType,
                   const bool& aWillWait) override;

  bool RecvRelease(const uint32_t& aId) override;

  bool RecvRemoveResourceManager() override;

private:
  void ActorDestroy(ActorDestroyReason aActorDestroyReason) override;

  struct MediaSystemResourceRequest {
    MediaSystemResourceRequest()
      : mId(-1), mResourceType(MediaSystemResourceType::INVALID_RESOURCE) {}
    MediaSystemResourceRequest(uint32_t aId, MediaSystemResourceType aResourceType)
      : mId(aId), mResourceType(aResourceType) {}
    int32_t mId;
    MediaSystemResourceType mResourceType;
  };

  bool mDestroyed;

  nsRefPtr<MediaSystemResourceService> mMediaSystemResourceService;

  nsClassHashtable<nsUint32HashKey, MediaSystemResourceRequest> mResourceRequests;
};

} 
} 

#endif
