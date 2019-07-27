



#include "js/UbiNode.h"

#include "jsapi-tests/tests.h"


BEGIN_TEST(test_ubiNodeZone)
{
    RootedObject global1(cx, JS::CurrentGlobalOrNull(cx));
    CHECK(global1);
    CHECK(JS::ubi::Node(global1).zone() == cx->zone());

    RootedObject global2(cx, JS_NewGlobalObject(cx, getGlobalClass(), nullptr,
                                                JS::FireOnNewGlobalHook));
    CHECK(global2);
    CHECK(global1->zone() != global2->zone());
    CHECK(JS::ubi::Node(global2).zone() == global2->zone());
    CHECK(JS::ubi::Node(global2).zone() != global1->zone());

    JS::CompileOptions options(cx);

    
    RootedString string1(cx, JS_NewStringCopyZ(cx, "Simpson's Individual Stringettes!"));
    CHECK(string1);
    RootedScript script1(cx);
    CHECK(JS::Compile(cx, global1, options, "", 0, &script1));

    {
        
        
        JSAutoCompartment ac(cx, global2);

        RootedString string2(cx, JS_NewStringCopyZ(cx, "A million household uses!"));
        CHECK(string2);
        RootedScript script2(cx);
        CHECK(JS::Compile(cx, global2, options, "", 0, &script2));

        CHECK(JS::ubi::Node(string1).zone() == global1->zone());
        CHECK(JS::ubi::Node(script1).zone() == global1->zone());

        CHECK(JS::ubi::Node(string2).zone() == global2->zone());
        CHECK(JS::ubi::Node(script2).zone() == global2->zone());
    }

    return true;
}
END_TEST(test_ubiNodeZone)
