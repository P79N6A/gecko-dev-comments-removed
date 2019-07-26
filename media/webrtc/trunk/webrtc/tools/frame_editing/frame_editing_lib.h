









#ifndef WEBRTC_TOOLS_FRAME_EDITING_FRAME_EDITING_H_
#define WEBRTC_TOOLS_FRAME_EDITING_FRAME_EDITING_H_

#include <string>

namespace webrtc {

















int EditFrames(const std::string& in_path, int width, int height,
                int first_frame_to_process, int interval,
                int last_frame_to_process, const std::string& out_path);
}  

#endif  
