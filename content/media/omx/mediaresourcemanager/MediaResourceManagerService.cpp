






#define LOG_TAG "MediaResourceManagerService"

#include <binder/IServiceManager.h>
#include <media/stagefright/foundation/AMessage.h>
#include <utils/Log.h>

#include "MediaResourceManagerClient.h"
#include "MediaResourceManagerService.h"

namespace android {

int waitBeforeAdding(const android::String16& serviceName)
{
  android::sp<android::IServiceManager> sm = android::defaultServiceManager();
  for ( int i = 0 ; i < 5; i++ ) {
    if ( sm->checkService ( serviceName ) != NULL ) {
      sleep(1);
    }
    else {
      
      return 0;
    }
  }
  
 return -1;
}


void
waitServiceManager()
{
  android::sp<android::IServiceManager> sm;
  do {
    sm = android::defaultServiceManager();
    if (sm.get()) {
      break;
    }
    usleep(50000); 
  } while(true);
}


void MediaResourceManagerService::instantiate() {
  waitServiceManager();
  waitBeforeAdding( android::String16("media.resource_manager") );

  defaultServiceManager()->addService(
            String16("media.resource_manager"), new MediaResourceManagerService());
}

MediaResourceManagerService::MediaResourceManagerService()
  : mVideoDecoderCount(VIDEO_DECODER_COUNT)
{
  mLooper = new ALooper;
  mLooper->setName("MediaResourceManagerService");

  mReflector = new AHandlerReflector<MediaResourceManagerService>(this);
  
  mLooper->registerHandler(mReflector);
  
  mLooper->start();
}

MediaResourceManagerService::~MediaResourceManagerService()
{
  
  mLooper->unregisterHandler(mReflector->id());
  
  mLooper->stop();
}

void MediaResourceManagerService::binderDied(const wp<IBinder>& who)
{
  if (who != NULL) {
    sp<IBinder> binder = who.promote();
    if (binder != NULL) {
      cancelClientLocked(binder);
    }
  }
}

void MediaResourceManagerService::requestMediaResource(const sp<IMediaResourceManagerClient>& client, int resourceType)
{
  if (resourceType != MediaResourceManagerClient::HW_VIDEO_DECODER) {
    
    return;
  }

  {
    Mutex::Autolock autoLock(mLock);
    sp<IBinder> binder = client->asBinder();
    mVideoCodecRequestQueue.push_back(binder);
    binder->linkToDeath(this);
  }

  sp<AMessage> notify =
            new AMessage(kNotifyRequest, mReflector->id());
  
  notify->post();
}

status_t MediaResourceManagerService::cancelClient(const sp<IMediaResourceManagerClient>& client)
{
  {
    Mutex::Autolock autoLock(mLock);
    sp<IBinder> binder = client->asBinder();
    cancelClientLocked(binder);
  }

  sp<AMessage> notify =
            new AMessage(kNotifyRequest, mReflector->id());
  
  notify->post();

  return NO_ERROR;
}


void MediaResourceManagerService::onMessageReceived(const sp<AMessage> &msg)
{
  Mutex::Autolock autoLock(mLock);

  
  if (mVideoCodecRequestQueue.empty()) {
    return;
  }

  
  int found = -1;
  for (int i=0 ; i<mVideoDecoderCount ; i++) {
    if (!mVideoDecoderSlots[i].mClient.get()) {
      found = i;
    }
  }

  
  if (found == -1) {
    return;
  }

  
  Fifo::iterator front(mVideoCodecRequestQueue.begin());
  mVideoDecoderSlots[found].mClient = *front;
  mVideoCodecRequestQueue.erase(front);
  
  sp<IMediaResourceManagerClient> client = interface_cast<IMediaResourceManagerClient>(mVideoDecoderSlots[found].mClient);
  client->statusChanged(MediaResourceManagerClient::CLIENT_STATE_RESOURCE_ASSIGNED);
}

void MediaResourceManagerService::cancelClientLocked(const sp<IBinder>& binder)
{
  
  Fifo::iterator it(mVideoCodecRequestQueue.begin());
  while (it != mVideoCodecRequestQueue.end()) {
    if (*it == binder) {
      it = mVideoCodecRequestQueue.erase(it);
      continue;
    }
    it++;
  }

  
  for (int i=0 ; i<mVideoDecoderCount ; i++) {
    if (mVideoDecoderSlots[i].mClient == binder) {
      mVideoDecoderSlots[i].mClient = NULL;
    }
  }
  binder->unlinkToDeath(this);
}

}; 

