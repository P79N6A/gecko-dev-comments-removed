









#include "video_capture_android.h"

#include <stdio.h>

#include "critical_section_wrapper.h"
#include "ref_count.h"
#include "trace.h"

namespace webrtc
{
#if defined(WEBRTC_ANDROID) && !defined(WEBRTC_CHROMIUM_BUILD)


WebRtc_Word32 SetCaptureAndroidVM(void* javaVM, void* javaContext) {
  return videocapturemodule::VideoCaptureAndroid::SetAndroidObjects(
      javaVM,
      javaContext);
}
#endif

namespace videocapturemodule
{

VideoCaptureModule* VideoCaptureImpl::Create(
    const WebRtc_Word32 id,
    const char* deviceUniqueIdUTF8) {

  RefCountImpl<videocapturemodule::VideoCaptureAndroid>* implementation =
      new RefCountImpl<videocapturemodule::VideoCaptureAndroid>(id);

  if (!implementation || implementation->Init(id, deviceUniqueIdUTF8) != 0) {
    delete implementation;
    implementation = NULL;
  }
  return implementation;
}








JavaVM* VideoCaptureAndroid::g_jvm = NULL;

jclass VideoCaptureAndroid::g_javaCmClass = NULL;

jclass VideoCaptureAndroid::g_javaCmDevInfoClass = NULL;

jobject VideoCaptureAndroid::g_javaCmDevInfoObject = NULL;
jobject VideoCaptureAndroid::g_javaContext = NULL;




WebRtc_Word32 VideoCaptureAndroid::SetAndroidObjects(void* javaVM,
                                                     void* javaContext) {

  g_jvm = static_cast<JavaVM*> (javaVM);
  g_javaContext = static_cast<jobject> (javaContext);

  if (javaVM) {
    JNIEnv* env = NULL;
    if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not get Java environment", __FUNCTION__);
      return -1;
    }
    
    jclass javaCmClassLocal = env->FindClass(AndroidJavaCaptureClass);
    if (!javaCmClassLocal) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not find java class", __FUNCTION__);
      return -1;
    }
    
    
    
    g_javaCmClass = static_cast<jclass>
        (env->NewGlobalRef(javaCmClassLocal));
    if (!g_javaCmClass) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: InitVideoEngineJava(): could not create"
                   " Java Camera class reference",
                   __FUNCTION__);
      return -1;
    }
    
    env->DeleteLocalRef(javaCmClassLocal);
    JNINativeMethod nativeFunctions =
        { "ProvideCameraFrame", "([BIJ)V",
          (void*) &VideoCaptureAndroid::ProvideCameraFrame };
    if (env->RegisterNatives(g_javaCmClass, &nativeFunctions, 1) == 0) {
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                   "%s: Registered native functions", __FUNCTION__);
    }
    else {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: Failed to register native functions",
                   __FUNCTION__);
      return -1;
    }

    
    jclass javaCmDevInfoClassLocal = env->FindClass(
        AndroidJavaCaptureDeviceInfoClass);
    if (!javaCmDevInfoClassLocal) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not find java class", __FUNCTION__);
      return -1;
    }

    
    
    
    g_javaCmDevInfoClass = static_cast<jclass>
        (env->NewGlobalRef(javaCmDevInfoClassLocal));
    if (!g_javaCmDevInfoClass) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: InitVideoEngineJava(): could not create Java "
                   "Camera Device info class reference",
                   __FUNCTION__);
      return -1;
    }
    
    env->DeleteLocalRef(javaCmDevInfoClassLocal);

    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "VideoCaptureDeviceInfoAndroid get method id");

    
    
    jmethodID cid = env->GetStaticMethodID(
        g_javaCmDevInfoClass,
        "CreateVideoCaptureDeviceInfoAndroid",
        "(ILandroid/content/Context;)"
        "Lorg/webrtc/videoengine/VideoCaptureDeviceInfoAndroid;");
    if (cid == NULL) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not get java"
                   "VideoCaptureDeviceInfoAndroid constructor ID",
                   __FUNCTION__);
      return -1;
    }

    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "%s: construct static java device object", __FUNCTION__);

    
    jobject javaCameraDeviceInfoObjLocal =
        env->CallStaticObjectMethod(g_javaCmDevInfoClass,
                                    cid, (int) -1,
                                    g_javaContext);
    if (!javaCameraDeviceInfoObjLocal) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, -1,
                   "%s: could not create Java Capture Device info object",
                   __FUNCTION__);
      return -1;
    }
    
    
    g_javaCmDevInfoObject = env->NewGlobalRef(javaCameraDeviceInfoObjLocal);
    if (!g_javaCmDevInfoObject) {
      WEBRTC_TRACE(webrtc::kTraceError,
                   webrtc::kTraceAudioDevice,
                   -1,
                   "%s: could not create Java"
                   "cameradevinceinfo object reference",
                   __FUNCTION__);
      return -1;
    }
    
    env->DeleteLocalRef(javaCameraDeviceInfoObjLocal);
    return 0;
  }
  else {
    WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
                 "%s: JVM is NULL, assuming deinit", __FUNCTION__);
    if (!g_jvm) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: SetAndroidObjects not called with a valid JVM.",
                   __FUNCTION__);
      return -1;
    }
    JNIEnv* env = NULL;
    bool attached = false;
    if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      
      
      jint res = g_jvm->AttachCurrentThread(&env, NULL);
      if ((res < 0) || !env) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                     -1, "%s: Could not attach thread to JVM (%d, %p)",
                     __FUNCTION__, res, env);
        return -1;
      }
      attached = true;
    }
    env->DeleteGlobalRef(g_javaCmDevInfoObject);
    env->DeleteGlobalRef(g_javaCmDevInfoClass);
    env->DeleteGlobalRef(g_javaCmClass);
    if (attached && g_jvm->DetachCurrentThread() < 0) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, -1,
                   "%s: Could not detach thread from JVM", __FUNCTION__);
      return -1;
    }
    return 0;
    env = (JNIEnv *) NULL;
  }
  return 0;
}

