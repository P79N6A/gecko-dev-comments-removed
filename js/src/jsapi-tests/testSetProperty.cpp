






#include "jsapi-tests/tests.h"

BEGIN_TEST(testSetProperty_NativeGetterStubSetter)
{
    JS::RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    CHECK(obj);
    JS::RootedValue vobj(cx, OBJECT_TO_JSVAL(obj));

    CHECK(JS_DefineProperty(cx, global, "globalProp", vobj,
                            JS_PropertyStub, JS_StrictPropertyStub,
                            JSPROP_ENUMERATE));

    CHECK(JS_DefineProperty(cx, obj, "prop", JSVAL_VOID,
                            NativeGet, JS_StrictPropertyStub,
                            JSPROP_SHARED));

    EXEC("'use strict';                                     \n"
         "var error, passed = false;                        \n"
         "try                                               \n"
         "{                                                 \n"
         "  this.globalProp.prop = 42;                      \n"
         "  throw new Error('setting property succeeded!'); \n"
         "}                                                 \n"
         "catch (e)                                         \n"
         "{                                                 \n"
         "  error = e;                                      \n"
         "  if (e instanceof TypeError)                     \n"
         "    passed = true;                                \n"
         "}                                                 \n"
         "                                                  \n"
         "if (!passed)                                      \n"
         "  throw error;                                    \n");

    EXEC("var error, passed = false;                        \n"
         "try                                               \n"
         "{                                                 \n"
         "  this.globalProp.prop = 42;                      \n"
         "  if (this.globalProp.prop === 17)                \n"
         "    passed = true;                                \n"
         "  else                                            \n"
         "    throw new Error('bad value after set!');      \n"
         "}                                                 \n"
         "catch (e)                                         \n"
         "{                                                 \n"
         "  error = e;                                      \n"
         "}                                                 \n"
         "                                                  \n"
         "if (!passed)                                      \n"
         "  throw error;                                    \n");

    return true;
}
static bool
NativeGet(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp)
{
    vp.set(INT_TO_JSVAL(17));
    return true;
}
END_TEST(testSetProperty_NativeGetterStubSetter)

BEGIN_TEST(testSetProperty_InheritedGlobalSetter)
{
    
    
    
    JS_ASSERT(JS_GetClass(global)->resolve == &JS_ResolveStub);

    CHECK(JS_DefineProperty(cx, global, "HOTLOOP", INT_TO_JSVAL(8), nullptr, nullptr, 0));
    EXEC("var n = 0;\n"
         "var global = this;\n"
         "function f() { n++; }\n"
         "Object.defineProperty(Object.prototype, 'x', {set: f});\n"
         "for (var i = 0; i < HOTLOOP; i++)\n"
         "    global.x = i;\n");
    EXEC("if (n != HOTLOOP)\n"
         "    throw 'FAIL';\n");
    return true;
}
END_TEST(testSetProperty_InheritedGlobalSetter)

