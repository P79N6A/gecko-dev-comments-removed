





#if !defined(MediaSystemResourceClient_h_)
#define MediaSystemResourceClient_h_

#include "MediaSystemResourceManager.h"
#include "MediaSystemResourceTypes.h"
#include "mozilla/Atomics.h"
#include "mozilla/media/MediaSystemResourceTypes.h"
#include "mozilla/Monitor.h"
#include "nsRefPtr.h"

namespace mozilla {

class MediaSystemResourceManager;







class MediaSystemResourceReservationListener {
public:
  virtual void ResourceReserved() = 0;
  virtual void ResourceReserveFailed() = 0;
};






class MediaSystemResourceClient
{
public:

  
  enum ResourceState {
    RESOURCE_STATE_START,
    RESOURCE_STATE_WAITING,
    RESOURCE_STATE_ACQUIRED,
    RESOURCE_STATE_NOT_ACQUIRED,
    RESOURCE_STATE_END
  };

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaSystemResourceClient)

  explicit MediaSystemResourceClient(MediaSystemResourceType aReourceType);

  bool SetListener(MediaSystemResourceReservationListener* aListener);

  
  
  void Acquire();

  
  
  
  
  
  
  
  bool AcquireSyncNoWait();

  void ReleaseResource();

private:
  ~MediaSystemResourceClient();

  nsRefPtr<MediaSystemResourceManager> mManager;
  const MediaSystemResourceType mResourceType;
  const uint32_t mId;

  
  
  MediaSystemResourceReservationListener* mListener;
  ResourceState mResourceState;
  bool mIsSync;
  ReentrantMonitor* mAcquireSyncWaitMonitor;
  bool* mAcquireSyncWaitDone;

  static mozilla::Atomic<uint32_t> sSerialCounter;

  friend class MediaSystemResourceManager;
};

} 

#endif
