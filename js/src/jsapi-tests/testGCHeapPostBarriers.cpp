






#include "js/RootingAPI.h"
#include "jsapi-tests/tests.h"

using mozilla::ScopedDeletePtr;

BEGIN_TEST(testGCHeapPostBarriers)
{
#ifdef JS_GC_ZEAL
    AutoLeaveZeal nozeal(cx);
#endif 

    
    JS_GC(cx->runtime());
    JS::RootedObject obj(cx, NurseryObject());
    CHECK(js::gc::IsInsideNursery(obj.get()));
    JS_GC(cx->runtime());
    CHECK(!js::gc::IsInsideNursery(obj.get()));
    JS::RootedObject tenuredObject(cx, obj);

    
    CHECK(TestHeapPostBarriers(NurseryObject()));
    CHECK(TestHeapPostBarriers(NurseryFunction()));

    return true;
}

MOZ_NEVER_INLINE bool
Passthrough(bool value)
{
    
    return value;
}

template <typename T>
bool
TestHeapPostBarriers(T initialObj)
{
    CHECK(initialObj != nullptr);
    CHECK(js::gc::IsInsideNursery(initialObj));

    
    ScopedDeletePtr<JS::Heap<T>> heapData(new JS::Heap<T>);
    CHECK(heapData.get());
    CHECK(Passthrough(heapData->get() == nullptr));
    *heapData = initialObj;

    
    uintptr_t initialObjAsInt = uintptr_t(initialObj);

    
    cx->minorGC(JS::gcreason::API);
    CHECK(uintptr_t(heapData->get()) != initialObjAsInt);
    CHECK(!js::gc::IsInsideNursery(heapData->get()));

    
    JS::Rooted<T> obj(cx, heapData->get());
    JS::RootedValue value(cx);
    CHECK(JS_GetProperty(cx, obj, "x", &value));
    CHECK(value.isInt32());
    CHECK(value.toInt32() == 42);

    return true;
}

JSObject* NurseryObject()
{
    JS::RootedObject obj(cx, JS_NewPlainObject(cx));
    if (!obj)
        return nullptr;
    JS_DefineProperty(cx, obj, "x", 42, 0);
    return obj;
}

JSFunction* NurseryFunction()
{
    



    return static_cast<JSFunction*>(NurseryObject());
}

END_TEST(testGCHeapPostBarriers)
