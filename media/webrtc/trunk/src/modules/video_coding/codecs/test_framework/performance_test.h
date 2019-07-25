









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_PERFORMANCE_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_PERFORMANCE_TEST_H_

#include "normal_async_test.h"
#include "thread_wrapper.h"
#include "rw_lock_wrapper.h"
#include "event_wrapper.h"

class PerformanceTest : public NormalAsyncTest
{
public:
    PerformanceTest(WebRtc_UWord32 bitRate, WebRtc_UWord8 numCodecs);
    virtual ~PerformanceTest();

    virtual void Perform();
    virtual void Print() {};

protected:
    PerformanceTest(WebRtc_UWord32 bitRate);
    virtual void Setup();
    virtual bool Encode();
    virtual int Decode(int lossValue = 0);
    virtual void Teardown();
    static bool RunThread(void* obj);
    bool PerformSingleTest();

    virtual webrtc::VideoEncoder* CreateEncoder() const { return NULL; };
    virtual webrtc::VideoDecoder* CreateDecoder() const { return NULL; };

    WebRtc_UWord8                 _numCodecs;
    PerformanceTest**             _tests;
    webrtc::VideoEncoder**        _encoders;
    webrtc::VideoDecoder**        _decoders;
    webrtc::ThreadWrapper**       _threads;
    webrtc::RWLockWrapper*        _rawImageLock;
    webrtc::EventWrapper**        _encodeEvents;
    FrameQueue                    _frameQueue;
    bool                          _stopped;
    webrtc::EncodedImageCallback* _encodeCompleteCallback;
    webrtc::DecodedImageCallback* _decodeCompleteCallback;
    FILE*                         _outFile;
};

#endif 
