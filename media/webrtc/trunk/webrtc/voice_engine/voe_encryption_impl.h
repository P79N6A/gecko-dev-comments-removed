









#ifndef WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_IMPL_H

#include "voe_encryption.h"

#include "shared_data.h"

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
