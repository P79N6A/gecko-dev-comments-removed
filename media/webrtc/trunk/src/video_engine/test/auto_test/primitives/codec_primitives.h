









#ifndef WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_CODEC_PRIMITIVES_H_
#define WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_CODEC_PRIMITIVES_H_

#include "video_engine/include/vie_codec.h"
#include "video_engine/include/vie_image_process.h"
#include "video_engine/test/auto_test/interface/vie_autotest_defines.h"
#include "video_engine/test/auto_test/primitives/general_primitives.h"

class TbInterfaces;









void TestCodecs(const TbInterfaces& interfaces,
                int capture_id,
                int video_channel,
                int forced_codec_width,
                int forced_codec_height);







void SetSendCodec(webrtc::VideoCodecType of_type,
                  webrtc::ViECodec* codec_interface,
                  int video_channel,
                  int forced_codec_width,
                  int forced_codec_height);

class ViEAutotestCodecObserver: public webrtc::ViEEncoderObserver,
                                public webrtc::ViEDecoderObserver {
 public:
  int incomingCodecCalled;
  int incomingRatecalled;
  int outgoingRatecalled;

  unsigned char lastPayloadType;
  unsigned short lastWidth;
  unsigned short lastHeight;

  unsigned int lastOutgoingFramerate;
  unsigned int lastOutgoingBitrate;
  unsigned int lastIncomingFramerate;
  unsigned int lastIncomingBitrate;

  webrtc::VideoCodec incomingCodec;

  ViEAutotestCodecObserver() {
    incomingCodecCalled = 0;
    incomingRatecalled = 0;
    outgoingRatecalled = 0;
    lastPayloadType = 0;
    lastWidth = 0;
    lastHeight = 0;
    lastOutgoingFramerate = 0;
    lastOutgoingBitrate = 0;
    lastIncomingFramerate = 0;
    lastIncomingBitrate = 0;
    memset(&incomingCodec, 0, sizeof(incomingCodec));
  }
  virtual void IncomingCodecChanged(const int videoChannel,
                                    const webrtc::VideoCodec& videoCodec) {
    incomingCodecCalled++;
    lastPayloadType = videoCodec.plType;
    lastWidth = videoCodec.width;
    lastHeight = videoCodec.height;

    memcpy(&incomingCodec, &videoCodec, sizeof(videoCodec));
  }

  virtual void IncomingRate(const int videoChannel,
                            const unsigned int framerate,
                            const unsigned int bitrate) {
    incomingRatecalled++;
    lastIncomingFramerate += framerate;
    lastIncomingBitrate += bitrate;
  }

  virtual void OutgoingRate(const int videoChannel,
                            const unsigned int framerate,
                            const unsigned int bitrate) {
    outgoingRatecalled++;
    lastOutgoingFramerate += framerate;
    lastOutgoingBitrate += bitrate;
  }

  virtual void RequestNewKeyFrame(const int videoChannel) {
  }
};

class FrameCounterEffectFilter : public webrtc::ViEEffectFilter
{
 public:
  int numFrames;
  FrameCounterEffectFilter() {
    numFrames = 0;
  }
  virtual ~FrameCounterEffectFilter() {
  }

  virtual int Transform(int size, unsigned char* frameBuffer,
                        unsigned int timeStamp90KHz, unsigned int width,
                        unsigned int height) {
    numFrames++;
    return 0;
  }
};

#endif  
