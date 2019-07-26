









#include "generic_codec_test.h"
#include <cmath>
#include <stdio.h>
#include "../source/event.h"
#include "rtp_rtcp.h"
#include "common_video/interface/i420_video_frame.h"
#include "test_macros.h"
#include "modules/video_coding/main/source/mock/fake_tick_time.h"

using namespace webrtc;

enum { kMaxWaitEncTimeMs = 100 };

int GenericCodecTest::RunTest(CmdArgs& args)
{
#if !defined(EVENT_DEBUG)
    printf("\n\nEnable debug events to run this test!\n\n");
    return -1;
#endif
    FakeTickTime clock(0);
    VideoCodingModule* vcm = VideoCodingModule::Create(1, &clock);
    GenericCodecTest* get = new GenericCodecTest(vcm, &clock);
    Trace::CreateTrace();
    Trace::SetTraceFile(
        (test::OutputPath() + "genericCodecTestTrace.txt").c_str());
    Trace::SetLevelFilter(webrtc::kTraceAll);
    get->Perform(args);
    Trace::ReturnTrace();
    delete get;
    VideoCodingModule::Destroy(vcm);
    return 0;
}

GenericCodecTest::GenericCodecTest(VideoCodingModule* vcm, FakeTickTime* clock):
_clock(clock),
_vcm(vcm),
_width(0),
_height(0),
_frameRate(0),
_lengthSourceFrame(0),
_timeStamp(0)
{
}

GenericCodecTest::~GenericCodecTest()
{
}

