















#ifndef ANDROID_IAUDIOFLINGERCLIENT_H
#define ANDROID_IAUDIOFLINGERCLIENT_H


#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <utils/KeyedVector.h>

namespace android {



class IAudioFlingerClient : public IInterface
{
public:
    DECLARE_META_INTERFACE(AudioFlingerClient);

    
    virtual void ioConfigChanged(int event, int ioHandle, void *param2) = 0;

};




class BnAudioFlingerClient : public BnInterface<IAudioFlingerClient>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};



}; 

#endif 
