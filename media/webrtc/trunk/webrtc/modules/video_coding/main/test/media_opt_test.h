










#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_MEDIA_OPT_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_MEDIA_OPT_TEST_H_


#include <string>

#include "receiver_tests.h"  
#include "rtp_rtcp.h"
#include "test_callbacks.h"
#include "test_util.h"
#include "video_coding.h"
#include "video_source.h"









class MediaOptTest
{
public:
    MediaOptTest(webrtc::VideoCodingModule* vcm,
                 webrtc::TickTimeBase* clock);
    ~MediaOptTest();

    static int RunTest(int testNum, CmdArgs& args);
    
    WebRtc_Word32 Perform();
    
    void Setup(int testType, CmdArgs& args);
    
    void GeneralSetup();
    
    void RTTest();
    void TearDown();
    
    void Print(int mode);

private:

    webrtc::VideoCodingModule*       _vcm;
    webrtc::RtpRtcp*                 _rtp;
    webrtc::RTPSendCompleteCallback* _outgoingTransport;
    RtpDataCallback*                 _dataCallback;

    webrtc::TickTimeBase*            _clock;
    std::string                      _inname;
    std::string                      _outname;
    std::string                      _actualSourcename;
    std::fstream                     _log;
    FILE*                            _sourceFile;
    FILE*                            _decodedFile;
    FILE*                            _actualSourceFile;
    FILE*                            _outputRes;
    WebRtc_UWord16                   _width;
    WebRtc_UWord16                   _height;
    WebRtc_UWord32                   _lengthSourceFrame;
    WebRtc_UWord32                   _timeStamp;
    float                            _frameRate;
    bool                             _nackEnabled;
    bool                             _fecEnabled;
    bool                             _nackFecEnabled;
    WebRtc_UWord8                    _rttMS;
    float                            _bitRate;
    double                           _lossRate;
    WebRtc_UWord32                   _renderDelayMs;
    WebRtc_Word32                    _frameCnt;
    float                            _sumEncBytes;
    WebRtc_Word32                    _numFramesDropped;
    std::string                      _codecName;
    webrtc::VideoCodecType           _sendCodecType;
    WebRtc_Word32                    _numberOfCores;

    
    FILE*                            _fpinp;
    FILE*                            _fpout;
    FILE*                            _fpout2;
    int                              _testType;
    int                              _testNum;
    int                              _numParRuns;

}; 

#endif 
