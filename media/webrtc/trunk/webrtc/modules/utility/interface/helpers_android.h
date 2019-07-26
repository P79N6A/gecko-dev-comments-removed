









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_HELPERS_ANDROID_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_HELPERS_ANDROID_H_

#include <jni.h>

namespace webrtc {



class AttachThreadScoped {
 public:
  explicit AttachThreadScoped(JavaVM* jvm);
  ~AttachThreadScoped();
  JNIEnv* env();

 private:
  bool attached_;
  JavaVM* jvm_;
  JNIEnv* env_;
};

}  

#endif  
