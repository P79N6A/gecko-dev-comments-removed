










#include <jni.h>


#ifndef _Included_org_webrtc_videoengineapp_ViEAndroidJavaAPI
#define _Included_org_webrtc_videoengineapp_ViEAndroidJavaAPI
#ifdef __cplusplus
extern "C" {
#endif





JNIEXPORT jboolean JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_NativeInit
(JNIEnv *, jobject, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_GetVideoEngine
(JNIEnv *, jobject);







JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_Init
(JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_Terminate
(JNIEnv *, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartSend
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopRender
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopSend
(JNIEnv *, jobject,jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartReceive
(JNIEnv *, jobject,jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopReceive
(JNIEnv *, jobject,jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_CreateChannel
(JNIEnv *, jobject,jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetLocalReceiver
(JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetSendDestination
(JNIEnv *, jobject, jint, jint, jbyteArray);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetReceiveCodec
(JNIEnv *, jobject, jint, jint, jint, jint, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetSendCodec
(JNIEnv *, jobject, jint, jint, jint, jint, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_AddRemoteRenderer
(JNIEnv *, jobject, jint, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_RemoveRemoteRenderer
(JNIEnv *, jobject,jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartRender
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartCamera
(JNIEnv *, jobject,jint channel,jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopCamera
(JNIEnv *, jobject,jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_GetCameraOrientation
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetRotation
(JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_EnableNACK
(JNIEnv *, jobject, jint, jboolean);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_EnablePLI
(JNIEnv *, jobject, jint, jboolean);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartSendNative
(JNIEnv *, jobject, jint, jint, jint, jbyteArray, jint, jint,
jint, jint, jint, jint, jobject, jint, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StartListenNative
(JNIEnv *, jobject, jint, jint, jint, jbyteArray, jint, jint,
jint, jint, jint, jint, jint, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_StopAllNative
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_SetCallback
(JNIEnv *, jobject, jint, jobject);






JNIEXPORT jboolean JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_Create
(JNIEnv *, jobject, jobject);






JNIEXPORT jboolean JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_Delete
(JNIEnv *, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_Init
(JNIEnv *, jobject, jint, jint, jint, jboolean, jboolean);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_Terminate
(JNIEnv *, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_CreateChannel
(JNIEnv *, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_DeleteChannel
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetLocalReceiver
(JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetSendDestination
(JNIEnv *, jobject, jint, jint, jstring);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StartListen
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StartPlayout
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StartSend
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StopListen
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StopPlayout
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StopSend
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetSpeakerVolume
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetLoudspeakerStatus
(JNIEnv *, jobject, jboolean);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StartPlayingFileLocally
(JNIEnv *, jobject, jint, jstring, jboolean);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StopPlayingFileLocally
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StartPlayingFileAsMicrophone
(JNIEnv *, jobject, jint, jstring, jboolean);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_StopPlayingFileAsMicrophone
(JNIEnv *, jobject, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_NumOfCodecs
(JNIEnv *, jobject);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetSendCodec
(JNIEnv *, jobject, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetECStatus
(JNIEnv *, jobject, jboolean, jint, jint, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetNSStatus
(JNIEnv *, jobject, jboolean, jint);






JNIEXPORT jint JNICALL
Java_org_webrtc_videoengineapp_ViEAndroidJavaAPI_VE_SetAGCStatus
(JNIEnv *, jobject, jboolean, jint);

#ifdef __cplusplus
}
#endif
#endif