WebRtc_Word32 VideoCaptureAndroid::AttachAndUseAndroidDeviceInfoObjects(
    JNIEnv*& env,
    jclass& javaCmDevInfoClass,
    jobject& javaCmDevInfoObject,
    bool& attached) {
  
  if (!g_jvm) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: SetAndroidObjects not called with a valid JVM.",
                 __FUNCTION__);
    return -1;
  }
  attached = false;
  if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    
    
    jint res = g_jvm->AttachCurrentThread(&env, NULL);
    if ((res < 0) || !env) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: Could not attach thread to JVM (%d, %p)",
                   __FUNCTION__, res, env);
      return -1;
    }
    attached = true;
  }
  javaCmDevInfoClass = g_javaCmDevInfoClass;
  javaCmDevInfoObject = g_javaCmDevInfoObject;
  return 0;

}

WebRtc_Word32 VideoCaptureAndroid::ReleaseAndroidDeviceInfoObjects(
    bool attached) {
  if (attached && g_jvm->DetachCurrentThread() < 0) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, -1,
                 "%s: Could not detach thread from JVM", __FUNCTION__);
    return -1;
  }
  return 0;
}








void JNICALL VideoCaptureAndroid::ProvideCameraFrame(JNIEnv * env,
                                                     jobject,
                                                     jbyteArray javaCameraFrame,
                                                     jint length,
                                                     jlong context) {
  VideoCaptureAndroid* captureModule =
      reinterpret_cast<VideoCaptureAndroid*>(context);
  WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture,
               -1, "%s: IncomingFrame %d", __FUNCTION__,length);
  jbyte* cameraFrame= env->GetByteArrayElements(javaCameraFrame,NULL);
  captureModule->IncomingFrame((WebRtc_UWord8*) cameraFrame,
                               length,captureModule->_frameInfo,0);
  env->ReleaseByteArrayElements(javaCameraFrame,cameraFrame,JNI_ABORT);
}



VideoCaptureAndroid::VideoCaptureAndroid(const WebRtc_Word32 id)
    : VideoCaptureImpl(id), _capInfo(id), _javaCaptureObj(NULL),
      _captureStarted(false) {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
               "%s: context %x", __FUNCTION__, (int) this);
}







