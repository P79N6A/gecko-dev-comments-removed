#ifndef mozilla_jni_Types_h__
#define mozilla_jni_Types_h__

#include <jni.h>

#include "mozilla/jni/Refs.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace jni {
namespace {














template<typename T> struct TypeAdapter;



template<class Cls> struct TypeAdapter<LocalRef<Cls>> {
    typedef decltype(Ref<Cls>(nullptr).Get()) JNIType;

    static constexpr auto Call = &JNIEnv::CallObjectMethodA;
    static constexpr auto StaticCall = &JNIEnv::CallStaticObjectMethodA;
    static constexpr auto Get = &JNIEnv::GetObjectField;
    static constexpr auto StaticGet = &JNIEnv::GetStaticObjectField;

    static LocalRef<Cls> ToNative(JNIEnv* env, jobject instance) {
        return LocalRef<Cls>::Adopt(env, instance);
    }

    static JNIType FromNative(JNIEnv*, LocalRef<Cls>&& instance) {
        return instance.Forget();
    }
};

template<class Cls> constexpr jobject
    (JNIEnv::*TypeAdapter<LocalRef<Cls>>::Call)(jobject, jmethodID, jvalue*);
template<class Cls> constexpr jobject
    (JNIEnv::*TypeAdapter<LocalRef<Cls>>::StaticCall)(jclass, jmethodID, jvalue*);
template<class Cls> constexpr jobject
    (JNIEnv::*TypeAdapter<LocalRef<Cls>>::Get)(jobject, jfieldID);
template<class Cls> constexpr jobject
    (JNIEnv::*TypeAdapter<LocalRef<Cls>>::StaticGet)(jclass, jfieldID);



template<class Cls> struct TypeAdapter<Ref<Cls>> {
    typedef decltype(Ref<Cls>(nullptr).Get()) JNIType;

    static constexpr auto Set = &JNIEnv::SetObjectField;
    static constexpr auto StaticSet = &JNIEnv::SetStaticObjectField;

    static Ref<Cls> ToNative(JNIEnv* env, jobject instance) {
        return Ref<Cls>::From(instance);
    }

    static JNIType FromNative(JNIEnv*, const Ref<Cls>& instance) {
        return instance.Get();
    }
};

template<class Cls> constexpr void
    (JNIEnv::*TypeAdapter<Ref<Cls>>::Set)(jobject, jfieldID, jobject);
template<class Cls> constexpr void
    (JNIEnv::*TypeAdapter<Ref<Cls>>::StaticSet)(jclass, jfieldID, jobject);



template<> struct TypeAdapter<Param<String>>
        : public TypeAdapter<String::Ref>
{};


#define DEFINE_PRIMITIVE_TYPE_ADAPTER(NativeType, JNIType, JNIName) \
    \
    template<> struct TypeAdapter<NativeType> { \
        typedef JNIType JNI##Type; \
    \
        static constexpr auto Call = &JNIEnv::Call ## JNIName ## MethodA; \
        static constexpr auto StaticCall = &JNIEnv::CallStatic ## JNIName ## MethodA; \
        static constexpr auto Get = &JNIEnv::Get ## JNIName ## Field; \
        static constexpr auto StaticGet = &JNIEnv::GetStatic ## JNIName ## Field; \
        static constexpr auto Set = &JNIEnv::Set ## JNIName ## Field; \
        static constexpr auto StaticSet = &JNIEnv::SetStatic ## JNIName ## Field; \
    \
        static JNIType FromNative(JNIEnv*, NativeType val) { \
            return static_cast<JNIType>(val); \
        } \
        static NativeType ToNative(JNIEnv*, JNIType val) { \
            return static_cast<NativeType>(val); \
        } \
    }; \
    \
    constexpr JNIType (JNIEnv::*TypeAdapter<NativeType>::Call) \
            (jobject, jmethodID, jvalue*); \
    constexpr JNIType (JNIEnv::*TypeAdapter<NativeType>::StaticCall) \
            (jclass, jmethodID, jvalue*); \
    constexpr JNIType (JNIEnv::*TypeAdapter<NativeType>::Get) \
            (jobject, jfieldID); \
    constexpr JNIType (JNIEnv::*TypeAdapter<NativeType>::StaticGet) \
            (jclass, jfieldID); \
    constexpr void (JNIEnv::*TypeAdapter<NativeType>::Set) \
            (jobject, jfieldID, JNIType); \
    constexpr void (JNIEnv::*TypeAdapter<NativeType>::StaticSet) \
            (jclass, jfieldID, JNIType)


DEFINE_PRIMITIVE_TYPE_ADAPTER(bool,     jboolean, Boolean);
DEFINE_PRIMITIVE_TYPE_ADAPTER(int8_t,   jbyte,    Byte);
DEFINE_PRIMITIVE_TYPE_ADAPTER(char16_t, jchar,    Char);
DEFINE_PRIMITIVE_TYPE_ADAPTER(int16_t,  jshort,   Short);
DEFINE_PRIMITIVE_TYPE_ADAPTER(int32_t,  jint,     Int);
DEFINE_PRIMITIVE_TYPE_ADAPTER(int64_t,  jlong,    Long);
DEFINE_PRIMITIVE_TYPE_ADAPTER(float,    jfloat,   Float);
DEFINE_PRIMITIVE_TYPE_ADAPTER(double,   jdouble,  Double);

#undef DEFINE_PRIMITIVE_TYPE_ADAPTER

} 
} 
} 

#endif 
