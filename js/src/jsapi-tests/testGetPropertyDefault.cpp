







#include "tests.h"

#define JSVAL_IS_FALSE(x) ((JSVAL_IS_BOOLEAN(x)) && !(JSVAL_TO_BOOLEAN(x)))
#define JSVAL_IS_TRUE(x)  ((JSVAL_IS_BOOLEAN(x)) && (JSVAL_TO_BOOLEAN(x)))

static JSBool
stringToId(JSContext *cx, const char *s, jsid *idp)
{
    JSString *str = JS_NewStringCopyZ(cx, s);
    if (!str)
        return false;

    return JS_ValueToId(cx, STRING_TO_JSVAL(str), idp);
}

BEGIN_TEST(testGetPropertyDefault_bug594060)
{
    {
        

        JS::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL));
        CHECK(obj);

        JS::RootedValue v0(cx, JSVAL_TRUE);
        CHECK(JS_SetProperty(cx, obj, "here", v0.address()));

        JS::RootedValue v1(cx);
        CHECK(JS_GetPropertyDefault(cx, obj, "here", JSVAL_FALSE, v1.address()));
        CHECK(JSVAL_IS_TRUE(v1));

        JS::RootedValue v2(cx);
        CHECK(JS_GetPropertyDefault(cx, obj, "nothere", JSVAL_FALSE, v2.address()));
        CHECK(JSVAL_IS_FALSE(v2));
    }

    {
        

        JS::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL));
        CHECK(obj);

        jsid hereid;
        CHECK(stringToId(cx, "here", &hereid));

        jsid nothereid;
        CHECK(stringToId(cx, "nothere", &nothereid));

        JS::RootedValue v0(cx, JSVAL_TRUE);
        CHECK(JS_SetPropertyById(cx, obj, hereid, v0.address()));

        JS::RootedValue v1(cx);
        CHECK(JS_GetPropertyByIdDefault(cx, obj, hereid, JSVAL_FALSE, v1.address()));
        CHECK(JSVAL_IS_TRUE(v1));

        JS::RootedValue v2(cx);
        CHECK(JS_GetPropertyByIdDefault(cx, obj, nothereid, JSVAL_FALSE, v2.address()));
        CHECK(JSVAL_IS_FALSE(v2));
    }

    return true;
}
END_TEST(testGetPropertyDefault_bug594060)
