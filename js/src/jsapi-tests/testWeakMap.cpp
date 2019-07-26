






#include "gc/Zone.h"

#include "jsapi-tests/tests.h"

using namespace JS;

#ifdef JSGC_USE_EXACT_ROOTING

BEGIN_TEST(testWeakMap_basicOperations)
{
    RootedObject map(cx, NewWeakMapObject(cx));
    CHECK(IsWeakMapObject(map));

    RootedObject key(cx, newKey());
    CHECK(key);
    CHECK(!IsWeakMapObject(key));

    RootedValue r(cx);
    CHECK(GetWeakMapEntry(cx, map, key, &r));
    CHECK(r.isUndefined());

    CHECK(checkSize(map, 0));

    RootedValue val(cx, Int32Value(1));
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

JSObject *newKey()
{
    return JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr());
}

bool
checkSize(HandleObject map, uint32_t expected)
{
    RootedObject keys(cx);
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

    RootedObject map(cx, NewWeakMapObject(cx));
    CHECK(map);

    RootedObject key(cx, newKey());
    CHECK(key);

    RootedObject delegate(cx, newDelegate());
    CHECK(delegate);

    SetKeyDelegate(key, delegate);

    



    CHECK(newCCW(map, delegate));
    GCDebugSlice(rt, true, 1000000);
#ifdef DEBUG
    CHECK(map->zone()->lastZoneGroupIndex() < delegate->zone()->lastZoneGroupIndex());
#endif

    
    RootedValue val(cx, Int32Value(1));
    CHECK(SetWeakMapEntry(cx, map, key, val));
    CHECK(checkSize(map, 1));

    
    key = nullptr;
    CHECK(newCCW(map, delegate));
    GCDebugSlice(rt, true, 100000);
    CHECK(checkSize(map, 1));

    



#ifdef DEBUG
    CHECK(map->zone()->lastZoneGroupIndex() == delegate->zone()->lastZoneGroupIndex());
#endif

    
    delegate = nullptr;
    JS_GC(rt);
    CHECK(checkSize(map, 0));

    return true;
}

static void SetKeyDelegate(JSObject *key, JSObject *delegate)
{
    JS_SetPrivate(key, delegate);
}

static JSObject *GetKeyDelegate(JSObject *obj)
{
    return static_cast<JSObject*>(JS_GetPrivate(obj));
}

JSObject *newKey()
{
    static const js::Class keyClass = {
        "keyWithDelgate",
        JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
        JS_PropertyStub,         
        JS_DeletePropertyStub,   
        JS_PropertyStub,         
        JS_StrictPropertyStub,   
        JS_EnumerateStub,
        JS_ResolveStub,
        JS_ConvertStub,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        JS_NULL_CLASS_SPEC,
        {
            nullptr,
            nullptr,
            nullptr,
            false,
            GetKeyDelegate
        },
        JS_NULL_OBJECT_OPS
    };

    RootedObject key(cx);
    key = JS_NewObject(cx,
                       reinterpret_cast<const JSClass *>(&keyClass),
                       JS::NullPtr(),
                       JS::NullPtr());
    if (!key)
        return nullptr;

    SetKeyDelegate(key, nullptr);

    return key;
}

JSObject *newCCW(HandleObject sourceZone, HandleObject destZone)
{
    




    RootedObject object(cx);
    {
        JSAutoCompartment ac(cx, destZone);
        object = JS_NewObject(cx, nullptr, NullPtr(), NullPtr());
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

JSObject *newDelegate()
{
    static const JSClass delegateClass = {
        "delegate",
        JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_RESERVED_SLOTS(1),
        JS_PropertyStub,
        JS_DeletePropertyStub,
        JS_PropertyStub,
        JS_StrictPropertyStub,
        JS_EnumerateStub,
        JS_ResolveStub,
        JS_ConvertStub,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        JS_GlobalObjectTraceHook
    };

    
    JS::CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    JS::RootedObject global(cx);
    global = JS_NewGlobalObject(cx, &delegateClass, nullptr, JS::FireOnNewGlobalHook, options);
    JS_SetReservedSlot(global, 0, Int32Value(42));

    



    JS_GC(rt);

    return global;
}

bool
checkSize(HandleObject map, uint32_t expected)
{
    RootedObject keys(cx);
    CHECK(JS_NondeterministicGetWeakMapKeys(cx, map, &keys));

    uint32_t length;
    CHECK(JS_GetArrayLength(cx, keys, &length));
    CHECK(length == expected);

    return true;
}
END_TEST(testWeakMap_keyDelegates)

#endif 
