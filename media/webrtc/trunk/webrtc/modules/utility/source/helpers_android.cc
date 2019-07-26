









#include "webrtc/modules/utility/interface/helpers_android.h"

#include <assert.h>
#include <stddef.h>

namespace webrtc {

AttachThreadScoped::AttachThreadScoped(JavaVM* jvm)
    : attached_(false), jvm_(jvm), env_(NULL) {
  jint ret_val = jvm->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_4);
  if (ret_val == JNI_EDETACHED) {
    
    ret_val = jvm_->AttachCurrentThread(&env_, NULL);
    attached_ = ret_val == JNI_OK;
    assert(attached_);
  }
}

AttachThreadScoped::~AttachThreadScoped() {
  if (attached_ && (jvm_->DetachCurrentThread() < 0)) {
    assert(false);
  }
}

JNIEnv* AttachThreadScoped::env() { return env_; }

}  
