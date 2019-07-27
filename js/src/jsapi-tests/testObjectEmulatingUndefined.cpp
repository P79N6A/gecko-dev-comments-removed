



#include "jsapi-tests/tests.h"

static const JSClass ObjectEmulatingUndefinedClass = {
    "ObjectEmulatingUndefined",
    JSCLASS_EMULATES_UNDEFINED
};

static bool
ObjectEmulatingUndefinedConstructor(JSContext* cx, unsigned argc, jsval* vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JSObject* obj = JS_NewObjectForConstructor(cx, &ObjectEmulatingUndefinedClass, args);
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

BEGIN_TEST(testObjectEmulatingUndefined_truthy)
{
    CHECK(JS_InitClass(cx, global, nullptr, &ObjectEmulatingUndefinedClass,
                       ObjectEmulatingUndefinedConstructor, 0,
                       nullptr, nullptr, nullptr, nullptr));

    JS::RootedValue result(cx);

    EVAL("if (new ObjectEmulatingUndefined()) true; else false;", &result);
    CHECK(result.isFalse());

    EVAL("if (!new ObjectEmulatingUndefined()) true; else false;", &result);
    CHECK(result.isTrue());

    EVAL("var obj = new ObjectEmulatingUndefined(); \n"
         "var res = []; \n"
         "for (var i = 0; i < 50; i++) \n"
         "  res.push(Boolean(obj)); \n"
         "res.every(function(v) { return v === false; });",
         &result);
    CHECK(result.isTrue());

    return true;
}
END_TEST(testObjectEmulatingUndefined_truthy)

BEGIN_TEST(testObjectEmulatingUndefined_equal)
{
    CHECK(JS_InitClass(cx, global, nullptr, &ObjectEmulatingUndefinedClass,
                       ObjectEmulatingUndefinedConstructor, 0,
                       nullptr, nullptr, nullptr, nullptr));

    JS::RootedValue result(cx);

    EVAL("if (new ObjectEmulatingUndefined() == undefined) true; else false;", &result);
    CHECK(result.isTrue());

    EVAL("if (new ObjectEmulatingUndefined() == null) true; else false;", &result);
    CHECK(result.isTrue());

    EVAL("if (new ObjectEmulatingUndefined() != undefined) true; else false;", &result);
    CHECK(result.isFalse());

    EVAL("if (new ObjectEmulatingUndefined() != null) true; else false;", &result);
    CHECK(result.isFalse());

    EVAL("var obj = new ObjectEmulatingUndefined(); \n"
         "var res = []; \n"
         "for (var i = 0; i < 50; i++) \n"
         "  res.push(obj == undefined); \n"
         "res.every(function(v) { return v === true; });",
         &result);
    CHECK(result.isTrue());

    EVAL("var obj = new ObjectEmulatingUndefined(); \n"
         "var res = []; \n"
         "for (var i = 0; i < 50; i++) \n"
         "  res.push(obj == null); \n"
         "res.every(function(v) { return v === true; });",
         &result);
    CHECK(result.isTrue());

    EVAL("var obj = new ObjectEmulatingUndefined(); \n"
         "var res = []; \n"
         "for (var i = 0; i < 50; i++) \n"
         "  res.push(obj != undefined); \n"
         "res.every(function(v) { return v === false; });",
         &result);
    CHECK(result.isTrue());

    EVAL("var obj = new ObjectEmulatingUndefined(); \n"
         "var res = []; \n"
         "for (var i = 0; i < 50; i++) \n"
         "  res.push(obj != null); \n"
         "res.every(function(v) { return v === false; });",
         &result);
    CHECK(result.isTrue());

    return true;
}
END_TEST(testObjectEmulatingUndefined_equal)
