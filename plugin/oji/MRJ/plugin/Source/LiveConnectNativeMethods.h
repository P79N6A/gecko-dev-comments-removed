







































#pragma once

#include "nsError.h"

#include "jni.h"

class MRJPlugin;

nsresult InitLiveConnectSupport(MRJPlugin* jvmPlugin);
nsresult ShutdownLiveConnectSupport(void);

jobject Wrap_JSObject(JNIEnv* env, jint js_obj);
jint Unwrap_JSObject(JNIEnv* env, jobject java_wrapper_obj);
