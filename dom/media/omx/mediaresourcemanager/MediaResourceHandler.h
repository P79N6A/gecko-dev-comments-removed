





#ifndef MEDIA_RESOURCE_HANDLER_H
#define MEDIA_RESOURCE_HANDLER_H

#include <utils/threads.h>

#include <mozilla/Attributes.h>

#include "MediaResourceManagerClient.h"

namespace android {

class MediaResourceHandler : public MediaResourceManagerClient::EventListener
{
public:
  


  struct ResourceListener : public virtual RefBase {
    


    virtual void resourceReserved() = 0;
    



    virtual void resourceCanceled() = 0;
  };

  MediaResourceHandler(const wp<ResourceListener> &aListener);

  virtual ~MediaResourceHandler();

  
  bool requestResource(IMediaResourceManagerService::ResourceType aType);
  
  void cancelResource();

  bool IsWaitingResource();

protected:
  
  virtual void statusChanged(int event);

private:
  
  MediaResourceHandler() MOZ_DELETE;
  MediaResourceHandler(const MediaResourceHandler &) MOZ_DELETE;
  const MediaResourceHandler &operator=(const MediaResourceHandler &) MOZ_DELETE;

  
  wp<ResourceListener> mListener;

  
  Mutex mLock;
  sp<IMediaResourceManagerClient> mClient;
  sp<IMediaResourceManagerService> mService;
  IMediaResourceManagerService::ResourceType mType;

  bool mWaitingResource;
};

} 

#endif 
