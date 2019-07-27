#ifndef mozilla_jni_Refs_h__
#define mozilla_jni_Refs_h__

#include <jni.h>

#include "mozilla/Move.h"
#include "mozilla/jni/Utils.h"

#include "nsError.h" 
#include "nsString.h"

namespace mozilla {
namespace jni {

class Accessor;
template<class T> class Constructor;
template<class T> class Field;
template<class T, typename R> class Method;


template<class Cls> class Ref;

template<class Cls> class LocalRef;

template<class Cls> class GlobalRef;





template<class Cls> struct ParamImpl { typedef Ref<Cls> Type; };
template<class Cls> using Param = typename ParamImpl<Cls>::Type;



enum class ExceptionMode
{
    
    ABORT,
    
    IGNORE,
    
    NSRESULT,
};




template<class Cls>
class Class
{
    friend class Accessor;
    template<class T> friend class Constructor;
    template<class T> friend class Field;
    template<class T, typename R> friend class Method;

private:
    static jclass sClassRef; 

protected:
    jobject mInstance; 

    Class(jobject instance) : mInstance(instance) {}
};



class Object : public Class<Object>
{
protected:
    Object(jobject instance) : Class(instance) {}

public:
    typedef jni::Ref<Object> Ref;
    typedef jni::LocalRef<Object>  LocalRef;
    typedef jni::GlobalRef<Object> GlobalRef;
    typedef const jni::Param<Object>& Param;
};



template<typename T>
class TypedObject : public Object
{
    typedef TypedObject<T> Self;

protected:
    TypedObject(jobject instance) : Object(instance) {}

public:
    typedef jni::Ref<Self> Ref;
    typedef jni::LocalRef<Self>  LocalRef;
    typedef jni::GlobalRef<Self> GlobalRef;
    typedef const jni::Param<Self>& Param;
};


typedef TypedObject<jstring>    String;
typedef TypedObject<jclass>     ClassObject;
typedef TypedObject<jthrowable> Throwable;

typedef TypedObject<jbooleanArray> BooleanArray;
typedef TypedObject<jbyteArray>    ByteArray;
typedef TypedObject<jcharArray>    CharArray;
typedef TypedObject<jshortArray>   ShortArray;
typedef TypedObject<jintArray>     IntArray;
typedef TypedObject<jlongArray>    LongArray;
typedef TypedObject<jfloatArray>   FloatArray;
typedef TypedObject<jdoubleArray>  DoubleArray;
typedef TypedObject<jobjectArray>  ObjectArray;

template<> struct ParamImpl<String> { class Type; };



template<class Cls, typename JNIType>
class RefBase : protected Cls
{
    typedef RefBase<Cls, JNIType> Self;
    typedef void (Self::*bool_type)() const;
    void non_null_reference() const {}

protected:
    RefBase(jobject instance) : Cls(instance) {}

public:
    
    static Ref<Cls> From(JNIType obj)
    {
        return Ref<Cls>(static_cast<jobject>(obj));
    }

    
    JNIType Get() const
    {
        return static_cast<JNIType>(Cls::mInstance);
    }

    bool operator==(const RefBase& other) const
    {
        
        return Cls::mInstance == other.mInstance &&
                GetJNIForThread()->IsSameObject(
                        Cls::mInstance, other.mInstance) != JNI_FALSE;
    }

    bool operator!=(const RefBase& other) const
    {
        return !operator==(other);
    }

    bool operator==(decltype(nullptr)) const
    {
        return !Cls::mInstance;
    }

    bool operator!=(decltype(nullptr)) const
    {
        return !!Cls::mInstance;
    }

    Cls* operator->()
    {
        MOZ_ASSERT(Cls::mInstance);
        return this;
    }

    const Cls* operator->() const
    {
        MOZ_ASSERT(Cls::mInstance);
        return this;
    }

    
    operator Ref<Object>() const;

    
    operator bool_type() const
    {
        return Cls::mInstance ? &Self::non_null_reference : nullptr;
    }

    
    
    

    
    
    

    
};



template<class Cls>
class Ref : public RefBase<Cls, jobject>
{
    template<class C, typename T> friend class RefBase;
    friend class jni::LocalRef<Cls>;
    friend class jni::GlobalRef<Cls>;

    typedef RefBase<Cls, jobject> Base;

protected:
    
    
    
    Ref(jobject instance) : Base(instance) {}

    
    
    
    Ref(const Ref& ref) : Base(ref.mInstance) {}

public:
    MOZ_IMPLICIT Ref(decltype(nullptr)) : Base(nullptr) {}
};


template<class Cls, typename JNIType>
RefBase<Cls, JNIType>::operator Ref<Object>() const
{
    return Ref<Object>(Cls::mInstance);
}


template<typename T>
class Ref<TypedObject<T>>
        : public RefBase<TypedObject<T>, T>
{
    friend class RefBase<TypedObject<T>, T>;
    friend class jni::LocalRef<TypedObject<T>>;
    friend class jni::GlobalRef<TypedObject<T>>;

    typedef RefBase<TypedObject<T>, T> Base;

protected:
    Ref(jobject instance) : Base(instance) {}

    Ref(const Ref& ref) : Base(ref.mInstance) {}

public:
    MOZ_IMPLICIT Ref(decltype(nullptr)) : Base(nullptr) {}
};


namespace {


template<class Cls> struct GenericObject { typedef Object Type; };
template<> struct GenericObject<Object> { typedef struct {} Type; };

} 

template<class Cls>
class LocalRef : public Ref<Cls>
{
    template<class C> friend class LocalRef;

private:
    
    
    
    
    
    
    typedef typename GenericObject<Cls>::Type GenericObject;