void
GenericCodecTest::Setup(CmdArgs& args)
{
    _timeStamp = 0;

    

    _inname= args.inputFile;
    if (args.outputFile.compare(""))
        _outname = test::OutputPath() + "GCTest_decoded.yuv";
    else
        _outname = args.outputFile;
    _encodedName = test::OutputPath() + "GCTest_encoded.vp8";
    _width = args.width;
    _height = args.height;
    _frameRate = args.frameRate;
    _lengthSourceFrame  = 3*_width*_height/2;

    

    if ((_sourceFile = fopen(_inname.c_str(), "rb")) == NULL)
    {
        printf("Cannot read file %s.\n", _inname.c_str());
        exit(1);
    }
    if ((_encodedFile = fopen(_encodedName.c_str(), "wb")) == NULL)
    {
        printf("Cannot write encoded file.\n");
        exit(1);
    }
    if ((_decodedFile = fopen(_outname.c_str(),  "wb")) == NULL)
    {
        printf("Cannot write file %s.\n", _outname.c_str());
        exit(1);
    }

    return;
}
WebRtc_Word32
GenericCodecTest::Perform(CmdArgs& args)
{
    WebRtc_Word32 ret;
    Setup(args);
    







    
    
    
    VideoCodec sendCodec, receiveCodec;
    sendCodec.maxBitrate = 8000;
    TEST(_vcm->NumberOfCodecs() > 0); 
    TEST(_vcm->Codec(0, &sendCodec)  == VCM_OK);
    _vcm->InitializeSender();
    _vcm->InitializeReceiver();
    WebRtc_Word32 NumberOfCodecs = _vcm->NumberOfCodecs();
    
    int i = 0;
    _vcm->Codec(0, &_sendCodec);
    TEST(_vcm->RegisterSendCodec(&_sendCodec, 4, 1440) == VCM_OK);
    
    I420VideoFrame sourceFrame;
    _vcm->InitializeSender();
    TEST(_vcm->Codec(kVideoCodecVP8, &sendCodec) == 0);
    TEST(_vcm->RegisterSendCodec(&sendCodec, -1, 1440) < 0); 
    sendCodec.maxBitrate = 8000;
    _vcm->RegisterSendCodec(&sendCodec, 1, 1440);
    _vcm->InitializeSender();
    _vcm->Codec(kVideoCodecVP8, &sendCodec);
    sendCodec.height = 0;
    TEST(_vcm->RegisterSendCodec(&sendCodec, 1, 1440) < 0); 
    _vcm->Codec(kVideoCodecVP8, &sendCodec);
    sendCodec.startBitrate = -2;
    TEST(_vcm->RegisterSendCodec(&sendCodec, 1, 1440) < 0); 
    _vcm->Codec(kVideoCodecVP8, &sendCodec);
    _vcm->InitializeSender();
    TEST(_vcm->SetChannelParameters(100, 0, 0) < 0);
    
    for (i=0; i< NumberOfCodecs; i++)
    {
        _vcm->Codec(i, &receiveCodec);
        _vcm->RegisterReceiveCodec(&receiveCodec, 1);
    }
    WebRtc_UWord8* tmpBuffer = new WebRtc_UWord8[_lengthSourceFrame];
    TEST(fread(tmpBuffer, 1, _lengthSourceFrame, _sourceFile) > 0);
    int half_width = (_width + 1) / 2;
    int half_height = (_height + 1) / 2;
    int size_y = _width * _height;
    int size_uv = half_width * half_height;
    sourceFrame.CreateFrame(size_y, tmpBuffer,
                            size_uv, tmpBuffer + size_y,
                            size_uv, tmpBuffer + size_y + size_uv,
                            _width, _height,
                            _width, half_width, half_width);
    sourceFrame.set_timestamp(_timeStamp++);
    TEST(_vcm->AddVideoFrame(sourceFrame) < 0 ); 
    _vcm->InitializeReceiver();
    TEST(_vcm->SetChannelParameters(100, 0, 0) < 0);

      
     
    
    
    rewind(_sourceFile);
    _vcm->InitializeReceiver();
    _vcm->InitializeSender();
    NumberOfCodecs = _vcm->NumberOfCodecs();
    
    _vcm->Codec(kVideoCodecVP8, &_sendCodec);
    _vcm->RegisterSendCodec(&_sendCodec, 4, 1440);
    _vcm->SendCodec(&sendCodec);
    sendCodec.startBitrate = 2000;

    
    
    sendCodec.maxFramerate = (WebRtc_UWord8)(_frameRate / 2);
    sendCodec.width = _width;
    sendCodec.height = _height;
    TEST(strncmp(_sendCodec.plName, "VP8", 3) == 0); 

    _decodeCallback = new VCMDecodeCompleteCallback(_decodedFile);
    _encodeCompleteCallback = new VCMEncodeCompleteCallback(_encodedFile);
    _vcm->RegisterReceiveCallback(_decodeCallback);
    _vcm->RegisterTransportCallback(_encodeCompleteCallback);
    _encodeCompleteCallback->RegisterReceiverVCM(_vcm);

    _vcm->RegisterSendCodec(&sendCodec, 4, 1440);
    _encodeCompleteCallback->SetCodecType(ConvertCodecType(sendCodec.plName));

    _vcm->InitializeReceiver();
    _vcm->Process();

    
    for (i = 0; i < _frameRate; i++)
    {
        TEST(fread(tmpBuffer, 1, _lengthSourceFrame, _sourceFile) > 0);
        sourceFrame.CreateFrame(size_y, tmpBuffer,
                                size_uv, tmpBuffer + size_y,
                                size_uv, tmpBuffer + size_y + size_uv,
                                _width, _height,
                                _width, half_width, half_width);
        _timeStamp += (WebRtc_UWord32)(9e4 / static_cast<float>(_frameRate));
        sourceFrame.set_timestamp(_timeStamp);
        TEST(_vcm->AddVideoFrame(sourceFrame) == VCM_OK);
        IncrementDebugClock(_frameRate);
        _vcm->Process();
    }
    sendCodec.maxFramerate = (WebRtc_UWord8)_frameRate;
    _vcm->InitializeSender();
    TEST(_vcm->RegisterReceiveCodec(&sendCodec, 1) == VCM_OK); 
    ret = 0;
    i = 0;
    while ((i < 25) && (ret == 0) )
    {
        ret = _vcm->Decode();
        TEST(ret == VCM_OK);
        if (ret < 0)
        {
            printf("error in frame # %d \n", i);
        }
        IncrementDebugClock(_frameRate);
        i++;
    }
    
    if (ret == 0)
    {
        printf("Encoder/Decoder individuality test complete - View output files \n");
    }
    
    _vcm->InitializeReceiver();
    TEST(_vcm->Decode() < 0); 


    
    
    
    
    VCMEncComplete_KeyReqTest keyReqTest_EncCompleteCallback(*_vcm);
    KeyFrameReqTest frameTypeCallback;
    _vcm->RegisterTransportCallback(&keyReqTest_EncCompleteCallback);
    _encodeCompleteCallback->RegisterReceiverVCM(_vcm);
    _vcm->RegisterSendCodec(&sendCodec, 4, 1440);
    _encodeCompleteCallback->SetCodecType(ConvertCodecType(sendCodec.plName));
    TEST(_vcm->SetVideoProtection(kProtectionKeyOnKeyLoss, true) == VCM_OK);
    TEST(_vcm->RegisterFrameTypeCallback(&frameTypeCallback) == VCM_OK);
    TEST(_vcm->RegisterReceiveCodec(&sendCodec, 1) == VCM_OK);
    TEST(_vcm->AddVideoFrame(sourceFrame) == VCM_OK);
    _timeStamp += (WebRtc_UWord32)(9e4 / static_cast<float>(_frameRate));
    sourceFrame.set_timestamp(_timeStamp);
    
    
    TEST(_vcm->AddVideoFrame(sourceFrame) == VCM_OK);
    TEST(_vcm->Decode() == VCM_OK);

    printf("API tests complete \n");

     
    
    
    





    double FullReq   =  0.1;
    
    printf("\n RATE CONTROL TEST\n");
    
    _vcm->InitializeSender();
    _vcm->InitializeReceiver();
    rewind(_sourceFile);
    sourceFrame.CreateEmptyFrame(_width, _height, _width,
                                 (_width + 1) / 2, (_width + 1) / 2);
    const float bitRate[] = {100, 400, 600, 1000, 2000};
    const float nBitrates = sizeof(bitRate)/sizeof(*bitRate);
    float _bitRate = 0;
    int _frameCnt = 0;
    float totalBytesOneSec;
    float totalBytes, actualBitrate;
    VCMFrameCount frameCount; 
    
    NumberOfCodecs = _vcm->NumberOfCodecs();
    
    _encodeCompleteCallback->SetFrameDimensions(_width, _height);
    SendStatsTest sendStats;
    for (int k = 0; k < NumberOfCodecs; k++)
    
    {
        
        _vcm->InitializeSender();
        _sendCodec.maxBitrate = 8000;
        TEST(_vcm->Codec(k, &_sendCodec)== VCM_OK);
        _vcm->RegisterSendCodec(&_sendCodec, 1, 1440);
        _vcm->RegisterTransportCallback(_encodeCompleteCallback);
        _encodeCompleteCallback->SetCodecType(ConvertCodecType(_sendCodec.plName));
        printf (" \n\n Codec type = %s \n\n",_sendCodec.plName);
        for (i = 0; i < nBitrates; i++)
        {
             _bitRate = static_cast<float>(bitRate[i]);
            
            _vcm->InitializeSender();
            _sendCodec.startBitrate = (int)_bitRate;
            _sendCodec.maxBitrate = 8000;
            _sendCodec.maxFramerate = _frameRate;
            _vcm->RegisterSendCodec(&_sendCodec, 1, 1440);
            _vcm->RegisterTransportCallback(_encodeCompleteCallback);
            
            _vcm->SetChannelParameters((WebRtc_UWord32)_bitRate, 0, 20);
            _frameCnt = 0;
            totalBytes = 0;
            _encodeCompleteCallback->Initialize();
            sendStats.SetTargetFrameRate(static_cast<WebRtc_UWord32>(_frameRate));
            _vcm->RegisterSendStatisticsCallback(&sendStats);
            while (fread(tmpBuffer, 1, _lengthSourceFrame, _sourceFile) ==
                _lengthSourceFrame)
            {
                _frameCnt++;
                sourceFrame.CreateFrame(size_y, tmpBuffer,
                                        size_uv, tmpBuffer + size_y,
                                        size_uv, tmpBuffer + size_y + size_uv,
                                        _width, _height,
                                        _width, (_width + 1) / 2,
                                        (_width + 1) / 2);
                _timeStamp += (WebRtc_UWord32)(9e4 / static_cast<float>(_frameRate));
                sourceFrame.set_timestamp(_timeStamp);

                ret = _vcm->AddVideoFrame(sourceFrame);
                IncrementDebugClock(_frameRate);
                
                


                
                
                if (_frameCnt == _frameRate)
                {
                    totalBytesOneSec =  _encodeCompleteCallback->EncodedBytes();
                }
                TEST(_vcm->TimeUntilNextProcess() >= 0);
            } 
            TEST(_vcm->TimeUntilNextProcess() == 0);
            _vcm->Process(); 
            
            
            
            totalBytes = _encodeCompleteCallback->EncodedBytes();
            actualBitrate = (float)(8.0/1000)*(totalBytes / (_frameCnt / _frameRate));

            printf("Complete Seq.: target bitrate: %.0f kbps, actual bitrate: %.1f kbps\n", _bitRate, actualBitrate);
            TEST((fabs(actualBitrate - _bitRate) < FullReq * _bitRate) ||
                 (strncmp(_sendCodec.plName, "I420", 4) == 0));

            
            actualBitrate = (float)(8.0/1000)*(totalBytesOneSec);
            
            
            
            rewind(_sourceFile);

            
            _vcm->SentFrameCount(frameCount);
            printf("frame count: %d delta, %d key\n", frameCount.numDeltaFrames, frameCount.numKeyFrames);
        }

    } 
    
    
    
    _vcm->InitializeSender();
    NumberOfCodecs = _vcm->NumberOfCodecs();
    bool encodeComplete = false;
    
    for (int k = 0; k < NumberOfCodecs; k++)
    {
        _vcm->Codec(k, &_sendCodec);
        _vcm->InitializeSender();
        _sendCodec.maxBitrate = 8000;
        _vcm->RegisterSendCodec(&_sendCodec, 4, 1440);
        _vcm->RegisterTransportCallback(_encodeCompleteCallback);

        _frameCnt = 0;
        encodeComplete = false;
        while (encodeComplete == false)
        {
            TEST(fread(tmpBuffer, 1, _lengthSourceFrame, _sourceFile) > 0);
            _frameCnt++;
            sourceFrame.CreateFrame(size_y, tmpBuffer,
                                    size_uv, tmpBuffer + size_y,
                                    size_uv, tmpBuffer + size_y + size_uv,
                                    _width, _height,
                                    _width, half_width, half_width);
            _timeStamp += (WebRtc_UWord32)(9e4 / static_cast<float>(_frameRate));
            sourceFrame.set_timestamp(_timeStamp);
            _vcm->AddVideoFrame(sourceFrame);
            encodeComplete = _encodeCompleteCallback->EncodeComplete();
        } 
        printf ("\n Codec type = %s \n", _sendCodec.plName);
        printf(" Encoder pipeline delay = %d frames\n", _frameCnt - 1);
    } 

    
    
    
    RTPSendCallback_SizeTest sendCallback;

    RtpRtcp::Configuration configuration;
    configuration.id = 1;
    configuration.audio = false;
    configuration.outgoing_transport = &sendCallback;

    RtpRtcp& rtpModule = *RtpRtcp::CreateRtpRtcp(configuration);

    VCMRTPEncodeCompleteCallback encCompleteCallback(&rtpModule);
    _vcm->InitializeSender();

    
    for (int k = 0; k < NumberOfCodecs; k++)
    {
        _vcm->Codec(k, &_sendCodec);
        if (strncmp(_sendCodec.plName, "I420", 4) == 0)
        {
            
            break;
        }
    }
    TEST(strncmp(_sendCodec.plName, "I420", 4) == 0);
    _vcm->InitializeSender();
    _sendCodec.maxFramerate = static_cast<WebRtc_UWord8>(_frameRate / 2.0 + 0.5f);
    _vcm->RegisterSendCodec(&_sendCodec, 4, 1440);
    _vcm->SetChannelParameters(2000, 0, 0);
    _vcm->RegisterTransportCallback(_encodeCompleteCallback);
    
    _vcm->SetChannelParameters((WebRtc_UWord32)_bitRate, 0, 20);
    _encodeCompleteCallback->Initialize();
    sendStats.SetTargetFrameRate(static_cast<WebRtc_UWord32>(_frameRate));
    _vcm->RegisterSendStatisticsCallback(&sendStats);
    rewind(_sourceFile);
    while (fread(tmpBuffer, 1, _lengthSourceFrame, _sourceFile) ==
        _lengthSourceFrame) {
        sourceFrame.CreateFrame(size_y, tmpBuffer,
                                size_uv, tmpBuffer + size_y,
                                size_uv, tmpBuffer + size_y + size_uv,
                                _width, _height,
                                _width, half_width, half_width);
        _timeStamp += (WebRtc_UWord32)(9e4 / static_cast<float>(_frameRate));
        sourceFrame.set_timestamp(_timeStamp);
        ret = _vcm->AddVideoFrame(sourceFrame);
        if (_vcm->TimeUntilNextProcess() <= 0)
        {
            _vcm->Process();
        }
        IncrementDebugClock(_frameRate);
    } 

    delete &rtpModule;
    Print();
    delete tmpBuffer;
    delete _decodeCallback;
    delete _encodeCompleteCallback;
    return 0;
}


