






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

struct DynamicContainer : public DynamicBase, public JS::DynamicTraceable
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