    JNIEnv* const mEnv;

    LocalRef(JNIEnv* env, jobject instance)
        : Ref<Cls>(instance)
        , mEnv(env)
    {}

    LocalRef& swap(LocalRef& other)
    {
        auto instance = other.mInstance;
        other.mInstance = Ref<Cls>::mInstance;
        Ref<Cls>::mInstance = instance;
        return *this;
    }

public:
    
    
    
    static LocalRef Adopt(jobject instance)
    {
        return LocalRef(GetJNIForThread(), instance);
    }

    static LocalRef Adopt(JNIEnv* env, jobject instance)
    {
        return LocalRef(env, instance);
    }

    
    LocalRef(const LocalRef<Cls>& ref)
        : Ref<Cls>(ref.mEnv->NewLocalRef(ref.mInstance))
        , mEnv(ref.mEnv)
    {}

    
    LocalRef(LocalRef<Cls>&& ref)
        : Ref<Cls>(ref.mInstance)
        , mEnv(ref.mEnv)
    {
        ref.mInstance = nullptr;
    }

    explicit LocalRef(JNIEnv* env = GetJNIForThread())
        : Ref<Cls>(nullptr)
        , mEnv(env)
    {}

    
    
    MOZ_IMPLICIT LocalRef(const Ref<Cls>& ref)
        : Ref<Cls>(nullptr)
        , mEnv(GetJNIForThread())
    {
        Ref<Cls>::mInstance = mEnv->NewLocalRef(ref.mInstance);
    }

    
    
    MOZ_IMPLICIT LocalRef(LocalRef<GenericObject>&& ref)
        : Ref<Cls>(ref.mInstance)
        , mEnv(ref.mEnv)
    {
        ref.mInstance = nullptr;
    }

    
    MOZ_IMPLICIT LocalRef(decltype(nullptr))
        : Ref<Cls>(nullptr)
        , mEnv(GetJNIForThread())
    {}

    ~LocalRef()
    {
        if (Ref<Cls>::mInstance) {
            mEnv->DeleteLocalRef(Ref<Cls>::mInstance);
            Ref<Cls>::mInstance = nullptr;
        }
    }

    
    JNIEnv* Env() const
    {
        return mEnv;
    }

    
    
    auto Forget() -> decltype(Ref<Cls>(nullptr).Get())
    {
        const auto obj = Ref<Cls>::Get();
        Ref<Cls>::mInstance = nullptr;
        return obj;
    }

    LocalRef<Cls>& operator=(LocalRef<Cls> ref)
    {
        return swap(ref);
    }

    LocalRef<Cls>& operator=(const Ref<Cls>& ref)
    {
        LocalRef<Cls> newRef(mEnv, ref.mInstance);
        return swap(newRef);
    }

    LocalRef<Cls>& operator=(LocalRef<GenericObject>&& ref)
    {
        LocalRef<Cls> newRef(mozilla::Forward<LocalRef<GenericObject>>(ref));
        return swap(newRef);
    }

    LocalRef<Cls>& operator=(decltype(nullptr))
    {
        LocalRef<Cls> newRef(mEnv, nullptr);
        return swap(newRef);
    }
};


template<class Cls>
class GlobalRef : public Ref<Cls>
{
private:
    static jobject NewGlobalRef(JNIEnv* env, jobject instance)
    {
        if (!instance) {
            return nullptr;
        }
        if (!env) {
            env = GetJNIForThread();
        }
        return env->NewGlobalRef(instance);
    }

    GlobalRef& swap(GlobalRef& other)
    {
        auto instance = other.mInstance;
        other.mInstance = Ref<Cls>::mInstance;
        Ref<Cls>::mInstance = instance;
        return *this;
    }

public:
    GlobalRef()
        : Ref<Cls>(nullptr)
    {}

    
    GlobalRef(const GlobalRef& ref)
        : Ref<Cls>(ref.mInstance)
    {}

    
    GlobalRef(GlobalRef&& ref)
        : Ref<Cls>(ref.mInstance)
    {
        ref.mInstance = nullptr;
    }

    MOZ_IMPLICIT GlobalRef(const Ref<Cls>& ref)
        : Ref<Cls>(NewGlobalRef(nullptr, ref.mInstance))
    {}

    GlobalRef(JNIEnv* env, const Ref<Cls>& ref)
        : Ref<Cls>(NewGlobalRef(env, ref.mInstance))
    {}

    
    MOZ_IMPLICIT GlobalRef(decltype(nullptr))
        : Ref<Cls>(nullptr)
    {}

