



#include "tests.h"






BEGIN_TEST(testResolveRecursion)
{
    static JSClass my_resolve_class = {
        "MyResolve",
        JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE,
        
        JS_PropertyStub,       
        JS_PropertyStub,       
        JS_PropertyStub,         
        JS_StrictPropertyStub, 
        JS_EnumerateStub,
        (JSResolveOp) my_resolve,
        JS_ConvertStub,
        JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };
    
    obj1 = JS_NewObject(cx, &my_resolve_class, NULL, NULL);
    CHECK(obj1);
    obj2 = JS_NewObject(cx, &my_resolve_class, NULL, NULL);
    CHECK(obj2);
    CHECK(JS_SetPrivate(cx, obj1, this));
    CHECK(JS_SetPrivate(cx, obj2, this));

    CHECK(JS_DefineProperty(cx, global, "obj1", OBJECT_TO_JSVAL(obj1), NULL, NULL, 0));
    CHECK(JS_DefineProperty(cx, global, "obj2", OBJECT_TO_JSVAL(obj2), NULL, NULL, 0));

    resolveEntryCount = 0;
    resolveExitCount = 0;

    
    jsval v;
    EVAL("obj1.x", &v);
    CHECK_SAME(v, JSVAL_FALSE);
    CHECK_EQUAL(resolveEntryCount, 4);
    CHECK_EQUAL(resolveExitCount, 4);
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
doResolve(JSObject *obj, jsid id, uintN flags, JSObject **objp)
{
    CHECK_EQUAL(resolveExitCount, 0);
    AutoIncrCounters incr(this);
    CHECK_EQUAL(obj, obj1 || obj == obj2);
    
    CHECK(JSID_IS_STRING(id));
    
    JSFlatString *str = JS_FlattenString(cx, JSID_TO_STRING(id));
    CHECK(str);
    jsval v;
    if (JS_FlatStringEqualsAscii(str, "x")) {
        if (obj == obj1) {
            
            CHECK_EQUAL(resolveEntryCount, 1);
            EVAL("obj2.y = true", &v);
            CHECK_SAME(v, JSVAL_TRUE);
            CHECK(JS_DefinePropertyById(cx, obj, id, JSVAL_FALSE, NULL, NULL, 0));
            *objp = obj;
            return true;
        }
        if (obj == obj2) {
            CHECK_EQUAL(resolveEntryCount, 4);
            *objp = NULL;
            return true;
        }
    } else if (JS_FlatStringEqualsAscii(str, "y")) {
        if (obj == obj2) {
            CHECK_EQUAL(resolveEntryCount, 2);
            CHECK(JS_DefinePropertyById(cx, obj, id, JSVAL_NULL, NULL, NULL, 0));
            EVAL("obj1.x", &v);
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj1.y", &v);
            CHECK_SAME(v, JSVAL_ZERO);
            *objp = obj;
            return true;
        }
        if (obj == obj1) {
            CHECK_EQUAL(resolveEntryCount, 3);
            EVAL("obj1.x", &v);
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj1.y", &v);
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj2.y", &v);
            CHECK(JSVAL_IS_NULL(v));
            EVAL("obj2.x", &v);
            CHECK(JSVAL_IS_VOID(v));
            EVAL("obj1.y = 0", &v);
            CHECK_SAME(v, JSVAL_ZERO);
            *objp = obj;
            return true;
        }
    }
    CHECK(false);
    return false;
}

static JSBool
my_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags, JSObject **objp)
{
    return static_cast<cls_testResolveRecursion *>(JS_GetPrivate(cx, obj))->
           doResolve(obj, id, flags, objp);
}

END_TEST(testResolveRecursion)
