





#include "GeneratedSDKWrappers.h"
#include "AndroidBridgeUtilities.h"
#include "nsXPCOMStrings.h"
#include "AndroidBridge.h"
#include "nsDebug.h"

namespace mozilla {
namespace widget {
namespace android {
jclass MediaCodec::mMediaCodecClass = 0;
jmethodID MediaCodec::jConfigure = 0;
jmethodID MediaCodec::jCreateByCodecName = 0;
jmethodID MediaCodec::jCreateDecoderByType = 0;
jmethodID MediaCodec::jCreateEncoderByType = 0;
jmethodID MediaCodec::jDequeueInputBuffer = 0;
jmethodID MediaCodec::jDequeueOutputBuffer = 0;
jmethodID MediaCodec::jFinalize = 0;
jmethodID MediaCodec::jFlush = 0;
jmethodID MediaCodec::jGetInputBuffers = 0;
jmethodID MediaCodec::jGetOutputBuffers = 0;
jmethodID MediaCodec::jGetOutputFormat = 0;
jmethodID MediaCodec::jQueueInputBuffer = 0;
jmethodID MediaCodec::jQueueSecureInputBuffer = 0;
jmethodID MediaCodec::jRelease = 0;
jmethodID MediaCodec::jReleaseOutputBuffer = 0;
jmethodID MediaCodec::jSetVideoScalingMode = 0;
jmethodID MediaCodec::jStart = 0;
jmethodID MediaCodec::jStop = 0;
jfieldID MediaCodec::jBUFFER_FLAG_CODEC_CONFIG = 0;
jfieldID MediaCodec::jBUFFER_FLAG_END_OF_STREAM = 0;
jfieldID MediaCodec::jBUFFER_FLAG_SYNC_FRAME = 0;
jfieldID MediaCodec::jCONFIGURE_FLAG_ENCODE = 0;
jfieldID MediaCodec::jCRYPTO_MODE_AES_CTR = 0;
jfieldID MediaCodec::jCRYPTO_MODE_UNENCRYPTED = 0;
jfieldID MediaCodec::jINFO_OUTPUT_BUFFERS_CHANGED = 0;
jfieldID MediaCodec::jINFO_OUTPUT_FORMAT_CHANGED = 0;
jfieldID MediaCodec::jINFO_TRY_AGAIN_LATER = 0;
jfieldID MediaCodec::jVIDEO_SCALING_MODE_SCALE_TO_FIT = 0;
jfieldID MediaCodec::jVIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING = 0;
void MediaCodec::InitStubs(JNIEnv *jEnv) {
    initInit();

    mMediaCodecClass = getClassGlobalRef("android/media/MediaCodec");
    jConfigure = getMethod("configure", "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V");
    jCreateByCodecName = getStaticMethod("createByCodecName", "(Ljava/lang/String;)Landroid/media/MediaCodec;");
    jCreateDecoderByType = getStaticMethod("createDecoderByType", "(Ljava/lang/String;)Landroid/media/MediaCodec;");
    jCreateEncoderByType = getStaticMethod("createEncoderByType", "(Ljava/lang/String;)Landroid/media/MediaCodec;");
    jDequeueInputBuffer = getMethod("dequeueInputBuffer", "(J)I");
    jDequeueOutputBuffer = getMethod("dequeueOutputBuffer", "(Landroid/media/MediaCodec$BufferInfo;J)I");
    jFinalize = getMethod("finalize", "()V");
    jFlush = getMethod("flush", "()V");
    jGetInputBuffers = getMethod("getInputBuffers", "()[Ljava/nio/ByteBuffer;");
    jGetOutputBuffers = getMethod("getOutputBuffers", "()[Ljava/nio/ByteBuffer;");
    jGetOutputFormat = getMethod("getOutputFormat", "()Landroid/media/MediaFormat;");
    jQueueInputBuffer = getMethod("queueInputBuffer", "(IIIJI)V");
    jQueueSecureInputBuffer = getMethod("queueSecureInputBuffer", "(IILandroid/media/MediaCodec$CryptoInfo;JI)V");
    jRelease = getMethod("release", "()V");
    jReleaseOutputBuffer = getMethod("releaseOutputBuffer", "(IZ)V");
    jSetVideoScalingMode = getMethod("setVideoScalingMode", "(I)V");
    jStart = getMethod("start", "()V");
    jStop = getMethod("stop", "()V");
    jBUFFER_FLAG_CODEC_CONFIG = getStaticField("BUFFER_FLAG_CODEC_CONFIG", "I");
    jBUFFER_FLAG_END_OF_STREAM = getStaticField("BUFFER_FLAG_END_OF_STREAM", "I");
    jBUFFER_FLAG_SYNC_FRAME = getStaticField("BUFFER_FLAG_SYNC_FRAME", "I");
    jCONFIGURE_FLAG_ENCODE = getStaticField("CONFIGURE_FLAG_ENCODE", "I");
    jCRYPTO_MODE_AES_CTR = getStaticField("CRYPTO_MODE_AES_CTR", "I");
    jCRYPTO_MODE_UNENCRYPTED = getStaticField("CRYPTO_MODE_UNENCRYPTED", "I");
    jINFO_OUTPUT_BUFFERS_CHANGED = getStaticField("INFO_OUTPUT_BUFFERS_CHANGED", "I");
    jINFO_OUTPUT_FORMAT_CHANGED = getStaticField("INFO_OUTPUT_FORMAT_CHANGED", "I");
    jINFO_TRY_AGAIN_LATER = getStaticField("INFO_TRY_AGAIN_LATER", "I");
    jVIDEO_SCALING_MODE_SCALE_TO_FIT = getStaticField("VIDEO_SCALING_MODE_SCALE_TO_FIT", "I");
    jVIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING = getStaticField("VIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING", "I");
}

MediaCodec* MediaCodec::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    MediaCodec* ret = new MediaCodec(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

bool MediaCodec::Configure(jobject a0, jobject a1, jobject a2, int32_t a3) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(3) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].l = a0;
    args[1].l = a1;
    args[2].l = a2;
    args[3].i = a3;

