








#ifndef WEBRTC_VIDEO_ENGINE_MOCK_MOCK_VIE_FRAME_PROVIDER_BASE_H_
#define WEBRTC_VIDEO_ENGINE_MOCK_MOCK_VIE_FRAME_PROVIDER_BASE_H_

#include "webrtc/video_engine/vie_frame_provider_base.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace webrtc {

class MockViEFrameCallback : public ViEFrameCallback {
 public:
  MOCK_METHOD4(DeliverFrame,
               void(int id,
                    I420VideoFrame* video_frame,
                    int num_csrcs,
                    const uint32_t CSRC[kRtpCsrcSize]));
  MOCK_METHOD2(DelayChanged, void(int id, int frame_delay));
  MOCK_METHOD3(GetPreferedFrameSettings,
               int(int* width, int* height, int* frame_rate));
  MOCK_METHOD1(ProviderDestroyed, void(int id));
};

}  

#endif  