void
GenericCodecTest::Print()
{
    printf(" \n\n VCM Generic Encoder Test: \n\n%i tests completed\n", vcmMacrosTests);
    if (vcmMacrosErrors > 0)
    {
        printf("%i FAILED\n\n", vcmMacrosErrors);
    }
    else
    {
        printf("ALL PASSED\n\n");
    }
}

float
GenericCodecTest::WaitForEncodedFrame() const
{
    WebRtc_Word64 startTime = _clock->MillisecondTimestamp();
    while (_clock->MillisecondTimestamp() - startTime < kMaxWaitEncTimeMs*10)
    {
        if (_encodeCompleteCallback->EncodeComplete())
        {
            return _encodeCompleteCallback->EncodedBytes();
        }
    }
    return 0;
}

void
GenericCodecTest::IncrementDebugClock(float frameRate)
{
    _clock->IncrementDebugClock(1000/frameRate);
}

int
RTPSendCallback_SizeTest::SendPacket(int channel, const void *data, int len)
{
    _nPackets++;
    _payloadSizeSum += len;
    
    TEST(len > 0 && static_cast<WebRtc_UWord32>(len - 12) <= _maxPayloadSize);
    return 0;
}

void
RTPSendCallback_SizeTest::SetMaxPayloadSize(WebRtc_UWord32 maxPayloadSize)
{
    _maxPayloadSize = maxPayloadSize;
}

