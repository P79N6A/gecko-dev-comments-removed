











#ifndef ANDROID

namespace webrtc {
namespace videocapturemodule {
void EnsureInitialized() {}
}  
}  

#else

#include <pthread.h>

#include "base/android/jni_android.h"
#include "webrtc/base/checks.h"
#include "webrtc/modules/video_capture/video_capture_internal.h"

namespace webrtc {
namespace videocapturemodule {

static pthread_once_t g_initialize_once = PTHREAD_ONCE_INIT;

void EnsureInitializedOnce() {
  JNIEnv* jni = ::base::android::AttachCurrentThread();
  jobject context = ::base::android::GetApplicationContext();
  JavaVM* jvm = NULL;
  CHECK_EQ(0, jni->GetJavaVM(&jvm));
  CHECK_EQ(0, webrtc::SetCaptureAndroidVM(jvm, context));
}

void EnsureInitialized() {
  CHECK_EQ(0, pthread_once(&g_initialize_once, &EnsureInitializedOnce));
}

}  
}  

#endif  