WebRtc_Word32 VideoCaptureAndroid::Init(const WebRtc_Word32 id,
                                        const char* deviceUniqueIdUTF8) {
  const int nameLength = strlen(deviceUniqueIdUTF8);
  if (nameLength >= kVideoCaptureUniqueNameLength) {
    return -1;
  }

  
  _deviceUniqueId = new char[nameLength + 1];
  memcpy(_deviceUniqueId, deviceUniqueIdUTF8, nameLength + 1);

  if (_capInfo.Init() != 0) {
    WEBRTC_TRACE(webrtc::kTraceError,
                 webrtc::kTraceVideoCapture,
                 _id,
                 "%s: Failed to initialize CaptureDeviceInfo",
                 __FUNCTION__);
    return -1;
  }

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1, "%s:",
               __FUNCTION__);
  
  if (!g_jvm) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                 "%s: Not a valid Java VM pointer", __FUNCTION__);
    return -1;
  }
  
  JNIEnv *env;
  bool isAttached = false;

  
  if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    
    
    jint res = g_jvm->AttachCurrentThread(&env, NULL);
    if ((res < 0) || !env) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                   "%s: Could not attach thread to JVM (%d, %p)",
                   __FUNCTION__, res, env);
      return -1;
    }
    isAttached = true;
  }

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
               "get method id");

  
  
  char signature[256];
  sprintf(signature, "(IJLjava/lang/String;)L%s;", AndroidJavaCaptureClass);

  jmethodID cid = env->GetMethodID(g_javaCmDevInfoClass, "AllocateCamera",
                                   signature);
  if (cid == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                 "%s: could not get constructor ID", __FUNCTION__);
    return -1; 
  }

  jstring capureIdString = env->NewStringUTF((char*) deviceUniqueIdUTF8);
  
  jobject javaCameraObjLocal = env->CallObjectMethod(g_javaCmDevInfoObject,
                                                     cid, (jint) id,
                                                     (jlong) this,
                                                     capureIdString);
  if (!javaCameraObjLocal) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                 "%s: could not create Java Capture object", __FUNCTION__);
    return -1;
  }

  
  
  _javaCaptureObj = env->NewGlobalRef(javaCameraObjLocal);
  if (!_javaCaptureObj) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioDevice, _id,
                 "%s: could not create Java camera object reference",
                 __FUNCTION__);
    return -1;
  }

  
  env->DeleteLocalRef(javaCameraObjLocal);

  
  if (isAttached) {
    if (g_jvm->DetachCurrentThread() < 0) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioDevice, _id,
                   "%s: Could not detach thread from JVM", __FUNCTION__);
    }
  }

  return 0;
}

VideoCaptureAndroid::~VideoCaptureAndroid() {
  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1, "%s:",
               __FUNCTION__);
  if (_javaCaptureObj == NULL || g_jvm == NULL) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: Nothing to clean", __FUNCTION__);
  }
  else {
    bool isAttached = false;
    
    JNIEnv *env;
    if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      
      
      jint res = g_jvm->AttachCurrentThread(&env, NULL);
      if ((res < 0) || !env) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                     _id,
                     "%s: Could not attach thread to JVM (%d, %p)",
                     __FUNCTION__, res, env);
      }
      else {
        isAttached = true;
      }
    }

    
    
    
    jmethodID cid = env->GetStaticMethodID(
        g_javaCmClass,
        "DeleteVideoCaptureAndroid",
        "(Lorg/webrtc/videoengine/VideoCaptureAndroid;)V");
    if (cid != NULL) {
      WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                   "%s: Call DeleteVideoCaptureAndroid", __FUNCTION__);
      
      env->CallStaticVoidMethod(g_javaCmClass, cid, _javaCaptureObj);

      
      env->DeleteGlobalRef(_javaCaptureObj);
      _javaCaptureObj = NULL;
    }
    else {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: Failed to find DeleteVideoCaptureAndroid id",
                   __FUNCTION__);
    }

    
    if (isAttached) {
      if (g_jvm->DetachCurrentThread() < 0) {
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioDevice,
                     _id, "%s: Could not detach thread from JVM",
                     __FUNCTION__);
      }
    }
  }
}

