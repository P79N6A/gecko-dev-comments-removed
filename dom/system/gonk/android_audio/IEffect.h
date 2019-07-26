















#ifndef ANDROID_IEFFECT_H
#define ANDROID_IEFFECT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>

namespace android {

class IEffect: public IInterface
{
public:
    DECLARE_META_INTERFACE(Effect);

    virtual status_t enable() = 0;

    virtual status_t disable() = 0;

    virtual status_t command(uint32_t cmdCode,
                             uint32_t cmdSize,
                             void *pCmdData,
                             uint32_t *pReplySize,
                             void *pReplyData) = 0;

    virtual void disconnect() = 0;

    virtual sp<IMemory> getCblk() const = 0;
};



class BnEffect: public BnInterface<IEffect>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; 

#endif 
