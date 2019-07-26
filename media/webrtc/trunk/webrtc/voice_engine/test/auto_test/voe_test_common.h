









#ifndef WEBRTC_VOICE_ENGINE_VOE_TEST_COMMON_H_
#define WEBRTC_VOICE_ENGINE_VOE_TEST_COMMON_H_

#ifdef WEBRTC_ANDROID
#include <android/log.h>
#define ANDROID_LOG_TAG "VoiceEngine Auto Test"
#define TEST_LOG(...) \
    __android_log_print(ANDROID_LOG_DEBUG, ANDROID_LOG_TAG, __VA_ARGS__)
#define TEST_LOG_ERROR(...) \
    __android_log_print(ANDROID_LOG_ERROR, ANDROID_LOG_TAG, __VA_ARGS__)
#define TEST_LOG_FLUSH
#else
#define TEST_LOG printf
#define TEST_LOG_ERROR printf
#define TEST_LOG_FLUSH fflush(NULL)
#endif


#include "webrtc/engine_configurations.h"


#define CODEC_TEST_TIME 400

#endif  
