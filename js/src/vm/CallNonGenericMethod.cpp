





#include "js/CallNonGenericMethod.h"

#include "jsfun.h"
#include "jsobj.h"

#include "proxy/Proxy.h"
#include "vm/ProxyObject.h"

using namespace js;

bool
JS::detail::CallMethodIfWrapped(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                CallArgs args)
{
    HandleValue thisv = args.thisv();
    JS_ASSERT(!test(thisv));

    if (thisv.isObject()) {
        JSObject &thisObj = args.thisv().toObject();
        if (thisObj.is<ProxyObject>())
            return Proxy::nativeCall(cx, test, impl, args);
    }

    ReportIncompatible(cx, args);
    return false;
}

