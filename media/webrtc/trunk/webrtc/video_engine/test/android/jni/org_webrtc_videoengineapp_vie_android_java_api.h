










#include <jni.h>


#ifndef _Included_org_webrtc_videoengineapp_ViEAndroidJavaAPI
#define _Included_org_webrtc_videoengineapp_ViEAndroidJavaAPI
#ifdef __cplusplus
extern "C" {
#endif





JNIEXPORT jboolean JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_NativeInit
  (JNIEnv *, jobject, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_GetVideoEngine
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_Init
  (JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_Terminate
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartSend
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopRender
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopSend
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartReceive
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopReceive
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_CreateChannel
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetLocalReceiver
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetSendDestination
  (JNIEnv *, jobject, jint, jint, jstring);






JNIEXPORT jobjectArray JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_GetCodecs
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetReceiveCodec
  (JNIEnv *, jobject, jint, jint, jint, jint, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetSendCodec
  (JNIEnv *, jobject, jint, jint, jint, jint, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_AddRemoteRenderer
  (JNIEnv *, jobject, jint, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_RemoveRemoteRenderer
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartRender
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartCamera
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopCamera
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_GetCameraOrientation
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetRotation
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetExternalMediaCodecDecoderRenderer
  (JNIEnv *, jobject, jint, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_EnableNACK
  (JNIEnv *, jobject, jint, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_EnablePLI
  (JNIEnv *, jobject, jint, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetCallback
  (JNIEnv *, jobject, jint, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartIncomingRTPDump
  (JNIEnv *, jobject, jint, jstring);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopIncomingRTPDump
  (JNIEnv *, jobject, jint);






JNIEXPORT jboolean JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1Create
  (JNIEnv *, jobject, jobject);






JNIEXPORT jboolean JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1Delete
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1Init
  (JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1Terminate
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1CreateChannel
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1DeleteChannel
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetLocalReceiver
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetSendDestination
  (JNIEnv *, jobject, jint, jint, jstring);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StartListen
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StartPlayout
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StartSend
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StopListen
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StopPlayout
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StopSend
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetSpeakerVolume
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetLoudspeakerStatus
  (JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StartPlayingFileLocally
  (JNIEnv *, jobject, jint, jstring, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StopPlayingFileLocally
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StartPlayingFileAsMicrophone
  (JNIEnv *, jobject, jint, jstring, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StopPlayingFileAsMicrophone
  (JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1NumOfCodecs
  (JNIEnv *, jobject);






JNIEXPORT jobjectArray JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1GetCodecs
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetSendCodec
  (JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetECStatus
  (JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetAGCStatus
  (JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1SetNSStatus
  (JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StartDebugRecording
  (JNIEnv *, jobject, jstring);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StopDebugRecording
  (JNIEnv *, jobject);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StartIncomingRTPDump
  (JNIEnv *, jobject, jint, jstring);






JNIEXPORT jint JNICALL Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VoE_1StopIncomingRTPDump
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif
