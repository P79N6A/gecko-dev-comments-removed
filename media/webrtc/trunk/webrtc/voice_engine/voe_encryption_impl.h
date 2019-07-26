









#ifndef WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_IMPL_H

#include "webrtc/voice_engine/include/voe_encryption.h"

#include "webrtc/voice_engine/shared_data.h"

namespace webrtc {

class VoEEncryptionImpl : public VoEEncryption
{
public:
    
    virtual int RegisterExternalEncryption(
        int channel,
        Encryption& encryption);

    virtual int DeRegisterExternalEncryption(int channel);

protected:
    VoEEncryptionImpl(voe::SharedData* shared);
    virtual ~VoEEncryptionImpl();

private:
    voe::SharedData* _shared;
};

}  

#endif  
