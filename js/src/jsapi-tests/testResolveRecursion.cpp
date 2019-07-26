






#include "jsapi-tests/tests.h"






BEGIN_TEST(testResolveRecursion)
{
    static const JSClass my_resolve_class = {
        "MyResolve",
        JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE,

        JS_PropertyStub,       
        JS_DeletePropertyStub, 
        JS_PropertyStub,       
        JS_StrictPropertyStub, 
        JS_EnumerateStub,
        (JSResolveOp) my_resolve,
        JS_ConvertStub
    };

    obj1 = obj2 = nullptr;
    JS_AddObjectRoot(cx, &obj1);
    JS_AddObjectRoot(cx, &obj2);

    obj1 = JS_NewObject(cx, &my_resolve_class, nullptr, nullptr);
    CHECK(obj1);
    obj2 = JS_NewObject(cx, &my_resolve_class, nullptr, nullptr);
    CHECK(obj2);
    JS_SetPrivate(obj1, this);
    JS_SetPrivate(obj2, this);

    CHECK(JS_DefineProperty(cx, global, "obj1", OBJECT_TO_JSVAL(obj1), nullptr, nullptr, 0));
    CHECK(JS_DefineProperty(cx, global, "obj2", OBJECT_TO_JSVAL(obj2), nullptr, nullptr, 0));

    resolveEntryCount = 0;
    resolveExitCount = 0;

    
    JS::RootedValue v(cx);
    EVAL("obj1.x", v.address());
    CHECK_SAME(v, JSVAL_FALSE);
    CHECK_EQUAL(resolveEntryCount, 4);
    CHECK_EQUAL(resolveExitCount, 4);

    JS_RemoveObjectRoot(cx, &obj1);
    JS_RemoveObjectRoot(cx, &obj2);
    return true;
}

JSObject *obj1;
JSObject *obj2;
unsigned resolveEntryCount;
unsigned resolveExitCount;

struct AutoIncrCounters {

    AutoIncrCounters(cls_testResolveRecursion *t) : t(t) {
        t->resolveEntryCount++;
    }

    ~AutoIncrCounters() {
        t->resolveExitCount++;
    }

    cls_testResolveRecursion *t;
};

bool
doResolve(JS::HandleObject obj, JS::HandleId id, unsigned flags, JS::MutableHandleObject objp)
{
    CHECK_EQUAL(resolveExitCount, 0);
    AutoIncrCounters incr(this);
    CHECK_EQUAL(obj, obj1 || obj == obj2);

    CHECK(JSID_IS_STRING(id));

    JSFlatString *str = JS_FlattenString(cx, JSID_TO_STRING(id));
    CHECK(str);
    JS::RootedValue v(cx);
    if (JS_FlatStringEqualsAscii(str, "x")) {
        if (obj == obj1) {
            
            CHECK_EQUAL(resolveEntryCount, 1);
            EVAL("obj2.y = true", v.address());
            CHECK_SAME(v, JSVAL_TRUE);
            CHECK(JS_DefinePropertyById(cx, obj, id, JSVAL_FALSE, nullptr, nullptr, 0));
            objp.set(obj);
            return true;
        }
        if (obj == obj2) {
            CHECK_EQUAL(resolveEntryCount, 4);
            objp.set(nullptr);
            return true;
        }
    } else if (JS_FlatStringEqualsAscii(str, "y")) {
        if (obj == obj2) {
            CHECK_EQUAL(resolveEntryCount, 2);
            CHECK(JS_DefinePropertyById(cx, obj, id, JSVAL_NULL, nullptr, nullptr, 0));
            EVAL("obj1.x", v.address());
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj1.y", v.address());
            CHECK_SAME(v, JSVAL_ZERO);
            objp.set(obj);
            return true;
        }
        if (obj == obj1) {
            CHECK_EQUAL(resolveEntryCount, 3);
            EVAL("obj1.x", v.address());
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj1.y", v.address());
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj2.y", v.address());
            CHECK(JSVAL_IS_NULL(v));
            EVAL("obj2.x", v.address());
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj1.y = 0", v.address());
            CHECK_SAME(v, JSVAL_ZERO);
            objp.set(obj);
            return true;
        }
    }
    CHECK(false);
    return false;
}

static bool
my_resolve(JSContext *cx, JS::HandleObject obj, JS::HandleId id, unsigned flags,
           JS::MutableHandleObject objp)
{
    return static_cast<cls_testResolveRecursion *>(JS_GetPrivate(obj))->
           doResolve(obj, id, flags, objp);
}

END_TEST(testResolveRecursion)