    ~GlobalRef()
    {
        if (Ref<Cls>::mInstance) {
            JNIEnv* const env = GetJNIForThread();
            env->DeleteGlobalRef(Ref<Cls>::mInstance);
            Ref<Cls>::mInstance = nullptr;
        }
    }

    
    
    auto Forget() -> decltype(Ref<Cls>(nullptr).Get())
    {
        const auto obj = Ref<Cls>::Get();
        Ref<Cls>::mInstance = nullptr;
        return obj;
    }

    GlobalRef<Cls>& operator=(GlobalRef<Cls> ref)
    {
        return swap(ref);
    }

    GlobalRef<Cls>& operator=(const Ref<Cls>& ref)
    {
        GlobalRef<Cls> newRef(ref);
        return swap(newRef);
    }

    GlobalRef<Cls>& operator=(decltype(nullptr))
    {
        GlobalRef<Cls> newRef(nullptr);
        return swap(newRef);
    }
};



template<>
class Ref<String> : public RefBase<String, jstring>
{
    friend class RefBase<String, jstring>;
    friend class jni::LocalRef<String>;
    friend class jni::GlobalRef<String>;

    typedef RefBase<TypedObject<jstring>, jstring> Base;

protected:
    Ref(jobject instance) : Base(instance) {}

    Ref(const Ref& ref) : Base(ref.mInstance) {}

public:
    MOZ_IMPLICIT Ref(decltype(nullptr)) : Base(nullptr) {}

    
    size_t Length() const
    {
        JNIEnv* const env = GetJNIForThread();
        return env->GetStringLength(Get());
    }

    
    operator nsString() const
    {
        MOZ_ASSERT(Object::mInstance);

        JNIEnv* const env = GetJNIForThread();
        const jchar* const str = env->GetStringChars(Get(), nullptr);
        const jsize len = env->GetStringLength(Get());

        nsString result(reinterpret_cast<const char16_t*>(str), len);
        env->ReleaseStringChars(Get(), str);
        return result;
    }

    
    operator nsCString() const
    {
        return NS_ConvertUTF16toUTF8(operator nsString());
    }
};




class ParamImpl<String>::Type : public Ref<String>
{
private:
    
    JNIEnv* const mEnv;

    static jstring GetString(JNIEnv* env, const nsAString& str)
    {
        const jstring result = env->NewString(
                reinterpret_cast<const jchar*>(str.BeginReading()),
                str.Length());
        HandleUncaughtException(env);
        return result;
    }

public:
    MOZ_IMPLICIT Type(const String::Ref& ref)
        : Ref<String>(ref.Get())
        , mEnv(nullptr)
    {}

    MOZ_IMPLICIT Type(const nsAString& str, JNIEnv* env = GetJNIForThread())
        : Ref<String>(GetString(env, str))
        , mEnv(env)
    {}

    MOZ_IMPLICIT Type(const char16_t* str, JNIEnv* env = GetJNIForThread())
        : Ref<String>(GetString(env, nsDependentString(str)))
        , mEnv(env)
    {}

    MOZ_IMPLICIT Type(const nsACString& str, JNIEnv* env = GetJNIForThread())
        : Ref<String>(GetString(env, NS_ConvertUTF8toUTF16(str)))
        , mEnv(env)
    {}

    MOZ_IMPLICIT Type(const char* str, JNIEnv* env = GetJNIForThread())
        : Ref<String>(GetString(env, NS_ConvertUTF8toUTF16(str)))
        , mEnv(env)
    {}

    ~Type()
    {
        if (mEnv) {
            mEnv->DeleteLocalRef(Get());
        }
    }
};






template<class Cls>
class ReturnToLocal
{
private:
    LocalRef<Cls>* const localRef;
    LocalRef<Object> objRef;

public:
    explicit ReturnToLocal(LocalRef<Cls>* ref) : localRef(ref) {}
    operator LocalRef<Object>*() { return &objRef; }

    ~ReturnToLocal()
    {
        if (objRef) {
            *localRef = mozilla::Move(objRef);
        }
    }
};

template<class Cls>
ReturnToLocal<Cls> ReturnTo(LocalRef<Cls>* ref)
{
    return ReturnToLocal<Cls>(ref);
}






template<class Cls>
class ReturnToGlobal
{
private:
    GlobalRef<Cls>* const globalRef;
    LocalRef<Object> objRef;
    LocalRef<Cls> clsRef;

public:
    explicit ReturnToGlobal(GlobalRef<Cls>* ref) : globalRef(ref) {}
    operator LocalRef<Object>*() { return &objRef; }
    operator LocalRef<Cls>*() { return &clsRef; }

    ~ReturnToGlobal()
    {
        if (objRef) {
            *globalRef = (clsRef = mozilla::Move(objRef));
        } else if (clsRef) {
            *globalRef = clsRef;
        }
    }
};

template<class Cls>
ReturnToGlobal<Cls> ReturnTo(GlobalRef<Cls>* ref)
{
    return ReturnToGlobal<Cls>(ref);
}

} 
} 

#endif 
