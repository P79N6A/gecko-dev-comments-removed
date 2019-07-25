















#ifndef ANDROID_HARDWARE_ICAMERA_APP_H
#define ANDROID_HARDWARE_ICAMERA_APP_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <utils/Timers.h>

namespace android {

class ICameraClient: public IInterface
{
public:
    DECLARE_META_INTERFACE(CameraClient);

    virtual void            notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2) = 0;
    virtual void            dataCallback(int32_t msgType, const sp<IMemory>& data) = 0;
    virtual void            dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& data) = 0;
};



class BnCameraClient: public BnInterface<ICameraClient>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; 

#endif