    env->CallVoidMethodA(wrapped_obj, jConfigure, args);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->PopLocalFrame(nullptr);
        return false;
    }
    env->PopLocalFrame(nullptr);
    return true;
}

jobject MediaCodec::CreateByCodecName(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mMediaCodecClass, jCreateByCodecName, j0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject MediaCodec::CreateDecoderByType(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mMediaCodecClass, jCreateDecoderByType, j0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject MediaCodec::CreateEncoderByType(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mMediaCodecClass, jCreateEncoderByType, j0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

int32_t MediaCodec::DequeueInputBuffer(int64_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jDequeueInputBuffer, a0);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->PopLocalFrame(nullptr);
        return MEDIACODEC_EXCEPTION_INDEX;
    }
    env->PopLocalFrame(nullptr);
    return temp;
}

int32_t MediaCodec::DequeueOutputBuffer(jobject a0, int64_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jDequeueOutputBuffer, a0, a1);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->PopLocalFrame(nullptr);
        return MEDIACODEC_EXCEPTION_INDEX;
    }
    env->PopLocalFrame(nullptr);
    return temp;
}

void MediaCodec::Finalize() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jFinalize);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaCodec::Flush() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jFlush);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jobjectArray MediaCodec::GetInputBuffers() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jGetInputBuffers);
    AndroidBridge::HandleUncaughtException(env);
    jobjectArray ret = static_cast<jobjectArray>(env->PopLocalFrame(temp));
    return ret;
}

jobjectArray MediaCodec::GetOutputBuffers() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jGetOutputBuffers);
    AndroidBridge::HandleUncaughtException(env);
    jobjectArray ret = static_cast<jobjectArray>(env->PopLocalFrame(temp));
    return ret;
}

jobject MediaCodec::GetOutputFormat() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jGetOutputFormat);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

void MediaCodec::QueueInputBuffer(int32_t a0, int32_t a1, int32_t a2, int64_t a3, int32_t a4) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(5) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[5];
    args[0].i = a0;
    args[1].i = a1;
    args[2].i = a2;
    args[3].j = a3;
    args[4].i = a4;

    env->CallVoidMethodA(wrapped_obj, jQueueInputBuffer, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaCodec::QueueSecureInputBuffer(int32_t a0, int32_t a1, jobject a2, int64_t a3, int32_t a4) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[5];
    args[0].i = a0;
    args[1].i = a1;
    args[2].l = a2;
    args[3].j = a3;
    args[4].i = a4;

    env->CallVoidMethodA(wrapped_obj, jQueueSecureInputBuffer, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaCodec::Release() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jRelease);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaCodec::ReleaseOutputBuffer(int32_t a0, bool a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jReleaseOutputBuffer, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaCodec::SetVideoScalingMode(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jSetVideoScalingMode, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

bool MediaCodec::Start() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jStart);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->PopLocalFrame(nullptr);
        return false;
    }
    env->PopLocalFrame(nullptr);
    return true;
}

void MediaCodec::Stop() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jStop);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

int32_t MediaCodec::getBUFFER_FLAG_CODEC_CONFIG() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jBUFFER_FLAG_CODEC_CONFIG);
}

int32_t MediaCodec::getBUFFER_FLAG_END_OF_STREAM() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jBUFFER_FLAG_END_OF_STREAM);
}

int32_t MediaCodec::getBUFFER_FLAG_SYNC_FRAME() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jBUFFER_FLAG_SYNC_FRAME);
}

int32_t MediaCodec::getCONFIGURE_FLAG_ENCODE() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jCONFIGURE_FLAG_ENCODE);
}

