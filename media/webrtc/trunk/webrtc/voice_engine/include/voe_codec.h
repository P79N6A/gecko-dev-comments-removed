





























#ifndef WEBRTC_VOICE_ENGINE_VOE_CODEC_H
#define WEBRTC_VOICE_ENGINE_VOE_CODEC_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;

class WEBRTC_DLLEXPORT VoECodec
{
public:
    
    
    
    static VoECodec* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    virtual int NumOfCodecs() = 0;

    
    virtual int GetCodec(int index, CodecInst& codec) = 0;

    
    virtual int SetSendCodec(int channel, const CodecInst& codec) = 0;

    
    
    virtual int GetSendCodec(int channel, CodecInst& codec) = 0;

    
    
    
    
    
    
    
    virtual int SetSecondarySendCodec(int channel, const CodecInst& codec,
                                      int red_payload_type) = 0;

    
    
    virtual int RemoveSecondarySendCodec(int channel) = 0;

    
    virtual int GetSecondarySendCodec(int channel, CodecInst& codec) = 0;

    
    virtual int GetRecCodec(int channel, CodecInst& codec) = 0;

    
    
    
    virtual int SetISACInitTargetRate(int channel, int rateBps,
                                      bool useFixedFrameSize = false) = 0;

    
    
    
    virtual int SetISACMaxRate(int channel, int rateBps) = 0;

    
    
    
    virtual int SetISACMaxPayloadSize(int channel, int sizeBytes) = 0;

    
    
    
    
    
    
    virtual int SetRecPayloadType(int channel, const CodecInst& codec) = 0;

    
    
    
    virtual int GetRecPayloadType(int channel, CodecInst& codec) = 0;

    
    
    virtual int SetSendCNPayloadType(
        int channel, int type, PayloadFrequencies frequency = kFreq16000Hz) = 0;


    
    
    
    virtual int SetVADStatus(int channel, bool enable,
                             VadModes mode = kVadConventional,
                             bool disableDTX = false) = 0;

    
    virtual int GetVADStatus(int channel, bool& enabled, VadModes& mode,
                             bool& disabledDTX) = 0;

    
    virtual int SetAMREncFormat(int channel, AmrMode mode) = 0;

    
    virtual int SetAMRDecFormat(int channel, AmrMode mode) = 0;

    
    virtual int SetAMRWbEncFormat(int channel, AmrMode mode) = 0;

    
    virtual int SetAMRWbDecFormat(int channel, AmrMode mode) = 0;

protected:
    VoECodec() {}
    virtual ~VoECodec() {}
};

} 

#endif  
