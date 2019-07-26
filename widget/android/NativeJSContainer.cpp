




#include "NativeJSContainer.h"
#include "AndroidBridge.h"
#include "mozilla/Vector.h"
#include "prthread.h"

using namespace mozilla;
using namespace mozilla::widget;

namespace mozilla {
namespace widget {

class NativeJSContainer
{
public:
    static void InitJNI(JNIEnv* env) {
        if (jNativeJSContainer) {
            return;
        }
        jNativeJSContainer = AndroidBridge::GetClassGlobalRef(
            env, "org/mozilla/gecko/util/NativeJSContainer");
        MOZ_ASSERT(jNativeJSContainer);
        jContainerNativeObject = AndroidBridge::GetFieldID(
            env, jNativeJSContainer, "mNativeObject", "J");
        MOZ_ASSERT(jContainerNativeObject);
        jContainerConstructor = AndroidBridge::GetMethodID(
            env, jNativeJSContainer, "<init>", "(J)V");
        MOZ_ASSERT(jContainerConstructor);

        jNativeJSObject = AndroidBridge::GetClassGlobalRef(
            env, "org/mozilla/gecko/util/NativeJSObject");
        MOZ_ASSERT(jNativeJSObject);
        jObjectContainer = AndroidBridge::GetFieldID(
            env, jNativeJSObject, "mContainer",
            "Lorg/mozilla/gecko/util/NativeJSContainer;");
        MOZ_ASSERT(jObjectContainer);
        jObjectIndex = AndroidBridge::GetFieldID(
            env, jNativeJSObject, "mObjectIndex", "I");
        MOZ_ASSERT(jObjectIndex);
        jObjectConstructor = AndroidBridge::GetMethodID(
            env, jNativeJSObject, "<init>",
            "(Lorg/mozilla/gecko/util/NativeJSContainer;I)V");
        MOZ_ASSERT(jContainerConstructor);

        jBundle = AndroidBridge::GetClassGlobalRef(
            env, "android/os/Bundle");
        MOZ_ASSERT(jBundle);
        jBundleConstructor = AndroidBridge::GetMethodID(
            env, jBundle, "<init>", "(I)V");
        MOZ_ASSERT(jBundleConstructor);
        jBundlePutBoolean = AndroidBridge::GetMethodID(
            env, jBundle, "putBoolean",
            "(Ljava/lang/String;Z)V");
        MOZ_ASSERT(jBundlePutBoolean);
        jBundlePutBundle = AndroidBridge::GetMethodID(
            env, jBundle, "putBundle",
            "(Ljava/lang/String;Landroid/os/Bundle;)V");
        MOZ_ASSERT(jBundlePutBundle);
        jBundlePutDouble = AndroidBridge::GetMethodID(
            env, jBundle, "putDouble",
            "(Ljava/lang/String;D)V");
        MOZ_ASSERT(jBundlePutDouble);
        jBundlePutInt = AndroidBridge::GetMethodID(
            env, jBundle, "putInt",
            "(Ljava/lang/String;I)V");
        MOZ_ASSERT(jBundlePutInt);
        jBundlePutString = AndroidBridge::GetMethodID(
            env, jBundle, "putString",
            "(Ljava/lang/String;Ljava/lang/String;)V");
        MOZ_ASSERT(jBundlePutString);
    }

    static jobject CreateInstance(JNIEnv* env, JSContext* cx,
                                  JS::HandleObject object) {
        return CreateInstance(env, new NativeJSContainer(cx, object));
    }

    static NativeJSContainer* FromInstance(JNIEnv* env, jobject instance) {
        MOZ_ASSERT(instance);

        const jlong fieldValue =
            env->GetLongField(instance, jContainerNativeObject);
        NativeJSContainer* const nativeObject =
            reinterpret_cast<NativeJSContainer* const>(
            static_cast<uintptr_t>(fieldValue));
        if (!nativeObject) {
            AndroidBridge::ThrowException(env,
                "java/lang/NullPointerException",
                "Uninitialized NativeJSContainer");
        }
        return nativeObject;
    }

