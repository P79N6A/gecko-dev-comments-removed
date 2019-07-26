









#include "coder.h"
#include "common_types.h"
#include "module_common_types.h"


#ifdef WIN32
    #define STR_CASE_CMP(x,y) ::_stricmp(x,y)
#else
    #define STR_CASE_CMP(x,y) ::strcasecmp(x,y)
#endif

namespace webrtc {
AudioCoder::AudioCoder(uint32_t instanceID)
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

int32_t AudioCoder::SetEncodeCodec(const CodecInst& codecInst,
                                   ACMAMRPackingFormat amrFormat)
{
    if(_acm->RegisterSendCodec((CodecInst&)codecInst) == -1)
    {
        return -1;
    }
    return 0;
}

int32_t AudioCoder::SetDecodeCodec(const CodecInst& codecInst,
                                   ACMAMRPackingFormat amrFormat)
{
    if(_acm->RegisterReceiveCodec((CodecInst&)codecInst) == -1)
    {
        return -1;
    }
    memcpy(&_receiveCodec,&codecInst,sizeof(CodecInst));
    return 0;
}

int32_t AudioCoder::Decode(AudioFrame& decodedAudio,
                           uint32_t sampFreqHz,
                           const int8_t*  incomingPayload,
                           int32_t  payloadLength)
{
    if (payloadLength > 0)
    {
        const uint8_t payloadType = _receiveCodec.pltype;
        _decodeTimestamp += _receiveCodec.pacsize;
        if(_acm->IncomingPayload((const uint8_t*) incomingPayload,
                                 payloadLength,
                                 payloadType,
                                 _decodeTimestamp) == -1)
        {
            return -1;
        }
    }
    return _acm->PlayoutData10Ms((uint16_t)sampFreqHz, &decodedAudio);
}

int32_t AudioCoder::PlayoutData(AudioFrame& decodedAudio,
                                uint16_t& sampFreqHz)
{
    return _acm->PlayoutData10Ms(sampFreqHz, &decodedAudio);
}

int32_t AudioCoder::Encode(const AudioFrame& audio,
                           int8_t* encodedData,
                           uint32_t& encodedLengthInBytes)
{
    
    
    AudioFrame audioFrame;
    audioFrame.CopyFrom(audio);
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

int32_t AudioCoder::SendData(
    FrameType ,
    uint8_t   ,
    uint32_t  ,
    const uint8_t*  payloadData,
    uint16_t  payloadSize,
    const RTPFragmentationHeader* )
{
    memcpy(_encodedData,payloadData,sizeof(uint8_t) * payloadSize);
    _encodedLengthInBytes = payloadSize;
    return 0;
}
} 
