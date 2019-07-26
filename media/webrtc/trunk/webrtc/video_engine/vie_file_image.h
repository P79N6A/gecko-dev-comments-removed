









#ifndef WEBRTC_VIDEO_ENGINE_VIE_FILE_IMAGE_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FILE_IMAGE_H_

#include "common_video/interface/i420_video_frame.h"
#include "typedefs.h"  
#include "video_engine/include/vie_file.h"

namespace webrtc {

class ViEFileImage {
 public:
  static int ConvertJPEGToVideoFrame(int engine_id,
                                     const char* file_nameUTF8,
                                     I420VideoFrame* video_frame);
  static int ConvertPictureToI420VideoFrame(int engine_id,
                                            const ViEPicture& picture,
                                            I420VideoFrame* video_frame);
};

}  

#endif  
