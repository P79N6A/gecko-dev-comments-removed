





#include "jsfriendapi.h"
#include "jsscript.h"
#include "jsstr.h"

#include "jsapi-tests/tests.h"

#include "jsscriptinlines.h"

using mozilla::UniquePtr;

static JSScript*
FreezeThaw(JSContext* cx, JS::HandleScript script)
{
    
    uint32_t nbytes;
    void* memory = JS_EncodeScript(cx, script, &nbytes);
    if (!memory)
        return nullptr;

    
    JSScript* script2 = JS_DecodeScript(cx, memory, nbytes);
    js_free(memory);
    return script2;
}

enum TestCase {
    TEST_FIRST,
    TEST_SCRIPT = TEST_FIRST,
    TEST_FUNCTION,
    TEST_SERIALIZED_FUNCTION,
    TEST_END
};

BEGIN_TEST(testXDR_bug506491)
{
    const char* s =
        "function makeClosure(s, name, value) {\n"
        "    eval(s);\n"
        "    Math.sin(value);\n"
        "    let n = name, v = value;\n"
        "    return function () { return String(v); };\n"
        "}\n"
        "var f = makeClosure('0;', 'status', 'ok');\n";

    
    JS::CompileOptions options(cx);
    options.setFileAndLine(__FILE__, __LINE__);
    JS::RootedScript script(cx);
    CHECK(JS_CompileScript(cx, s, strlen(s), options, &script));
    CHECK(script);

    script = FreezeThaw(cx, script);
    CHECK(script);

    
    JS::RootedValue v2(cx);
    CHECK(JS_ExecuteScript(cx, script, &v2));

    
    JS_GC(rt);

    
    EVAL("f() === 'ok';\n", &v2);
    JS::RootedValue trueval(cx, JS::TrueValue());
    CHECK_SAME(v2, trueval);
    return true;
}
END_TEST(testXDR_bug506491)

BEGIN_TEST(testXDR_bug516827)
{
    
    JS::CompileOptions options(cx);
    options.setFileAndLine(__FILE__, __LINE__);
    JS::RootedScript script(cx);
    CHECK(JS_CompileScript(cx, "", 0, options, &script));
    CHECK(script);

    script = FreezeThaw(cx, script);
    CHECK(script);

    
    CHECK(JS_ExecuteScript(cx, script));
    return true;
}
END_TEST(testXDR_bug516827)

BEGIN_TEST(testXDR_source)
{
    const char* samples[] = {
        
        "function f(x) { return x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x }",
        "short",
        nullptr
    };
    for (const char** s = samples; *s; s++) {
        JS::CompileOptions options(cx);
        options.setFileAndLine(__FILE__, __LINE__);
        JS::RootedScript script(cx);
        CHECK(JS_CompileScript(cx, *s, strlen(*s), options, &script));
        CHECK(script);
        script = FreezeThaw(cx, script);
        CHECK(script);
        JSString* out = JS_DecompileScript(cx, script, "testing", 0);
        CHECK(out);
        bool equal;
        CHECK(JS_StringEqualsAscii(cx, out, *s, &equal));
        CHECK(equal);
    }
    return true;
}
END_TEST(testXDR_source)

BEGIN_TEST(testXDR_sourceMap)
{
    const char* sourceMaps[] = {
        "http://example.com/source-map.json",
        "file:///var/source-map.json",
        nullptr
    };
    JS::RootedScript script(cx);
    for (const char** sm = sourceMaps; *sm; sm++) {
        JS::CompileOptions options(cx);
        options.setFileAndLine(__FILE__, __LINE__);
        CHECK(JS_CompileScript(cx, "", 0, options, &script));
        CHECK(script);

        size_t len = strlen(*sm);
        UniquePtr<char16_t,JS::FreePolicy> expected_wrapper(js::InflateString(cx, *sm, &len));
        char16_t *expected = expected_wrapper.get();
        CHECK(expected);

        
        CHECK(script->scriptSource()->setSourceMapURL(cx, expected));
        script = FreezeThaw(cx, script);
        CHECK(script);
        CHECK(script->scriptSource());
        CHECK(script->scriptSource()->hasSourceMapURL());

        const char16_t* actual = script->scriptSource()->sourceMapURL();
        CHECK(actual);

        while (*expected) {
            CHECK(*actual);
            CHECK(*expected == *actual);
            expected++;
            actual++;
        }
        CHECK(!*actual);
    }
    return true;
}
END_TEST(testXDR_sourceMap)
