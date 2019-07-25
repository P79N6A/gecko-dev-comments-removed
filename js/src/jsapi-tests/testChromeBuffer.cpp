



#include "tests.h"

JSPrincipals system_principals = {
    1
};

JSClass global_class = {
    "global",
    JSCLASS_IS_GLOBAL | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

JSObject *trusted_glob = NULL;
JSObject *trusted_fun = NULL;

JSBool
CallTrusted(JSContext *cx, unsigned argc, jsval *vp)
{
    if (!JS_SaveFrameChain(cx))
        return JS_FALSE;

    JSBool ok = JS_FALSE;
    {
        JSAutoEnterCompartment ac;
        ok = ac.enter(cx, trusted_glob);
        if (!ok)
            goto out;
        ok = JS_CallFunctionValue(cx, NULL, OBJECT_TO_JSVAL(trusted_fun),
                                  0, NULL, vp);
    }
  out:
    JS_RestoreFrameChain(cx);
    return ok;
}

BEGIN_TEST(testChromeBuffer)
{
    JS_SetTrustedPrincipals(rt, &system_principals);

    trusted_glob = JS_NewGlobalObject(cx, &global_class, &system_principals);
    CHECK(trusted_glob);

    if (!JS_AddNamedObjectRoot(cx, &trusted_glob, "trusted-global"))
        return false;
    if (!JS_AddNamedObjectRoot(cx, &trusted_fun, "trusted-function"))
        return false;

    JSFunction *fun;

    




    {
        {
            JSAutoEnterCompartment ac;
            CHECK(ac.enter(cx, trusted_glob));
            const char *paramName = "x";
            const char *bytes = "return x ? 1 + trusted(x-1) : 0";
            JS::HandleObject global = JS::HandleObject::fromMarkedLocation(&trusted_glob);
            CHECK(fun = JS_CompileFunctionForPrincipals(cx, global, &system_principals,
                                                        "trusted", 1, &paramName, bytes, strlen(bytes),
                                                        "", 0));
            trusted_fun = JS_GetFunctionObject(fun);
        }

        jsval v = OBJECT_TO_JSVAL(trusted_fun);
        CHECK(JS_WrapValue(cx, &v));

        const char *paramName = "trusted";
        const char *bytes = "try {                                      "
                            "  return untrusted(trusted);               "
                            "} catch (e) {                              "
                            "  return trusted(100);                     "
                            "}                                          ";
        CHECK(fun = JS_CompileFunction(cx, global, "untrusted", 1, &paramName,
                                       bytes, strlen(bytes), "", 0));

        jsval rval;
        CHECK(JS_CallFunction(cx, NULL, fun, 1, &v, &rval));
        CHECK(JSVAL_TO_INT(rval) == 100);
    }

    



    {
        {
            JSAutoEnterCompartment ac;
            CHECK(ac.enter(cx, trusted_glob));
            const char *paramName = "untrusted";
            const char *bytes = "try {                                  "
                                "  untrusted();                         "
                                "} catch (e) {                          "
                                "  return 'From trusted: ' + e;         "
                                "}                                      ";
            JS::HandleObject global = JS::HandleObject::fromMarkedLocation(&trusted_glob);
            CHECK(fun = JS_CompileFunctionForPrincipals(cx, global, &system_principals,
                                                        "trusted", 1, &paramName, bytes, strlen(bytes),
                                                        "", 0));
            trusted_fun = JS_GetFunctionObject(fun);
        }

        jsval v = OBJECT_TO_JSVAL(trusted_fun);
        CHECK(JS_WrapValue(cx, &v));

        const char *paramName = "trusted";
        const char *bytes = "try {                                      "
                            "  return untrusted(trusted);               "
                            "} catch (e) {                              "
                            "  return trusted(untrusted);               "
                            "}                                          ";
        CHECK(fun = JS_CompileFunction(cx, global, "untrusted", 1, &paramName,
                                       bytes, strlen(bytes), "", 0));

        jsval rval;
        CHECK(JS_CallFunction(cx, NULL, fun, 1, &v, &rval));
        JSBool match;
        CHECK(JS_StringEqualsAscii(cx, JSVAL_TO_STRING(rval), "From trusted: InternalError: too much recursion", &match));
        CHECK(match);
    }

    



    {
        {
            JSAutoEnterCompartment ac;
            CHECK(ac.enter(cx, trusted_glob));
            const char *bytes = "return 42";
            JS::HandleObject global = JS::HandleObject::fromMarkedLocation(&trusted_glob);
            CHECK(fun = JS_CompileFunctionForPrincipals(cx, global, &system_principals,
                                                        "trusted", 0, NULL, bytes, strlen(bytes),
                                                        "", 0));
            trusted_fun = JS_GetFunctionObject(fun);
        }

        JSFunction *fun = JS_NewFunction(cx, CallTrusted, 0, 0, global, "callTrusted");
        JS::Anchor<JSObject *> callTrusted(JS_GetFunctionObject(fun));

        const char *paramName = "f";
        const char *bytes = "try {                                      "
                            "  return untrusted(trusted);               "
                            "} catch (e) {                              "
                            "  return f();                              "
                            "}                                          ";
        CHECK(fun = JS_CompileFunction(cx, global, "untrusted", 1, &paramName,
                                       bytes, strlen(bytes), "", 0));

        jsval arg = OBJECT_TO_JSVAL(callTrusted.get());
        jsval rval;
        CHECK(JS_CallFunction(cx, NULL, fun, 1, &arg, &rval));
        CHECK(JSVAL_TO_INT(rval) == 42);
    }

    return true;
}
virtual void uninit() {
    JS_RemoveObjectRoot(cx, &trusted_glob);
    JS_RemoveObjectRoot(cx, &trusted_fun);
    JSAPITest::uninit();
}
END_TEST(testChromeBuffer)
