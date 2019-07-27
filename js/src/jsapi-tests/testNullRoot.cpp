






#include "jsapi-tests/tests.h"

BEGIN_TEST(testNullRoot)
{
    obj.init(cx, nullptr);
    str.init(cx, nullptr);
    script.init(cx, nullptr);

    
    JS_GC(cx->runtime());

    return true;
}

JS::PersistentRootedObject obj;
JS::PersistentRootedString str;
JS::PersistentRootedScript script;
END_TEST(testNullRoot)
