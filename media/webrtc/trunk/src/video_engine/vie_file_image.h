









#ifndef WEBRTC_VIDEO_ENGINE_VIE_FILE_IMAGE_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FILE_IMAGE_H_

#include "modules/interface/module_common_types.h"
#include "typedefs.h"  
#include "video_engine/include/vie_file.h"

namespace webrtc {

class ViEFileImage {
 public:
  static int ConvertJPEGToVideoFrame(int engine_id,
                                     const char* file_nameUTF8,
                                     VideoFrame* video_frame);
  static int ConvertPictureToVideoFrame(int engine_id,
                                        const ViEPicture& picture,
                                        VideoFrame* video_frame);
};

}  

#endif  
