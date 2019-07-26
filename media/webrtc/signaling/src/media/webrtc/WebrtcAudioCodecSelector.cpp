






































#ifndef _USE_CPVE

#include "CC_Common.h"
#include "WebrtcLogging.h"

#include "WebrtcAudioCodecSelector.h"
#include "voe_codec.h"
#include "csf_common.h"

static const char* logTag = "AudioCodecSelector";

using namespace std;
using namespace CSF;

typedef struct _CSFCodecMapping
{
    AudioPayloadType csfAudioPayloadType;
    WebrtcAudioPayloadType webrtcAudioPayloadType;
} CSFCodecMapping;

static CSFCodecMapping WebrtcMappingInfo[] =
{
    { AudioPayloadType_G711ALAW64K,       WebrtcAudioPayloadType_PCMA },
    { AudioPayloadType_G711ALAW56K,       WebrtcAudioPayloadType_PCMA },
    { AudioPayloadType_G711ULAW64K,       WebrtcAudioPayloadType_PCMU },
    { AudioPayloadType_G711ULAW56K,       WebrtcAudioPayloadType_PCMU },
    { AudioPayloadType_G722_56K,          WebrtcAudioPayloadType_G722 },
    { AudioPayloadType_G722_64K,          WebrtcAudioPayloadType_G722 },
    { AudioPayloadType_G722_48K,          WebrtcAudioPayloadType_G722 },
    { AudioPayloadType_RFC2833,           WebrtcAudioPayloadType_TELEPHONE_EVENT }
};

WebrtcAudioCodecSelector::WebrtcAudioCodecSelector()
    : voeCodec(NULL)
{
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::constructor" );
}

WebrtcAudioCodecSelector::~WebrtcAudioCodecSelector()
{
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::destructor" );
    release();
}

void WebrtcAudioCodecSelector::release()
{
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::release" );

    
    if (voeCodec != NULL)
    {
        if (voeCodec->Release() != 0)
        {
            LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::release voeCodec->Release() failed" );
        }
        else
        {
            LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::release voeCodec released" );
        }

        voeCodec = NULL;

        std::map<int, webrtc::CodecInst*>::iterator iterVoeCodecs;
        for( iterVoeCodecs = codecMap.begin(); iterVoeCodecs != codecMap.end(); ++iterVoeCodecs ) {
            delete iterVoeCodecs->second;
        }

        codecMap.clear();
    }
}

int WebrtcAudioCodecSelector::init( webrtc::VoiceEngine* voeVoice, bool useLowBandwidthCodecOnly, bool advertiseG722Codec )
{
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init useLowBandwidthCodecOnly=%d, advertiseG722Codec=%d", useLowBandwidthCodecOnly, advertiseG722Codec );

    voeCodec = webrtc::VoECodec::GetInterface( voeVoice );

    if (voeCodec == NULL)
    {
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init cannot get reference to VOE codec interface" );
        return -1;
    }

    
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init clearing map" );
    codecMap.clear();

    
    int numOfSupportedCodecs = voeCodec->NumOfCodecs();

    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init found %d supported codec(s)", numOfSupportedCodecs );

    
    for (int codecIndex = 0; codecIndex < numOfSupportedCodecs; codecIndex++)
    {
        webrtc::CodecInst supportedCodec;

        if (voeCodec->GetCodec(codecIndex, supportedCodec) == -1)
        {
            LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init codecIndex=%d: cannot get supported codec information", codecIndex );
            continue;
        }

        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init codecIndex=%d: channels=%d, pacsize=%d, plfreq=%d, plname=%s, pltype=%d, rate=%d",
                codecIndex, supportedCodec.channels, supportedCodec.pacsize, supportedCodec.plfreq,
                supportedCodec.plname, supportedCodec.pltype, supportedCodec.rate );

        
        for (int i=0; i< (int) csf_countof(WebrtcMappingInfo); i++)
        {
            WebrtcAudioPayloadType webrtcPayload = WebrtcMappingInfo[i].webrtcAudioPayloadType;

            if (supportedCodec.pltype == webrtcPayload)
            {
                bool addCodec = false;

                AudioPayloadType csfPayload = WebrtcMappingInfo[i].csfAudioPayloadType;

                switch (csfPayload)
                {
                case AudioPayloadType_G711ALAW64K:
                case AudioPayloadType_G711ULAW64K:
                    if (!useLowBandwidthCodecOnly)
                    {
                        addCodec = true;
                    }
                    break;

                case AudioPayloadType_G722_56K:
                case AudioPayloadType_G722_64K:
                    if (!useLowBandwidthCodecOnly &&
                        advertiseG722Codec)
                    {
                        addCodec =  true;
                    }
                    break;

                
                
                
                
                
                
                
                
                

                case AudioPayloadType_RFC2833:
                    addCodec =  true;
                    break;

                case AudioPayloadType_G711ALAW56K:
                case AudioPayloadType_G711ULAW56K:
                case AudioPayloadType_G722_48K:
                case AudioPayloadType_ILBC20:
                case AudioPayloadType_ILBC30:
                case AudioPayloadType_ISAC:
                  break;
                    
                } 

                if (addCodec)
                {
                    
                    webrtc::CodecInst* mappedCodec = new webrtc::CodecInst; 
                    memcpy(mappedCodec, &supportedCodec, sizeof(webrtc::CodecInst));

                    codecMap[csfPayload] = mappedCodec;

                    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init added mapping payload %d to VoE codec %s", csfPayload, mappedCodec->plname);
                }
                else
                {
                    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init no mapping found for VoE codec %s (payload %d)", supportedCodec.plname, webrtcPayload );
                }
            }
        } 

    } 

    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::init %d codec(s) added to map", (int)codecMap.size() );

    
    return 0;
}

