









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_NORMAL_ASYNC_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_NORMAL_ASYNC_TEST_H_

#include "modules/video_coding/codecs/test_framework/normal_async_test.h"

class VP8NormalAsyncTest : public NormalAsyncTest
{
public:
    VP8NormalAsyncTest(WebRtc_UWord32 bitRate);
    VP8NormalAsyncTest(WebRtc_UWord32 bitRate, unsigned int testNo);
    VP8NormalAsyncTest() : NormalAsyncTest("VP8 Normal Test 1", "Tests VP8 normal execution", 1) {}
protected:
    VP8NormalAsyncTest(std::string name, std::string description, unsigned int testNo) : NormalAsyncTest(name, description, testNo) {}
    virtual void CodecSpecific_InitBitrate();
    virtual void CodecSettings(int width, int height, WebRtc_UWord32 frameRate=30, WebRtc_UWord32 bitRate=0);
    virtual webrtc::CodecSpecificInfo* CreateEncoderSpecificInfo() const;
    virtual WebRtc_Word32 ReceivedDecodedReferenceFrame(const WebRtc_UWord64 pictureId);
private:
    mutable bool  _hasReceivedRPSI;
    WebRtc_UWord64  _pictureIdRPSI;
};

#endif