int32_t MediaCodec::getCRYPTO_MODE_AES_CTR() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jCRYPTO_MODE_AES_CTR);
}

int32_t MediaCodec::getCRYPTO_MODE_UNENCRYPTED() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jCRYPTO_MODE_UNENCRYPTED);
}

int32_t MediaCodec::getINFO_OUTPUT_BUFFERS_CHANGED() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jINFO_OUTPUT_BUFFERS_CHANGED);
}

int32_t MediaCodec::getINFO_OUTPUT_FORMAT_CHANGED() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jINFO_OUTPUT_FORMAT_CHANGED);
}

int32_t MediaCodec::getINFO_TRY_AGAIN_LATER() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jINFO_TRY_AGAIN_LATER);
}

int32_t MediaCodec::getVIDEO_SCALING_MODE_SCALE_TO_FIT() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jVIDEO_SCALING_MODE_SCALE_TO_FIT);
}

int32_t MediaCodec::getVIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticIntField(mMediaCodecClass, jVIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING);
}
jclass MediaFormat::mMediaFormatClass = 0;
jmethodID MediaFormat::jMediaFormat = 0;
jmethodID MediaFormat::jContainsKey = 0;
jmethodID MediaFormat::jCreateAudioFormat = 0;
jmethodID MediaFormat::jCreateVideoFormat = 0;
jmethodID MediaFormat::jGetByteBuffer = 0;
jmethodID MediaFormat::jGetFloat = 0;
jmethodID MediaFormat::jGetInteger = 0;
jmethodID MediaFormat::jGetLong = 0;
jmethodID MediaFormat::jGetString = 0;
jmethodID MediaFormat::jSetByteBuffer = 0;
jmethodID MediaFormat::jSetFloat = 0;
jmethodID MediaFormat::jSetInteger = 0;
jmethodID MediaFormat::jSetLong = 0;
jmethodID MediaFormat::jSetString = 0;
jmethodID MediaFormat::jToString = 0;
jfieldID MediaFormat::jKEY_AAC_PROFILE = 0;
jfieldID MediaFormat::jKEY_BIT_RATE = 0;
jfieldID MediaFormat::jKEY_CHANNEL_COUNT = 0;
jfieldID MediaFormat::jKEY_CHANNEL_MASK = 0;
jfieldID MediaFormat::jKEY_COLOR_FORMAT = 0;
jfieldID MediaFormat::jKEY_DURATION = 0;
jfieldID MediaFormat::jKEY_FLAC_COMPRESSION_LEVEL = 0;
jfieldID MediaFormat::jKEY_FRAME_RATE = 0;
jfieldID MediaFormat::jKEY_HEIGHT = 0;
jfieldID MediaFormat::jKEY_IS_ADTS = 0;
jfieldID MediaFormat::jKEY_I_FRAME_INTERVAL = 0;
jfieldID MediaFormat::jKEY_MAX_INPUT_SIZE = 0;
jfieldID MediaFormat::jKEY_MIME = 0;
jfieldID MediaFormat::jKEY_SAMPLE_RATE = 0;
jfieldID MediaFormat::jKEY_WIDTH = 0;
void MediaFormat::InitStubs(JNIEnv *jEnv) {
    initInit();

    mMediaFormatClass = getClassGlobalRef("android/media/MediaFormat");
    jMediaFormat = getMethod("<init>", "()V");
    jContainsKey = getMethod("containsKey", "(Ljava/lang/String;)Z");
    jCreateAudioFormat = getStaticMethod("createAudioFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;");
    jCreateVideoFormat = getStaticMethod("createVideoFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;");
    jGetByteBuffer = getMethod("getByteBuffer", "(Ljava/lang/String;)Ljava/nio/ByteBuffer;");
    jGetFloat = getMethod("getFloat", "(Ljava/lang/String;)F");
    jGetInteger = getMethod("getInteger", "(Ljava/lang/String;)I");
    jGetLong = getMethod("getLong", "(Ljava/lang/String;)J");
    jGetString = getMethod("getString", "(Ljava/lang/String;)Ljava/lang/String;");
    jSetByteBuffer = getMethod("setByteBuffer", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V");
    jSetFloat = getMethod("setFloat", "(Ljava/lang/String;F)V");
    jSetInteger = getMethod("setInteger", "(Ljava/lang/String;I)V");
    jSetLong = getMethod("setLong", "(Ljava/lang/String;J)V");
    jSetString = getMethod("setString", "(Ljava/lang/String;Ljava/lang/String;)V");
    jToString = getMethod("toString", "()Ljava/lang/String;");
    jKEY_AAC_PROFILE = getStaticField("KEY_AAC_PROFILE", "Ljava/lang/String;");
    jKEY_BIT_RATE = getStaticField("KEY_BIT_RATE", "Ljava/lang/String;");
    jKEY_CHANNEL_COUNT = getStaticField("KEY_CHANNEL_COUNT", "Ljava/lang/String;");
    jKEY_CHANNEL_MASK = getStaticField("KEY_CHANNEL_MASK", "Ljava/lang/String;");
    jKEY_COLOR_FORMAT = getStaticField("KEY_COLOR_FORMAT", "Ljava/lang/String;");
    jKEY_DURATION = getStaticField("KEY_DURATION", "Ljava/lang/String;");
    jKEY_FLAC_COMPRESSION_LEVEL = getStaticField("KEY_FLAC_COMPRESSION_LEVEL", "Ljava/lang/String;");
    jKEY_FRAME_RATE = getStaticField("KEY_FRAME_RATE", "Ljava/lang/String;");
    jKEY_HEIGHT = getStaticField("KEY_HEIGHT", "Ljava/lang/String;");
    jKEY_IS_ADTS = getStaticField("KEY_IS_ADTS", "Ljava/lang/String;");
    jKEY_I_FRAME_INTERVAL = getStaticField("KEY_I_FRAME_INTERVAL", "Ljava/lang/String;");
    jKEY_MAX_INPUT_SIZE = getStaticField("KEY_MAX_INPUT_SIZE", "Ljava/lang/String;");
    jKEY_MIME = getStaticField("KEY_MIME", "Ljava/lang/String;");
    jKEY_SAMPLE_RATE = getStaticField("KEY_SAMPLE_RATE", "Ljava/lang/String;");
    jKEY_WIDTH = getStaticField("KEY_WIDTH", "Ljava/lang/String;");
}

MediaFormat* MediaFormat::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    MediaFormat* ret = new MediaFormat(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

MediaFormat::MediaFormat() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    Init(env->NewObject(mMediaFormatClass, jMediaFormat), env);
    env->PopLocalFrame(nullptr);
}

bool MediaFormat::ContainsKey(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    bool temp = env->CallBooleanMethod(wrapped_obj, jContainsKey, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jobject MediaFormat::CreateAudioFormat(const nsAString& a0, int32_t a1, int32_t a2) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].i = a1;
    args[2].i = a2;

    jobject temp = env->CallStaticObjectMethodA(mMediaFormatClass, jCreateAudioFormat, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject MediaFormat::CreateVideoFormat(const nsAString& a0, int32_t a1, int32_t a2) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].i = a1;
    args[2].i = a2;

    jobject temp = env->CallStaticObjectMethodA(mMediaFormatClass, jCreateVideoFormat, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject MediaFormat::GetByteBuffer(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallObjectMethod(wrapped_obj, jGetByteBuffer, j0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jfloat MediaFormat::GetFloat(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jfloat temp = env->CallFloatMethod(wrapped_obj, jGetFloat, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int32_t MediaFormat::GetInteger(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    int32_t temp = env->CallIntMethod(wrapped_obj, jGetInteger, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int64_t MediaFormat::GetLong(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    int64_t temp = env->CallLongMethod(wrapped_obj, jGetLong, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jstring MediaFormat::GetString(const nsAString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallObjectMethod(wrapped_obj, jGetString, j0);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void MediaFormat::SetByteBuffer(const nsAString& a0, jobject a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallVoidMethod(wrapped_obj, jSetByteBuffer, j0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaFormat::SetFloat(const nsAString& a0, jfloat a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallVoidMethod(wrapped_obj, jSetFloat, j0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaFormat::SetInteger(const nsAString& a0, int32_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallVoidMethod(wrapped_obj, jSetInteger, j0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaFormat::SetLong(const nsAString& a0, int64_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallVoidMethod(wrapped_obj, jSetLong, j0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MediaFormat::SetString(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);
    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    env->CallVoidMethod(wrapped_obj, jSetString, j0, j1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jstring MediaFormat::ToString() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jToString);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jstring MediaFormat::getKEY_AAC_PROFILE() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_AAC_PROFILE));
}

jstring MediaFormat::getKEY_BIT_RATE() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_BIT_RATE));
}

jstring MediaFormat::getKEY_CHANNEL_COUNT() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_CHANNEL_COUNT));
}

jstring MediaFormat::getKEY_CHANNEL_MASK() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_CHANNEL_MASK));
}

jstring MediaFormat::getKEY_COLOR_FORMAT() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_COLOR_FORMAT));
}

jstring MediaFormat::getKEY_DURATION() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_DURATION));
}

jstring MediaFormat::getKEY_FLAC_COMPRESSION_LEVEL() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_FLAC_COMPRESSION_LEVEL));
}

