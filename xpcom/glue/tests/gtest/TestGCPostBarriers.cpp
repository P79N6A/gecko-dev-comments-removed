










#include "jsapi.h"
#include "nsTArray.h"

#include "gtest/gtest.h"

#include "js/TracingAPI.h"
#include "js/HeapAPI.h"

#include "mozilla/CycleCollectedJSRuntime.h"

using namespace JS;
using namespace mozilla;

template<class ArrayT>
static void
TraceArray(JSTracer* trc, void* data)
{
  ArrayT* array = static_cast<ArrayT *>(data);
  for (unsigned i = 0; i < array->Length(); ++i)
    JS_CallObjectTracer(trc, &array->ElementAt(i), "array-element");
}





const size_t ElementCount = 100;
const size_t InitialElements = ElementCount / 10;

template<class ArrayT>
static void
RunTest(JSRuntime* rt, JSContext* cx, ArrayT* array)
{
  JS_GC(rt);

  ASSERT_TRUE(array != nullptr);
  JS_AddExtraGCRootsTracer(rt, TraceArray<ArrayT>, array);

  



  RootedValue value(cx);
  const char* property = "foo";
  for (size_t i = 0; i < ElementCount; ++i) {
    RootedObject obj(cx, JS_NewPlainObject(cx));
    ASSERT_FALSE(JS::ObjectIsTenured(obj));
    value = Int32Value(i);
    ASSERT_TRUE(JS_SetProperty(cx, obj, property, value));
    array->AppendElement(obj);
  }

  



  JS_GC(rt);

  


  for (size_t i = 0; i < ElementCount; ++i) {
    RootedObject obj(cx, array->ElementAt(i));
    ASSERT_TRUE(JS::ObjectIsTenured(obj));
    ASSERT_TRUE(JS_GetProperty(cx, obj, property, &value));
    ASSERT_TRUE(value.isInt32());
    ASSERT_EQ(static_cast<int32_t>(i), value.toInt32());
  }

  JS_RemoveExtraGCRootsTracer(rt, TraceArray<ArrayT>, array);
}

static void
CreateGlobalAndRunTest(JSRuntime* rt, JSContext* cx)
{
  static const JSClass GlobalClass = {
    "global", JSCLASS_GLOBAL_FLAGS,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    JS_GlobalObjectTraceHook
  };

  JS::CompartmentOptions options;
  options.setVersion(JSVERSION_LATEST);
  JS::PersistentRootedObject global(cx);
  global = JS_NewGlobalObject(cx, &GlobalClass, nullptr, JS::FireOnNewGlobalHook, options);
  ASSERT_TRUE(global != nullptr);

  JSCompartment *oldCompartment = JS_EnterCompartment(cx, global);

  typedef Heap<JSObject*> ElementT;

  {
    nsTArray<ElementT>* array = new nsTArray<ElementT>(InitialElements);
    RunTest(rt, cx, array);
    delete array;
  }

  {
    FallibleTArray<ElementT>* array = new FallibleTArray<ElementT>(InitialElements);
    RunTest(rt, cx, array);
    delete array;
  }

  {
    nsAutoTArray<ElementT, InitialElements> array;
    RunTest(rt, cx, &array);
  }

  {
    AutoFallibleTArray<ElementT, InitialElements> array;
    RunTest(rt, cx, &array);
  }

  JS_LeaveCompartment(cx, oldCompartment);
}

TEST(GCPostBarriers, nsTArray) {
  CycleCollectedJSRuntime* ccrt = CycleCollectedJSRuntime::Get();
  ASSERT_TRUE(ccrt != nullptr);
  JSRuntime* rt = ccrt->Runtime();
  ASSERT_TRUE(rt != nullptr);

  JSContext *cx = JS_NewContext(rt, 8192);
  ASSERT_TRUE(cx != nullptr);
  JS_BeginRequest(cx);

  CreateGlobalAndRunTest(rt, cx);

  JS_EndRequest(cx);
  JS_DestroyContext(cx);
}
