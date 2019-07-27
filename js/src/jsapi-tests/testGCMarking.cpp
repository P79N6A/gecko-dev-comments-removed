






#include "jsapi-tests/tests.h"

class CCWTestTracer : public JS::CallbackTracer {
    void trace(void** thingp, JS::TraceKind kind) {
        numberOfThingsTraced++;

        printf("*thingp         = %p\n", *thingp);
        printf("*expectedThingp = %p\n", *expectedThingp);

        printf("kind         = %d\n", static_cast<int>(kind));
        printf("expectedKind = %d\n", static_cast<int>(expectedKind));

        if (*thingp != *expectedThingp || kind != expectedKind)
            okay = false;
    }

  public:
    bool          okay;
    size_t        numberOfThingsTraced;
    void**        expectedThingp;
    JS::TraceKind expectedKind;

    CCWTestTracer(JSContext* cx, void** expectedThingp, JS::TraceKind expectedKind)
      : JS::CallbackTracer(JS_GetRuntime(cx)),
        okay(true),
        numberOfThingsTraced(0),
        expectedThingp(expectedThingp),
        expectedKind(expectedKind)
    { }
};

BEGIN_TEST(testTracingIncomingCCWs)
{
    

    JS::RootedObject global1(cx, JS::CurrentGlobalOrNull(cx));
    CHECK(global1);
    JS::RootedObject global2(cx, JS_NewGlobalObject(cx, getGlobalClass(), nullptr,
                                                    JS::FireOnNewGlobalHook));
    CHECK(global2);
    CHECK(global1->zone() != global2->zone());

    

    JS::RootedObject obj(cx, JS_NewPlainObject(cx));
    CHECK(obj->zone() == global1->zone());

    JSAutoCompartment ac(cx, global2);
    JS::RootedObject wrapper(cx, obj);
    CHECK(JS_WrapObject(cx, &wrapper));
    JS::RootedValue v(cx, JS::ObjectValue(*wrapper));
    CHECK(JS_SetProperty(cx, global2, "ccw", v));

    

    JS::ZoneSet zones;
    CHECK(zones.init());
    CHECK(zones.put(global1->zone()));

    void* thing = obj.get();
    CCWTestTracer trc(cx, &thing, JS::TraceKind::Object);
    JS_TraceIncomingCCWs(&trc, zones);
    CHECK(trc.numberOfThingsTraced == 1);
    CHECK(trc.okay);

    return true;
}
END_TEST(testTracingIncomingCCWs)
