
#include <jni.h>


#ifndef _Included_org_webrtc_voiceengine_test_AudioDeviceAndroidTest
#define _Included_org_webrtc_voiceengine_test_AudioDeviceAndroidTest
#ifdef __cplusplus
extern "C" {
#endif





JNIEXPORT jboolean JNICALL Java_org_webrtc_voiceengine_test_AudioDeviceAndroidTest_NativeInit
  (JNIEnv *, jclass);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AudioDeviceAndroidTest_RunTest
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif
