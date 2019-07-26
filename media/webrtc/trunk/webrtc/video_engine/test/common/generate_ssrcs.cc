









#include "webrtc/video_engine/test/common/generate_ssrcs.h"

#include <assert.h>
#include <stdlib.h>

#include "webrtc/video_engine/new_include/video_send_stream.h"

namespace webrtc {
namespace test {

void GenerateRandomSsrcs(VideoSendStream::Config* config,
                         std::map<uint32_t, bool>* reserved_ssrcs) {
  size_t num_ssrcs = config->codec.numberOfSimulcastStreams;
  std::vector<uint32_t>* ssrcs = &config->rtp.ssrcs;
  assert(ssrcs->size() == 0);

  if (num_ssrcs == 0) {
    num_ssrcs = 1;
  }

  while (ssrcs->size() < num_ssrcs) {
    uint32_t rand_ssrc = static_cast<uint32_t>(rand() + 1);  

    
    while (true) {
      if (!(*reserved_ssrcs)[rand_ssrc]) {
        (*reserved_ssrcs)[rand_ssrc] = true;
        break;
      }
      ++rand_ssrc;
    }

    ssrcs->push_back(rand_ssrc);
  }
}

}  
}  