jstring MediaFormat::getKEY_FRAME_RATE() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_FRAME_RATE));
}

jstring MediaFormat::getKEY_HEIGHT() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_HEIGHT));
}

jstring MediaFormat::getKEY_IS_ADTS() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_IS_ADTS));
}

jstring MediaFormat::getKEY_I_FRAME_INTERVAL() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_I_FRAME_INTERVAL));
}

jstring MediaFormat::getKEY_MAX_INPUT_SIZE() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_MAX_INPUT_SIZE));
}

jstring MediaFormat::getKEY_MIME() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_MIME));
}

jstring MediaFormat::getKEY_SAMPLE_RATE() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_SAMPLE_RATE));
}

jstring MediaFormat::getKEY_WIDTH() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jstring>(env->GetStaticObjectField(mMediaFormatClass, jKEY_WIDTH));
}
jclass ByteBuffer::mByteBufferClass = 0;
jmethodID ByteBuffer::j_get = 0;
jmethodID ByteBuffer::j_put = 0;
jmethodID ByteBuffer::jAllocate = 0;
jmethodID ByteBuffer::jAllocateDirect = 0;
jmethodID ByteBuffer::jArray = 0;
jmethodID ByteBuffer::jArray1 = 0;
jmethodID ByteBuffer::jArrayOffset = 0;
jmethodID ByteBuffer::jAsCharBuffer = 0;
jmethodID ByteBuffer::jAsDoubleBuffer = 0;
jmethodID ByteBuffer::jAsFloatBuffer = 0;
jmethodID ByteBuffer::jAsIntBuffer = 0;
jmethodID ByteBuffer::jAsLongBuffer = 0;
jmethodID ByteBuffer::jAsReadOnlyBuffer = 0;
jmethodID ByteBuffer::jAsShortBuffer = 0;
jmethodID ByteBuffer::jCompact = 0;
jmethodID ByteBuffer::jCompareTo = 0;
jmethodID ByteBuffer::jCompareTo1 = 0;
jmethodID ByteBuffer::jDuplicate = 0;
jmethodID ByteBuffer::jEquals = 0;
jmethodID ByteBuffer::jGet = 0;
jmethodID ByteBuffer::jGet1 = 0;
jmethodID ByteBuffer::jGet10 = 0;
jmethodID ByteBuffer::jGet11 = 0;
jmethodID ByteBuffer::jGetChar = 0;
jmethodID ByteBuffer::jGetChar1 = 0;
jmethodID ByteBuffer::jGetDouble = 0;
jmethodID ByteBuffer::jGetDouble1 = 0;
jmethodID ByteBuffer::jGetFloat = 0;
jmethodID ByteBuffer::jGetFloat1 = 0;
jmethodID ByteBuffer::jGetInt = 0;
jmethodID ByteBuffer::jGetInt1 = 0;
jmethodID ByteBuffer::jGetLong = 0;
jmethodID ByteBuffer::jGetLong1 = 0;
jmethodID ByteBuffer::jGetShort = 0;
jmethodID ByteBuffer::jGetShort1 = 0;
jmethodID ByteBuffer::jHasArray = 0;
jmethodID ByteBuffer::jHashCode = 0;
jmethodID ByteBuffer::jIsDirect = 0;
jmethodID ByteBuffer::jOrder = 0;
jmethodID ByteBuffer::jOrder1 = 0;
jmethodID ByteBuffer::jPut = 0;
jmethodID ByteBuffer::jPut1 = 0;
jmethodID ByteBuffer::jPut12 = 0;
jmethodID ByteBuffer::jPut13 = 0;
jmethodID ByteBuffer::jPut14 = 0;
jmethodID ByteBuffer::jPutChar = 0;
jmethodID ByteBuffer::jPutChar1 = 0;
jmethodID ByteBuffer::jPutDouble = 0;
jmethodID ByteBuffer::jPutDouble1 = 0;
jmethodID ByteBuffer::jPutFloat = 0;
jmethodID ByteBuffer::jPutFloat1 = 0;
jmethodID ByteBuffer::jPutInt = 0;
jmethodID ByteBuffer::jPutInt1 = 0;
jmethodID ByteBuffer::jPutLong = 0;
jmethodID ByteBuffer::jPutLong1 = 0;
jmethodID ByteBuffer::jPutShort = 0;
jmethodID ByteBuffer::jPutShort1 = 0;
jmethodID ByteBuffer::jSlice = 0;
jmethodID ByteBuffer::jToString = 0;
jmethodID ByteBuffer::jWrap = 0;
jmethodID ByteBuffer::jWrap1 = 0;
jfieldID ByteBuffer::jBigEndian = 0;
jfieldID ByteBuffer::jHb = 0;
jfieldID ByteBuffer::jIsReadOnly = 0;
jfieldID ByteBuffer::jNativeByteOrder = 0;
jfieldID ByteBuffer::jOffset = 0;
void ByteBuffer::InitStubs(JNIEnv *jEnv) {
    initInit();

    mByteBufferClass = getClassGlobalRef("java/nio/ByteBuffer");
    
    
    jAllocate = getStaticMethod("allocate", "(I)Ljava/nio/ByteBuffer;");
    jAllocateDirect = getStaticMethod("allocateDirect", "(I)Ljava/nio/ByteBuffer;");
    jArray = getMethod("array", "()Ljava/lang/Object;");
    jArray1 = getMethod("array", "()[B");
    jArrayOffset = getMethod("arrayOffset", "()I");
    jAsCharBuffer = getMethod("asCharBuffer", "()Ljava/nio/CharBuffer;");
    jAsDoubleBuffer = getMethod("asDoubleBuffer", "()Ljava/nio/DoubleBuffer;");
    jAsFloatBuffer = getMethod("asFloatBuffer", "()Ljava/nio/FloatBuffer;");
    jAsIntBuffer = getMethod("asIntBuffer", "()Ljava/nio/IntBuffer;");
    jAsLongBuffer = getMethod("asLongBuffer", "()Ljava/nio/LongBuffer;");
    jAsReadOnlyBuffer = getMethod("asReadOnlyBuffer", "()Ljava/nio/ByteBuffer;");
    jAsShortBuffer = getMethod("asShortBuffer", "()Ljava/nio/ShortBuffer;");
    jCompact = getMethod("compact", "()Ljava/nio/ByteBuffer;");
    jCompareTo = getMethod("compareTo", "(Ljava/lang/Object;)I");
    jCompareTo1 = getMethod("compareTo", "(Ljava/nio/ByteBuffer;)I");
    jDuplicate = getMethod("duplicate", "()Ljava/nio/ByteBuffer;");
    jEquals = getMethod("equals", "(Ljava/lang/Object;)Z");
    jGet = getMethod("get", "()B");
    jGet1 = getMethod("get", "(I)B");
    jGet10 = getMethod("get", "([B)Ljava/nio/ByteBuffer;");
    jGet11 = getMethod("get", "([BII)Ljava/nio/ByteBuffer;");
    jGetChar = getMethod("getChar", "()C");
    jGetChar1 = getMethod("getChar", "(I)C");
    jGetDouble = getMethod("getDouble", "()D");
    jGetDouble1 = getMethod("getDouble", "(I)D");
    jGetFloat = getMethod("getFloat", "()F");
    jGetFloat1 = getMethod("getFloat", "(I)F");
    jGetInt = getMethod("getInt", "()I");
    jGetInt1 = getMethod("getInt", "(I)I");
    jGetLong = getMethod("getLong", "()J");
    jGetLong1 = getMethod("getLong", "(I)J");
    jGetShort = getMethod("getShort", "()S");
    jGetShort1 = getMethod("getShort", "(I)S");
    jHasArray = getMethod("hasArray", "()Z");
    jHashCode = getMethod("hashCode", "()I");
    jIsDirect = getMethod("isDirect", "()Z");
    jOrder = getMethod("order", "()Ljava/nio/ByteOrder;");
    jOrder1 = getMethod("order", "(Ljava/nio/ByteOrder;)Ljava/nio/ByteBuffer;");
    jPut = getMethod("put", "(B)Ljava/nio/ByteBuffer;");
    jPut1 = getMethod("put", "(IB)Ljava/nio/ByteBuffer;");
    jPut12 = getMethod("put", "(Ljava/nio/ByteBuffer;)Ljava/nio/ByteBuffer;");
    jPut13 = getMethod("put", "([B)Ljava/nio/ByteBuffer;");
    jPut14 = getMethod("put", "([BII)Ljava/nio/ByteBuffer;");
    jPutChar = getMethod("putChar", "(C)Ljava/nio/ByteBuffer;");
    jPutChar1 = getMethod("putChar", "(IC)Ljava/nio/ByteBuffer;");
    jPutDouble = getMethod("putDouble", "(D)Ljava/nio/ByteBuffer;");
    jPutDouble1 = getMethod("putDouble", "(ID)Ljava/nio/ByteBuffer;");
    jPutFloat = getMethod("putFloat", "(F)Ljava/nio/ByteBuffer;");
    jPutFloat1 = getMethod("putFloat", "(IF)Ljava/nio/ByteBuffer;");
    jPutInt = getMethod("putInt", "(I)Ljava/nio/ByteBuffer;");
    jPutInt1 = getMethod("putInt", "(II)Ljava/nio/ByteBuffer;");
    jPutLong = getMethod("putLong", "(IJ)Ljava/nio/ByteBuffer;");
    jPutLong1 = getMethod("putLong", "(J)Ljava/nio/ByteBuffer;");
    jPutShort = getMethod("putShort", "(IS)Ljava/nio/ByteBuffer;");
    jPutShort1 = getMethod("putShort", "(S)Ljava/nio/ByteBuffer;");
    jSlice = getMethod("slice", "()Ljava/nio/ByteBuffer;");
    jToString = getMethod("toString", "()Ljava/lang/String;");
    jWrap = getStaticMethod("wrap", "([B)Ljava/nio/ByteBuffer;");
    jWrap1 = getStaticMethod("wrap", "([BII)Ljava/nio/ByteBuffer;");
    






}

