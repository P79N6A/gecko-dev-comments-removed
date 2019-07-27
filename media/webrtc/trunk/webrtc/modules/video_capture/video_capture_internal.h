









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_VIDEO_CAPTURE_INTERNAL_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_VIDEO_CAPTURE_INTERNAL_H_

#ifdef ANDROID
#include <jni.h>

namespace webrtc {



int32_t SetCaptureAndroidVM(JavaVM* javaVM, jobject context);

}  

#endif  

#endif  
