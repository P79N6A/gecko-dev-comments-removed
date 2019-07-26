
#include <jni.h>


#ifndef _Included_org_webrtc_voiceengine_test_AndroidTest
#define _Included_org_webrtc_voiceengine_test_AndroidTest
#ifdef __cplusplus
extern "C" {
#endif





JNIEXPORT jboolean JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_NativeInit
  (JNIEnv *, jclass);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_RunAutoTest
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jboolean JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_Create
  (JNIEnv *, jobject);






JNIEXPORT jboolean JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_Delete
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_Init
  (JNIEnv *, jobject, jboolean, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_Terminate
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_CreateChannel
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_DeleteChannel
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetLocalReceiver
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetSendDestination
  (JNIEnv *, jobject, jint, jint, jstring);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StartListen
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StartPlayout
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StartSend
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StopListen
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StopPlayout
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StopSend
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StartPlayingFileLocally
  (JNIEnv *, jobject, jint, jstring, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StopPlayingFileLocally
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StartRecordingPlayout
  (JNIEnv *, jobject, jint, jstring, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StopRecordingPlayout
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StartPlayingFileAsMicrophone
  (JNIEnv *, jobject, jint, jstring, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_StopPlayingFileAsMicrophone
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_NumOfCodecs
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetSendCodec
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetVADStatus
  (JNIEnv *, jobject, jint, jboolean, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetNSStatus
  (JNIEnv *, jobject, jboolean, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetAGCStatus
  (JNIEnv *, jobject, jboolean, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetECStatus
  (JNIEnv *, jobject, jboolean, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetSpeakerVolume
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_voiceengine_test_AndroidTest_SetLoudspeakerStatus
  (JNIEnv *, jobject, jboolean);

#ifdef __cplusplus
}
#endif
#endif
