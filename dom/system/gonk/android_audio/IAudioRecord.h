















#ifndef IAUDIORECORD_H_
#define IAUDIORECORD_H_

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>


namespace android {



class IAudioRecord : public IInterface
{
public: 
    DECLARE_META_INTERFACE(AudioRecord);

    


    virtual status_t    start() = 0;

    



    virtual void        stop() = 0;

    
    virtual sp<IMemory> getCblk() const = 0;    
};



class BnAudioRecord : public BnInterface<IAudioRecord>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};



}; 

#endif 
