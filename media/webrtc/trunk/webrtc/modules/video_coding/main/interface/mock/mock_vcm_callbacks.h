









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_INTERFACE_MOCK_MOCK_VCM_CALLBACKS_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_INTERFACE_MOCK_MOCK_VCM_CALLBACKS_H_

#include "gmock/gmock.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class MockVCMFrameTypeCallback : public VCMFrameTypeCallback {
 public:
  MOCK_METHOD0(RequestKeyFrame, int32_t());
  MOCK_METHOD1(SliceLossIndicationRequest,
               WebRtc_Word32(const WebRtc_UWord64 pictureId));
};

class MockPacketRequestCallback : public VCMPacketRequestCallback {
 public:
  MOCK_METHOD2(ResendPackets, int32_t(const uint16_t* sequenceNumbers,
                                      uint16_t length));
};

}  

#endif  
