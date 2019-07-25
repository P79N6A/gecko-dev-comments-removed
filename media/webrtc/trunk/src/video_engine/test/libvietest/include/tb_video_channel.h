









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_TB_VIDEO_CHANNEL_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_TB_VIDEO_CHANNEL_H_

#include "video_engine/test/libvietest/include/tb_interfaces.h"

class TbVideoChannel
{
public:
    TbVideoChannel(TbInterfaces& Engine,
                   webrtc::VideoCodecType sendCodec = webrtc::kVideoCodecVP8,
                   int width = 352, int height = 288, int frameRate = 30,
                   int startBitrate = 300);

    ~TbVideoChannel(void);

    void SetFrameSettings(int width, int height, int frameRate);

    void StartSend(const unsigned short rtpPort = 11000,
                   const char* ipAddress = "127.0.0.1");

    void StopSend();

    void StartReceive(const unsigned short rtpPort = 11000);

    void StopReceive();

    int videoChannel;
private:
    TbInterfaces& ViE;
};

#endif  
