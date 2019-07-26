





#ifndef ANDROID_MEDIARESOURCEMANAGERSERVICE_H
#define ANDROID_MEDIARESOURCEMANAGERSERVICE_H

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandlerReflector.h>
#include <media/stagefright/foundation/ALooper.h>
#include <utils/List.h>
#include <utils/RefBase.h>

#include "IMediaResourceManagerClient.h"
#include "IMediaResourceManagerService.h"

namespace android {






class MediaResourceManagerService: public BnMediaResourceManagerService,
                                   public IBinder::DeathRecipient
{
public:
  
  enum { VIDEO_DECODER_COUNT = 1 };

  enum {
    kNotifyRequest = 'noti'
  };

  
  
  static  void instantiate();

  
  virtual void binderDied(const wp<IBinder>& who);

  
  virtual void requestMediaResource(const sp<IMediaResourceManagerClient>& client, int resourceType);
  virtual status_t cancelClient(const sp<IMediaResourceManagerClient>& client);

  
  
  void onMessageReceived(const sp<AMessage> &msg);

protected:
  MediaResourceManagerService();
  virtual ~MediaResourceManagerService();

protected:
  
  
  struct ResourceSlot {
    ResourceSlot ()
      {
      }
      sp<IBinder> mClient;
    };

  void cancelClientLocked(const sp<IBinder>& binder);

  
  ResourceSlot mVideoDecoderSlots[VIDEO_DECODER_COUNT];
  
  int mVideoDecoderCount;

  
  
  Mutex mLock;
  typedef List<sp<IBinder> > Fifo;
  
  
  Fifo mVideoCodecRequestQueue;

  
  
  
  
  sp<ALooper> mLooper;
  
  
  
  sp<AHandlerReflector<MediaResourceManagerService> > mReflector;

};

}; 

#endif