int  WebrtcAudioCodecSelector::advertiseCodecs( CodecRequestType requestType )
{
    return AudioCodecMask_G711  |AudioCodecMask_G722;
}

int WebrtcAudioCodecSelector::select( int payloadType, int dynamicPayloadType, int packPeriod, webrtc::CodecInst& selectedCodec )
{
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::select payloadType=%d, dynamicPayloadType=%d, packPeriod=%d", payloadType, dynamicPayloadType, packPeriod );

    
    
    int packetSize = packPeriod;

    webrtc::CodecInst* supportedCodec = codecMap[payloadType];

    if (supportedCodec == NULL)
    {
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::select no VoE codec found for payload %d", payloadType);
        return -1; 
    }

    memcpy(&selectedCodec, supportedCodec, sizeof(webrtc::CodecInst));

    
    if (dynamicPayloadType != -1)
    {
        selectedCodec.pltype = dynamicPayloadType;
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::select pltype = %d", selectedCodec.pltype);
    }

    
    int pacsize;    

    switch ( payloadType )
    {
    case AudioPayloadType_G711ALAW64K:
    case AudioPayloadType_G711ULAW64K:
        
        
        pacsize = ( 8 * packetSize );
        if (( pacsize == 80 ) || ( pacsize == 160 ) ||
            ( pacsize == 240) || ( pacsize == 320 ) ||
            ( pacsize == 400) || ( pacsize == 480 ))
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    case AudioPayloadType_ILBC20:
        
        pacsize = ( 8 * packetSize );
        if ( pacsize == 160 )
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    case AudioPayloadType_ILBC30:
        
        pacsize = ( 8 * packetSize );
        if ( pacsize == 240 )
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    case AudioPayloadType_G722_56K:
    case AudioPayloadType_G722_64K:
        
        pacsize = ( 16 * packetSize );
        if ( pacsize == 320 )
        {
            selectedCodec.pacsize = pacsize;
        }
        break;

    default:
        break;
    }

    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::select found codec %s (payload=%d, packetSize=%d)",
            selectedCodec.plname, selectedCodec.pltype, selectedCodec.pacsize);

    
    return 0;
}

int WebrtcAudioCodecSelector::setSend(int channel, const webrtc::CodecInst& codec,int payloadType,bool vad)
{
    webrtc::PayloadFrequencies freq;
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setSend channel=%d codec %s (payload=%d, packetSize=%d)",
            channel, codec.plname, codec.pltype, codec.pacsize);

    if (voeCodec == NULL)
    {
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setSend voeCodec is null" );
        return -1;
    }
    bool vadEnable=vad;

    if (voeCodec->SetVADStatus( channel,  vadEnable,  webrtc::kVadConventional,false) != 0)
    {
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::SetVADStatus cannot set VAD  to channel %d", channel );
        return -1;
    }
    
    
    switch (codec.plfreq)
    {
        case SamplingFreq8000Hz:
            freq=webrtc::kFreq8000Hz;
        break;
        case SamplingFreq16000Hz:
            freq=webrtc::kFreq16000Hz;
            break;
       case SamplingFreq32000Hz:
            freq=webrtc::kFreq32000Hz;
            break;            
       default:
            freq=webrtc::kFreq8000Hz;

    }
    
    if (voeCodec->SetSendCNPayloadType(channel, ComfortNoisePayloadType,  freq) != 0)
    {
        LOG_WEBRTC_INFO( logTag, "SetSendCNPayloadType cannot set CN payload type  to channel %d", channel );
        
    }
    
    if (voeCodec->SetSendCodec(channel, codec) != 0)
    {
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setSend cannot set send codec to channel %d", channel );
        return -1;
    }

    
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setSend applied codec %s (payload=%d, packetSize=%d) to channel %d",
            codec.plname, codec.pltype, codec.pacsize, channel);

    
    return 0;
}

int WebrtcAudioCodecSelector::setReceive(int channel, const webrtc::CodecInst& codec)
{
    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setReceive channel=%d codec %s (payload=%d, packetSize=%d)",
            channel, codec.plname, codec.pltype, codec.pacsize);

    if (voeCodec == NULL)
    {
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setSend voeCodec is null" );
        return -1;
    }

    if (voeCodec->SetRecPayloadType(channel, codec) != 0)
    {
        LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setReceive cannot set receive codec to channel %d", channel );
        return -1;
    }

    LOG_WEBRTC_INFO( logTag, "WebrtcAudioCodecSelector::setReceive applied codec %s (payload=%d, packetSize=%d) to channel %d",
            codec.plname, codec.pltype, codec.pacsize, channel);

    
    return 0;
}

#endif
