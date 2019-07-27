



#include "js/Class.h"
#include "jsapi-tests/tests.h"

using namespace JS;

struct BarkWhenTracedClass {
    static int finalizeCount;
    static int traceCount;

    static const JSClass class_;
    static void finalize(JSFreeOp *fop, JSObject *obj) { finalizeCount++; }
    static void trace(JSTracer *trc, JSObject *obj) { traceCount++; }
    static void reset() { finalizeCount = 0; traceCount = 0; }
};

int BarkWhenTracedClass::finalizeCount;
int BarkWhenTracedClass::traceCount;

const JSClass BarkWhenTracedClass::class_ = {
  "BarkWhenTracedClass", 0,
  JS_PropertyStub,
  JS_DeletePropertyStub,
  JS_PropertyStub,
  JS_StrictPropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  finalize,
  nullptr,
  nullptr,
  nullptr,
  trace
};

struct Kennel {
    PersistentRootedObject obj;
    explicit Kennel(JSContext *cx) : obj(cx) { }
    Kennel(JSContext *cx, const HandleObject &woof) : obj(cx, woof) { }
};




MOZ_NEVER_INLINE static Kennel *
Allocate(JSContext *cx)
{
    RootedObject barker(cx, JS_NewObject(cx, &BarkWhenTracedClass::class_, JS::NullPtr(), JS::NullPtr()));
    if (!barker)
        return nullptr;

    return new Kennel(cx, barker);
}


static bool
GCFinalizesNBarkers(JSContext *cx, int n)
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

    Kennel *kennel = Allocate(cx);
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

    Kennel *kennel = Allocate(cx);
    CHECK(kennel);

    CHECK(GCFinalizesNBarkers(cx, 0));

    
    Kennel *newKennel = new Kennel(*kennel);

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

    Kennel *kennel = Allocate(cx);
    CHECK(kennel);

    CHECK(GCFinalizesNBarkers(cx, 0));

    
    Kennel *kennel2 = new Kennel(cx);

    
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