    static void DisposeInstance(JNIEnv* env, jobject instance) {
        NativeJSContainer* const container = FromInstance(env, instance);
        if (container) {
            env->SetLongField(instance, jContainerNativeObject,
                static_cast<jlong>(reinterpret_cast<uintptr_t>(nullptr)));
            delete container;
        }
    }

    static jobject CloneInstance(JNIEnv* env, jobject instance) {
        NativeJSContainer* const container = FromInstance(env, instance);
        if (!container || !container->EnsureObject(env)) {
            return nullptr;
        }
        JSContext* const cx = container->mThreadContext;
        JS::RootedObject object(cx, container->mJSObject);
        MOZ_ASSERT(object);

        JSAutoStructuredCloneBuffer buffer;
        if (!buffer.write(cx, JS::RootedValue(cx, JS::ObjectValue(*object)))) {
            AndroidBridge::ThrowException(env,
                "java/lang/UnsupportedOperationException",
                "Cannot serialize object");
            return nullptr;
        }
        return CreateInstance(env, new NativeJSContainer(cx, Move(buffer)));
    }

    static jobject GetInstanceFromObject(JNIEnv* env, jobject object) {
        MOZ_ASSERT(object);

        const jobject instance = env->GetObjectField(object, jObjectContainer);
        MOZ_ASSERT(instance);
        return instance;
    }

    static JSContext* GetContextFromObject(JNIEnv* env, jobject object) {
        MOZ_ASSERT(object);
        AutoLocalJNIFrame frame(env, 1);

        NativeJSContainer* const container =
            FromInstance(env, GetInstanceFromObject(env, object));
        if (!container) {
            return nullptr;
        }
        return container->mThreadContext;
    }

    static JSObject* GetObjectFromObject(JNIEnv* env, jobject object) {
        MOZ_ASSERT(object);
        AutoLocalJNIFrame frame(env, 1);

        NativeJSContainer* const container =
            FromInstance(env, GetInstanceFromObject(env, object));
        if (!container ||
            !container->EnsureObject(env)) { 
            return nullptr;
        }
        const jint index = env->GetIntField(object, jObjectIndex);
        if (index < 0) {
            
            return container->mJSObject;
        }
        return container->mRootedObjects[index];
    }

    static jobject CreateObjectInstance(JNIEnv* env, jobject object,
                                        JSContext* cx,
                                        JS::HandleObject jsObject) {
        MOZ_ASSERT(object);
        MOZ_ASSERT(jsObject);
        AutoLocalJNIFrame frame(env, 2);

        jobject instance = GetInstanceFromObject(env, object);
        NativeJSContainer* const container = FromInstance(env, instance);
        if (!container) {
            return nullptr;
        }
        size_t newIndex = container->mRootedObjects.length();
        if (!container->mRootedObjects.append(jsObject)) {
            AndroidBridge::ThrowException(env,
                "java/lang/OutOfMemoryError", "Cannot allocate object");
            return nullptr;
        }
        const jobject newObject =
            env->NewObject(jNativeJSObject, jObjectConstructor,
                           instance, newIndex);
        AndroidBridge::HandleUncaughtException(env);
        MOZ_ASSERT(newObject);
        return frame.Pop(newObject);
    }

    
    bool EnsureObject(JNIEnv* env) {
        if (mJSObject) {
            if (PR_GetCurrentThread() != mThread) {
                AndroidBridge::ThrowException(env,
                    "java/lang/IllegalThreadStateException",
                    "Using NativeJSObject off its thread");
                return false;
            }
            return true;
        }
        if (!SwitchContextToCurrentThread()) {
            AndroidBridge::ThrowException(env,
                "java/lang/IllegalThreadStateException",
                "Not available for this thread");
            return false;
        }

        JS::RootedValue value(mThreadContext);
        MOZ_ASSERT(mBuffer.data());
        MOZ_ALWAYS_TRUE(mBuffer.read(mThreadContext, &value));
        if (value.isObject()) {
            mJSObject = &value.toObject();
        }
        if (!mJSObject) {
            AndroidBridge::ThrowException(env,
                "java/lang/IllegalStateException", "Cannot deserialize data");
            return false;
        }
        mBuffer.clear();
        return true;
    }

