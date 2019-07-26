






#ifdef JSGC_GENERATIONAL

#include "gc/Barrier.h"
#include "jsapi-tests/tests.h"

using namespace JS;
using namespace js;

struct AutoIgnoreRootingHazards {
    
    static volatile int depth;
    AutoIgnoreRootingHazards() { depth++; }
    ~AutoIgnoreRootingHazards() { depth--; }
};
volatile int AutoIgnoreRootingHazards::depth = 0;

BEGIN_TEST(testGCStoreBufferRemoval)
{
    
    JS_GC(cx->runtime());
    JS::RootedObject obj(cx, NurseryObject());
    CHECK(js::gc::IsInsideNursery(obj.get()));
    JS_GC(cx->runtime());
    CHECK(!js::gc::IsInsideNursery(obj.get()));
    JS::RootedObject tenuredObject(cx, obj);

    
    AutoIgnoreRootingHazards ignore;

    
    {
        JSObject *badObject = reinterpret_cast<JSObject*>(1);
        JSObject *punnedPtr = nullptr;
        RelocatablePtrObject* relocPtr =
            reinterpret_cast<RelocatablePtrObject*>(&punnedPtr);
        new (relocPtr) RelocatablePtrObject;
        *relocPtr = NurseryObject();
        relocPtr->~RelocatablePtrObject();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (relocPtr) RelocatablePtrObject;
        *relocPtr = NurseryObject();
        *relocPtr = tenuredObject;
        relocPtr->~RelocatablePtrObject();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (relocPtr) RelocatablePtrObject;
        *relocPtr = NurseryObject();
        *relocPtr = nullptr;
        relocPtr->~RelocatablePtrObject();
        punnedPtr = badObject;
        JS_GC(cx->runtime());
    }

    
    {
        Value punnedValue;
        RelocatableValue *relocValue = reinterpret_cast<RelocatableValue*>(&punnedValue);
        new (relocValue) RelocatableValue;
        *relocValue = ObjectValue(*NurseryObject());
        relocValue->~RelocatableValue();
        punnedValue = ObjectValueCrashOnTouch();
        JS_GC(cx->runtime());

        new (relocValue) RelocatableValue;
        *relocValue = ObjectValue(*NurseryObject());
        *relocValue = ObjectValue(*tenuredObject);
        relocValue->~RelocatableValue();
        punnedValue = ObjectValueCrashOnTouch();
        JS_GC(cx->runtime());

        new (relocValue) RelocatableValue;
        *relocValue = ObjectValue(*NurseryObject());
        *relocValue = NullValue();
        relocValue->~RelocatableValue();
        punnedValue = ObjectValueCrashOnTouch();
        JS_GC(cx->runtime());
    }

    
    {
        JSObject *badObject = reinterpret_cast<JSObject*>(1);
        JSObject *punnedPtr = nullptr;
        Heap<JSObject*>* heapPtr =
            reinterpret_cast<Heap<JSObject*>*>(&punnedPtr);
        new (heapPtr) Heap<JSObject*>;
        *heapPtr = NurseryObject();
        heapPtr->~Heap<JSObject*>();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (heapPtr) Heap<JSObject*>;
        *heapPtr = NurseryObject();
        *heapPtr = tenuredObject;
        heapPtr->~Heap<JSObject*>();
        punnedPtr = badObject;
        JS_GC(cx->runtime());

        new (heapPtr) Heap<JSObject*>;
        *heapPtr = NurseryObject();
        *heapPtr = nullptr;
        heapPtr->~Heap<JSObject*>();
        punnedPtr = badObject;
        JS_GC(cx->runtime());
    }

    return true;
}

JSObject *NurseryObject()
{
    return JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr());
}
END_TEST(testGCStoreBufferRemoval)

#endif
