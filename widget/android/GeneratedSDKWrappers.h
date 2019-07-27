









#ifndef GeneratedSDKWrappers_h__
#define GeneratedSDKWrappers_h__

#include "nsXPCOMStrings.h"
#include "AndroidJavaWrappers.h"

namespace mozilla {
namespace widget {
namespace android {

#define MEDIACODEC_EXCEPTION_INDEX -255

void InitSDKStubs(JNIEnv *jEnv);

class MediaCodec : public AutoGlobalWrappedJavaObject {
public:
    static void InitStubs(JNIEnv *jEnv);
    static MediaCodec* Wrap(jobject obj);
    MediaCodec(jobject obj, JNIEnv* env) : AutoGlobalWrappedJavaObject(obj, env) {};
    bool Configure(jobject a0, jobject a1, jobject a2, int32_t a3);
    static jobject CreateByCodecName(const nsAString& a0);
    static jobject CreateDecoderByType(const nsAString& a0);
    static jobject CreateEncoderByType(const nsAString& a0);
    int32_t DequeueInputBuffer(int64_t a0);
    int32_t DequeueOutputBuffer(jobject a0, int64_t a1);
    void Finalize();
    void Flush();
    jobjectArray GetInputBuffers();
    jobjectArray GetOutputBuffers();
    jobject GetOutputFormat();
    void QueueInputBuffer(int32_t a0, int32_t a1, int32_t a2, int64_t a3, int32_t a4);
    void QueueSecureInputBuffer(int32_t a0, int32_t a1, jobject a2, int64_t a3, int32_t a4);
    void Release();
    void ReleaseOutputBuffer(int32_t a0, bool a1);
    void SetVideoScalingMode(int32_t a0);
    bool Start();
    void Stop();
    static int32_t getBUFFER_FLAG_CODEC_CONFIG();
    static int32_t getBUFFER_FLAG_END_OF_STREAM();
    static int32_t getBUFFER_FLAG_SYNC_FRAME();
    static int32_t getCONFIGURE_FLAG_ENCODE();
    static int32_t getCRYPTO_MODE_AES_CTR();
    static int32_t getCRYPTO_MODE_UNENCRYPTED();
    static int32_t getINFO_OUTPUT_BUFFERS_CHANGED();
    static int32_t getINFO_OUTPUT_FORMAT_CHANGED();
    static int32_t getINFO_TRY_AGAIN_LATER();
    static int32_t getVIDEO_SCALING_MODE_SCALE_TO_FIT();
    static int32_t getVIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING();
    MediaCodec() : AutoGlobalWrappedJavaObject() {};
protected:
    static jclass mMediaCodecClass;
    static jmethodID jConfigure;
    static jmethodID jCreateByCodecName;
    static jmethodID jCreateDecoderByType;
    static jmethodID jCreateEncoderByType;
    static jmethodID jDequeueInputBuffer;
    static jmethodID jDequeueOutputBuffer;
    static jmethodID jFinalize;
    static jmethodID jFlush;
    static jmethodID jGetInputBuffers;
    static jmethodID jGetOutputBuffers;
    static jmethodID jGetOutputFormat;
    static jmethodID jQueueInputBuffer;
    static jmethodID jQueueSecureInputBuffer;
    static jmethodID jRelease;
    static jmethodID jReleaseOutputBuffer;
    static jmethodID jSetVideoScalingMode;
    static jmethodID jStart;
    static jmethodID jStop;
    static jfieldID jBUFFER_FLAG_CODEC_CONFIG;
    static jfieldID jBUFFER_FLAG_END_OF_STREAM;
    static jfieldID jBUFFER_FLAG_SYNC_FRAME;
    static jfieldID jCONFIGURE_FLAG_ENCODE;
    static jfieldID jCRYPTO_MODE_AES_CTR;
    static jfieldID jCRYPTO_MODE_UNENCRYPTED;
    static jfieldID jINFO_OUTPUT_BUFFERS_CHANGED;
    static jfieldID jINFO_OUTPUT_FORMAT_CHANGED;
    static jfieldID jINFO_TRY_AGAIN_LATER;
    static jfieldID jVIDEO_SCALING_MODE_SCALE_TO_FIT;
    static jfieldID jVIDEO_SCALING_MODE_SCALE_TO_FIT_WITH_CROPPING;
};

class MediaFormat : public AutoGlobalWrappedJavaObject {
public:
    static void InitStubs(JNIEnv *jEnv);
    static MediaFormat* Wrap(jobject obj);
    MediaFormat(jobject obj, JNIEnv* env) : AutoGlobalWrappedJavaObject(obj, env) {};
    MediaFormat();
    bool ContainsKey(const nsAString& a0);
    static jobject CreateAudioFormat(const nsAString& a0, int32_t a1, int32_t a2);
    static jobject CreateVideoFormat(const nsAString& a0, int32_t a1, int32_t a2);
    jobject GetByteBuffer(const nsAString& a0);
    jfloat GetFloat(const nsAString& a0);
    int32_t GetInteger(const nsAString& a0);
    int64_t GetLong(const nsAString& a0);
    jstring GetString(const nsAString& a0);
    void SetByteBuffer(const nsAString& a0, jobject a1);
    void SetFloat(const nsAString& a0, jfloat a1);
    void SetInteger(const nsAString& a0, int32_t a1);
    void SetLong(const nsAString& a0, int64_t a1);
    void SetString(const nsAString& a0, const nsAString& a1);
    jstring ToString();
    static jstring getKEY_AAC_PROFILE();
    static jstring getKEY_BIT_RATE();
    static jstring getKEY_CHANNEL_COUNT();
    static jstring getKEY_CHANNEL_MASK();
    static jstring getKEY_COLOR_FORMAT();
    static jstring getKEY_DURATION();
    static jstring getKEY_FLAC_COMPRESSION_LEVEL();
    static jstring getKEY_FRAME_RATE();
    static jstring getKEY_HEIGHT();
    static jstring getKEY_IS_ADTS();
    static jstring getKEY_I_FRAME_INTERVAL();
    static jstring getKEY_MAX_INPUT_SIZE();
    static jstring getKEY_MIME();
    static jstring getKEY_SAMPLE_RATE();
    static jstring getKEY_WIDTH();
protected:
    static jclass mMediaFormatClass;
    static jmethodID jMediaFormat;
    static jmethodID jContainsKey;
    static jmethodID jCreateAudioFormat;
    static jmethodID jCreateVideoFormat;
    static jmethodID jGetByteBuffer;
    static jmethodID jGetFloat;
    static jmethodID jGetInteger;
    static jmethodID jGetLong;
    static jmethodID jGetString;
    static jmethodID jSetByteBuffer;
    static jmethodID jSetFloat;
    static jmethodID jSetInteger;
    static jmethodID jSetLong;
    static jmethodID jSetString;
    static jmethodID jToString;
    static jfieldID jKEY_AAC_PROFILE;
    static jfieldID jKEY_BIT_RATE;
    static jfieldID jKEY_CHANNEL_COUNT;
    static jfieldID jKEY_CHANNEL_MASK;
    static jfieldID jKEY_COLOR_FORMAT;
    static jfieldID jKEY_DURATION;
    static jfieldID jKEY_FLAC_COMPRESSION_LEVEL;
    static jfieldID jKEY_FRAME_RATE;
    static jfieldID jKEY_HEIGHT;
    static jfieldID jKEY_IS_ADTS;
    static jfieldID jKEY_I_FRAME_INTERVAL;
    static jfieldID jKEY_MAX_INPUT_SIZE;
    static jfieldID jKEY_MIME;
    static jfieldID jKEY_SAMPLE_RATE;
    static jfieldID jKEY_WIDTH;
};

class ByteBuffer : public AutoGlobalWrappedJavaObject {
public:
    static void InitStubs(JNIEnv *jEnv);
    static ByteBuffer* Wrap(jobject obj);
    ByteBuffer(jobject obj, JNIEnv* env) : AutoGlobalWrappedJavaObject(obj, env) {};
    int8_t _get(int32_t a0);
    void _put(int32_t a0, int8_t a1);
    static jobject Allocate(int32_t a0);
    static jobject AllocateDirect(int32_t a0);
    jobject Array();
    jbyteArray Array1();
    int32_t ArrayOffset();
    jstring AsCharBuffer();
    jobject AsDoubleBuffer();
    jobject AsFloatBuffer();
    jobject AsIntBuffer();
    jobject AsLongBuffer();
    jobject AsReadOnlyBuffer();
    jobject AsShortBuffer();
    jobject Compact();
    int32_t CompareTo(jobject a0);
    int32_t CompareTo1(jobject a0);
    jobject Duplicate();
    bool Equals(jobject a0);
    int8_t Get();
    int8_t Get1(int32_t a0);
    jobject Get1(jbyteArray a0);
    jobject Get1(jbyteArray a0, int32_t a1, int32_t a2);
    uint16_t GetChar();
    uint16_t GetChar1(int32_t a0);
    jdouble GetDouble();
    jdouble GetDouble1(int32_t a0);
    jfloat GetFloat();
    jfloat GetFloat1(int32_t a0);
    int32_t GetInt();
    int32_t GetInt1(int32_t a0);
    int64_t GetLong();
    int64_t GetLong1(int32_t a0);
    int16_t GetShort();
    int16_t GetShort1(int32_t a0);
    bool HasArray();
    int32_t HashCode();
    bool IsDirect();
    jobject Order();
    jobject Order1(jobject a0);
    jobject Put(int8_t a0);
    jobject Put1(int32_t a0, int8_t a1);
    jobject Put1(jobject a0);
    jobject Put1(jbyteArray a0);
    jobject Put1(jbyteArray a0, int32_t a1, int32_t a2);
    jobject PutChar(uint16_t a0);
    jobject PutChar1(int32_t a0, uint16_t a1);
    jobject PutDouble(jdouble a0);
    jobject PutDouble1(int32_t a0, jdouble a1);
    jobject PutFloat(jfloat a0);
    jobject PutFloat1(int32_t a0, jfloat a1);
    jobject PutInt(int32_t a0);
    jobject PutInt1(int32_t a0, int32_t a1);
    jobject PutLong(int32_t a0, int64_t a1);
    jobject PutLong1(int64_t a0);
    jobject PutShort(int32_t a0, int16_t a1);
    jobject PutShort1(int16_t a0);
    jobject Slice();
    jstring ToString();
    static jobject Wrap1(jbyteArray a0);
    static jobject Wrap2(jbyteArray a0, int32_t a1, int32_t a2);
    bool getBigEndian();
    void setBigEndian(bool a0);
    jbyteArray getHb();
    bool getIsReadOnly();
    void setIsReadOnly(bool a0);
    bool getNativeByteOrder();
    void setNativeByteOrder(bool a0);
    int32_t getOffset();
    ByteBuffer() : AutoGlobalWrappedJavaObject() {};
protected:
    static jclass mByteBufferClass;
    static jmethodID j_get;
    static jmethodID j_put;
    static jmethodID jAllocate;
    static jmethodID jAllocateDirect;
    static jmethodID jArray;
    static jmethodID jArray1;
    static jmethodID jArrayOffset;
    static jmethodID jAsCharBuffer;
    static jmethodID jAsDoubleBuffer;
    static jmethodID jAsFloatBuffer;
    static jmethodID jAsIntBuffer;
    static jmethodID jAsLongBuffer;
    static jmethodID jAsReadOnlyBuffer;
    static jmethodID jAsShortBuffer;
    static jmethodID jCompact;
    static jmethodID jCompareTo;
    static jmethodID jCompareTo1;
    static jmethodID jDuplicate;
    static jmethodID jEquals;
    static jmethodID jGet;
    static jmethodID jGet1;
    static jmethodID jGet10;
    static jmethodID jGet11;
    static jmethodID jGetChar;
    static jmethodID jGetChar1;
    static jmethodID jGetDouble;
    static jmethodID jGetDouble1;
    static jmethodID jGetFloat;
    static jmethodID jGetFloat1;
    static jmethodID jGetInt;
    static jmethodID jGetInt1;
    static jmethodID jGetLong;
    static jmethodID jGetLong1;
    static jmethodID jGetShort;
    static jmethodID jGetShort1;
    static jmethodID jHasArray;
    static jmethodID jHashCode;
    static jmethodID jIsDirect;
    static jmethodID jOrder;
    static jmethodID jOrder1;
    static jmethodID jPut;
    static jmethodID jPut1;
    static jmethodID jPut12;
    static jmethodID jPut13;
    static jmethodID jPut14;
    static jmethodID jPutChar;
    static jmethodID jPutChar1;
    static jmethodID jPutDouble;
    static jmethodID jPutDouble1;
    static jmethodID jPutFloat;
    static jmethodID jPutFloat1;
    static jmethodID jPutInt;
    static jmethodID jPutInt1;
    static jmethodID jPutLong;
    static jmethodID jPutLong1;
    static jmethodID jPutShort;
    static jmethodID jPutShort1;
    static jmethodID jSlice;
    static jmethodID jToString;
    static jmethodID jWrap;
    static jmethodID jWrap1;
    static jfieldID jBigEndian;
    static jfieldID jHb;
    static jfieldID jIsReadOnly;
    static jfieldID jNativeByteOrder;
    static jfieldID jOffset;
};

class BufferInfo : public AutoGlobalWrappedJavaObject {
public:
    static void InitStubs(JNIEnv *jEnv);
    static BufferInfo* Wrap(jobject obj);
    BufferInfo(jobject obj, JNIEnv* env) : AutoGlobalWrappedJavaObject(obj, env) {};
    BufferInfo();
    void Set(int32_t a0, int32_t a1, int64_t a2, int32_t a3);
    int32_t getFlags();
    void setFlags(int32_t a0);
    int32_t getOffset();
    void setOffset(int32_t a0);
    int64_t getPresentationTimeUs();
    void setPresentationTimeUs(int64_t a0);
    int32_t getSize();
    void setSize(int32_t a0);
protected:
    static jclass mBufferInfoClass;
    static jmethodID jBufferInfo;
    static jmethodID jSet;
    static jfieldID jFlags;
    static jfieldID jOffset;
    static jfieldID jPresentationTimeUs;
    static jfieldID jSize;
};

} 
} 
} 
#endif
