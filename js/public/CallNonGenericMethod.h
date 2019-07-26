





#ifndef js_CallNonGenericMethod_h
#define js_CallNonGenericMethod_h

#include "jstypes.h"

#include "js/CallArgs.h"

namespace JS {


typedef bool (*IsAcceptableThis)(Handle<Value> v);



typedef bool (*NativeImpl)(JSContext *cx, CallArgs args);

namespace detail {


extern JS_PUBLIC_API(bool)
CallMethodIfWrapped(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args);

} 
































































template<IsAcceptableThis Test, NativeImpl Impl>
JS_ALWAYS_INLINE bool
CallNonGenericMethod(JSContext *cx, CallArgs args)
{
    HandleValue thisv = args.thisv();
    if (Test(thisv))
        return Impl(cx, args);

    return detail::CallMethodIfWrapped(cx, Test, Impl, args);
}

JS_ALWAYS_INLINE bool
CallNonGenericMethod(JSContext *cx, IsAcceptableThis Test, NativeImpl Impl, CallArgs args)
{
    HandleValue thisv = args.thisv();
    if (Test(thisv))
        return Impl(cx, args);

    return detail::CallMethodIfWrapped(cx, Test, Impl, args);
}

} 

#endif 
