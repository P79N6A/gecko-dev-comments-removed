









#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <cassert>

#include "gtest/gtest.h"
#include "testsupport/fileutils.h"
#include "tick_util.h"
#include "unit_test.h"
#include "video_source.h"

using namespace webrtc;

UnitTest::UnitTest()
:
CodecTest("UnitTest", "Unit test"),
_tests(0),
_errors(0),
_source(NULL),
_refFrame(NULL),
_refEncFrame(NULL),
_refDecFrame(NULL),
_refEncFrameLength(0),
_sourceFile(NULL),
_encodeCompleteCallback(NULL),
_decodeCompleteCallback(NULL)
{
}

UnitTest::UnitTest(std::string name, std::string description)
:
CodecTest(name, description),
_tests(0),
_errors(0),
_source(NULL),
_refFrame(NULL),
_refEncFrame(NULL),
_refDecFrame(NULL),
_refEncFrameLength(0),
_sourceFile(NULL),
_encodeCompleteCallback(NULL),
_decodeCompleteCallback(NULL)
{
}

UnitTest::~UnitTest()
{
    if (_encodeCompleteCallback) {
        delete _encodeCompleteCallback;
    }

    if (_decodeCompleteCallback) {
        delete _decodeCompleteCallback;
    }

    if (_source) {
        delete _source;
    }

    if (_refFrame) {
        delete [] _refFrame;
    }

    if (_refDecFrame) {
        delete [] _refDecFrame;
    }

    if (_sourceBuffer) {
        delete [] _sourceBuffer;
    }

    if (_sourceFile) {
        fclose(_sourceFile);
    }

    if (_refEncFrame) {
        delete [] _refEncFrame;
    }
}

WebRtc_Word32
UnitTestEncodeCompleteCallback::Encoded(EncodedImage& encodedImage,
                                        const webrtc::CodecSpecificInfo* codecSpecificInfo,
                                        const webrtc::RTPFragmentationHeader*
                                        fragmentation)
{
    _encodedVideoBuffer->VerifyAndAllocate(encodedImage._size);
    _encodedVideoBuffer->CopyBuffer(encodedImage._size, encodedImage._buffer);
    _encodedVideoBuffer->UpdateLength(encodedImage._length);
    _encodedVideoBuffer->SetFrameType(encodedImage._frameType);
    _encodedVideoBuffer->SetCaptureWidth(
        (WebRtc_UWord16)encodedImage._encodedWidth);
    _encodedVideoBuffer->SetCaptureHeight(
        (WebRtc_UWord16)encodedImage._encodedHeight);
    _encodedVideoBuffer->SetTimeStamp(encodedImage._timeStamp);
    _encodeComplete = true;
    _encodedFrameType = encodedImage._frameType;
    return 0;
}

WebRtc_Word32 UnitTestDecodeCompleteCallback::Decoded(VideoFrame& image)
{
    _decodedVideoBuffer->CopyBuffer(image.Length(), image.Buffer());
    _decodedVideoBuffer->SetWidth(image.Width());
    _decodedVideoBuffer->SetHeight(image.Height());
    _decodedVideoBuffer->SetTimeStamp(image.TimeStamp());
    _decodeComplete = true;
    return 0;
}

bool
UnitTestEncodeCompleteCallback::EncodeComplete()
{
    if (_encodeComplete)
    {
        _encodeComplete = false;
        return true;
    }
    return false;
}

VideoFrameType
UnitTestEncodeCompleteCallback::EncodedFrameType() const
{
    return _encodedFrameType;
}

bool
UnitTestDecodeCompleteCallback::DecodeComplete()
{
    if (_decodeComplete)
    {
        _decodeComplete = false;
        return true;
    }
    return false;
}

WebRtc_UWord32
UnitTest::WaitForEncodedFrame() const
{
    WebRtc_Word64 startTime = TickTime::MillisecondTimestamp();
    while (TickTime::MillisecondTimestamp() - startTime < kMaxWaitEncTimeMs)
    {
        if (_encodeCompleteCallback->EncodeComplete())
        {
            return _encodedVideoBuffer.GetLength();
        }
    }
    return 0;
}

