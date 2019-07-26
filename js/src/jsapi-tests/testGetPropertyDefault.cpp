







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
        

        js::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL));
        CHECK(obj);

        jsval v0 = JSVAL_TRUE;
        CHECK(JS_SetProperty(cx, obj, "here", &v0));

        jsval v1;
        CHECK(JS_GetPropertyDefault(cx, obj, "here", JSVAL_FALSE, &v1));
        CHECK(JSVAL_IS_TRUE(v1));

        jsval v2;
        CHECK(JS_GetPropertyDefault(cx, obj, "nothere", JSVAL_FALSE, &v2));
        CHECK(JSVAL_IS_FALSE(v2));
    }

    {
        

        js::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL));
        CHECK(obj);

        jsid hereid;
        CHECK(stringToId(cx, "here", &hereid));

        jsid nothereid;
        CHECK(stringToId(cx, "nothere", &nothereid));

        jsval v0 = JSVAL_TRUE;
        CHECK(JS_SetPropertyById(cx, obj, hereid, &v0));

        jsval v1;
        CHECK(JS_GetPropertyByIdDefault(cx, obj, hereid, JSVAL_FALSE, &v1));
        CHECK(JSVAL_IS_TRUE(v1));

        jsval v2;
        CHECK(JS_GetPropertyByIdDefault(cx, obj, nothereid, JSVAL_FALSE, &v2));
        CHECK(JSVAL_IS_FALSE(v2));
    }

    return true;
}
END_TEST(testGetPropertyDefault_bug594060)
