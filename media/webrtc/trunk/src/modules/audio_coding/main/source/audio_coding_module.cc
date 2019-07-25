










#include "acm_dtmf_detection.h"
#include "audio_coding_module.h"
#include "audio_coding_module_impl.h"
#include "trace.h"

namespace webrtc
{


AudioCodingModule*
AudioCodingModule::Create(
    const WebRtc_Word32 id)
{
    return new AudioCodingModuleImpl(id);
}


void
AudioCodingModule::Destroy(
        AudioCodingModule* module)
{
    delete static_cast<AudioCodingModuleImpl*> (module);
}


WebRtc_UWord8 AudioCodingModule::NumberOfCodecs()
{
    return static_cast<WebRtc_UWord8>(ACMCodecDB::kNumCodecs);
}


WebRtc_Word32
AudioCodingModule::Codec(
    const WebRtc_UWord8 listId,
    CodecInst&          codec)
{
    
    return ACMCodecDB::Codec(listId, &codec);
}


WebRtc_Word32
AudioCodingModule::Codec(
    const char* payloadName,
    CodecInst&          codec,
    const WebRtc_Word32 samplingFreqHz)
{
    
    for(int codecCntr = 0; codecCntr < ACMCodecDB::kNumCodecs; codecCntr++)
    {
        
        ACMCodecDB::Codec(codecCntr, &codec);

        if(!STR_CASE_CMP(codec.plname, payloadName))
        {
            
            if((samplingFreqHz == codec.plfreq) || (samplingFreqHz == -1))
            {
                
                return 0;
            }
        }
    }

    
    
    codec.plname[0] = '\0';
    codec.pltype    = -1;
    codec.pacsize   = 0;
    codec.rate      = 0;
    codec.plfreq    = 0;
    return -1;
}


WebRtc_Word32
AudioCodingModule::Codec(
    const char* payloadName,
    const WebRtc_Word32 samplingFreqHz)
{
    CodecInst codec;

    
    for(int codecCntr = 0; codecCntr < ACMCodecDB::kNumCodecs; codecCntr++)
    {
        
        ACMCodecDB::Codec(codecCntr, &codec);

        if(!STR_CASE_CMP(codec.plname, payloadName))
        {
            
            if((samplingFreqHz == codec.plfreq) || (samplingFreqHz == -1))
            {
                
                return codecCntr;
            }
        }
    }

    
    return -1;
}


bool
AudioCodingModule::IsCodecValid(
    const CodecInst& codec)
{
    int mirrorID;
    char errMsg[500];

    int codecNumber = ACMCodecDB::CodecNumber(&codec, &mirrorID, errMsg, 500);

    if(codecNumber < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, -1, errMsg);
        return false;
    }
    else
    {
        return true;
    }
}

} 
