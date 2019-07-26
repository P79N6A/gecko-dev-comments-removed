









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_PACKET_LOSS_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_PACKET_LOSS_TEST_H_

#include <list>

#include "normal_async_test.h"

class PacketLossTest : public NormalAsyncTest
{
public:
    PacketLossTest();
    virtual ~PacketLossTest() {if(_lastFrame) {delete [] _lastFrame; _lastFrame = NULL;}}
    virtual void Encoded(const webrtc::EncodedImage& encodedImage);
    virtual void Decoded(const webrtc::I420VideoFrame& decodedImage);
protected:
    PacketLossTest(std::string name, std::string description);
    PacketLossTest(std::string name,
                   std::string description,
                   double lossRate,
                   bool useNack,
                   unsigned int rttFrames = 0);

    virtual void Setup();
    virtual void Teardown();
    virtual void CodecSpecific_InitBitrate();
    virtual int DoPacketLoss();
    virtual int NextPacket(int size, unsigned char **pkg);
    virtual int ByteLoss(int size, unsigned char *pkg, int bytesToLose);
    virtual void InsertPacket(webrtc::VideoFrame *buf, unsigned char *pkg,
                              int size);
    int _inBufIdx;
    int _outBufIdx;

    
    
    
    double _lossRate;
    
    double _lossProbability;

    int _totalKept;
    int _totalThrown;
    int _sumChannelBytes;
    std::list<WebRtc_UWord32> _frameQueue;
    WebRtc_UWord8* _lastFrame;
    WebRtc_UWord32 _lastFrameLength;
};


#endif 
