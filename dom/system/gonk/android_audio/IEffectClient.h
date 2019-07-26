















#ifndef ANDROID_IEFFECTCLIENT_H
#define ANDROID_IEFFECTCLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>

namespace android {

class IEffectClient: public IInterface
{
public:
    DECLARE_META_INTERFACE(EffectClient);

    virtual void controlStatusChanged(bool controlGranted) = 0;
    virtual void enableStatusChanged(bool enabled) = 0;
    virtual void commandExecuted(uint32_t cmdCode,
                                 uint32_t cmdSize,
                                 void *pCmdData,
                                 uint32_t replySize,
                                 void *pReplyData) = 0;
};



class BnEffectClient: public BnInterface<IEffectClient>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; 

#endif 
