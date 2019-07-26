









#include <string.h> 
#include <android/log.h>

#include "org_webrtc_voiceengine_test_AudioDeviceAndroidTest.h"

#include "../../../../interface/audio_device.h"

#define LOG_TAG "WebRTC ADM Native"

void api_test();
void func_test(int);

typedef struct
{
    
    JavaVM* jvm;
} AdmData;

static AdmData admData;

jint JNI_OnLoad(JavaVM* vm, void* )
{
    if (!vm)
    {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                            "JNI_OnLoad did not receive a valid VM pointer");
        return -1;
    }

    
    JNIEnv* env;
    if (JNI_OK != vm->GetEnv(reinterpret_cast<void**> (&env),
                             JNI_VERSION_1_4))
    {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                            "JNI_OnLoad could not get JNI env");
        return -1;
    }

    
    
    
    
    
    

    
    
    
    
    
    
    
    
    

    
    memset(&admData, 0, sizeof(admData));

    
    admData.jvm = vm;

    return JNI_VERSION_1_4;
}

JNIEXPORT jboolean JNICALL
Java_org_webrtc_voiceengine_test_AudioDeviceAndroidTest_NativeInit(JNIEnv * env,
                                                                   jclass)
{
    
    

    return true;
}

JNIEXPORT jint JNICALL
Java_org_webrtc_voiceengine_test_AudioDeviceAndroidTest_RunTest(JNIEnv *env,
                                                                jobject context,
                                                                jint test)
{
    
    webrtc::AudioDeviceModule::SetAndroidObjects(admData.jvm, env, context);

    
    if (0 == test)
    {
        api_test();
    }
    else
    {
        func_test(test);
    }

    
    webrtc::AudioDeviceModule::SetAndroidObjects(NULL, NULL, NULL);

    return 0;
}
