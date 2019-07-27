






#include "js/RootingAPI.h"
#include "js/TraceableHashTable.h"

#include "jsapi-tests/tests.h"

BEGIN_TEST(testGCExactRooting)
{
    JS::RootedObject rootCx(cx, JS_NewPlainObject(cx));
    JS::RootedObject rootRt(cx->runtime(), JS_NewPlainObject(cx));

    JS_GC(cx->runtime());

    
    JS_DefineProperty(cx, rootCx, "foo", JS::UndefinedHandleValue, 0);
    JS_DefineProperty(cx, rootRt, "foo", JS::UndefinedHandleValue, 0);

    return true;
}
END_TEST(testGCExactRooting)

BEGIN_TEST(testGCSuppressions)
{
    JS::AutoAssertOnGC nogc;
    JS::AutoCheckCannotGC checkgc;
    JS::AutoSuppressGCAnalysis noanalysis;

    JS::AutoAssertOnGC nogcRt(cx->runtime());
    JS::AutoCheckCannotGC checkgcRt(cx->runtime());
    JS::AutoSuppressGCAnalysis noanalysisRt(cx->runtime());

    return true;
}
END_TEST(testGCSuppressions)

struct MyContainer : public JS::StaticTraceable
{
    RelocatablePtrObject obj;
    RelocatablePtrString str;

    MyContainer() : obj(nullptr), str(nullptr) {}
    static void trace(MyContainer* self, JSTracer* trc) {
        if (self->obj)
            js::TraceEdge(trc, &self->obj, "test container");
        if (self->str)
            js::TraceEdge(trc, &self->str, "test container");
    }
};

namespace js {
template <>
struct RootedBase<MyContainer> {
    RelocatablePtrObject& obj() { return static_cast<Rooted<MyContainer>*>(this)->get().obj; }
    RelocatablePtrString& str() { return static_cast<Rooted<MyContainer>*>(this)->get().str; }
};
} 

BEGIN_TEST(testGCRootedStaticStructInternalStackStorage)
{
    JS::Rooted<MyContainer> container(cx);
    container.get().obj = JS_NewObject(cx, nullptr);
    container.get().str = JS_NewStringCopyZ(cx, "Hello");

    JS_GC(cx->runtime());
    JS_GC(cx->runtime());

    JS::RootedObject obj(cx, container.get().obj);
    JS::RootedValue val(cx, StringValue(container.get().str));
    CHECK(JS_SetProperty(cx, obj, "foo", val));
    return true;
}
END_TEST(testGCRootedStaticStructInternalStackStorage)

BEGIN_TEST(testGCRootedStaticStructInternalStackStorageAugmented)
{
    JS::Rooted<MyContainer> container(cx);
    container.obj() = JS_NewObject(cx, nullptr);
    container.str() = JS_NewStringCopyZ(cx, "Hello");

    JS_GC(cx->runtime());
    JS_GC(cx->runtime());

    JS::RootedObject obj(cx, container.obj());
    JS::RootedValue val(cx, StringValue(container.str()));
    CHECK(JS_SetProperty(cx, obj, "foo", val));
    return true;
}
END_TEST(testGCRootedStaticStructInternalStackStorageAugmented)

struct DynamicBase : public JS::DynamicTraceable
{
    RelocatablePtrObject obj;
    DynamicBase() : obj(nullptr) {}

    void trace(JSTracer* trc) override {
        if (obj)
            js::TraceEdge(trc, &obj, "test container");
    }
};

struct DynamicContainer : public DynamicBase
{
    RelocatablePtrString str;
    DynamicContainer() : str(nullptr) {}

    void trace(JSTracer* trc) override {
        this->DynamicBase::trace(trc);
        if (str)
            js::TraceEdge(trc, &str, "test container");
    }
};

