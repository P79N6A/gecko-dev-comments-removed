










#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_MEDIA_OPT_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_MEDIA_OPT_TEST_H_


#include <string>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/test/receiver_tests.h"  
#include "webrtc/modules/video_coding/main/test/test_callbacks.h"
#include "webrtc/modules/video_coding/main/test/test_util.h"
#include "webrtc/modules/video_coding/main/test/video_source.h"









class MediaOptTest
{
public:
    MediaOptTest(webrtc::VideoCodingModule* vcm,
                 webrtc::Clock* clock);
    ~MediaOptTest();

    static int RunTest(int testNum, CmdArgs& args);
    
    int32_t Perform();
    
    void Setup(int testType, CmdArgs& args);
    
    void GeneralSetup();
    
    void RTTest();
    void TearDown();
    
    void Print(int mode);

private:

    webrtc::VideoCodingModule*       _vcm;
    webrtc::RtpReceiver*             rtp_receiver_;
    webrtc::RtpRtcp*                 _rtp;
    webrtc::RTPSendCompleteCallback* _outgoingTransport;
    RtpDataCallback*                 _dataCallback;

    webrtc::Clock*                   _clock;
    std::string                      _inname;
    std::string                      _outname;
    std::string                      _actualSourcename;
    std::fstream                     _log;
    FILE*                            _sourceFile;
    FILE*                            _decodedFile;
    FILE*                            _actualSourceFile;
    FILE*                            _outputRes;
    uint16_t                   _width;
    uint16_t                   _height;
    uint32_t                   _lengthSourceFrame;
    uint32_t                   _timeStamp;
    float                            _frameRate;
    bool                             _nackEnabled;
    bool                             _fecEnabled;
    bool                             _nackFecEnabled;
    uint8_t                    _rttMS;
    float                            _bitRate;
    double                           _lossRate;
    uint32_t                   _renderDelayMs;
    int32_t                    _frameCnt;
    float                            _sumEncBytes;
    int32_t                    _numFramesDropped;
    std::string                      _codecName;
    webrtc::VideoCodecType           _sendCodecType;
    int32_t                    _numberOfCores;

    
    FILE*                            _fpinp;
    FILE*                            _fpout;
    FILE*                            _fpout2;
    int                              _testType;
    int                              _testNum;
    int                              _numParRuns;

}; 

#endif 
