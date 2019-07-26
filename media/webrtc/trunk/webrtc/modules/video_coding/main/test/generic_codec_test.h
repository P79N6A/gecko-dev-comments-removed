









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_GENERIC_CODEC_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_GENERIC_CODEC_TEST_H_

#include "webrtc/modules/video_coding/main/interface/video_coding.h"

#include <fstream>
#include <string.h>

#include "webrtc/modules/video_coding/main/test/test_callbacks.h"
#include "webrtc/modules/video_coding/main/test/test_util.h"









namespace webrtc {

int VCMGenericCodecTest(CmdArgs& args);

class SimulatedClock;

class GenericCodecTest
{
public:
    GenericCodecTest(webrtc::VideoCodingModule* vcm,
                     webrtc::SimulatedClock* clock);
    ~GenericCodecTest();
    static int RunTest(CmdArgs& args);
    int32_t Perform(CmdArgs& args);
    float WaitForEncodedFrame() const;

private:
    void Setup(CmdArgs& args);
    void Print();
    int32_t TearDown();
    void IncrementDebugClock(float frameRate);

    webrtc::SimulatedClock*              _clock;
    webrtc::VideoCodingModule*           _vcm;
    webrtc::VideoCodec                   _sendCodec;
    webrtc::VideoCodec                   _receiveCodec;
    std::string                          _inname;
    std::string                          _outname;
    std::string                          _encodedName;
    int32_t                        _sumEncBytes;
    FILE*                                _sourceFile;
    FILE*                                _decodedFile;
    FILE*                                _encodedFile;
    uint16_t                       _width;
    uint16_t                       _height;
    float                                _frameRate;
    uint32_t                       _lengthSourceFrame;
    uint32_t                       _timeStamp;
    VCMDecodeCompleteCallback*           _decodeCallback;
    VCMEncodeCompleteCallback*           _encodeCompleteCallback;

}; 

class RTPSendCallback_SizeTest : public webrtc::Transport
{
public:
    
    RTPSendCallback_SizeTest() : _maxPayloadSize(0), _payloadSizeSum(0), _nPackets(0) {}
    virtual int SendPacket(int channel, const void *data, int len);
    virtual int SendRTCPPacket(int channel, const void *data, int len) {return 0;}
    void SetMaxPayloadSize(uint32_t maxPayloadSize);
    void Reset();
    float AveragePayloadSize() const;
private:
    uint32_t         _maxPayloadSize;
    uint32_t         _payloadSizeSum;
    uint32_t         _nPackets;
};

class VCMEncComplete_KeyReqTest : public webrtc::VCMPacketizationCallback
{
public:
    VCMEncComplete_KeyReqTest(webrtc::VideoCodingModule &vcm) : _vcm(vcm), _seqNo(0), _timeStamp(0) {}
    int32_t SendData(
            const webrtc::FrameType frameType,
            const uint8_t payloadType,
            uint32_t timeStamp,
            int64_t capture_time_ms,
            const uint8_t* payloadData,
            const uint32_t payloadSize,
            const webrtc::RTPFragmentationHeader& fragmentationHeader,
            const webrtc::RTPVideoHeader* videoHdr);
private:
    webrtc::VideoCodingModule& _vcm;
    uint16_t _seqNo;
    uint32_t _timeStamp;
}; 

}  

#endif 
