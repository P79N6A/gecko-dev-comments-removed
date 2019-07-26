









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_NORMAL_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_NORMAL_TEST_H_

#include "video_coding.h"
#include "test_util.h"

#include <map>

class NormalTest;



class VCMNTEncodeCompleteCallback : public webrtc::VCMPacketizationCallback
{
 public:
  
  VCMNTEncodeCompleteCallback(FILE* encodedFile, NormalTest& test);
  virtual ~VCMNTEncodeCompleteCallback();
  
  void RegisterTransportCallback(webrtc::VCMPacketizationCallback* transport);
  
  
  WebRtc_Word32
  SendData(const webrtc::FrameType frameType,
           const WebRtc_UWord8 payloadType,
           const WebRtc_UWord32 timeStamp,
           int64_t capture_time_ms,
           const WebRtc_UWord8* payloadData,
           const WebRtc_UWord32 payloadSize,
           const webrtc::RTPFragmentationHeader& fragmentationHeader,
           const webrtc::RTPVideoHeader* videoHdr);

  
  
  void RegisterReceiverVCM(webrtc::VideoCodingModule *vcm);
  
  WebRtc_Word32 EncodedBytes();
  
  WebRtc_UWord32 SkipCnt();;
  


 private:
  FILE*                       _encodedFile;
  WebRtc_UWord32              _encodedBytes;
  WebRtc_UWord32              _skipCnt;
  webrtc::VideoCodingModule*  _VCMReceiver;
  webrtc::FrameType           _frameType;
  WebRtc_UWord16              _seqNo;
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
    
    WebRtc_Word32 FrameToRender(webrtc::I420VideoFrame& videoFrame);
    WebRtc_Word32 DecodedBytes();
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
               webrtc::TickTimeBase* clock);
    ~NormalTest();
    static int RunTest(const CmdArgs& args);
    WebRtc_Word32    Perform(const CmdArgs& args);
    
    int   Width() const { return _width; };
    int   Height() const { return _height; };
    webrtc::VideoCodecType VideoType() const { return _videoType; };


protected:
    
    void            Setup(const CmdArgs& args);
   
    void            Teardown();
    
    void            Print();
    
    void            FrameEncoded(WebRtc_UWord32 timeStamp);
    
    void            FrameDecoded(WebRtc_UWord32 timeStamp);

    webrtc::TickTimeBase*            _clock;
    webrtc::VideoCodingModule*       _vcm;
    webrtc::VideoCodec               _sendCodec;
    webrtc::VideoCodec               _receiveCodec;
    std::string                      _inname;
    std::string                      _outname;
    std::string                      _encodedName;
    WebRtc_Word32                    _sumEncBytes;
    FILE*                            _sourceFile;
    FILE*                            _decodedFile;
    FILE*                            _encodedFile;
    std::fstream                     _log;
    int                              _width;
    int                              _height;
    float                            _frameRate;
    float                            _bitRate;
    WebRtc_UWord32                   _lengthSourceFrame;
    WebRtc_UWord32                   _timeStamp;
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
    WebRtc_Word32                    _frameCnt;
    WebRtc_Word32                    _encFrameCnt;
    WebRtc_Word32                    _decFrameCnt;

}; 

#endif 
