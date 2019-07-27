






#include "jsapi-tests/tests.h"

static bool
GlobalEnumerate(JSContext* cx, JS::Handle<JSObject*> obj)
{
    return JS_EnumerateStandardClasses(cx, obj);
}

static bool
GlobalResolve(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolvedp)
{
    return JS_ResolveStandardClass(cx, obj, id, resolvedp);
}

BEGIN_TEST(testRedefineGlobalEval)
{
    static const JSClass cls = {
        "global", JSCLASS_GLOBAL_FLAGS,
        nullptr, nullptr, nullptr, nullptr,
        GlobalEnumerate, GlobalResolve, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr,
        JS_GlobalObjectTraceHook
    };

    
    JS::CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    JS::Rooted<JSObject*> g(cx, JS_NewGlobalObject(cx, &cls, nullptr, JS::FireOnNewGlobalHook, options));
    if (!g)
        return false;

    JSAutoCompartment ac(cx, g);
    JS::Rooted<JS::Value> v(cx);
    CHECK(JS_GetProperty(cx, g, "Object", &v));

    static const char data[] = "Object.defineProperty(this, 'eval', { configurable: false });";
    JS::CompileOptions opts(cx);
    CHECK(JS::Evaluate(cx, opts.setFileAndLine(__FILE__, __LINE__),
                       data, mozilla::ArrayLength(data) - 1, &v));

    return true;
}
END_TEST(testRedefineGlobalEval)
