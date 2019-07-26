






#ifdef JSGC_GENERATIONAL

#include "js/RootingAPI.h"
#include "jsapi-tests/tests.h"

BEGIN_TEST(testGCHeapPostBarriers)
{
    
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

template <typename T>
bool
TestHeapPostBarriers(T initalObj)
{
    CHECK(initalObj != nullptr);
    CHECK(js::gc::IsInsideNursery(initalObj));

    
    JS::Heap<T> *heapData = new JS::Heap<T>();
    CHECK(heapData);
    CHECK(heapData->get() == nullptr);
    heapData->set(initalObj);

    
    js::MinorGC(cx, JS::gcreason::API);
    CHECK(heapData->get() != initalObj);
    CHECK(!js::gc::IsInsideNursery(heapData->get()));

    
    JS::Rooted<T> obj(cx, heapData->get());
    JS::RootedValue value(cx);
    CHECK(JS_GetProperty(cx, obj, "x", &value));
    CHECK(value.isInt32());
    CHECK(value.toInt32() == 42);

    delete heapData;
    return true;
}

JSObject *NurseryObject()
{
    JS::RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    if (!obj)
        return nullptr;
    JS_DefineProperty(cx, obj, "x", 42, 0);
    return obj;
}

JSFunction *NurseryFunction()
{
    



    return static_cast<JSFunction *>(NurseryObject());
}

END_TEST(testGCHeapPostBarriers)

#endif
