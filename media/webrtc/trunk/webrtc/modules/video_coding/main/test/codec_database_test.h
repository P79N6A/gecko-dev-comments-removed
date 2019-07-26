









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_CODEC_DATABASE_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_CODEC_DATABASE_TEST_H_

#include "video_coding.h"
#include "test_util.h"

#include <string.h>








class CodecDataBaseTest
{
public:
    CodecDataBaseTest(webrtc::VideoCodingModule* vcm);
    ~CodecDataBaseTest();
    static int RunTest(CmdArgs& args);
    WebRtc_Word32 Perform(CmdArgs& args);
private:
    void TearDown();
    void Setup(CmdArgs& args);
    void Print();
    webrtc::VideoCodingModule*       _vcm;
    std::string                      _inname;
    std::string                      _outname;
    std::string                      _encodedName;
    FILE*                            _sourceFile;
    FILE*                            _decodedFile;
    FILE*                            _encodedFile;
    WebRtc_UWord16                   _width;
    WebRtc_UWord16                   _height;
    WebRtc_UWord32                   _lengthSourceFrame;
    WebRtc_UWord32                   _timeStamp;
    float                            _frameRate;
}; 

#endif 
