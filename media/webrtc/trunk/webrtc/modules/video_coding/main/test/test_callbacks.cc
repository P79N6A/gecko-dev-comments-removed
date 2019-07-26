









#include "test_callbacks.h"

#include <cmath>

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/main/source/tick_time_base.h"
#include "rtp_dump.h"
#include "test_macros.h"

namespace webrtc {







VCMEncodeCompleteCallback::VCMEncodeCompleteCallback(FILE* encodedFile):
    _encodedFile(encodedFile),
    _encodedBytes(0),
    _VCMReceiver(NULL),
    _seqNo(0),
    _encodeComplete(false),
    _width(0),
    _height(0),
    _codecType(kRTPVideoNoVideo)
{
    
}
VCMEncodeCompleteCallback::~VCMEncodeCompleteCallback()
{
}

void
VCMEncodeCompleteCallback::RegisterTransportCallback(
                                            VCMPacketizationCallback* transport)
{
}

WebRtc_Word32
VCMEncodeCompleteCallback::SendData(
        const FrameType frameType,
        const WebRtc_UWord8  payloadType,
        const WebRtc_UWord32 timeStamp,
        int64_t capture_time_ms,
        const WebRtc_UWord8* payloadData,
        const WebRtc_UWord32 payloadSize,
        const RTPFragmentationHeader& fragmentationHeader,
        const RTPVideoHeader* videoHdr)
{
    
    _frameType = frameType;
    
    if (fwrite(payloadData, 1, payloadSize, _encodedFile) !=  payloadSize) {
      return -1;
    }
    WebRtcRTPHeader rtpInfo;
    rtpInfo.header.markerBit = true; 
    rtpInfo.type.Video.isFirstPacket = true;
    rtpInfo.type.Video.codec = _codecType;
    rtpInfo.type.Video.height = (WebRtc_UWord16)_height;
    rtpInfo.type.Video.width = (WebRtc_UWord16)_width;
    switch (_codecType)
    {
    case webrtc::kRTPVideoVP8:
        rtpInfo.type.Video.codecHeader.VP8.InitRTPVideoHeaderVP8();
        rtpInfo.type.Video.codecHeader.VP8.nonReference =
            videoHdr->codecHeader.VP8.nonReference;
        rtpInfo.type.Video.codecHeader.VP8.pictureId =
            videoHdr->codecHeader.VP8.pictureId;
        break;
    case webrtc::kRTPVideoI420:
        break;
    default:
        assert(false);
        return -1;
    }

    rtpInfo.header.payloadType = payloadType;
    rtpInfo.header.sequenceNumber = _seqNo++;
    rtpInfo.header.ssrc = 0;
    rtpInfo.header.timestamp = timeStamp;
    rtpInfo.frameType = frameType;
    
    

    _encodedBytes += payloadSize;
    
    int ret = _VCMReceiver->IncomingPacket(payloadData, payloadSize, rtpInfo);
    _encodeComplete = true;

    return ret;
}

float
VCMEncodeCompleteCallback::EncodedBytes()
{
    return _encodedBytes;
}

bool
VCMEncodeCompleteCallback::EncodeComplete()
{
    if (_encodeComplete)
    {
        _encodeComplete = false;
        return true;
    }
    return false;
}

void
VCMEncodeCompleteCallback::Initialize()
{
    _encodeComplete = false;
    _encodedBytes = 0;
    _seqNo = 0;
    return;
}

void
VCMEncodeCompleteCallback::ResetByteCount()
{
    _encodedBytes = 0;
}








WebRtc_Word32
VCMRTPEncodeCompleteCallback::SendData(
        const FrameType frameType,
        const WebRtc_UWord8  payloadType,
        const WebRtc_UWord32 timeStamp,
        int64_t capture_time_ms,
        const WebRtc_UWord8* payloadData,
        const WebRtc_UWord32 payloadSize,
        const RTPFragmentationHeader& fragmentationHeader,
        const RTPVideoHeader* videoHdr)
{
    _frameType = frameType;
    _encodedBytes+= payloadSize;
    _encodeComplete = true;
    return _RTPModule->SendOutgoingData(frameType,
                                        payloadType,
                                        timeStamp,
                                        capture_time_ms,
                                        payloadData,
                                        payloadSize,
                                        &fragmentationHeader,
                                        videoHdr);
}

float
VCMRTPEncodeCompleteCallback::EncodedBytes()
{
    
    float tmp = _encodedBytes;
    _encodedBytes = 0;
    return tmp;
 }

bool
VCMRTPEncodeCompleteCallback::EncodeComplete()
{
    if (_encodeComplete)
    {
        _encodeComplete = false;
        return true;
    }
    return false;
}



WebRtc_Word32
VCMDecodeCompleteCallback::FrameToRender(I420VideoFrame& videoFrame)
{
  if (PrintI420VideoFrame(videoFrame, _decodedFile) < 0) {
    return -1;
  }
  _decodedBytes+= CalcBufferSize(kI420, videoFrame.width(),
                                 videoFrame.height());
  return VCM_OK;
 }

WebRtc_Word32
VCMDecodeCompleteCallback::DecodedBytes()
{
    return _decodedBytes;
}

RTPSendCompleteCallback::RTPSendCompleteCallback(TickTimeBase* clock,
                                                 const char* filename):
    _clock(clock),
    _sendCount(0),
    _rtp(NULL),
    _lossPct(0),
    _burstLength(0),
    _networkDelayMs(0),
    _jitterVar(0),
    _prevLossState(0),
    _totalSentLength(0),
    _rtpPackets(),
    _rtpDump(NULL)
{
    if (filename != NULL)
    {
        _rtpDump = RtpDump::CreateRtpDump();
        _rtpDump->Start(filename);
    }
}

RTPSendCompleteCallback::~RTPSendCompleteCallback()
{
    if (_rtpDump != NULL)
    {
        _rtpDump->Stop();
        RtpDump::DestroyRtpDump(_rtpDump);
    }
    
    while (!_rtpPackets.empty())
    {
        
        delete _rtpPackets.front();
        _rtpPackets.pop_front();
    }
}

int
RTPSendCompleteCallback::SendPacket(int channel, const void *data, int len)
{
    _sendCount++;
    _totalSentLength += len;

    if (_rtpDump != NULL)
    {
        if (_rtpDump->DumpPacket((const WebRtc_UWord8*)data, len) != 0)
        {
            return -1;
        }
    }

    bool transmitPacket = true;
    transmitPacket = PacketLoss();

    WebRtc_UWord64 now = _clock->MillisecondTimestamp();
    
    if (transmitPacket)
    {
        RtpPacket* newPacket = new RtpPacket();
        memcpy(newPacket->data, data, len);
        newPacket->length = len;
        
        
        
        WebRtc_Word32
        simulatedDelay = (WebRtc_Word32)NormalDist(_networkDelayMs,
                                                   sqrt(_jitterVar));
        newPacket->receiveTime = now + simulatedDelay;
        _rtpPackets.push_back(newPacket);
    }

    
    RtpPacket* packet = NULL;

    while (!_rtpPackets.empty())
    {
        
        packet = _rtpPackets.front();
        WebRtc_Word64 timeToReceive = packet->receiveTime - now;
        if (timeToReceive > 0)
        {
            
            break;
        }

        _rtpPackets.pop_front();
        assert(_rtp);  
        
        if (_rtp->IncomingPacket((const WebRtc_UWord8*)packet->data,
                                 packet->length) < 0)
        {
            delete packet;
            packet = NULL;
            
            return -1;
        }
        delete packet;
        packet = NULL;
    }
    return len; 
}

int
RTPSendCompleteCallback::SendRTCPPacket(int channel, const void *data, int len)
{
    
    return SendPacket(channel, data, len);
}

void
RTPSendCompleteCallback::SetLossPct(double lossPct)
{
    _lossPct = lossPct;
    return;
}

void
RTPSendCompleteCallback::SetBurstLength(double burstLength)
{
    _burstLength = burstLength;
    return;
}

bool
RTPSendCompleteCallback::PacketLoss()
{
    bool transmitPacket = true;
    if (_burstLength <= 1.0)
    {
        
        if (UnifomLoss(_lossPct))
        {
            
            transmitPacket = false;
        }
    }
    else
    {
        
        
        

        
        

        
        

        
        

        
        
        double probTrans10 = 100 * (1.0 / _burstLength);
        double probTrans11 = (100.0 - probTrans10);
        double probTrans01 = (probTrans10 * ( _lossPct / (100.0 - _lossPct)));

        
        

        if (_prevLossState == 0 )
        {
            
            if (UnifomLoss(probTrans01))
            {
                
                _prevLossState = 1;
                transmitPacket = false;
            }
        }
        else if (_prevLossState == 1)
        {
            _prevLossState = 0;
            
            if (UnifomLoss(probTrans11))
            {
                
                _prevLossState = 1;
                transmitPacket = false;
             }
        }
    }
    return transmitPacket;
}


bool
RTPSendCompleteCallback::UnifomLoss(double lossPct)
{
    double randVal = (std::rand() + 1.0)/(RAND_MAX + 1.0);
    return randVal < lossPct/100;
}

WebRtc_Word32
PacketRequester::ResendPackets(const WebRtc_UWord16* sequenceNumbers,
                               WebRtc_UWord16 length)
{
    return _rtp.SendNACK(sequenceNumbers, length);
}

WebRtc_Word32
SendStatsTest::SendStatistics(const WebRtc_UWord32 bitRate,
                              const WebRtc_UWord32 frameRate)
{
    TEST(frameRate <= _frameRate);
    TEST(bitRate > 0 && bitRate < 100000);
    printf("VCM 1 sec: Bit rate: %u\tFrame rate: %u\n", bitRate, frameRate);
    return 0;
}

WebRtc_Word32 KeyFrameReqTest::RequestKeyFrame() {
  printf("Key frame requested\n");
  return 0;
}


VideoProtectionCallback::VideoProtectionCallback():
delta_fec_params_(),
key_fec_params_()
{
    memset(&delta_fec_params_, 0, sizeof(delta_fec_params_));
    memset(&key_fec_params_, 0, sizeof(key_fec_params_));
}

VideoProtectionCallback::~VideoProtectionCallback()
{
    
}

WebRtc_Word32
VideoProtectionCallback::ProtectionRequest(
    const FecProtectionParams* delta_fec_params,
    const FecProtectionParams* key_fec_params,
    WebRtc_UWord32* sent_video_rate_bps,
    WebRtc_UWord32* sent_nack_rate_bps,
    WebRtc_UWord32* sent_fec_rate_bps)
{
    key_fec_params_ = *key_fec_params;
    delta_fec_params_ = *delta_fec_params;

    
    if (_rtp->SetFecParameters(&delta_fec_params_,
                               &key_fec_params_) != 0)
    {
        printf("Error in Setting FEC rate\n");
        return -1;

    }
    return 0;

}

FecProtectionParams VideoProtectionCallback::DeltaFecParameters() const
{
    return delta_fec_params_;
}

FecProtectionParams VideoProtectionCallback::KeyFecParameters() const
{
    return key_fec_params_;
}
}  
