#ifndef mozilla_jni_Natives_h__
#define mozilla_jni_Natives_h__

#include <jni.h>

#include "mozilla/jni/Accessors.h"
#include "mozilla/jni/Refs.h"
#include "mozilla/jni/Types.h"
#include "mozilla/jni/Utils.h"

namespace mozilla {
namespace jni{


template<class Impl>
Impl* GetInstancePtr(JNIEnv* env, jobject instance)
{
    
    return nullptr;
}

namespace detail {










template<bool IsStatic, typename ReturnType,
         class Traits, class Impl, class Args>
class NativeStubImpl;


template<typename ReturnType, class Traits, class Impl, typename... Args>
class NativeStubImpl<false, ReturnType, Traits, Impl, jni::Args<Args...>>
{
    typedef typename Traits::Owner Owner;
    typedef typename TypeAdapter<ReturnType>::JNIType ReturnJNIType;

public:
    
    template<ReturnType (Impl::*Method) (Args...)>
    static ReturnJNIType Wrap(JNIEnv* env, jobject instance,
                              typename TypeAdapter<Args>::JNIType... args)
    {
        Impl* const impl = GetInstancePtr<Impl>(env, instance);
        if (!impl) {
            return ReturnJNIType();
        }
        return TypeAdapter<ReturnType>::FromNative(env,
                (impl->*Method)(TypeAdapter<Args>::ToNative(env, args)...));
    }

    
    template<ReturnType (Impl::*Method) (typename Owner::Param, Args...)>
    static ReturnJNIType Wrap(JNIEnv* env, jobject instance,
                              typename TypeAdapter<Args>::JNIType... args)
    {
        Impl* const impl = GetInstancePtr<Impl>(env, instance);
        if (!impl) {
            return ReturnJNIType();
        }
        return TypeAdapter<ReturnType>::FromNative(env,
                (impl->*Method)(Owner::Ref::From(instance),
                                TypeAdapter<Args>::ToNative(env, args)...));
    }
};


template<class Traits, class Impl, typename... Args>
class NativeStubImpl<false, void, Traits, Impl, jni::Args<Args...>>
{
    typedef typename Traits::Owner Owner;

public:
    
    template<void (Impl::*Method) (Args...)>
    static void Wrap(JNIEnv* env, jobject instance,
                     typename TypeAdapter<Args>::JNIType... args)
    {
        Impl* const impl = GetInstancePtr<Impl>(env, instance);
        if (!impl) {
            return;
        }
        (impl->*Method)(TypeAdapter<Args>::ToNative(env, args)...);
    }

    
    template<void (Impl::*Method) (typename Owner::Param, Args...)>
    static void Wrap(JNIEnv* env, jobject instance,
                     typename TypeAdapter<Args>::JNIType... args)
    {
        Impl* const impl = GetInstancePtr<Impl>(env, instance);
        if (!impl) {
            return;
        }
        (impl->*Method)(Owner::Ref::From(instance),
                        TypeAdapter<Args>::ToNative(env, args)...);
    }
};


template<typename ReturnType, class Traits, class Impl, typename... Args>
class NativeStubImpl<true, ReturnType, Traits, Impl, jni::Args<Args...>>
{
    typedef typename TypeAdapter<ReturnType>::JNIType ReturnJNIType;

public:
    
    template<ReturnType (*Method) (Args...)>
    static ReturnJNIType Wrap(JNIEnv* env, jclass,
                              typename TypeAdapter<Args>::JNIType... args)
    {
        return TypeAdapter<ReturnType>::FromNative(env,
                (*Method)(TypeAdapter<Args>::ToNative(env, args)...));
    }

    
    template<ReturnType (*Method) (ClassObject::Param, Args...)>
    static ReturnJNIType Wrap(JNIEnv* env, jclass cls,
                              typename TypeAdapter<Args>::JNIType... args)
    {
        return TypeAdapter<ReturnType>::FromNative(env,
                (*Method)(ClassObject::Ref::From(cls),
                          TypeAdapter<Args>::ToNative(env, args)...));
    }
};


template<class Traits, class Impl, typename... Args>
class NativeStubImpl<true, void, Traits, Impl, jni::Args<Args...>>
{
public:
    
    template<void (*Method) (Args...)>
    static void Wrap(JNIEnv* env, jclass,
                     typename TypeAdapter<Args>::JNIType... args)
    {
        (*Method)(TypeAdapter<Args>::ToNative(env, args)...);
    }

    
    template<void (*Method) (ClassObject::Param, Args...)>
    static void Wrap(JNIEnv* env, jclass cls,
                     typename TypeAdapter<Args>::JNIType... args)
    {
        (*Method)(ClassObject::Ref::From(cls),
                  TypeAdapter<Args>::ToNative(env, args)...);
    }
};

} 



template<class Traits, class Impl>
struct NativeStub : detail::NativeStubImpl<Traits::isStatic,
                                           typename Traits::ReturnType,
                                           Traits, Impl, typename Traits::Args>
{
};



template<class Traits, typename Ret, typename... Args>
constexpr JNINativeMethod MakeNativeMethod(Ret (*stub)(JNIEnv*, Args...))
{
    return {
        Traits::name,
        Traits::signature,
        reinterpret_cast<void*>(stub)
    };
}


template<class Cls, class Impl>
class NativeImpl
{
    typedef typename Cls::template Natives<Impl> Natives;

    static bool sInited;

public:
    static void Init() {
        if (sInited) {
            return;
        }
        JNIEnv* const env = GetJNIForThread();
        env->RegisterNatives(Accessor::EnsureClassRef<Cls>(env),
                             Natives::methods,
                             sizeof(Natives::methods) / sizeof(JNINativeMethod));
        sInited = true;
    }

    NativeImpl() {
        
        Init();
    }
};


template<class C, class I>
bool NativeImpl<C, I>::sInited;

} 
} 

#endif 
