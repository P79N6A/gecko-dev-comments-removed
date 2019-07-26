









#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_GENERATE_SSRCS_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_GENERATE_SSRCS_H_

#include <map>
#include <vector>

#include "webrtc/typedefs.h"
#include "webrtc/video_engine/new_include/video_send_stream.h"

namespace webrtc {
namespace test {

void GenerateRandomSsrcs(VideoSendStream::Config* config,
                         std::map<uint32_t, bool>* reserved_ssrcs);

}  
}  

#endif  
