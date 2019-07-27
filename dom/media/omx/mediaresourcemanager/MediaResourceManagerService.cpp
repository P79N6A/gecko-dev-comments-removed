






#define LOG_TAG "MediaResourceManagerService"

#include <mozilla/Assertions.h>

#include <binder/IServiceManager.h>
#include <media/stagefright/foundation/AMessage.h>
#include <utils/Log.h>

#include "MediaResourceManagerClient.h"
#include "MediaResourceManagerService.h"

namespace android {

const char* MediaResourceManagerService::kMsgKeyResourceType = "res-type";


void MediaResourceManagerService::instantiate() {
  defaultServiceManager()->addService(
            String16("media.resource_manager"),
            new MediaResourceManagerService());
}

MediaResourceManagerService::MediaResourceManagerService()
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
    Mutex::Autolock autoLock(mLock);
    sp<IBinder> binder = who.promote();
    if (binder != NULL) {
      mResources.forgetClient(binder);
    }
  }
}

status_t MediaResourceManagerService::requestMediaResource(const sp<IMediaResourceManagerClient>& client,
                                                           int resourceType, bool willWait)
{
  ResourceType type = static_cast<ResourceType>(resourceType);
  
  switch (type) {
    case HW_VIDEO_DECODER:
    case HW_VIDEO_ENCODER:
      break;
    default:
      
      return BAD_TYPE;
  }

  Mutex::Autolock autoLock(mLock);

  
  
  if (!willWait &&
      (mResources.findAvailableResource(type, mResources.countRequests(type) + 1) ==
       NAME_NOT_FOUND)) {
    return RESOURCE_NOT_AVAILABLE;
  }
  
  
  
  

  sp<IBinder> binder = client->asBinder();
  mResources.enqueueRequest(binder, type);
  binder->linkToDeath(this);

  sp<AMessage> notify = new AMessage(kNotifyRequest, mReflector->id());
  notify->setInt32(kMsgKeyResourceType, resourceType);
  
  notify->post();

  return OK;
}

status_t MediaResourceManagerService::cancelClient(const sp<IMediaResourceManagerClient>& client,
                                                   int resourceType)
{
  Mutex::Autolock autoLock(mLock);

  sp<IBinder> binder = client->asBinder();
  cancelClientLocked(binder, static_cast<ResourceType>(resourceType));

  sp<AMessage> notify = new AMessage(kNotifyRequest, mReflector->id());
  notify->setInt32(kMsgKeyResourceType, resourceType);
  
  
  
  
  notify->post();

  return NO_ERROR;
}


static int32_t getResourceType(const sp<AMessage>& message)
{
  int32_t resourceType = MediaResourceManagerService::INVALID_RESOURCE_TYPE;
  return message->findInt32(MediaResourceManagerService::kMsgKeyResourceType, &resourceType) ?
          resourceType : MediaResourceManagerService::INVALID_RESOURCE_TYPE;
}


void MediaResourceManagerService::onMessageReceived(const sp<AMessage> &msg)
{
  Mutex::Autolock autoLock(mLock);
  ResourceType type = static_cast<ResourceType>(getResourceType(msg));

  
  
  
  

  
  
  int found = mResources.findAvailableResource(type);
  if (found == NAME_NOT_FOUND) {
    return;
  }

  
  if (!mResources.hasRequest(type)) {
    return;
  }

  const sp<IBinder>& req = mResources.nextRequest(type);
  mResources.aquireResource(req, type, found);
  
  sp<IMediaResourceManagerClient> client = interface_cast<IMediaResourceManagerClient>(req);
  client->statusChanged(MediaResourceManagerClient::CLIENT_STATE_RESOURCE_ASSIGNED);
  mResources.dequeueRequest(type);
}

void MediaResourceManagerService::cancelClientLocked(const sp<IBinder>& binder,
                                                     ResourceType resourceType)
{
  mResources.forgetClient(binder, resourceType);
  binder->unlinkToDeath(this);
}

MediaResourceManagerService::ResourceTable::ResourceTable()
{
  
  for (int type = 0; type < NUM_OF_RESOURCE_TYPES; type++) {
    ssize_t index = mMap.add(static_cast<ResourceType>(type), Resources());
    Resources& resources = mMap.editValueAt(index);
    int available;
    switch (type) {
      case HW_VIDEO_DECODER:
        available = VIDEO_DECODER_COUNT;
        break;
      case HW_VIDEO_ENCODER:
        available = VIDEO_ENCODER_COUNT;
        break;
      default:
        available = 0;
        break;
    }
    resources.mSlots.insertAt(0, available);
  }
}

MediaResourceManagerService::ResourceTable::~ResourceTable() {
  
  mMap.clear();
}

bool MediaResourceManagerService::ResourceTable::supportsType(ResourceType type)
{
  return mMap.indexOfKey(type) != NAME_NOT_FOUND;
}