WebRtc_UWord32
UnitTest::WaitForDecodedFrame() const
{
    WebRtc_Word64 startTime = TickTime::MillisecondTimestamp();
    while (TickTime::MillisecondTimestamp() - startTime < kMaxWaitDecTimeMs)
    {
        if (_decodeCompleteCallback->DecodeComplete())
        {
            return _decodedVideoBuffer.GetLength();
        }
    }
    return 0;
}

WebRtc_UWord32
UnitTest::CodecSpecific_SetBitrate(WebRtc_UWord32 bitRate,
                                   WebRtc_UWord32 )
{
    return _encoder->SetRates(bitRate, _inst.maxFramerate);
}

void
UnitTest::Setup()
{
    
    if (_sourceFile != NULL)
    {
        return;
    }

    if (_encodeCompleteCallback == NULL)
    {
        _encodeCompleteCallback =
            new UnitTestEncodeCompleteCallback(&_encodedVideoBuffer);
    }
    if (_decodeCompleteCallback == NULL)
    {
        _decodeCompleteCallback =
            new UnitTestDecodeCompleteCallback(&_decodedVideoBuffer);
    }

    _encoder->RegisterEncodeCompleteCallback(_encodeCompleteCallback);
    _decoder->RegisterDecodeCompleteCallback(_decodeCompleteCallback);

    _source = new VideoSource(webrtc::test::ProjectRootPath() +
                              "resources/foreman_cif.yuv", kCIF);

    _lengthSourceFrame = _source->GetFrameLength();
    _refFrame = new unsigned char[_lengthSourceFrame];
    _refDecFrame = new unsigned char[_lengthSourceFrame];
    _sourceBuffer = new unsigned char [_lengthSourceFrame];
    _sourceFile = fopen(_source->GetFileName().c_str(), "rb");
    ASSERT_TRUE(_sourceFile != NULL);

    _inst.maxFramerate = _source->GetFrameRate();
    _bitRate = 300;
    _inst.startBitrate = 300;
    _inst.maxBitrate = 4000;
    _inst.width = _source->GetWidth();
    _inst.height = _source->GetHeight();
    _inst.codecSpecific.VP8.denoisingOn = true;

    
    _inputVideoBuffer.VerifyAndAllocate(_lengthSourceFrame);
    ASSERT_TRUE(fread(_refFrame, 1, _lengthSourceFrame, _sourceFile)
                           == _lengthSourceFrame);
    _inputVideoBuffer.CopyBuffer(_lengthSourceFrame, _refFrame);
    _inputVideoBuffer.SetWidth(_source->GetWidth());
    _inputVideoBuffer.SetHeight(_source->GetHeight());
    rewind(_sourceFile);

    
    _encodedVideoBuffer.VerifyAndAllocate(_lengthSourceFrame);

    VideoFrame image;
    VideoBufferToRawImage(_inputVideoBuffer, image);

    
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) == WEBRTC_VIDEO_CODEC_OK);
    VideoFrameType videoFrameType = kDeltaFrame;
    _encoder->Encode(image, NULL, videoFrameType);
    _refEncFrameLength = WaitForEncodedFrame();
    ASSERT_TRUE(_refEncFrameLength > 0);
    _refEncFrame = new unsigned char[_refEncFrameLength];
    memcpy(_refEncFrame, _encodedVideoBuffer.GetBuffer(), _refEncFrameLength);

    
    _decodedVideoBuffer.VerifyAndAllocate(_lengthSourceFrame);
    EXPECT_TRUE(_decoder->InitDecode(&_inst, 1) == WEBRTC_VIDEO_CODEC_OK);
    ASSERT_FALSE(SetCodecSpecificParameters() != WEBRTC_VIDEO_CODEC_OK);

    unsigned int frameLength = 0;
    int i=0;
    while (frameLength == 0)
    {
        if (i > 0)
        {
            
            _inputVideoBuffer.VerifyAndAllocate(_lengthSourceFrame);
            ASSERT_TRUE(fread(_refFrame, 1, _lengthSourceFrame,
                _sourceFile) == _lengthSourceFrame);
            _inputVideoBuffer.CopyBuffer(_lengthSourceFrame, _refFrame);
            _inputVideoBuffer.SetWidth(_source->GetWidth());
            _inputVideoBuffer.SetHeight(_source->GetHeight());
            VideoBufferToRawImage(_inputVideoBuffer, image);
            _encoder->Encode(image, NULL, videoFrameType);
            ASSERT_TRUE(WaitForEncodedFrame() > 0);
        }
        EncodedImage encodedImage;
        VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
        ASSERT_TRUE(_decoder->Decode(encodedImage, 0, NULL)
                               == WEBRTC_VIDEO_CODEC_OK);
        frameLength = WaitForDecodedFrame();
        _encodedVideoBuffer.Reset();
        _encodedVideoBuffer.UpdateLength(0);
        i++;
    }
    rewind(_sourceFile);
    EXPECT_TRUE(frameLength == _lengthSourceFrame);
    memcpy(_refDecFrame, _decodedVideoBuffer.GetBuffer(), _lengthSourceFrame);
}

