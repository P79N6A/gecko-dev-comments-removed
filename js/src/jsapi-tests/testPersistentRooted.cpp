



#include "js/Class.h"
#include "jsapi-tests/tests.h"

using namespace JS;

struct BarkWhenTracedClass {
    static int finalizeCount;
    static int traceCount;

    static const JSClass class_;
    static void finalize(JSFreeOp* fop, JSObject* obj) { finalizeCount++; }
    static void trace(JSTracer* trc, JSObject* obj) { traceCount++; }
    static void reset() { finalizeCount = 0; traceCount = 0; }
};

int BarkWhenTracedClass::finalizeCount;
int BarkWhenTracedClass::traceCount;

const JSClass BarkWhenTracedClass::class_ = {
    "BarkWhenTracedClass", JSCLASS_IMPLEMENTS_BARRIERS,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    finalize,
    nullptr,
    nullptr,
    nullptr,
    trace
};

struct Kennel {
    PersistentRootedObject obj;
    Kennel() { }
    explicit Kennel(JSContext* cx) : obj(cx) { }
    Kennel(JSContext* cx, const HandleObject& woof) : obj(cx, woof) { }
    void init(JSContext* cx, const HandleObject& woof) {
        obj.init(cx, woof);
    }
    void clear() {
        obj = nullptr;
    }
};




MOZ_NEVER_INLINE static Kennel*
Allocate(JSContext* cx)
{
    RootedObject barker(cx, JS_NewObject(cx, &BarkWhenTracedClass::class_));
    if (!barker)
        return nullptr;

    return new Kennel(cx, barker);
}


static bool
GCFinalizesNBarkers(JSContext* cx, int n)
{
    int preGCTrace = BarkWhenTracedClass::traceCount;
    int preGCFinalize = BarkWhenTracedClass::finalizeCount;

    JS_GC(JS_GetRuntime(cx));

    return (BarkWhenTracedClass::finalizeCount == preGCFinalize + n &&
            BarkWhenTracedClass::traceCount > preGCTrace);
}


BEGIN_TEST(test_PersistentRooted)
{
    BarkWhenTracedClass::reset();

    Kennel* kennel = Allocate(cx);
    CHECK(kennel);

    
    CHECK(GCFinalizesNBarkers(cx, 0));

    delete(kennel);

    
    JS_GC(JS_GetRuntime(cx));
    CHECK(BarkWhenTracedClass::finalizeCount == 1);

    return true;
}
END_TEST(test_PersistentRooted)


BEGIN_TEST(test_PersistentRootedNull)
{
    BarkWhenTracedClass::reset();

    Kennel kennel(cx);
    CHECK(!kennel.obj);

    JS_GC(JS_GetRuntime(cx));
    CHECK(BarkWhenTracedClass::finalizeCount == 0);

    return true;
}
END_TEST(test_PersistentRootedNull)


BEGIN_TEST(test_PersistentRootedCopy)
{
    BarkWhenTracedClass::reset();

    Kennel* kennel = Allocate(cx);
    CHECK(kennel);

    CHECK(GCFinalizesNBarkers(cx, 0));

    
    Kennel* newKennel = new Kennel(*kennel);

    CHECK(GCFinalizesNBarkers(cx, 0));

    delete(kennel);

    CHECK(GCFinalizesNBarkers(cx, 0));

    delete(newKennel);

    
    
    JS_GC(JS_GetRuntime(cx));
    CHECK(BarkWhenTracedClass::finalizeCount == 1);

    return true;
}
END_TEST(test_PersistentRootedCopy)


BEGIN_TEST(test_PersistentRootedAssign)
{
    BarkWhenTracedClass::reset();

    Kennel* kennel = Allocate(cx);
    CHECK(kennel);

    CHECK(GCFinalizesNBarkers(cx, 0));

    
    Kennel* kennel2 = new Kennel(cx);

    
    *kennel2 = *kennel;

    
    CHECK(GCFinalizesNBarkers(cx, 0));

    delete(kennel2);

    
    CHECK(GCFinalizesNBarkers(cx, 0));

    
    kennel2 = Allocate(cx);
    CHECK(kennel);

    *kennel = *kennel2;

    
    CHECK(GCFinalizesNBarkers(cx, 1));

    delete(kennel);
    delete(kennel2);

    
    
    JS_GC(JS_GetRuntime(cx));
    CHECK(BarkWhenTracedClass::finalizeCount == 2);

    return true;
}
END_TEST(test_PersistentRootedAssign)

static PersistentRootedObject gGlobalRoot;


BEGIN_TEST(test_GlobalPersistentRooted)
{
    BarkWhenTracedClass::reset();

    CHECK(!gGlobalRoot.initialized());

    {
        RootedObject barker(cx, JS_NewObject(cx, &BarkWhenTracedClass::class_));
        CHECK(barker);

        gGlobalRoot.init(cx, barker);
    }

    CHECK(gGlobalRoot.initialized());

    
    CHECK(GCFinalizesNBarkers(cx, 0));

    gGlobalRoot.reset();
    CHECK(!gGlobalRoot.initialized());

    
    JS_GC(JS_GetRuntime(cx));
    CHECK(BarkWhenTracedClass::finalizeCount == 1);

    return true;
}
END_TEST(test_GlobalPersistentRooted)
