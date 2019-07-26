





#ifndef ANDROID_IMEDIARESOURCEMANAGERSERVICE_H
#define ANDROID_IMEDIARESOURCEMANAGERSERVICE_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>

#include "IMediaResourceManagerClient.h"


namespace android {



class IMediaResourceManagerService : public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaResourceManagerService);

    
    enum ResourceType {
      HW_VIDEO_DECODER = 0,
      HW_AUDIO_DECODER,  
      HW_VIDEO_ENCODER,
      HW_CAMERA,          
      NUM_OF_RESOURCE_TYPES,
      INVALID_RESOURCE_TYPE = -1
    };

    enum ErrorCode {
        RESOURCE_NOT_AVAILABLE = -EAGAIN
    };

    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t requestMediaResource(const sp<IMediaResourceManagerClient>& client, int resourceType, bool willWait) = 0;
    
    
    virtual status_t cancelClient(const sp<IMediaResourceManagerClient>& client, int resourceType) = 0;
};




class BnMediaResourceManagerService : public BnInterface<IMediaResourceManagerService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};



}; 

#endif 