void
RTPSendCallback_SizeTest::Reset()
{
    _nPackets = 0;
    _payloadSizeSum = 0;
}

float
RTPSendCallback_SizeTest::AveragePayloadSize() const
{
    if (_nPackets > 0)
    {
        return _payloadSizeSum / static_cast<float>(_nPackets);
    }
    return 0;
}

WebRtc_Word32
VCMEncComplete_KeyReqTest::SendData(
        const FrameType frameType,
        const WebRtc_UWord8 payloadType,
        const WebRtc_UWord32 timeStamp,
        int64_t capture_time_ms,
        const WebRtc_UWord8* payloadData,
        const WebRtc_UWord32 payloadSize,
        const RTPFragmentationHeader& ,
        const webrtc::RTPVideoHeader* )
{
    WebRtcRTPHeader rtpInfo;
    rtpInfo.header.markerBit = true; 
    rtpInfo.type.Video.codecHeader.VP8.InitRTPVideoHeaderVP8();
    rtpInfo.type.Video.codec = kRTPVideoVP8;
    rtpInfo.header.payloadType = payloadType;
    rtpInfo.header.sequenceNumber = _seqNo;
    _seqNo += 2;
    rtpInfo.header.ssrc = 0;
    rtpInfo.header.timestamp = _timeStamp;
    _timeStamp += 3000;
    rtpInfo.type.Video.isFirstPacket = false;
    rtpInfo.frameType = kVideoFrameKey;
    return _vcm.IncomingPacket(payloadData, payloadSize, rtpInfo);
}