ByteBuffer* ByteBuffer::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    ByteBuffer* ret = new ByteBuffer(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

int8_t ByteBuffer::_get(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int8_t temp = env->CallByteMethod(wrapped_obj, j_get, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

void ByteBuffer::_put(int32_t a0, int8_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, j_put, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jobject ByteBuffer::Allocate(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mByteBufferClass, jAllocate, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::AllocateDirect(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mByteBufferClass, jAllocateDirect, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Array() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jArray);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jbyteArray ByteBuffer::Array1() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jArray1);
    AndroidBridge::HandleUncaughtException(env);
    jbyteArray ret = static_cast<jbyteArray>(env->PopLocalFrame(temp));
    return ret;
}

int32_t ByteBuffer::ArrayOffset() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jArrayOffset);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jstring ByteBuffer::AsCharBuffer() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jAsCharBuffer);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::AsDoubleBuffer() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jAsDoubleBuffer);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::AsFloatBuffer() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jAsFloatBuffer);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::AsIntBuffer() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jAsIntBuffer);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::AsLongBuffer() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jAsLongBuffer);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::AsReadOnlyBuffer() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jAsReadOnlyBuffer);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::AsShortBuffer() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jAsShortBuffer);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Compact() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jCompact);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

int32_t ByteBuffer::CompareTo(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jCompareTo, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int32_t ByteBuffer::CompareTo1(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jCompareTo1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jobject ByteBuffer::Duplicate() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jDuplicate);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

bool ByteBuffer::Equals(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallBooleanMethod(wrapped_obj, jEquals, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int8_t ByteBuffer::Get() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int8_t temp = env->CallByteMethod(wrapped_obj, jGet);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int8_t ByteBuffer::Get1(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int8_t temp = env->CallByteMethod(wrapped_obj, jGet1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jobject ByteBuffer::Get1(jbyteArray a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jGet10, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Get1(jbyteArray a0, int32_t a1, int32_t a2) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].l = a0;
    args[1].i = a1;
    args[2].i = a2;

    jobject temp = env->CallObjectMethodA(wrapped_obj, jGet11, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

uint16_t ByteBuffer::GetChar() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    uint16_t temp = env->CallCharMethod(wrapped_obj, jGetChar);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

uint16_t ByteBuffer::GetChar1(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    uint16_t temp = env->CallCharMethod(wrapped_obj, jGetChar1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jdouble ByteBuffer::GetDouble() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jdouble temp = env->CallDoubleMethod(wrapped_obj, jGetDouble);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jdouble ByteBuffer::GetDouble1(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jdouble temp = env->CallDoubleMethod(wrapped_obj, jGetDouble1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jfloat ByteBuffer::GetFloat() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jfloat temp = env->CallFloatMethod(wrapped_obj, jGetFloat);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jfloat ByteBuffer::GetFloat1(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jfloat temp = env->CallFloatMethod(wrapped_obj, jGetFloat1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int32_t ByteBuffer::GetInt() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jGetInt);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int32_t ByteBuffer::GetInt1(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jGetInt1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int64_t ByteBuffer::GetLong() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int64_t temp = env->CallLongMethod(wrapped_obj, jGetLong);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int64_t ByteBuffer::GetLong1(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int64_t temp = env->CallLongMethod(wrapped_obj, jGetLong1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int16_t ByteBuffer::GetShort() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int16_t temp = env->CallShortMethod(wrapped_obj, jGetShort);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int16_t ByteBuffer::GetShort1(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int16_t temp = env->CallShortMethod(wrapped_obj, jGetShort1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

bool ByteBuffer::HasArray() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallBooleanMethod(wrapped_obj, jHasArray);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int32_t ByteBuffer::HashCode() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallIntMethod(wrapped_obj, jHashCode);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

bool ByteBuffer::IsDirect() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallBooleanMethod(wrapped_obj, jIsDirect);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jobject ByteBuffer::Order() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jOrder);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Order1(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jOrder1, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Put(int8_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPut, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Put1(int32_t a0, int8_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPut1, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Put1(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPut12, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Put1(jbyteArray a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPut13, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Put1(jbyteArray a0, int32_t a1, int32_t a2) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].l = a0;
    args[1].i = a1;
    args[2].i = a2;

    jobject temp = env->CallObjectMethodA(wrapped_obj, jPut14, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutChar(uint16_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutChar, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutChar1(int32_t a0, uint16_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutChar1, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutDouble(jdouble a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutDouble, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutDouble1(int32_t a0, jdouble a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutDouble1, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutFloat(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutFloat, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutFloat1(int32_t a0, jfloat a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutFloat1, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutInt(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutInt, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutInt1(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutInt1, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutLong(int32_t a0, int64_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutLong, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutLong1(int64_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutLong1, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutShort(int32_t a0, int16_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutShort, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::PutShort1(int16_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jPutShort1, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Slice() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jSlice);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jstring ByteBuffer::ToString() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jToString);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Wrap1(jbyteArray a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mByteBufferClass, jWrap, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject ByteBuffer::Wrap2(jbyteArray a0, int32_t a1, int32_t a2) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].l = a0;
    args[1].i = a1;
    args[2].i = a2;

    jobject temp = env->CallStaticObjectMethodA(mByteBufferClass, jWrap1, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

bool ByteBuffer::getBigEndian() {
    JNIEnv *env = GetJNIForThread();
    return env->GetBooleanField(wrapped_obj, jBigEndian);
}

void ByteBuffer::setBigEndian(bool a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetBooleanField(wrapped_obj, jBigEndian, a0);
}

jbyteArray ByteBuffer::getHb() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jbyteArray>(env->GetObjectField(wrapped_obj, jHb));
}

bool ByteBuffer::getIsReadOnly() {
    JNIEnv *env = GetJNIForThread();
    return env->GetBooleanField(wrapped_obj, jIsReadOnly);
}

void ByteBuffer::setIsReadOnly(bool a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetBooleanField(wrapped_obj, jIsReadOnly, a0);
}

bool ByteBuffer::getNativeByteOrder() {
    JNIEnv *env = GetJNIForThread();
    return env->GetBooleanField(wrapped_obj, jNativeByteOrder);
}

void ByteBuffer::setNativeByteOrder(bool a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetBooleanField(wrapped_obj, jNativeByteOrder, a0);
}

int32_t ByteBuffer::getOffset() {
    JNIEnv *env = GetJNIForThread();
    return env->GetIntField(wrapped_obj, jOffset);
}

jclass BufferInfo::mBufferInfoClass = 0;
jmethodID BufferInfo::jBufferInfo = 0;
jmethodID BufferInfo::jSet = 0;
jfieldID BufferInfo::jFlags = 0;
jfieldID BufferInfo::jOffset = 0;
jfieldID BufferInfo::jPresentationTimeUs = 0;
jfieldID BufferInfo::jSize = 0;
void BufferInfo::InitStubs(JNIEnv *jEnv) {
    initInit();

    mBufferInfoClass = getClassGlobalRef("android/media/MediaCodec$BufferInfo");
    jBufferInfo = getMethod("<init>", "()V");
    jSet = getMethod("set", "(IIJI)V");
    jFlags = getField("flags", "I");
    jOffset = getField("offset", "I");
    jPresentationTimeUs = getField("presentationTimeUs", "J");
    jSize = getField("size", "I");
}

BufferInfo* BufferInfo::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    BufferInfo* ret = new BufferInfo(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

BufferInfo::BufferInfo() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    Init(env->NewObject(mBufferInfoClass, jBufferInfo), env);
    env->PopLocalFrame(nullptr);
}

void BufferInfo::Set(int32_t a0, int32_t a1, int64_t a2, int32_t a3) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].i = a0;
    args[1].i = a1;
    args[2].j = a2;
    args[3].i = a3;

    env->CallVoidMethodA(wrapped_obj, jSet, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

int32_t BufferInfo::getFlags() {
    JNIEnv *env = GetJNIForThread();
    return env->GetIntField(wrapped_obj, jFlags);
}

void BufferInfo::setFlags(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetIntField(wrapped_obj, jFlags, a0);
}

int32_t BufferInfo::getOffset() {
    JNIEnv *env = GetJNIForThread();
    return env->GetIntField(wrapped_obj, jOffset);
}

void BufferInfo::setOffset(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetIntField(wrapped_obj, jOffset, a0);
}

int64_t BufferInfo::getPresentationTimeUs() {
    JNIEnv *env = GetJNIForThread();
    return env->GetLongField(wrapped_obj, jPresentationTimeUs);
}

void BufferInfo::setPresentationTimeUs(int64_t a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetLongField(wrapped_obj, jPresentationTimeUs, a0);
}

int32_t BufferInfo::getSize() {
    JNIEnv *env = GetJNIForThread();
    return env->GetIntField(wrapped_obj, jSize);
}

void BufferInfo::setSize(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetIntField(wrapped_obj, jSize, a0);
}

void InitSDKStubs(JNIEnv *jEnv) {
    MediaCodec::InitStubs(jEnv);
    MediaFormat::InitStubs(jEnv);
    ByteBuffer::InitStubs(jEnv);
    BufferInfo::InitStubs(jEnv);
}
} 
} 
} 
