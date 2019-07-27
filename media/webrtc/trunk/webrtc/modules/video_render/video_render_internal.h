









#ifndef WEBRTC_MODULES_VIDEO_RENDER_VIDEO_RENDER_INTERNAL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_VIDEO_RENDER_INTERNAL_H_

#ifdef ANDROID
#include <jni.h>

namespace webrtc {



int32_t SetRenderAndroidVM(JavaVM* javaVM);

}  

#endif  

#endif  
