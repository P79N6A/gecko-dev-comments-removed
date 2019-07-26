









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_TEST_UTIL_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_TEST_UTIL_H_





#include <string.h>
#include <fstream>
#include <cstdlib>

#include "module_common_types.h"
#include "testsupport/fileutils.h"


class CmdArgs
{
 public:
  CmdArgs()
      : codecName("VP8"),
        codecType(webrtc::kVideoCodecVP8),
        width(352),
        height(288),
        bitRate(500),
        frameRate(30),
        packetLoss(0),
        rtt(0),
        protectionMode(0),
        camaEnable(0),
        inputFile(webrtc::test::ProjectRootPath() +
                  "/resources/foreman_cif.yuv"),
        outputFile(webrtc::test::OutputPath() +
                   "video_coding_test_output_352x288.yuv"),
        fv_outputfile(webrtc::test::OutputPath() + "features.txt"),
        testNum(0) {}
     std::string codecName;
     webrtc::VideoCodecType codecType;
     int width;
     int height;
     int bitRate;
     int frameRate;
     int packetLoss;
     int rtt;
     int protectionMode;
     int camaEnable;
     std::string inputFile;
     std::string outputFile;
     std::string fv_outputfile;
     int testNum;
};


int MTRxTxTest(CmdArgs& args);
double NormalDist(double mean, double stdDev);

struct RtpPacket {
  WebRtc_Word8 data[1650]; 
  WebRtc_Word32 length;
  WebRtc_Word64 receiveTime;
};



webrtc::RTPVideoCodecTypes
ConvertCodecType(const char* plname);

#endif
