










#include <jni.h>


#ifndef _Included_org_webrtc_capturemoduleandroidtest_VideoCaptureModuleTest
#define _Included_org_webrtc_capturemoduleandroidtest_VideoCaptureModuleTest
#ifdef __cplusplus
extern "C" {
#endif





JNIEXPORT jint JNICALL Java_org_webrtc_capturemoduleandroidtest_VideoCaptureModuleTest_RunTest
  (JNIEnv *, jobject, jobject);

JNIEXPORT jint JNICALL Java_org_webrtc_capturemoduleandroidtest_VideoCaptureModuleTest_RenderInit
(JNIEnv * env, jobject context,jobject surface);

JNIEXPORT jint JNICALL Java_org_webrtc_capturemoduleandroidtest_VideoCaptureModuleTest_StartCapture
(JNIEnv *, jobject);

JNIEXPORT jint JNICALL Java_org_webrtc_capturemoduleandroidtest_VideoCaptureModuleTest_StopCapture
(JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
