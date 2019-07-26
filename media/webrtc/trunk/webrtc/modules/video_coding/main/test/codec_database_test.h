









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
    int32_t Perform(CmdArgs& args);
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
    uint16_t                   _width;
    uint16_t                   _height;
    uint32_t                   _lengthSourceFrame;
    uint32_t                   _timeStamp;
    float                            _frameRate;
}; 

#endif 
