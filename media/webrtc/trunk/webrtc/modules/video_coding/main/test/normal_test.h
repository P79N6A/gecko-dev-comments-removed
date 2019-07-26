









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_NORMAL_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_NORMAL_TEST_H_

#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/test/test_util.h"
#include "webrtc/modules/video_coding/main/test/video_source.h"

#include <fstream>
#include <map>

class NormalTest;



class VCMNTEncodeCompleteCallback : public webrtc::VCMPacketizationCallback
{
 public:
  
  VCMNTEncodeCompleteCallback(FILE* encodedFile, NormalTest& test);
  virtual ~VCMNTEncodeCompleteCallback();
  
  void RegisterTransportCallback(webrtc::VCMPacketizationCallback* transport);
  
  
  int32_t
  SendData(const webrtc::FrameType frameType,
           const uint8_t payloadType,
           const uint32_t timeStamp,
           int64_t capture_time_ms,
           const uint8_t* payloadData,
           const uint32_t payloadSize,
           const webrtc::RTPFragmentationHeader& fragmentationHeader,
           const webrtc::RTPVideoHeader* videoHdr);

  
  
  void RegisterReceiverVCM(webrtc::VideoCodingModule *vcm);
  
  int32_t EncodedBytes();
  
  uint32_t SkipCnt();;
  


 private:
  FILE*                       _encodedFile;
  uint32_t              _encodedBytes;
  uint32_t              _skipCnt;
  webrtc::VideoCodingModule*  _VCMReceiver;
  webrtc::FrameType           _frameType;
  uint16_t              _seqNo;
  NormalTest&                 _test;
}; 

class VCMNTDecodeCompleCallback: public webrtc::VCMReceiveCallback
{
public:
    VCMNTDecodeCompleCallback(std::string outname): 
        _decodedFile(NULL),
        _outname(outname),
        _decodedBytes(0),
        _currentWidth(0),
        _currentHeight(0) {}
    virtual ~VCMNTDecodeCompleCallback();
    void SetUserReceiveCallback(webrtc::VCMReceiveCallback* receiveCallback);
    
    int32_t FrameToRender(webrtc::I420VideoFrame& videoFrame);
    int32_t DecodedBytes();
private:
    FILE*             _decodedFile;
    std::string       _outname;
    int               _decodedBytes;
    int               _currentWidth;
    int               _currentHeight;
}; 

class NormalTest
{
public:
    NormalTest(webrtc::VideoCodingModule* vcm,
               webrtc::Clock* clock);
    ~NormalTest();
    static int RunTest(const CmdArgs& args);
    int32_t    Perform(const CmdArgs& args);
    
    int   Width() const { return _width; };
    int   Height() const { return _height; };
    webrtc::VideoCodecType VideoType() const { return _videoType; };


protected:
    
    void            Setup(const CmdArgs& args);
   
    void            Teardown();
    
    void            Print();
    
    void            FrameEncoded(uint32_t timeStamp);
    
    void            FrameDecoded(uint32_t timeStamp);

    webrtc::Clock*                   _clock;
    webrtc::VideoCodingModule*       _vcm;
    webrtc::VideoCodec               _sendCodec;
    webrtc::VideoCodec               _receiveCodec;
    std::string                      _inname;
    std::string                      _outname;
    std::string                      _encodedName;
    int32_t                    _sumEncBytes;
    FILE*                            _sourceFile;
    FILE*                            _decodedFile;
    FILE*                            _encodedFile;
    std::fstream                     _log;
    int                              _width;
    int                              _height;
    float                            _frameRate;
    float                            _bitRate;
    uint32_t                   _lengthSourceFrame;
    uint32_t                   _timeStamp;
    webrtc::VideoCodecType           _videoType;
    double                           _totalEncodeTime;
    double                           _totalDecodeTime;
    double                           _decodeCompleteTime;
    double                           _encodeCompleteTime;
    double                           _totalEncodePipeTime;
    double                           _totalDecodePipeTime;
    double                           _testTotalTime;
    std::map<int, double>            _encodeTimes;
    std::map<int, double>            _decodeTimes;
    int32_t                    _frameCnt;
    int32_t                    _encFrameCnt;
    int32_t                    _decFrameCnt;

}; 

#endif 
