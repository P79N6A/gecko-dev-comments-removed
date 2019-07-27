






#include "gc/Zone.h"

#include "jsapi-tests/tests.h"

JSObject* keyDelegate = nullptr;

BEGIN_TEST(testWeakMap_basicOperations)
{
    JS::RootedObject map(cx, JS::NewWeakMapObject(cx));
    CHECK(IsWeakMapObject(map));

    JS::RootedObject key(cx, newKey());
    CHECK(key);
    CHECK(!IsWeakMapObject(key));

    JS::RootedValue r(cx);
    CHECK(GetWeakMapEntry(cx, map, key, &r));
    CHECK(r.isUndefined());

    CHECK(checkSize(map, 0));

    JS::RootedValue val(cx, JS::Int32Value(1));
    CHECK(SetWeakMapEntry(cx, map, key, val));

    CHECK(GetWeakMapEntry(cx, map, key, &r));
    CHECK(r == val);
    CHECK(checkSize(map, 1));

    JS_GC(rt);

    CHECK(GetWeakMapEntry(cx, map, key, &r));
    CHECK(r == val);
    CHECK(checkSize(map, 1));

    key = nullptr;
    JS_GC(rt);

    CHECK(checkSize(map, 0));

    return true;
}

JSObject* newKey()
{
    return JS_NewPlainObject(cx);
}

bool
checkSize(JS::HandleObject map, uint32_t expected)
{
    JS::RootedObject keys(cx);
    CHECK(JS_NondeterministicGetWeakMapKeys(cx, map, &keys));

    uint32_t length;
    CHECK(JS_GetArrayLength(cx, keys, &length));
    CHECK(length == expected);

    return true;
}
END_TEST(testWeakMap_basicOperations)

BEGIN_TEST(testWeakMap_keyDelegates)
{
    JS_SetGCParameter(rt, JSGC_MODE, JSGC_MODE_INCREMENTAL);
    JS_GC(rt);

    JS::RootedObject map(cx, JS::NewWeakMapObject(cx));
    CHECK(map);

    JS::RootedObject key(cx, newKey());
    CHECK(key);

    JS::RootedObject delegate(cx, newDelegate());
    CHECK(delegate);
    keyDelegate = delegate;

    



    CHECK(newCCW(map, delegate));
    js::SliceBudget budget(js::WorkBudget(1000000));
    rt->gc.startDebugGC(GC_NORMAL, budget);
    CHECK(!JS::IsIncrementalGCInProgress(rt));
#ifdef DEBUG
    CHECK(map->zone()->lastZoneGroupIndex() < delegate->zone()->lastZoneGroupIndex());
#endif

    
    JS::RootedValue val(cx, JS::Int32Value(1));
    CHECK(SetWeakMapEntry(cx, map, key, val));
    CHECK(checkSize(map, 1));

    
    key = nullptr;
    CHECK(newCCW(map, delegate));
    budget = js::SliceBudget(js::WorkBudget(100000));
    rt->gc.startDebugGC(GC_NORMAL, budget);
    CHECK(!JS::IsIncrementalGCInProgress(rt));
    CHECK(checkSize(map, 1));

    



#ifdef DEBUG
    CHECK(map->zone()->lastZoneGroupIndex() == delegate->zone()->lastZoneGroupIndex());
#endif

    
    delegate = nullptr;
    keyDelegate = nullptr;
    JS_GC(rt);
    CHECK(checkSize(map, 0));

    return true;
}

static void DelegateObjectMoved(JSObject* obj, const JSObject* old)
{
    if (!keyDelegate)
        return;  

    MOZ_RELEASE_ASSERT(keyDelegate == old);
    keyDelegate = obj;
}

static JSObject* GetKeyDelegate(JSObject* obj)
{
    return keyDelegate;
}

JSObject* newKey()
{
    static const js::Class keyClass = {
        "keyWithDelgate",
        JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        JS_NULL_CLASS_SPEC,
        {
            nullptr,
            nullptr,
            false,
            GetKeyDelegate
        },
        JS_NULL_OBJECT_OPS
    };

    JS::RootedObject key(cx, JS_NewObject(cx, Jsvalify(&keyClass)));
    if (!key)
        return nullptr;

    return key;
}

JSObject* newCCW(JS::HandleObject sourceZone, JS::HandleObject destZone)
{
    




    JS::RootedObject object(cx);
    {
        JSAutoCompartment ac(cx, destZone);
        object = JS_NewPlainObject(cx);
        if (!object)
            return nullptr;
    }
    {
        JSAutoCompartment ac(cx, sourceZone);
        if (!JS_WrapObject(cx, &object))
            return nullptr;
    }
    return object;
}

JSObject* newDelegate()
{
    static const js::Class delegateClass = {
        "delegate",
        JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_RESERVED_SLOTS(1),
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        nullptr, 
        JS_GlobalObjectTraceHook,
        JS_NULL_CLASS_SPEC,
        {
            nullptr,
            nullptr,
            false,
            nullptr,
            DelegateObjectMoved
        },
        JS_NULL_OBJECT_OPS
    };

    
    JS::CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    JS::RootedObject global(cx);
    global = JS_NewGlobalObject(cx, Jsvalify(&delegateClass), nullptr, JS::FireOnNewGlobalHook,
                                options);
    JS_SetReservedSlot(global, 0, JS::Int32Value(42));

    return global;
}

bool
checkSize(JS::HandleObject map, uint32_t expected)
{
    JS::RootedObject keys(cx);
    CHECK(JS_NondeterministicGetWeakMapKeys(cx, map, &keys));

    uint32_t length;
    CHECK(JS_GetArrayLength(cx, keys, &length));
    CHECK(length == expected);

    return true;
}
END_TEST(testWeakMap_keyDelegates)
