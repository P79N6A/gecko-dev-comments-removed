






















#ifndef WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_H
#define WEBRTC_VOICE_ENGINE_VOE_ENCRYPTION_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VoEEncryption
{
public:
    
    
    
    static VoEEncryption* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    virtual int RegisterExternalEncryption(
        int channel, Encryption& encryption) = 0;

    
    
    virtual int DeRegisterExternalEncryption(int channel) = 0;

    
    virtual int EnableSRTPSend(int channel, CipherTypes cipherType,
        int cipherKeyLength, AuthenticationTypes authType, int authKeyLength,
        int authTagLength, SecurityLevels level, const unsigned char key[30],
        bool useForRTCP = false) = 0;

    
    virtual int DisableSRTPSend(int channel) = 0;

    
    virtual int EnableSRTPReceive(int channel, CipherTypes cipherType,
        int cipherKeyLength, AuthenticationTypes authType, int authKeyLength,
        int authTagLength, SecurityLevels level, const unsigned char key[30],
        bool useForRTCP = false) = 0;

    
    virtual int DisableSRTPReceive(int channel) = 0;

protected:
    VoEEncryption() {}
    virtual ~VoEEncryption() {}
};

}  

#endif  
