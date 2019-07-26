















#ifndef ANDROID_IAUDIOTRACK_H
#define ANDROID_IAUDIOTRACK_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>


namespace android {



class IAudioTrack : public IInterface
{
public: 
    DECLARE_META_INTERFACE(AudioTrack);

    


    virtual status_t    start() = 0;

    



    virtual void        stop() = 0;

    


    virtual void        flush() = 0;

    


    virtual void        mute(bool) = 0;
    
    



    virtual void        pause() = 0;

    


    virtual status_t    attachAuxEffect(int effectId) = 0;

    
    virtual sp<IMemory> getCblk() const = 0;    
};



class BnAudioTrack : public BnInterface<IAudioTrack>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};



}; 

#endif 
