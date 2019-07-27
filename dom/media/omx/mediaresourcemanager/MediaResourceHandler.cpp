





#include "MediaResourceHandler.h"
#include "mozilla/NullPtr.h"

namespace android {

MediaResourceHandler::MediaResourceHandler(const wp<ResourceListener> &aListener)
  : mListener(aListener)
  , mType(IMediaResourceManagerService::INVALID_RESOURCE_TYPE)
  , mWaitingResource(false)
{
}

MediaResourceHandler::~MediaResourceHandler()
{
  cancelResource();
}

bool
MediaResourceHandler::IsWaitingResource()
{
  Mutex::Autolock al(mLock);
  return mWaitingResource;
}

bool
MediaResourceHandler::requestResource(IMediaResourceManagerService::ResourceType aType)
{
  Mutex::Autolock al(mLock);

  if (mClient != nullptr && mService != nullptr) {
    return false;
  }

  sp<MediaResourceManagerClient> client = new MediaResourceManagerClient(this);
  sp<IMediaResourceManagerService> service = client->getMediaResourceManagerService();

  if (service == nullptr) {
    return false;
  }

  if (service->requestMediaResource(client, (int)aType, true) != OK) {
    return false;
  }

  mClient = client;
  mService = service;
  mType = aType;
  mWaitingResource = true;

  return true;
}

void
MediaResourceHandler::cancelResource()
{
  Mutex::Autolock al(mLock);

  if (mClient != nullptr && mService != nullptr) {
    mService->cancelClient(mClient, (int)mType);
  }

  mWaitingResource = false;
  mClient = nullptr;
  mService = nullptr;
}


void
MediaResourceHandler::statusChanged(int aEvent)
{
  sp<ResourceListener> listener;

  Mutex::Autolock autoLock(mLock);

  listener = mListener.promote();
  if (listener == nullptr) {
    return;
  }

  mWaitingResource = false;

  MediaResourceManagerClient::State state = (MediaResourceManagerClient::State)aEvent;
  if (state == MediaResourceManagerClient::CLIENT_STATE_RESOURCE_ASSIGNED) {
    listener->resourceReserved();
  } else {
    listener->resourceCanceled();
  }
}

} 
