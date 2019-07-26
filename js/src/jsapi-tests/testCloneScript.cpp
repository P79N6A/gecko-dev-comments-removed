








#include "js/OldDebugAPI.h"
#include "jsapi-tests/tests.h"

BEGIN_TEST(test_cloneScript)
{
    JS::RootedObject A(cx, createGlobal());
    JS::RootedObject B(cx, createGlobal());

    CHECK(A);
    CHECK(B);

    const char *source =
        "var i = 0;\n"
        "var sum = 0;\n"
        "while (i < 10) {\n"
        "    sum += i;\n"
        "    ++i;\n"
        "}\n"
        "(sum);\n";

    JS::RootedObject obj(cx);

    
    {
        JSAutoCompartment a(cx, A);
        JSFunction *fun;
        JS::CompileOptions options(cx);
        options.setFileAndLine(__FILE__, 1);
        CHECK(fun = JS_CompileFunction(cx, A, "f", 0, nullptr, source,
                                       strlen(source), options));
        CHECK(obj = JS_GetFunctionObject(fun));
    }

    
    {
        JSAutoCompartment b(cx, B);
        CHECK(JS_CloneFunctionObject(cx, obj, B));
    }

    return true;
}
END_TEST(test_cloneScript)

static void
DestroyPrincipals(JSPrincipals *principals)
{
    delete principals;
}

struct Principals : public JSPrincipals
{
  public:
    Principals()
    {
        refcount = 0;
    }
};

class AutoDropPrincipals
{
    JSRuntime *rt;
    JSPrincipals *principals;

  public:
    AutoDropPrincipals(JSRuntime *rt, JSPrincipals *principals)
      : rt(rt), principals(principals)
    {
        JS_HoldPrincipals(principals);
    }

    ~AutoDropPrincipals()
    {
        JS_DropPrincipals(rt, principals);
    }
};

BEGIN_TEST(test_cloneScriptWithPrincipals)
{
    JS_InitDestroyPrincipalsCallback(rt, DestroyPrincipals);

    JSPrincipals *principalsA = new Principals();
    AutoDropPrincipals dropA(rt, principalsA);
    JSPrincipals *principalsB = new Principals();
    AutoDropPrincipals dropB(rt, principalsB);

    JS::RootedObject A(cx, createGlobal(principalsA));
    JS::RootedObject B(cx, createGlobal(principalsB));

    CHECK(A);
    CHECK(B);

    const char *argnames[] = { "arg" };
    const char *source = "return function() { return arg; }";

    JS::RootedObject obj(cx);

    
    {
        JSAutoCompartment a(cx, A);
        JS::CompileOptions options(cx);
        options.setFileAndLine(__FILE__, 1);
        JS::RootedFunction fun(cx, JS_CompileFunction(cx, A, "f",
                mozilla::ArrayLength(argnames), argnames, source,
                strlen(source), options));
        CHECK(fun);

        JSScript *script;
        CHECK(script = JS_GetFunctionScript(cx, fun));

        CHECK(JS_GetScriptPrincipals(script) == principalsA);
        CHECK(obj = JS_GetFunctionObject(fun));
    }

    
    {
        JSAutoCompartment b(cx, B);
        JS::RootedObject cloned(cx);
        CHECK(cloned = JS_CloneFunctionObject(cx, obj, B));

        JS::RootedFunction fun(cx);
        JS::RootedValue clonedValue(cx, JS::ObjectValue(*cloned));
        CHECK(fun = JS_ValueToFunction(cx, clonedValue));

        JSScript *script;
        CHECK(script = JS_GetFunctionScript(cx, fun));

        CHECK(JS_GetScriptPrincipals(script) == principalsB);

        JS::RootedValue v(cx);
        JS::RootedValue arg(cx, JS::Int32Value(1));
        CHECK(JS_CallFunctionValue(cx, B, clonedValue, JS::HandleValueArray(arg), &v));
        CHECK(v.isObject());

        JSObject *funobj = &v.toObject();
        CHECK(JS_ObjectIsFunction(cx, funobj));
        CHECK(fun = JS_ValueToFunction(cx, v));
        CHECK(script = JS_GetFunctionScript(cx, fun));
        CHECK(JS_GetScriptPrincipals(script) == principalsB);
    }

    return true;
}
END_TEST(test_cloneScriptWithPrincipals)
