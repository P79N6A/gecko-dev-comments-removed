






#include "gc/Nursery.h"
#include "js/GCAPI.h"
#include "jsapi-tests/tests.h"

static int ranFinalizer = 0;

void
_finalize(js::FreeOp* fop, JSObject* obj)
{
    JS::AutoAssertGCCallback suppress(obj);
    ++ranFinalizer;
}

static const js::Class TenuredClass = {
    "TenuredClass",
    0,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    _finalize,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    JS_NULL_OBJECT_OPS
};

static const js::Class NurseryClass = {
    "NurseryClass",
    JSCLASS_FINALIZE_FROM_NURSERY | JSCLASS_HAS_RESERVED_SLOTS(1),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    _finalize,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    JS_NULL_OBJECT_OPS
};

BEGIN_TEST(testGCNurseryFinalizer)
{
#ifdef JS_GC_ZEAL
    
    
    AutoLeaveZeal nozeal(cx);
#endif 

    JS::RootedObject obj(cx);

    obj = JS_NewObject(cx, Jsvalify(&TenuredClass));
    CHECK(!js::gc::IsInsideNursery(obj));

    
    rt->gc.minorGC(JS::gcreason::EVICT_NURSERY);
    CHECK(ranFinalizer == 0);

    
    obj = JS_NewPlainObject(cx);
    obj = JS_NewPlainObject(cx);
    obj = JS_NewPlainObject(cx);
    CHECK(js::gc::IsInsideNursery(obj));
    obj = nullptr;
    rt->gc.minorGC(JS::gcreason::EVICT_NURSERY);
    CHECK(ranFinalizer == 0);

    
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    CHECK(js::gc::IsInsideNursery(obj));
    obj = nullptr;
    rt->gc.minorGC(JS::gcreason::EVICT_NURSERY);
    CHECK(ranFinalizer == 1);
    ranFinalizer = 0;

    
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    CHECK(js::gc::IsInsideNursery(obj));
    obj = nullptr;
    rt->gc.minorGC(JS::gcreason::EVICT_NURSERY);
    CHECK(ranFinalizer == 3);
    ranFinalizer = 0;

    
    obj = JS_NewPlainObject(cx);
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    obj = JS_NewPlainObject(cx);
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    obj = JS_NewPlainObject(cx);
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    obj = JS_NewPlainObject(cx);
    obj = JS_NewObject(cx, Jsvalify(&NurseryClass));
    obj = JS_NewPlainObject(cx);
    CHECK(js::gc::IsInsideNursery(obj));
    obj = nullptr;
    rt->gc.minorGC(JS::gcreason::EVICT_NURSERY);
    CHECK(ranFinalizer == 4);
    ranFinalizer = 0;

    return true;
}
END_TEST(testGCNurseryFinalizer)
