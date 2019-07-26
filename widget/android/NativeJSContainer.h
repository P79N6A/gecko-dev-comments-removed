




#ifndef NativeJSObject_h__
#define NativeJSObject_h__

#include <jni.h>
#include "jsapi.h"

namespace mozilla {
namespace widget {

jobject CreateNativeJSContainer(JNIEnv* env, JSContext* cx, JS::HandleObject object);

} 
} 

#endif 

