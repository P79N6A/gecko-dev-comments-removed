









#include "coder.h"
#include "common_types.h"
#include "module_common_types.h"


#ifdef WIN32
    #define STR_CASE_CMP(x,y) ::_stricmp(x,y)
#else
    #define STR_CASE_CMP(x,y) ::strcasecmp(x,y)
#endif

namespace webrtc {
AudioCoder::AudioCoder(WebRtc_UWord32 instanceID)
    : _acm(AudioCodingModule::Create(instanceID)),
      _receiveCodec(),
      _encodeTimestamp(0),
      _encodedData(NULL),
      _encodedLengthInBytes(0),
      _decodeTimestamp(0)
{
    _acm->InitializeSender();
    _acm->InitializeReceiver();
    _acm->RegisterTransportCallback(this);
}

AudioCoder::~AudioCoder()
{
    AudioCodingModule::Destroy(_acm);
}

WebRtc_Word32 AudioCoder::SetEncodeCodec(const CodecInst& codecInst,
					 ACMAMRPackingFormat amrFormat)
{
    if(_acm->RegisterSendCodec((CodecInst&)codecInst) == -1)
    {
        return -1;
    }
    return 0;
}

WebRtc_Word32 AudioCoder::SetDecodeCodec(const CodecInst& codecInst,
					 ACMAMRPackingFormat amrFormat)
{
    if(_acm->RegisterReceiveCodec((CodecInst&)codecInst) == -1)
    {
        return -1;
    }
    memcpy(&_receiveCodec,&codecInst,sizeof(CodecInst));
    return 0;
}

WebRtc_Word32 AudioCoder::Decode(AudioFrame& decodedAudio,
				 WebRtc_UWord32 sampFreqHz,
				 const WebRtc_Word8*  incomingPayload,
				 WebRtc_Word32  payloadLength)
{
    if (payloadLength > 0)
    {
        const WebRtc_UWord8 payloadType = _receiveCodec.pltype;
        _decodeTimestamp += _receiveCodec.pacsize;
        if(_acm->IncomingPayload((const WebRtc_UWord8*) incomingPayload,
                                 payloadLength,
                                 payloadType,
                                 _decodeTimestamp) == -1)
        {
            return -1;
        }
    }
    return _acm->PlayoutData10Ms((WebRtc_UWord16)sampFreqHz,
				 (AudioFrame&)decodedAudio);
}

WebRtc_Word32 AudioCoder::PlayoutData(AudioFrame& decodedAudio,
				      WebRtc_UWord16& sampFreqHz)
{
    return _acm->PlayoutData10Ms(sampFreqHz, (AudioFrame&)decodedAudio);
}

WebRtc_Word32 AudioCoder::Encode(const AudioFrame& audio,
				 WebRtc_Word8* encodedData,
				 WebRtc_UWord32& encodedLengthInBytes)
{
    
    
    AudioFrame audioFrame = audio;
    audioFrame.timestamp_ = _encodeTimestamp;
    _encodeTimestamp += audioFrame.samples_per_channel_;

    
    
    _encodedLengthInBytes = 0;
    if(_acm->Add10MsData((AudioFrame&)audioFrame) == -1)
    {
        return -1;
    }
    _encodedData = encodedData;
    if(_acm->Process() == -1)
    {
        return -1;
    }
    encodedLengthInBytes = _encodedLengthInBytes;
    return 0;
}

WebRtc_Word32 AudioCoder::SendData(
    FrameType ,
    WebRtc_UWord8   ,
    WebRtc_UWord32  ,
    const WebRtc_UWord8*  payloadData,
    WebRtc_UWord16  payloadSize,
    const RTPFragmentationHeader* )
{
    memcpy(_encodedData,payloadData,sizeof(WebRtc_UWord8) * payloadSize);
    _encodedLengthInBytes = payloadSize;
    return 0;
}
} 