    static jclass jBundle;
    static jmethodID jBundleConstructor;
    static jmethodID jBundlePutBoolean;
    static jmethodID jBundlePutBundle;
    static jmethodID jBundlePutDouble;
    static jmethodID jBundlePutInt;
    static jmethodID jBundlePutString;

private:
    static jclass jNativeJSContainer;
    static jfieldID jContainerNativeObject;
    static jmethodID jContainerConstructor;
    static jclass jNativeJSObject;
    static jfieldID jObjectContainer;
    static jfieldID jObjectIndex;
    static jmethodID jObjectConstructor;

    static jobject CreateInstance(JNIEnv* env,
                                  NativeJSContainer* nativeObject) {
        InitJNI(env);
        const jobject newObject =
            env->NewObject(jNativeJSContainer, jContainerConstructor,
                           static_cast<jlong>(
                           reinterpret_cast<uintptr_t>(nativeObject)));
        AndroidBridge::HandleUncaughtException(env);
        MOZ_ASSERT(newObject);
        return newObject;
    }

    
    PRThread* mThread;
    
    JSContext* mThreadContext;
    
    JS::Heap<JSObject*> mJSObject;
    
    JSAutoStructuredCloneBuffer mBuffer;
    
    Vector<JS::Heap<JSObject*>, 4> mRootedObjects;

    
    NativeJSContainer(JSContext* cx, JS::HandleObject object)
            : mThread(PR_GetCurrentThread())
            , mThreadContext(cx)
            , mJSObject(object)
    {
    }

    
    NativeJSContainer(JSContext* cx, JSAutoStructuredCloneBuffer&& buffer)
            : mThread(PR_GetCurrentThread())
            , mThreadContext(cx)
            , mBuffer(Forward<JSAutoStructuredCloneBuffer>(buffer))
    {
    }

    bool SwitchContextToCurrentThread() {
        PRThread* const currentThread = PR_GetCurrentThread();
        if (currentThread == mThread) {
            return true;
        }
        return false;
    }
};

jclass NativeJSContainer::jNativeJSContainer = 0;
jfieldID NativeJSContainer::jContainerNativeObject = 0;
jmethodID NativeJSContainer::jContainerConstructor = 0;
jclass NativeJSContainer::jNativeJSObject = 0;
jfieldID NativeJSContainer::jObjectContainer = 0;
jfieldID NativeJSContainer::jObjectIndex = 0;
jmethodID NativeJSContainer::jObjectConstructor = 0;
jclass NativeJSContainer::jBundle = 0;
jmethodID NativeJSContainer::jBundleConstructor = 0;
jmethodID NativeJSContainer::jBundlePutBoolean = 0;
jmethodID NativeJSContainer::jBundlePutBundle = 0;
jmethodID NativeJSContainer::jBundlePutDouble = 0;
jmethodID NativeJSContainer::jBundlePutInt = 0;
jmethodID NativeJSContainer::jBundlePutString = 0;

jobject
CreateNativeJSContainer(JNIEnv* env, JSContext* cx, JS::HandleObject object)
{
    return NativeJSContainer::CreateInstance(env, cx, object);
}

} 
} 

namespace {

class JSJNIString
{
public:
    JSJNIString(JNIEnv* env, jstring str)
        : mEnv(env)
        , mJNIString(str)
        , mJSString(!str ? nullptr :
            reinterpret_cast<const jschar*>(env->GetStringChars(str, nullptr)))
    {
    }
    ~JSJNIString() {
        if (mJNIString) {
            mEnv->ReleaseStringChars(mJNIString,
                reinterpret_cast<const jchar*>(mJSString));
        }
    }
    operator const jschar*() const {
        return mJSString;
    }
    size_t Length() const {
        return static_cast<size_t>(mEnv->GetStringLength(mJNIString));
    }
private:
    JNIEnv* const mEnv;
    const jstring mJNIString;
    const jschar* const mJSString;
};

bool
CheckJSCall(JNIEnv* env, bool result)
{
    if (!result) {
        AndroidBridge::ThrowException(env,
            "java/lang/UnsupportedOperationException", "JSAPI call failed");
    }
    return result;
}

bool
CheckJNIArgument(JNIEnv* env, jobject arg)
{
    if (!arg) {
        AndroidBridge::ThrowException(env,
            "java/lang/IllegalArgumentException", "Null argument");
        return false;
    }
    return true;
}

bool
AppendJSON(const jschar* buf, uint32_t len, void* data)
{
    static_cast<nsAutoString*>(data)->Append(buf, len);
    return true;
}

template <typename U, typename V,
          bool (JS::Value::*IsMethod)() const,
          V (JS::Value::*ToMethod)() const>
struct PrimitiveProperty
{
    typedef U Type; 
    typedef V NativeType; 

