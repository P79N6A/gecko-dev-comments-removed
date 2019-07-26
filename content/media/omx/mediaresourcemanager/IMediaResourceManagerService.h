





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

    
    virtual void requestMediaResource(const sp<IMediaResourceManagerClient>& client, int resourceType) = 0;
    
    virtual status_t cancelClient(const sp<IMediaResourceManagerClient>& client) = 0;
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