ssize_t MediaResourceManagerService::ResourceTable::findAvailableResource(ResourceType type,
                                                                          size_t numberNeeded)
{
  MOZ_ASSERT(numberNeeded > 0);
  ssize_t found = mMap.indexOfKey(type);
  if (found == NAME_NOT_FOUND) {
    
    return found;
  }
  const Slots& slots = mMap.valueAt(found).mSlots;

  found = NAME_NOT_FOUND;
  for (size_t i = 0; i < slots.size(); i++) {
    if (slots[i].mClient != nullptr) {
      
      continue;
    }
    if (--numberNeeded == 0) {
      found = i;
      break;
    }
  }

  return found;
}

bool MediaResourceManagerService::ResourceTable::isOwnedByClient(const sp<IBinder>& client,
                                                                 ResourceType type,
                                                                 size_t index)
{
  ResourceSlot* slot = resourceOfTypeAt(type, index);
  return slot && slot->mClient == client;
}

status_t MediaResourceManagerService::ResourceTable::aquireResource(const sp<IBinder>& client,
                                                                    ResourceType type,
                                                                    size_t index)
{
  ResourceSlot* slot = resourceOfTypeAt(type, index);
  
  MOZ_ASSERT(slot && slot->mClient == nullptr);
  if (!slot) {
    return NAME_NOT_FOUND;
  } else if (slot->mClient != nullptr) {
    
    return PERMISSION_DENIED;
  }

  slot->mClient = client;


  return OK;
}

MediaResourceManagerService::ResourceSlot*
MediaResourceManagerService::ResourceTable::resourceOfTypeAt(ResourceType type,
                                                             size_t index)
{
  ssize_t found = mMap.indexOfKey(type);
  if (found == NAME_NOT_FOUND) {
    
    return nullptr;
  }

  Slots& slots = mMap.editValueAt(found).mSlots;
  MOZ_ASSERT(index < slots.size());
  if (index >= slots.size()) {
    
    return nullptr;
  }
  return &(slots.editItemAt(index));
}

bool MediaResourceManagerService::ResourceTable::hasRequest(ResourceType type)
{
  ssize_t found = mMap.indexOfKey(type);
  if (found == NAME_NOT_FOUND) {
    
    return nullptr;
  }

  const Fifo& queue = mMap.valueAt(found).mRequestQueue;
  return !queue.empty();
}

uint32_t MediaResourceManagerService::ResourceTable::countRequests(ResourceType type)
{
  ssize_t found = mMap.indexOfKey(type);
  if (found == NAME_NOT_FOUND) {
    
    return 0;
  }

  const Fifo& queue = mMap.valueAt(found).mRequestQueue;
  return queue.size();
}

const sp<IBinder>& MediaResourceManagerService::ResourceTable::nextRequest(ResourceType type)
{
  ssize_t found = mMap.indexOfKey(type);
  if (found == NAME_NOT_FOUND) {
    
    return nullptr;
  }

  const Fifo& queue = mMap.valueAt(found).mRequestQueue;
  return *(queue.begin());
}

status_t MediaResourceManagerService::ResourceTable::enqueueRequest(const sp<IBinder>& client,
                                                                    ResourceType type)
{
  ssize_t found = mMap.indexOfKey(type);
  if (found == NAME_NOT_FOUND) {
    
    return found;
  }

  mMap.editValueAt(found).mRequestQueue.push_back(client);
  return OK;
}

status_t MediaResourceManagerService::ResourceTable::dequeueRequest(ResourceType type)
{
  ssize_t found = mMap.indexOfKey(type);
  if (found == NAME_NOT_FOUND) {
    
    return found;
  }

  Fifo& queue = mMap.editValueAt(found).mRequestQueue;
  queue.erase(queue.begin());
  return OK;
}

status_t MediaResourceManagerService::ResourceTable::forgetClient(const sp<IBinder>& client)
{
  
  for (int i = 0; i < mMap.size(); i++) {
    forgetClient(client, mMap.keyAt(i));
  }
  return OK;
}

status_t MediaResourceManagerService::ResourceTable::forgetClient(const sp<IBinder>& client, ResourceType type)
{
  MOZ_ASSERT(supportsType(type));

  Resources& resources = mMap.editValueFor(type);

  
  Fifo& queue = resources.mRequestQueue;
  Fifo::iterator it(queue.begin());
  while (it != queue.end()) {
    if ((*it).get() == client.get()) {
      queue.erase(it);
      break;
    }
    it++;
  }

  
  Slots& slots = resources.mSlots;
  for (int i = 0; i < slots.size(); i++) {
    ResourceSlot& slot = slots.editItemAt(i);
    if (client.get() == slot.mClient.get()) {
      slot.mClient = nullptr;
    }
  }

  return OK;
}

}; 