    static bool InValue(JS::HandleValue val) {
        return (static_cast<const JS::Value&>(val).*IsMethod)();
    }

    static Type FromValue(JNIEnv* env, jobject instance,
                          JSContext* cx, JS::HandleValue val) {
        return static_cast<Type>(
            (static_cast<const JS::Value&>(val).*ToMethod)());
    }
};




typedef PrimitiveProperty<jboolean, bool,
    &JS::Value::isBoolean, &JS::Value::toBoolean> BooleanProperty;

typedef PrimitiveProperty<jdouble, double,
    &JS::Value::isNumber, &JS::Value::toNumber> DoubleProperty;

typedef PrimitiveProperty<jint, int32_t,
    &JS::Value::isInt32, &JS::Value::toInt32> IntProperty;

struct StringProperty
{
    typedef jstring Type;

    static bool InValue(JS::HandleValue val) {
        return val.isString();
    }

    static Type FromValue(JNIEnv* env, jobject instance,
                          JSContext* cx, JS::HandleValue val) {
        const JS::RootedString str(cx, val.toString());
        return FromValue(env, instance, cx, str);
    }

    static Type FromValue(JNIEnv* env, jobject instance,
                          JSContext* cx, const JS::HandleString str) {
        size_t strLen = 0;
        const jschar* const strChars =
            JS_GetStringCharsAndLength(cx, str, &strLen);
        if (!CheckJSCall(env, !!strChars)) {
            return nullptr;
        }
        jstring ret = env->NewString(
            reinterpret_cast<const jchar*>(strChars), strLen);
        AndroidBridge::HandleUncaughtException(env);
        MOZ_ASSERT(ret);
        return ret;
    }
};

template <jobject (*FactoryMethod)
    (JNIEnv*, jobject, JSContext*, JS::HandleObject)>
struct BaseObjectProperty
{
    typedef jobject Type;

    static bool InValue(JS::HandleValue val) {
        return val.isObjectOrNull();
    }

    static Type FromValue(JNIEnv* env, jobject instance,
                          JSContext* cx, JS::HandleValue val) {
        if (val.isNull()) {
            return nullptr;
        }
        JS::RootedObject object(cx, &val.toObject());
        return FactoryMethod(env, instance, cx, object);
    }
};

jobject GetBundle(JNIEnv*, jobject, JSContext*, JS::HandleObject);


typedef BaseObjectProperty<
    NativeJSContainer::CreateObjectInstance> ObjectProperty;


typedef BaseObjectProperty<GetBundle> BundleProperty;

struct HasProperty
{
    typedef jboolean Type;

    static bool InValue(JS::HandleValue val) {
        return true;
    }