WebRtc_Word32 VideoCaptureAndroid::StartCapture(
    const VideoCaptureCapability& capability) {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);

  bool isAttached = false;
  WebRtc_Word32 result = 0;
  
  JNIEnv *env;
  if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    
    
    jint res = g_jvm->AttachCurrentThread(&env, NULL);
    if ((res < 0) || !env) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                   "%s: Could not attach thread to JVM (%d, %p)",
                   __FUNCTION__, res, env);
    }
    else {
      isAttached = true;
    }
  }

  if (_capInfo.GetBestMatchedCapability(_deviceUniqueId, capability,
                                        _frameInfo) < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: GetBestMatchedCapability failed. Req cap w%d h%d",
                 __FUNCTION__, capability.width, capability.height);
    return -1;
  }

  
  _captureDelay = _frameInfo.expectedCaptureDelay;

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
               "%s: _frameInfo w%d h%d", __FUNCTION__, _frameInfo.width,
               _frameInfo.height);

  
  
  jmethodID cid = env->GetMethodID(g_javaCmClass, "StartCapture", "(III)I");
  if (cid != NULL) {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "%s: Call StartCapture", __FUNCTION__);
    
    result = env->CallIntMethod(_javaCaptureObj, cid, _frameInfo.width,
                                _frameInfo.height, _frameInfo.maxFPS);
  }
  else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: Failed to find StartCapture id", __FUNCTION__);
  }

  
  if (isAttached) {
    if (g_jvm->DetachCurrentThread() < 0) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioDevice, _id,
                   "%s: Could not detach thread from JVM", __FUNCTION__);
    }
  }
  if (result == 0) {
    _requestedCapability = capability;
    _captureStarted = true;
  }
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: result %d", __FUNCTION__, result);
  return result;
}

WebRtc_Word32 VideoCaptureAndroid::StopCapture() {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);

  bool isAttached = false;
  WebRtc_Word32 result = 0;
  
  JNIEnv *env = NULL;
  if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    
    
    jint res = g_jvm->AttachCurrentThread(&env, NULL);
    if ((res < 0) || !env) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                   "%s: Could not attach thread to JVM (%d, %p)",
                   __FUNCTION__, res, env);
    }
    else {
      isAttached = true;
    }
  }

  memset(&_requestedCapability, 0, sizeof(_requestedCapability));
  memset(&_frameInfo, 0, sizeof(_frameInfo));

  
  jmethodID cid = env->GetMethodID(g_javaCmClass, "StopCapture", "()I");
  if (cid != NULL) {
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, -1,
                 "%s: Call StopCapture", __FUNCTION__);
    
    result = env->CallIntMethod(_javaCaptureObj, cid);
  }
  else {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                 "%s: Failed to find StopCapture id", __FUNCTION__);
  }

  
  if (isAttached) {
    if (g_jvm->DetachCurrentThread() < 0) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioDevice, _id,
                   "%s: Could not detach thread from JVM", __FUNCTION__);
    }
  }
  _captureStarted = false;

  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: result %d", __FUNCTION__, result);
  return result;
}

bool VideoCaptureAndroid::CaptureStarted() {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);
  return _captureStarted;
}

WebRtc_Word32 VideoCaptureAndroid::CaptureSettings(
    VideoCaptureCapability& settings) {
  CriticalSectionScoped cs(&_apiCs);
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCapture, -1,
               "%s: ", __FUNCTION__);
  settings = _requestedCapability;
  return 0;
}

WebRtc_Word32 VideoCaptureAndroid::SetCaptureRotation(
    VideoCaptureRotation rotation) {
  CriticalSectionScoped cs(&_apiCs);
  if (VideoCaptureImpl::SetCaptureRotation(rotation) == 0) {
    if (!g_jvm)
      return -1;

    
    JNIEnv *env;
    bool isAttached = false;

    
    if (g_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      
      
      jint res = g_jvm->AttachCurrentThread(&env, NULL);
      if ((res < 0) || !env) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                     _id,
                     "%s: Could not attach thread to JVM (%d, %p)",
                     __FUNCTION__, res, env);
        return -1;
      }
      isAttached = true;
    }

    jmethodID cid = env->GetMethodID(g_javaCmClass, "SetPreviewRotation",
                                     "(I)V");
    if (cid == NULL) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                   "%s: could not get java SetPreviewRotation ID",
                   __FUNCTION__);
      return -1;
    }
    jint rotateFrame = 0;
    switch (rotation) {
      case kCameraRotate0:
        rotateFrame = 0;
        break;
      case kCameraRotate90:
        rotateFrame = 90;
        break;
      case kCameraRotate180:
        rotateFrame = 180;
        break;
      case kCameraRotate270:
        rotateFrame = 270;
        break;
    }
    env->CallVoidMethod(_javaCaptureObj, cid, rotateFrame);

    
    if (isAttached) {
      if (g_jvm->DetachCurrentThread() < 0) {
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioDevice,
                     _id, "%s: Could not detach thread from JVM",
                     __FUNCTION__);
      }
    }

  }
  return 0;
}

}  
}  
