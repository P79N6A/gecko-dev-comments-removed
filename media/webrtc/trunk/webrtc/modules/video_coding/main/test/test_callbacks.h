









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_TEST_CALLBACKS_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_TEST_CALLBACKS_H_






#include <string.h>
#include <cstdlib>
#include <fstream>
#include <list>

#include "module_common_types.h"
#include "rtp_rtcp.h"
#include "test_util.h"
#include "trace.h"
#include "video_coding.h"

namespace webrtc
{
class RtpDump;


class VCMEncodeCompleteCallback: public VCMPacketizationCallback
{
public:
    
    VCMEncodeCompleteCallback(FILE* encodedFile);
    virtual ~VCMEncodeCompleteCallback();
    
    void RegisterTransportCallback(VCMPacketizationCallback* transport);
    
    
    WebRtc_Word32 SendData(const FrameType frameType,
                           const WebRtc_UWord8 payloadType,
                           const WebRtc_UWord32 timeStamp,
                           int64_t capture_time_ms,
                           const WebRtc_UWord8* payloadData,
                           const WebRtc_UWord32 payloadSize,
                           const RTPFragmentationHeader& fragmentationHeader,
                           const RTPVideoHeader* videoHdr);
    
    void RegisterReceiverVCM(VideoCodingModule *vcm) {_VCMReceiver = vcm;}
    
    
    
    float EncodedBytes();
    
    bool EncodeComplete();
    
    void SetCodecType(RTPVideoCodecTypes codecType)
    {_codecType = codecType;}
    
    void SetFrameDimensions(WebRtc_Word32 width, WebRtc_Word32 height)
    {
        _width = width;
        _height = height;
    }
    
    void Initialize();
    void ResetByteCount();

    

private:
    FILE*              _encodedFile;
    float              _encodedBytes;
    VideoCodingModule* _VCMReceiver;
    FrameType          _frameType;
    WebRtc_UWord16     _seqNo;
    bool               _encodeComplete;
    WebRtc_Word32      _width;
    WebRtc_Word32      _height;
    RTPVideoCodecTypes _codecType;

}; 



class VCMRTPEncodeCompleteCallback: public VCMPacketizationCallback
{
public:
    VCMRTPEncodeCompleteCallback(RtpRtcp* rtp) :
        _encodedBytes(0),
        _encodeComplete(false),
        _RTPModule(rtp) {}

    virtual ~VCMRTPEncodeCompleteCallback() {}
    
    
    WebRtc_Word32 SendData(const FrameType frameType,
                           const WebRtc_UWord8 payloadType,
                           const WebRtc_UWord32 timeStamp,
                           int64_t capture_time_ms,
                           const WebRtc_UWord8* payloadData,
                           const WebRtc_UWord32 payloadSize,
                           const RTPFragmentationHeader& fragmentationHeader,
                           const RTPVideoHeader* videoHdr);
    
    
    float EncodedBytes();
    
    bool EncodeComplete();
    
    void SetCodecType(RTPVideoCodecTypes codecType)
    {_codecType = codecType;}

    
    void SetFrameDimensions(WebRtc_Word16 width, WebRtc_Word16 height)
    {
        _width = width;
        _height = height;
    }

private:
    float              _encodedBytes;
    FrameType          _frameType;
    bool               _encodeComplete;
    RtpRtcp*           _RTPModule;
    WebRtc_Word16      _width;
    WebRtc_Word16      _height;
    RTPVideoCodecTypes _codecType;
}; 



class VCMDecodeCompleteCallback: public VCMReceiveCallback
{
public:
    VCMDecodeCompleteCallback(FILE* decodedFile) :
        _decodedFile(decodedFile), _decodedBytes(0) {}
    virtual ~VCMDecodeCompleteCallback() {}
    
    WebRtc_Word32 FrameToRender(webrtc::I420VideoFrame& videoFrame);
    WebRtc_Word32 DecodedBytes();
private:
    FILE*               _decodedFile;
    WebRtc_UWord32      _decodedBytes;
}; 





class RTPSendCompleteCallback: public Transport
{
public:
    
    RTPSendCompleteCallback(TickTimeBase* clock,
                            const char* filename = NULL);
    virtual ~RTPSendCompleteCallback();

    void SetRtpModule(RtpRtcp* rtp_module) { _rtp = rtp_module; }
    
    virtual int SendPacket(int channel, const void *data, int len);
    
    virtual int SendRTCPPacket(int channel, const void *data, int len);
    
    void SetLossPct(double lossPct);
    
    void SetBurstLength(double burstLength);
    
    void SetNetworkDelay(WebRtc_UWord32 networkDelayMs)
                        {_networkDelayMs = networkDelayMs;};
    
    void SetJitterVar(WebRtc_UWord32 jitterVar)
                      {_jitterVar = jitterVar;};
    
    int SendCount() {return _sendCount; }
    
    WebRtc_UWord32 TotalSentLength() {return _totalSentLength;}
protected:
    
    bool PacketLoss();
    
    bool UnifomLoss(double lossPct);

    TickTimeBase*           _clock;
    WebRtc_UWord32          _sendCount;
    RtpRtcp*                _rtp;
    double                  _lossPct;
    double                  _burstLength;
    WebRtc_UWord32          _networkDelayMs;
    double                  _jitterVar;
    bool                    _prevLossState;
    WebRtc_UWord32          _totalSentLength;
    std::list<RtpPacket*>   _rtpPackets;
    RtpDump*                _rtpDump;
};


class PacketRequester: public VCMPacketRequestCallback
{
public:
    PacketRequester(RtpRtcp& rtp) :
        _rtp(rtp) {}
    WebRtc_Word32 ResendPackets(const WebRtc_UWord16* sequenceNumbers,
            WebRtc_UWord16 length);
private:
    webrtc::RtpRtcp& _rtp;
};


class KeyFrameReqTest: public VCMFrameTypeCallback
{
public:
    WebRtc_Word32 RequestKeyFrame();
};



class SendStatsTest: public webrtc::VCMSendStatisticsCallback
{
public:
    SendStatsTest() : _frameRate(15) {}
    WebRtc_Word32 SendStatistics(const WebRtc_UWord32 bitRate,
            const WebRtc_UWord32 frameRate);
    void SetTargetFrameRate(WebRtc_UWord32 frameRate) {_frameRate = frameRate;}
private:
    WebRtc_UWord32 _frameRate;
};



class VideoProtectionCallback: public VCMProtectionCallback
{
public:
    VideoProtectionCallback();
    virtual ~VideoProtectionCallback();
    void RegisterRtpModule(RtpRtcp* rtp) {_rtp = rtp;}
    WebRtc_Word32 ProtectionRequest(
        const FecProtectionParams* delta_fec_params,
        const FecProtectionParams* key_fec_params,
        WebRtc_UWord32* sent_video_rate_bps,
        WebRtc_UWord32* sent_nack_rate_bps,
        WebRtc_UWord32* sent_fec_rate_bps);
    FecProtectionParams DeltaFecParameters() const;
    FecProtectionParams KeyFecParameters() const;
private:
    RtpRtcp* _rtp;
    FecProtectionParams delta_fec_params_;
    FecProtectionParams key_fec_params_;
};
}  
#endif
