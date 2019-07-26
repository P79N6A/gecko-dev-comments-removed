









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_TEST_CALLBACKS_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_TEST_CALLBACKS_H_






#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <list>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/test/test_util.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc
{
class RTPPayloadRegistry;
class RtpDump;


class VCMEncodeCompleteCallback: public VCMPacketizationCallback
{
public:
    
    VCMEncodeCompleteCallback(FILE* encodedFile);
    virtual ~VCMEncodeCompleteCallback();
    
    void RegisterTransportCallback(VCMPacketizationCallback* transport);
    
    
    int32_t SendData(const FrameType frameType,
                           const uint8_t payloadType,
                           const uint32_t timeStamp,
                           int64_t capture_time_ms,
                           const uint8_t* payloadData,
                           const uint32_t payloadSize,
                           const RTPFragmentationHeader& fragmentationHeader,
                           const RTPVideoHeader* videoHdr);
    
    void RegisterReceiverVCM(VideoCodingModule *vcm) {_VCMReceiver = vcm;}
    
    
    
    float EncodedBytes();
    
    bool EncodeComplete();
    
    void SetCodecType(RtpVideoCodecTypes codecType)
    {_codecType = codecType;}
    
    void SetFrameDimensions(int32_t width, int32_t height)
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
    uint16_t     _seqNo;
    bool               _encodeComplete;
    int32_t      _width;
    int32_t      _height;
    RtpVideoCodecTypes _codecType;

}; 



class VCMRTPEncodeCompleteCallback: public VCMPacketizationCallback
{
public:
    VCMRTPEncodeCompleteCallback(RtpRtcp* rtp) :
        _encodedBytes(0),
        _encodeComplete(false),
        _RTPModule(rtp) {}

    virtual ~VCMRTPEncodeCompleteCallback() {}
    
    
    int32_t SendData(const FrameType frameType,
                           const uint8_t payloadType,
                           const uint32_t timeStamp,
                           int64_t capture_time_ms,
                           const uint8_t* payloadData,
                           const uint32_t payloadSize,
                           const RTPFragmentationHeader& fragmentationHeader,
                           const RTPVideoHeader* videoHdr);
    
    
    float EncodedBytes();
    
    bool EncodeComplete();
    
    void SetCodecType(RtpVideoCodecTypes codecType)
    {_codecType = codecType;}

    
    void SetFrameDimensions(int16_t width, int16_t height)
    {
        _width = width;
        _height = height;
    }

private:
    float              _encodedBytes;
    FrameType          _frameType;
    bool               _encodeComplete;
    RtpRtcp*           _RTPModule;
    int16_t      _width;
    int16_t      _height;
    RtpVideoCodecTypes _codecType;
}; 



class VCMDecodeCompleteCallback: public VCMReceiveCallback
{
public:
    VCMDecodeCompleteCallback(FILE* decodedFile) :
        _decodedFile(decodedFile), _decodedBytes(0) {}
    virtual ~VCMDecodeCompleteCallback() {}
    
    int32_t FrameToRender(webrtc::I420VideoFrame& videoFrame);
    int32_t DecodedBytes();
private:
    FILE*               _decodedFile;
    uint32_t      _decodedBytes;
}; 





class RTPSendCompleteCallback: public Transport
{
public:
    
    RTPSendCompleteCallback(Clock* clock,
                            const char* filename = NULL);
    virtual ~RTPSendCompleteCallback();

    void SetRtpModule(RtpRtcp* rtp_module) { _rtp = rtp_module; }
    
    virtual int SendPacket(int channel, const void *data, int len);
    
    virtual int SendRTCPPacket(int channel, const void *data, int len);
    
    void SetLossPct(double lossPct);
    
    void SetBurstLength(double burstLength);
    
    void SetNetworkDelay(uint32_t networkDelayMs)
                        {_networkDelayMs = networkDelayMs;};
    
    void SetJitterVar(uint32_t jitterVar)
                      {_jitterVar = jitterVar;};
    
    int SendCount() {return _sendCount; }
    
    uint32_t TotalSentLength() {return _totalSentLength;}
protected:
    
    bool PacketLoss();
    
    bool UnifomLoss(double lossPct);

    Clock*                  _clock;
    uint32_t          _sendCount;
    RTPPayloadRegistry* rtp_payload_registry_;
    RtpReceiver* rtp_receiver_;
    RtpRtcp*                _rtp;
    double                  _lossPct;
    double                  _burstLength;
    uint32_t          _networkDelayMs;
    double                  _jitterVar;
    bool                    _prevLossState;
    uint32_t          _totalSentLength;
    std::list<RtpPacket*>   _rtpPackets;
    RtpDump*                _rtpDump;
};


class PacketRequester: public VCMPacketRequestCallback
{
public:
    PacketRequester(RtpRtcp& rtp) :
        _rtp(rtp) {}
    int32_t ResendPackets(const uint16_t* sequenceNumbers,
            uint16_t length);
private:
    webrtc::RtpRtcp& _rtp;
};


class KeyFrameReqTest: public VCMFrameTypeCallback
{
public:
    int32_t RequestKeyFrame();
};



class SendStatsTest: public webrtc::VCMSendStatisticsCallback
{
public:
    SendStatsTest() : _framerate(15), _bitrate(500) {}
    int32_t SendStatistics(const uint32_t bitRate,
            const uint32_t frameRate);
    void set_framerate(uint32_t frameRate) {_framerate = frameRate;}
    void set_bitrate(uint32_t bitrate) {_bitrate = bitrate;}
private:
    uint32_t _framerate;
    uint32_t _bitrate;
};



class VideoProtectionCallback: public VCMProtectionCallback
{
public:
    VideoProtectionCallback();
    virtual ~VideoProtectionCallback();
    void RegisterRtpModule(RtpRtcp* rtp) {_rtp = rtp;}
    int32_t ProtectionRequest(
        const FecProtectionParams* delta_fec_params,
        const FecProtectionParams* key_fec_params,
        uint32_t* sent_video_rate_bps,
        uint32_t* sent_nack_rate_bps,
        uint32_t* sent_fec_rate_bps);
    FecProtectionParams DeltaFecParameters() const;
    FecProtectionParams KeyFecParameters() const;
private:
    RtpRtcp* _rtp;
    FecProtectionParams delta_fec_params_;
    FecProtectionParams key_fec_params_;
};
}  
#endif
