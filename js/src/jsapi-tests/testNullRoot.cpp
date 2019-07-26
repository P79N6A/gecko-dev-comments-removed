






#include "jsapi-tests/tests.h"

BEGIN_TEST(testNullRoot)
{
    obj = nullptr;
    CHECK(JS::AddObjectRoot(cx, &obj));

    str = nullptr;
    CHECK(JS::AddStringRoot(cx, &str));

    script = nullptr;
    CHECK(JS::AddNamedScriptRoot(cx, &script, "testNullRoot's script"));

    
    JS_GC(cx->runtime());

    JS::RemoveObjectRoot(cx, &obj);
    JS::RemoveStringRoot(cx, &str);
    JS::RemoveScriptRoot(cx, &script);
    return true;
}

JS::Heap<JSObject *> obj;
JS::Heap<JSString *> str;
JS::Heap<JSScript *> script;
END_TEST(testNullRoot)