namespace js {
template <>
struct RootedBase<DynamicContainer> {
    RelocatablePtrObject& obj() { return static_cast<Rooted<DynamicContainer>*>(this)->get().obj; }
    RelocatablePtrString& str() { return static_cast<Rooted<DynamicContainer>*>(this)->get().str; }
};
} 

BEGIN_TEST(testGCRootedDynamicStructInternalStackStorage)
{
    JS::Rooted<DynamicContainer> container(cx);
    container.get().obj = JS_NewObject(cx, nullptr);
    container.get().str = JS_NewStringCopyZ(cx, "Hello");

    JS_GC(cx->runtime());
    JS_GC(cx->runtime());

    JS::RootedObject obj(cx, container.get().obj);
    JS::RootedValue val(cx, StringValue(container.get().str));
    CHECK(JS_SetProperty(cx, obj, "foo", val));
    return true;
}
END_TEST(testGCRootedDynamicStructInternalStackStorage)

BEGIN_TEST(testGCRootedDynamicStructInternalStackStorageAugmented)
{
    JS::Rooted<DynamicContainer> container(cx);
    container.obj() = JS_NewObject(cx, nullptr);
    container.str() = JS_NewStringCopyZ(cx, "Hello");

    JS_GC(cx->runtime());
    JS_GC(cx->runtime());

    JS::RootedObject obj(cx, container.obj());
    JS::RootedValue val(cx, StringValue(container.str()));
    CHECK(JS_SetProperty(cx, obj, "foo", val));
    return true;
}
END_TEST(testGCRootedDynamicStructInternalStackStorageAugmented)

using MyHashMap = js::TraceableHashMap<js::Shape*, JSObject*>;

BEGIN_TEST(testGCRootedHashMap)
{
    JS::Rooted<MyHashMap> map(cx, MyHashMap(cx));
    CHECK(map.init(15));
    CHECK(map.initialized());

    for (size_t i = 0; i < 10; ++i) {
        RootedObject obj(cx, JS_NewObject(cx, nullptr));
        RootedValue val(cx, UndefinedValue());
        char buffer[2];
        buffer[0] = 'a' + i;
        buffer[1] = '\0';
        CHECK(JS_SetProperty(cx, obj, buffer, val));
        CHECK(map.putNew(obj->as<NativeObject>().lastProperty(), obj));
    }

    JS_GC(rt);
    JS_GC(rt);

    for (auto r = map.all(); !r.empty(); r.popFront()) {
        RootedObject obj(cx, r.front().value());
        CHECK(obj->as<NativeObject>().lastProperty() == r.front().key());
    }

    return true;
}
END_TEST(testGCRootedHashMap)

static bool
FillMyHashMap(JSContext* cx, MutableHandle<MyHashMap> map)
{
    for (size_t i = 0; i < 10; ++i) {
        RootedObject obj(cx, JS_NewObject(cx, nullptr));
        RootedValue val(cx, UndefinedValue());
        char buffer[2];
        buffer[0] = 'a' + i;
        buffer[1] = '\0';
        if (!JS_SetProperty(cx, obj, buffer, val))
            return false;
        if (!map.putNew(obj->as<NativeObject>().lastProperty(), obj))
            return false;
    }
    return true;
}

static bool
CheckMyHashMap(JSContext* cx, Handle<MyHashMap> map)
{
    for (auto r = map.all(); !r.empty(); r.popFront()) {
        RootedObject obj(cx, r.front().value());
        if (obj->as<NativeObject>().lastProperty() != r.front().key())
            return false;
    }
    return true;
}

BEGIN_TEST(testGCHandleHashMap)
{
    JS::Rooted<MyHashMap> map(cx, MyHashMap(cx));
    CHECK(map.init(15));
    CHECK(map.initialized());

    CHECK(FillMyHashMap(cx, &map));

    JS_GC(rt);
    JS_GC(rt);

    CHECK(CheckMyHashMap(cx, map));

    return true;
}
END_TEST(testGCHandleHashMap)