void
UnitTest::Teardown()
{
    
    if (_sourceFile == NULL)
    {
        return;
    }

    _encoder->Release();
    _decoder->Release();

    fclose(_sourceFile);
    _sourceFile = NULL;
    delete [] _refFrame;
    _refFrame = NULL;
    delete [] _refEncFrame;
    _refEncFrame = NULL;
    delete [] _refDecFrame;
    _refDecFrame = NULL;
    delete [] _sourceBuffer;
    _sourceBuffer = NULL;
}

void
UnitTest::Print()
{
}

int
UnitTest::DecodeWithoutAssert()
{
    EncodedImage encodedImage;
    VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
    int ret = _decoder->Decode(encodedImage, 0, NULL);
    int frameLength = WaitForDecodedFrame();
    _encodedVideoBuffer.Reset();
    _encodedVideoBuffer.UpdateLength(0);
    return ret == WEBRTC_VIDEO_CODEC_OK ? frameLength : ret;
}

int
UnitTest::Decode()
{
    EncodedImage encodedImage;
    VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
    if (encodedImage._length == 0)
    {
        return WEBRTC_VIDEO_CODEC_OK;
    }
    int ret = _decoder->Decode(encodedImage, 0, NULL);
    unsigned int frameLength = WaitForDecodedFrame();
    assert(ret == WEBRTC_VIDEO_CODEC_OK && (frameLength == 0 || frameLength
        == _lengthSourceFrame));
    EXPECT_TRUE(ret == WEBRTC_VIDEO_CODEC_OK && (frameLength == 0 || frameLength
        == _lengthSourceFrame));
    _encodedVideoBuffer.Reset();
    _encodedVideoBuffer.UpdateLength(0);
    return ret == WEBRTC_VIDEO_CODEC_OK ? frameLength : ret;
}


