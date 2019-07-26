









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_UNIT_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_UNIT_TEST_H_

#include "test.h"
#include "event_wrapper.h"







#ifdef _WIN32
#pragma warning(disable : 4127)
#endif

class VideoSource;
class UnitTestEncodeCompleteCallback;
class UnitTestDecodeCompleteCallback;

class UnitTest : public CodecTest
{
public:
    UnitTest();
    virtual ~UnitTest();
    virtual void Setup();
    virtual void Perform();
    virtual void Print();

protected:
    UnitTest(std::string name, std::string description);
    virtual WebRtc_UWord32 CodecSpecific_SetBitrate(
        WebRtc_UWord32 bitRate,
        WebRtc_UWord32 );
    virtual void Teardown();
    virtual void RateControlTests();
    virtual int Decode();
    virtual int DecodeWithoutAssert();
    virtual int SetCodecSpecificParameters() {return 0;};

    virtual bool CheckIfBitExact(const void *ptrA, unsigned int aLengthBytes,
                                 const void *ptrB, unsigned int bLengthBytes);

    WebRtc_UWord32 WaitForEncodedFrame() const;
    WebRtc_UWord32 WaitForDecodedFrame() const;

    int _tests;
    int _errors;

    VideoSource* _source;
    unsigned char* _refFrame;
    unsigned char* _refEncFrame;
    unsigned char* _refDecFrame;
    unsigned int _refEncFrameLength;
    FILE* _sourceFile;

    UnitTestEncodeCompleteCallback* _encodeCompleteCallback;
    UnitTestDecodeCompleteCallback* _decodeCompleteCallback;
    enum { kMaxWaitEncTimeMs = 100 };
    enum { kMaxWaitDecTimeMs = 25 };
};

class UnitTestEncodeCompleteCallback : public webrtc::EncodedImageCallback
{
public:
    UnitTestEncodeCompleteCallback(webrtc::VideoFrame* buffer,
                                   WebRtc_UWord32 decoderSpecificSize = 0,
                                   void* decoderSpecificInfo = NULL) :
      _encodedVideoBuffer(buffer),
      _encodeComplete(false) {}
    WebRtc_Word32 Encoded(webrtc::EncodedImage& encodedImage,
                          const webrtc::CodecSpecificInfo* codecSpecificInfo,
                          const webrtc::RTPFragmentationHeader*
                          fragmentation = NULL);
    bool EncodeComplete();
    
    webrtc::VideoFrameType EncodedFrameType() const;
private:
    webrtc::VideoFrame* _encodedVideoBuffer;
    bool _encodeComplete;
    webrtc::VideoFrameType _encodedFrameType;
};

class UnitTestDecodeCompleteCallback : public webrtc::DecodedImageCallback
{
public:
    UnitTestDecodeCompleteCallback(webrtc::I420VideoFrame* buffer) :
        _decodedVideoBuffer(buffer), _decodeComplete(false) {}
    WebRtc_Word32 Decoded(webrtc::I420VideoFrame& image);
    bool DecodeComplete();
private:
    webrtc::I420VideoFrame* _decodedVideoBuffer;
    bool _decodeComplete;
};

#endif 

