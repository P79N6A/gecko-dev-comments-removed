









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_PACKET_LOSS_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_PACKET_LOSS_TEST_H_

#include "modules/video_coding/codecs/test_framework/packet_loss_test.h"

class VP8PacketLossTest : public PacketLossTest
{
public:
    VP8PacketLossTest();
    VP8PacketLossTest(double lossRate, bool useNack, int rttFrames);

protected:
    VP8PacketLossTest(std::string name, std::string description);
    virtual int ByteLoss(int size, unsigned char *pkg, int bytesToLose);
    WebRtc_Word32 ReceivedDecodedReferenceFrame(const WebRtc_UWord64 pictureId);
    
    
    virtual bool PacketLoss(double lossRate, int numLosses);

    webrtc::CodecSpecificInfo* CreateEncoderSpecificInfo() const;

};

#endif 
