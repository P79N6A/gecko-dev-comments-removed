





#ifndef ANDROID_MEDIARESOURCEMANAGERSERVICE_H
#define ANDROID_MEDIARESOURCEMANAGERSERVICE_H

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandlerReflector.h>
#include <media/stagefright/foundation/ALooper.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>

#include "IMediaResourceManagerClient.h"
#include "IMediaResourceManagerService.h"

namespace android {






class MediaResourceManagerService: public BnMediaResourceManagerService,
                                   public IBinder::DeathRecipient
{
public:
  
  enum
  {
    VIDEO_DECODER_COUNT = 1,
    VIDEO_ENCODER_COUNT = 1
  };

  enum
  {
    kNotifyRequest = 'noti',
  };

  static const char* kMsgKeyResourceType;

  
  
  static  void instantiate();

  
  virtual void binderDied(const wp<IBinder>& who);

  
  virtual status_t requestMediaResource(const sp<IMediaResourceManagerClient>& client,
                                        int resourceType, bool willWait);
  virtual status_t cancelClient(const sp<IMediaResourceManagerClient>& client,
                                int resourceType);

  
  
  void onMessageReceived(const sp<AMessage> &msg);

protected:
  MediaResourceManagerService();
  virtual ~MediaResourceManagerService();

private:
  
  
  struct ResourceSlot
  {
    sp<IBinder> mClient;
  };
  typedef Vector<ResourceSlot> Slots;

  typedef List<sp<IBinder> > Fifo;
  struct Resources
  {
    
    
    Fifo mRequestQueue;
    
    
    Slots mSlots;
  };

  typedef KeyedVector<ResourceType, Resources> ResourcesMap;
  
  class ResourceTable
  {
    ResourceTable();
    ~ResourceTable();
    
    bool supportsType(ResourceType type);
    ssize_t findAvailableResource(ResourceType type, size_t number_needed = 1);
    bool isOwnedByClient(const sp<IBinder>& client, ResourceType type, size_t index);
    status_t aquireResource(const sp<IBinder>& client, ResourceType type, size_t index);
    ResourceSlot* resourceOfTypeAt(ResourceType type, size_t index);
    
    bool hasRequest(ResourceType type);
    uint32_t countRequests(ResourceType type);
    const sp<IBinder>& nextRequest(ResourceType type);
    status_t enqueueRequest(const sp<IBinder>& client, ResourceType type);
    status_t dequeueRequest(ResourceType type);
    status_t forgetClient(const sp<IBinder>& client, ResourceType type);
    status_t forgetClient(const sp<IBinder>& client);

    friend class MediaResourceManagerService;

    
    ResourcesMap mMap;
  };

  void cancelClientLocked(const sp<IBinder>& binder, ResourceType resourceType);

  
  
  
  
  sp<ALooper> mLooper;
  
  
  
  sp<AHandlerReflector<MediaResourceManagerService> > mReflector;

  
  Mutex mLock;

  
  ResourceTable mResources;
};

}; 

#endif