    static Type FromValue(JNIEnv* env, jobject instance,
                          JSContext* cx, JS::HandleValue val) {
        return JNI_TRUE;
    }
};

MOZ_BEGIN_ENUM_CLASS(FallbackOption)
    THROW,
    RETURN,
MOZ_END_ENUM_CLASS(FallbackOption)

template <class Property>
typename Property::Type
GetProperty(JNIEnv* env, jobject instance, jstring name,
            FallbackOption option = FallbackOption::THROW,
            typename Property::Type fallback = typename Property::Type())
{
    MOZ_ASSERT(env);
    MOZ_ASSERT(instance);

    JSContext* const cx =
        NativeJSContainer::GetContextFromObject(env, instance);
    if (!cx) {
        return typename Property::Type();
    }

    const JS::RootedObject object(cx,
        NativeJSContainer::GetObjectFromObject(env, instance));
    const JSJNIString strName(env, name);
    JS::RootedValue val(cx);

    if (!object ||
        !CheckJNIArgument(env, name) ||
        !CheckJSCall(env,
            JS_GetUCProperty(cx, object, strName, strName.Length(), &val))) {
        return typename Property::Type();
    }
    if (val.isUndefined()) {
        if (option == FallbackOption::THROW) {
            AndroidBridge::ThrowException(env,
                "java/lang/IllegalArgumentException",
                "Property does not exist");
        }
        return fallback;
    }
    if (!Property::InValue(val)) {
        AndroidBridge::ThrowException(env,
            "java/lang/IllegalArgumentException",
            "Property type mismatch");
        return fallback;
    }
    return Property::FromValue(env, instance, cx, val);
}

jobject
GetBundle(JNIEnv* env, jobject instance, JSContext* cx, JS::HandleObject obj)
{
    AutoLocalJNIFrame frame(env, 1);

    const JS::AutoIdArray ids(cx, JS_Enumerate(cx, obj));
    if (!CheckJSCall(env, !!ids)) {
        return nullptr;
    }

    const size_t length = ids.length();
    const jobject newBundle = env->NewObject(
        NativeJSContainer::jBundle,
        NativeJSContainer::jBundleConstructor,
        static_cast<jint>(length));
    AndroidBridge::HandleUncaughtException(env);
    if (!newBundle) {
        return nullptr;
    }

    for (size_t i = 0; i < ids.length(); i++) {
        AutoLocalJNIFrame loopFrame(env, 2);

        const JS::RootedId id(cx, ids[i]);
        JS::RootedValue idVal(cx);
        if (!CheckJSCall(env, JS_IdToValue(cx, id, &idVal))) {
            return nullptr;
        }

        const JS::RootedString idStr(cx, JS::ToString(cx, idVal));
        if (!CheckJSCall(env, !!idStr)) {
            return nullptr;
        }

        const jstring name =
            StringProperty::FromValue(env, instance, cx, idStr);
        JS::RootedValue val(cx);
        if (!name ||
            !CheckJSCall(env, JS_GetPropertyById(cx, obj, id, &val))) {
            return nullptr;
        }

#define PUT_IN_BUNDLE_IF_TYPE_IS(Type)                              \
        if (Type##Property::InValue(val)) {                         \
            env->CallVoidMethod(                                    \
                newBundle,                                          \
                NativeJSContainer::jBundlePut##Type,                \
                name,                                               \
                Type##Property::FromValue(env, instance, cx, val)); \
            AndroidBridge::HandleUncaughtException(env);            \
            continue;                                               \
        }                                                           \
        ((void) 0) // Accommodate trailing semicolon.

        PUT_IN_BUNDLE_IF_TYPE_IS(Boolean);
        
        PUT_IN_BUNDLE_IF_TYPE_IS(Int);
        PUT_IN_BUNDLE_IF_TYPE_IS(Double);
        PUT_IN_BUNDLE_IF_TYPE_IS(String);
        
        PUT_IN_BUNDLE_IF_TYPE_IS(Bundle);

#undef PUT_IN_BUNDLE_IF_TYPE_IS

        
        AndroidBridge::ThrowException(env,
            "java/lang/UnsupportedOperationException",
            "Unsupported property type");
        return nullptr;
    }
    return frame.Pop(newBundle);
}

} 

extern "C" {

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_util_NativeJSContainer_dispose(JNIEnv* env, jobject instance)
{
    MOZ_ASSERT(env);
    NativeJSContainer::DisposeInstance(env, instance);
}

NS_EXPORT jobject JNICALL
Java_org_mozilla_gecko_util_NativeJSContainer_clone(JNIEnv* env, jobject instance)
{
    MOZ_ASSERT(env);
    return NativeJSContainer::CloneInstance(env, instance);
}

NS_EXPORT jboolean JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_getBoolean(JNIEnv* env, jobject instance, jstring name)
{
    return GetProperty<BooleanProperty>(env, instance, name);
}

NS_EXPORT jboolean JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_optBoolean(JNIEnv* env, jobject instance,
                                                      jstring name, jboolean fallback)
{
    return GetProperty<BooleanProperty>(env, instance, name, FallbackOption::RETURN, fallback);
}

NS_EXPORT jobject JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_getBundle(JNIEnv* env, jobject instance, jstring name)
{
    return GetProperty<BundleProperty>(env, instance, name);
}

NS_EXPORT jobject JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_optBundle(JNIEnv* env, jobject instance,
                                                     jstring name, jobject fallback)
{
    return GetProperty<BundleProperty>(env, instance, name, FallbackOption::RETURN, fallback);
}

NS_EXPORT jdouble JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_getDouble(JNIEnv* env, jobject instance, jstring name)
{
    return GetProperty<DoubleProperty>(env, instance, name);
}

NS_EXPORT jdouble JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_optDouble(JNIEnv* env, jobject instance,
                                                     jstring name, jdouble fallback)
{
    return GetProperty<DoubleProperty>(env, instance, name, FallbackOption::RETURN, fallback);
}

NS_EXPORT jint JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_getInt(JNIEnv* env, jobject instance, jstring name)
{
    return GetProperty<IntProperty>(env, instance, name);
}

NS_EXPORT jint JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_optInt(JNIEnv* env, jobject instance,
                                                  jstring name, jint fallback)
{
    return GetProperty<IntProperty>(env, instance, name, FallbackOption::RETURN, fallback);
}

NS_EXPORT jobject JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_getObject(JNIEnv* env, jobject instance, jstring name)
{
    return GetProperty<ObjectProperty>(env, instance, name);
}

NS_EXPORT jobject JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_optObject(JNIEnv* env, jobject instance,
                                                     jstring name, jobject fallback)
{
    return GetProperty<ObjectProperty>(env, instance, name, FallbackOption::RETURN, fallback);
}

NS_EXPORT jstring JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_getString(JNIEnv* env, jobject instance, jstring name)
{
    return GetProperty<StringProperty>(env, instance, name);
}

NS_EXPORT jstring JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_optString(JNIEnv* env, jobject instance,
                                                     jstring name, jstring fallback)
{
    return GetProperty<StringProperty>(env, instance, name, FallbackOption::RETURN, fallback);
}

NS_EXPORT jboolean JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_has(JNIEnv* env, jobject instance, jstring name)
{
    return GetProperty<HasProperty>(env, instance, name, FallbackOption::RETURN, JNI_FALSE);
}

NS_EXPORT jobject JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_toBundle(JNIEnv* env, jobject instance)
{
    MOZ_ASSERT(env);
    MOZ_ASSERT(instance);

    JSContext* const cx = NativeJSContainer::GetContextFromObject(env, instance);
    if (!cx) {
        return nullptr;
    }

    const JS::RootedObject object(cx, NativeJSContainer::GetObjectFromObject(env, instance));
    if (!object) {
        return nullptr;
    }
    return GetBundle(env, instance, cx, object);
}

NS_EXPORT jstring JNICALL
Java_org_mozilla_gecko_util_NativeJSObject_toString(JNIEnv* env, jobject instance)
{
    MOZ_ASSERT(env);
    MOZ_ASSERT(instance);

    JSContext* const cx = NativeJSContainer::GetContextFromObject(env, instance);
    if (!cx) {
        return nullptr;
    }

    const JS::RootedObject object(cx, NativeJSContainer::GetObjectFromObject(env, instance));
    JS::RootedValue value(cx, JS::ObjectValue(*object));
    nsAutoString json;

    if (!object ||
        !CheckJSCall(env,
            JS_Stringify(cx, &value, JS::NullPtr(), JS::NullHandleValue, AppendJSON, &json))) {
        return nullptr;
    }
    jstring ret = env->NewString(reinterpret_cast<const jchar*>(json.get()), json.Length());
    AndroidBridge::HandleUncaughtException(env);
    MOZ_ASSERT(ret);
    return ret;
}

} 
