





#ifndef ANDROID_IMEDIARESOURCEMANAGERCLIENT_H
#define ANDROID_IMEDIARESOURCEMANAGERCLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>

namespace android {



class IMediaResourceManagerClient : public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaResourceManagerClient);

    
    virtual void statusChanged(int event) = 0;

};




class BnMediaResourceManagerClient : public BnInterface<IMediaResourceManagerClient>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};



}; 

#endif 