void
UnitTest::Perform()
{
    UnitTest::Setup();
    int frameLength;
    VideoFrame inputImage;
    EncodedImage encodedImage;
    VideoFrameType videoFrameType = kDeltaFrame;

    

    
    
    EXPECT_TRUE(_encoder->Release() == WEBRTC_VIDEO_CODEC_OK);
    VideoBufferToRawImage(_inputVideoBuffer, inputImage);
    EXPECT_TRUE(_encoder->Encode(inputImage, NULL, videoFrameType)
               == WEBRTC_VIDEO_CODEC_UNINITIALIZED);

    
    
    EXPECT_TRUE(_encoder->InitEncode(NULL, 1, 1440) ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    
    WebRtc_Word32 tmpBitRate = _inst.startBitrate;
    WebRtc_Word32 tmpMaxBitRate = _inst.maxBitrate;
    _inst.startBitrate = 4000;
    _inst.maxBitrate = 3000;
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440)  ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    _inst.startBitrate = tmpBitRate;
    _inst.maxBitrate = tmpMaxBitRate; 

    
    _inst.maxFramerate = 0;
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    
    
    
    _inst.maxFramerate = 30;

    
    _inst.startBitrate = -1;
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    _inst.maxBitrate = _inst.startBitrate - 1;
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    _inst.maxBitrate = 0;
    _inst.startBitrate = 300;

    
    _inst.maxBitrate = 200;
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    _inst.maxBitrate = 4000;

    
    _inst.width = 0;
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) < 0);
    _inst.width = _source->GetWidth();

    
    _inst.height = 0;
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) < 0);
    _inst.height = _source->GetHeight();

    
    EXPECT_TRUE(_encoder->InitEncode(&_inst, -1, 1440) ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);

    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) == WEBRTC_VIDEO_CODEC_OK);

    

    
    _inputVideoBuffer.Free();
    inputImage.Free();
    EXPECT_TRUE(_encoder->Encode(inputImage, NULL, videoFrameType) ==
        WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    _inputVideoBuffer.VerifyAndAllocate(_lengthSourceFrame);
    _inputVideoBuffer.CopyBuffer(_lengthSourceFrame, _refFrame);
    _inputVideoBuffer.SetWidth(_source->GetWidth());
    _inputVideoBuffer.SetHeight(_source->GetHeight());

    

    
    VideoBufferToRawImage(_inputVideoBuffer, inputImage);
    for (int i = 1; i <= 60; i++)
    {
        VideoFrameType frameType = !(i % 2) ? kKeyFrame : kDeltaFrame;
        EXPECT_TRUE(_encoder->Encode(inputImage, NULL, frameType) ==
            WEBRTC_VIDEO_CODEC_OK);
        EXPECT_TRUE(WaitForEncodedFrame() > 0);
    }

    
    _encodedVideoBuffer.UpdateLength(0);
    _encodedVideoBuffer.Reset();
    EXPECT_TRUE(_encoder->Encode(inputImage, NULL, videoFrameType) ==
        WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(WaitForEncodedFrame() > 0);

    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) == WEBRTC_VIDEO_CODEC_OK);
    _encoder->Encode(inputImage, NULL, videoFrameType);
    frameLength = WaitForEncodedFrame();
    EXPECT_TRUE(frameLength > 0);
    EXPECT_TRUE(CheckIfBitExact(_refEncFrame, _refEncFrameLength,
            _encodedVideoBuffer.GetBuffer(), frameLength) == true);

    
    _encodedVideoBuffer.UpdateLength(0);
    _encodedVideoBuffer.Reset();
    EXPECT_TRUE(_encoder->Encode(inputImage, NULL, videoFrameType) ==
        WEBRTC_VIDEO_CODEC_OK);
    WaitForEncodedFrame();
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) == WEBRTC_VIDEO_CODEC_OK);
    _encoder->Encode(inputImage, NULL, videoFrameType);
    frameLength = WaitForEncodedFrame();
    EXPECT_TRUE(frameLength > 0);
    EXPECT_TRUE(CheckIfBitExact(_refEncFrame, _refEncFrameLength,
        _encodedVideoBuffer.GetBuffer(), frameLength) == true);

    
    _encodedVideoBuffer.UpdateLength(0);
    _encodedVideoBuffer.Reset();
    EXPECT_TRUE(_encoder->Encode(inputImage, NULL, videoFrameType) ==
        WEBRTC_VIDEO_CODEC_OK);
    WaitForEncodedFrame();
    EXPECT_TRUE(_encoder->Release() == WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) == WEBRTC_VIDEO_CODEC_OK);
    _encoder->Encode(inputImage, NULL, videoFrameType);
    frameLength = WaitForEncodedFrame();
    EXPECT_TRUE(frameLength > 0);
    EXPECT_TRUE(CheckIfBitExact(_refEncFrame, _refEncFrameLength,
        _encodedVideoBuffer.GetBuffer(), frameLength) == true);

    

    
    
    EXPECT_TRUE(_decoder->Release() == WEBRTC_VIDEO_CODEC_OK);
    VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
    EXPECT_TRUE(_decoder->Decode(encodedImage, false, NULL) ==
        WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    WaitForDecodedFrame();
    EXPECT_TRUE(_decoder->Reset() == WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    EXPECT_TRUE(_decoder->InitDecode(&_inst, 1) == WEBRTC_VIDEO_CODEC_OK);
    ASSERT_FALSE(SetCodecSpecificParameters() != WEBRTC_VIDEO_CODEC_OK);

    
    
    _encodedVideoBuffer.Free();
    VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
    encodedImage._length = 10;  
    EXPECT_EQ(_decoder->Decode(encodedImage, false, NULL),
              WEBRTC_VIDEO_CODEC_ERR_PARAMETER);
    _encodedVideoBuffer.VerifyAndAllocate(_lengthSourceFrame);

    
    unsigned char* tmpBuf = new unsigned char[_lengthSourceFrame];

    
    
    
    EXPECT_TRUE(_decoder->Reset() == WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(_decoder->InitDecode(&_inst, 1) == WEBRTC_VIDEO_CODEC_OK);
    ASSERT_FALSE(SetCodecSpecificParameters() != WEBRTC_VIDEO_CODEC_OK);
    for (int i = 0; i < 100; i++)
    {
        ASSERT_TRUE(fread(tmpBuf, 1, _refEncFrameLength, _sourceFile)
            == _refEncFrameLength);
        _encodedVideoBuffer.CopyBuffer(_refEncFrameLength, tmpBuf);
        VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
        int ret = _decoder->Decode(encodedImage, false, NULL);
        EXPECT_TRUE(ret <= 0);
        if (ret == 0)
        {
            EXPECT_TRUE(WaitForDecodedFrame() == 0);
        }

        memset(tmpBuf, 0, _refEncFrameLength);
        _encodedVideoBuffer.CopyBuffer(_refEncFrameLength, tmpBuf);
        VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
        ret = _decoder->Decode(encodedImage, false, NULL);
        EXPECT_TRUE(ret <= 0);
        if (ret == 0)
        {
            EXPECT_TRUE(WaitForDecodedFrame() == 0);
        }
    }
    rewind(_sourceFile);

    _encodedVideoBuffer.UpdateLength(_refEncFrameLength);
    _encodedVideoBuffer.CopyBuffer(_refEncFrameLength, _refEncFrame);

    
    EXPECT_TRUE(_decoder->InitDecode(&_inst, 1) == WEBRTC_VIDEO_CODEC_OK);
    ASSERT_FALSE(SetCodecSpecificParameters() != WEBRTC_VIDEO_CODEC_OK);
    frameLength = 0;
    VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
    while (frameLength == 0)
    {
        _decoder->Decode(encodedImage, false, NULL);
        frameLength = WaitForDecodedFrame();
    }
    EXPECT_TRUE(CheckIfBitExact(_decodedVideoBuffer.GetBuffer(), frameLength,
        _refDecFrame, _lengthSourceFrame) == true);

    
    EXPECT_TRUE(_decoder->Reset() == WEBRTC_VIDEO_CODEC_OK);
    frameLength = 0;
    VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
    while (frameLength == 0)
    {
        _decoder->Decode(encodedImage, false, NULL);
        frameLength = WaitForDecodedFrame();
    }
    EXPECT_TRUE(CheckIfBitExact(_decodedVideoBuffer.GetBuffer(), frameLength,
        _refDecFrame, _lengthSourceFrame) == true);

    
    
    {
        
        
        EXPECT_TRUE((_inst.width % 2 == 0) && (_inst.height % 2 == 0));

        VideoCodec tempInst;
        memcpy(&tempInst, &_inst, sizeof(VideoCodec));
        tempInst.width /= 2;
        tempInst.height /= 2;

        
        EXPECT_TRUE(_encoder->Release() == WEBRTC_VIDEO_CODEC_OK);
        EXPECT_TRUE(_encoder->InitEncode(&tempInst, 1, 1440) ==
            WEBRTC_VIDEO_CODEC_OK);
        VideoFrame tempInput;
        unsigned int tmpLength = inputImage.Length() / 4;
        tempInput.CopyFrame(tmpLength, inputImage.Buffer());
        tempInput.SetWidth(tempInst.width);
        tempInput.SetHeight(tempInst.height);
        VideoFrameType videoFrameType = kDeltaFrame;
        _encoder->Encode(tempInput, NULL, videoFrameType);
        frameLength = WaitForEncodedFrame();
        EXPECT_TRUE(frameLength > 0);
        tempInput.Free();
        
        EXPECT_TRUE(_decoder->Reset() == WEBRTC_VIDEO_CODEC_OK);
        frameLength = 0;
        VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
        while (frameLength == 0)
        {
            _decoder->Decode(encodedImage, false, NULL);
            frameLength = WaitForDecodedFrame();
        }

        
        EXPECT_TRUE(_encoder->Release() == WEBRTC_VIDEO_CODEC_OK);
        EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) ==
            WEBRTC_VIDEO_CODEC_OK);
        _encoder->Encode(inputImage, NULL, videoFrameType);
        frameLength = WaitForEncodedFrame();
        EXPECT_TRUE(frameLength > 0);

        
        EXPECT_TRUE(_decoder->Reset() == WEBRTC_VIDEO_CODEC_OK);
        frameLength = 0;
        VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
        while (frameLength == 0)
        {
            _decoder->Decode(encodedImage, false, NULL);
            frameLength = WaitForDecodedFrame();
        }

        
        EXPECT_TRUE(CheckIfBitExact(_decodedVideoBuffer.GetBuffer(), frameLength,
            _refDecFrame, _lengthSourceFrame) == true);

    }

    
    EXPECT_TRUE(_decoder->Release() == WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(_decoder->InitDecode(&_inst, 1) == WEBRTC_VIDEO_CODEC_OK);
    ASSERT_FALSE(SetCodecSpecificParameters() != WEBRTC_VIDEO_CODEC_OK);
    frameLength = 0;
    VideoEncodedBufferToEncodedImage(_encodedVideoBuffer, encodedImage);
    while (frameLength == 0)
    {
        _decoder->Decode(encodedImage, false, NULL);
        frameLength = WaitForDecodedFrame();
    }
    EXPECT_TRUE(CheckIfBitExact(_decodedVideoBuffer.GetBuffer(), frameLength,
        _refDecFrame, _lengthSourceFrame) == true);
    _encodedVideoBuffer.UpdateLength(0);
    _encodedVideoBuffer.Reset();

    delete [] tmpBuf;

    
    int frames = 0;
    
    _inst.maxBitrate = 0;

    
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) == WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(_decoder->Reset() == WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(_decoder->InitDecode(&_inst, 1) == WEBRTC_VIDEO_CODEC_OK);
    ASSERT_FALSE(SetCodecSpecificParameters() != WEBRTC_VIDEO_CODEC_OK);

    frames = 0;
    int frameDelay = 0;
    int encTimeStamp;
    _decodedVideoBuffer.SetTimeStamp(0);
    while (fread(_sourceBuffer, 1, _lengthSourceFrame, _sourceFile) ==
        _lengthSourceFrame)
    {
        _inputVideoBuffer.CopyBuffer(_lengthSourceFrame, _sourceBuffer);
        _inputVideoBuffer.SetTimeStamp(frames);
        VideoBufferToRawImage(_inputVideoBuffer, inputImage);
        VideoFrameType videoFrameType = kDeltaFrame;
        ASSERT_TRUE(_encoder->Encode(inputImage, NULL, videoFrameType) ==
            WEBRTC_VIDEO_CODEC_OK);
        frameLength = WaitForEncodedFrame();
        
        EXPECT_TRUE(frameLength > 0);
        encTimeStamp = _encodedVideoBuffer.GetTimeStamp();
        EXPECT_TRUE(_inputVideoBuffer.GetTimeStamp() ==
                static_cast<unsigned>(encTimeStamp));

        frameLength = Decode();
        if (frameLength == 0)
        {
            frameDelay++;
        }

        encTimeStamp -= frameDelay;
        if (encTimeStamp < 0)
        {
            encTimeStamp = 0;
        }
        EXPECT_TRUE(_decodedVideoBuffer.GetTimeStamp() ==
                static_cast<unsigned>(encTimeStamp));
        frames++;
    }
    ASSERT_TRUE(feof(_sourceFile) != 0);
    rewind(_sourceFile);

    RateControlTests();
    inputImage.Free();

    Teardown();
}

void
UnitTest::RateControlTests()
{
    int frames = 0;
    VideoFrame inputImage;
    WebRtc_UWord32 frameLength;

    
    _inst.maxBitrate = 0;
    
    EXPECT_TRUE(_encoder->InitEncode(&_inst, 1, 1440) == WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(_decoder->Reset() == WEBRTC_VIDEO_CODEC_OK);
    EXPECT_TRUE(_decoder->InitDecode(&_inst, 1) == WEBRTC_VIDEO_CODEC_OK);
    
    const int bitRate[] = {30, 100, 500, 1000, 2000};
    const int nBitrates = sizeof(bitRate)/sizeof(*bitRate);

    printf("\nRate control test\n");
    for (int i = 0; i < nBitrates; i++)
    {
        _bitRate = bitRate[i];
        int totalBytes = 0;
        _inst.startBitrate = _bitRate;
        _encoder->InitEncode(&_inst, 4, 1440);
        _decoder->Reset();
        _decoder->InitDecode(&_inst, 1);
        frames = 0;

        if (_bitRate > _inst.maxBitrate)
        {
            CodecSpecific_SetBitrate(_bitRate, _inst.maxFramerate);
        }
        else
        {
            CodecSpecific_SetBitrate(_bitRate, _inst.maxFramerate);
        }

        while (fread(_sourceBuffer, 1, _lengthSourceFrame, _sourceFile) ==
            _lengthSourceFrame)
        {
            _inputVideoBuffer.CopyBuffer(_lengthSourceFrame, _sourceBuffer);
            _inputVideoBuffer.SetTimeStamp(_inputVideoBuffer.GetTimeStamp() +
                static_cast<WebRtc_UWord32>(9e4 /
                    static_cast<float>(_inst.maxFramerate)));
            VideoBufferToRawImage(_inputVideoBuffer, inputImage);
            VideoFrameType videoFrameType = kDeltaFrame;
            ASSERT_EQ(_encoder->Encode(inputImage, NULL, videoFrameType),
                      WEBRTC_VIDEO_CODEC_OK);
            frameLength = WaitForEncodedFrame();
            ASSERT_GE(frameLength, 0u);
            totalBytes += frameLength;
            frames++;

            _encodedVideoBuffer.UpdateLength(0);
            _encodedVideoBuffer.Reset();
        }
        WebRtc_UWord32 actualBitrate =
            (totalBytes  / frames * _inst.maxFramerate * 8)/1000;
        printf("Target bitrate: %d kbps, actual bitrate: %d kbps\n", _bitRate,
            actualBitrate);
        
        if (_bitRate >= 100 && _bitRate <= 2500)
        {
            EXPECT_TRUE(abs(WebRtc_Word32(actualBitrate - _bitRate)) <
                0.12 * _bitRate); 
        }
        ASSERT_TRUE(feof(_sourceFile) != 0);
        rewind(_sourceFile);
    }
    inputImage.Free();
}

bool
UnitTest::CheckIfBitExact(const void* ptrA, unsigned int aLengthBytes,
                          const void* ptrB, unsigned int bLengthBytes)
{
    if (aLengthBytes != bLengthBytes)
    {
        return false;
    }

    return memcmp(ptrA, ptrB, aLengthBytes) == 0;
}
