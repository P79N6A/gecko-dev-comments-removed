









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_DUAL_DECODER_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_DUAL_DECODER_TEST_H_

#include "vp8.h"
#include "normal_async_test.h"

class DualDecoderCompleteCallback;

class VP8DualDecoderTest : public VP8NormalAsyncTest
{
public:
    VP8DualDecoderTest(float bitRate);
    VP8DualDecoderTest();
    virtual ~VP8DualDecoderTest();
    virtual void Perform();
protected:
    VP8DualDecoderTest(std::string name, std::string description,
                       unsigned int testNo)
    : VP8NormalAsyncTest(name, description, testNo) {}
    virtual int Decode(int lossValue = 0);

    webrtc::VP8Decoder*     _decoder2;
    TestVideoBuffer         _decodedVideoBuffer2;
    static bool CheckIfBitExact(const void *ptrA, unsigned int aLengthBytes, 
        const void *ptrB, unsigned int bLengthBytes);
private:
};

class DualDecoderCompleteCallback : public webrtc::DecodedImageCallback
{
public:
    DualDecoderCompleteCallback(TestVideoBuffer* buffer)
    : _decodedVideoBuffer(buffer), _decodeComplete(false) {}
    WebRtc_Word32 Decoded(webrtc::VideoFrame& decodedImage);
    bool DecodeComplete();
private:
    TestVideoBuffer* _decodedVideoBuffer;
    bool _decodeComplete;
};


#endif
