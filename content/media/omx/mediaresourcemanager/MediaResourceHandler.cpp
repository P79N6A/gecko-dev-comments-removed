





#include "MediaResourceHandler.h"

#include "mozilla/NullPtr.h"

namespace android {

MediaResourceHandler::MediaResourceHandler(const wp<ResourceListener> &aListener)
  : mListener(aListener)
  , mState(MediaResourceManagerClient::CLIENT_STATE_WAIT_FOR_RESOURCE)
  , mType(IMediaResourceManagerService::INVALID_RESOURCE_TYPE)
{
}

MediaResourceHandler::~MediaResourceHandler()
{
  cancelResource();
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

  return true;
}

void
MediaResourceHandler::cancelResource()
{
  Mutex::Autolock al(mLock);

  if (mClient != nullptr && mService != nullptr) {
    mService->cancelClient(mClient, (int)mType);
  }

  mClient = nullptr;
  mService = nullptr;
}


void
MediaResourceHandler::statusChanged(int aEvent)
{
  sp<ResourceListener> listener;

  Mutex::Autolock autoLock(mLock);

  MediaResourceManagerClient::State state = (MediaResourceManagerClient::State)aEvent;
  if (state == mState) {
    return;
  }

  mState = state;

  listener = mListener.promote();
  if (listener == nullptr) {
    return;
  }

  if (mState == MediaResourceManagerClient::CLIENT_STATE_RESOURCE_ASSIGNED) {
    listener->resourceReserved();
  } else {
    listener->resourceCanceled();
  }
}

} 
