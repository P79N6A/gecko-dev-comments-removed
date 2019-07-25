





































#include "Worker.h"

#include "jsapi.h"
#include "jsfriendapi.h"

#include "EventTarget.h"
#include "RuntimeService.h"
#include "WorkerPrivate.h"

#include "WorkerInlines.h"

#define PROPERTY_FLAGS \
  JSPROP_ENUMERATE | JSPROP_SHARED

#define FUNCTION_FLAGS \
  JSPROP_ENUMERATE

USING_WORKERS_NAMESPACE

namespace {

class Worker
{
  static JSClass sClass;
  static JSPropertySpec sProperties[];
  static JSFunctionSpec sFunctions[];

  enum
  {
    STRING_onerror = 0,
    STRING_onmessage,

    STRING_COUNT
  };

  static const char* const sEventStrings[STRING_COUNT];

protected:
  enum {
    
    
    CONSTRUCTOR_SLOT_PARENT = 0
  };

public:
  static JSClass*
  Class()
  {
    return &sClass;
  }

  static JSObject*
  InitClass(JSContext* aCx, JSObject* aObj, JSObject* aParentProto,
            bool aMainRuntime)
  {
    JSObject* proto = js::InitClassWithReserved(aCx, aObj, aParentProto, &sClass, Construct,
                                                0, sProperties, sFunctions, NULL, NULL);
    if (!proto) {
      return NULL;
    }

    if (!aMainRuntime) {
      WorkerPrivate* parent = GetWorkerPrivateFromContext(aCx);
      parent->AssertIsOnWorkerThread();

      JSObject* constructor = JS_GetConstructor(aCx, proto);
      if (!constructor)
        return NULL;
      js::SetFunctionNativeReserved(constructor, CONSTRUCTOR_SLOT_PARENT,
                                    PRIVATE_TO_JSVAL(parent));
    }

    return proto;
  }

  static void
  ClearPrivateSlot(JSContext* aCx, JSObject* aObj, bool aSaveEventHandlers)
  {
    JS_ASSERT(!JS_IsExceptionPending(aCx));

    WorkerPrivate* worker = GetJSPrivateSafeish<WorkerPrivate>(aObj);
    JS_ASSERT(worker);

    if (aSaveEventHandlers) {
      for (int index = 0; index < STRING_COUNT; index++) {
        const char* name = sEventStrings[index];
        jsval listener;
        if (!worker->GetEventListenerOnEventTarget(aCx, name + 2, &listener) ||
            !JS_DefineProperty(aCx, aObj, name, listener, NULL, NULL,
                               (PROPERTY_FLAGS & ~JSPROP_SHARED))) {
          JS_ClearPendingException(aCx);
        }
      }
    }

    SetJSPrivateSafeish(aObj, NULL);
  }

  static WorkerPrivate*
  GetInstancePrivate(JSContext* aCx, JSObject* aObj, const char* aFunctionName);

protected:
  static JSBool
  ConstructInternal(JSContext* aCx, uintN aArgc, jsval* aVp,
                    bool aIsChromeWorker)
  {
    if (!aArgc) {
      JS_ReportError(aCx, "Constructor requires at least one argument!");
      return false;
    }

    JSString* scriptURL = JS_ValueToString(aCx, JS_ARGV(aCx, aVp)[0]);
    if (!scriptURL) {
      return false;
    }

    jsval priv = js::GetFunctionNativeReserved(JSVAL_TO_OBJECT(JS_CALLEE(aCx, aVp)),
                                               CONSTRUCTOR_SLOT_PARENT);

    RuntimeService* runtimeService;
    WorkerPrivate* parent;

    if (JSVAL_IS_VOID(priv)) {
      runtimeService = RuntimeService::GetOrCreateService();
      if (!runtimeService) {
        JS_ReportError(aCx, "Failed to create runtime service!");
        return false;
      }
      parent = NULL;
    }
    else {
      runtimeService = RuntimeService::GetService();
      parent = static_cast<WorkerPrivate*>(JSVAL_TO_PRIVATE(priv));
      parent->AssertIsOnWorkerThread();
    }

    JSObject* obj = JS_NewObject(aCx, &sClass, nsnull, nsnull);
    if (!obj) {
      return false;
    }

    WorkerPrivate* worker = WorkerPrivate::Create(aCx, obj, parent, scriptURL,
                                                  aIsChromeWorker);
    if (!worker) {
      return false;
    }

    
    SetJSPrivateSafeish(obj, worker);

    if (!runtimeService->RegisterWorker(aCx, worker)) {
      return false;
    }

    JS_SET_RVAL(aCx, aVp, OBJECT_TO_JSVAL(obj));
    return true;
  }

private:
  
  
  
  Worker();
  ~Worker();

  static JSBool
  GetEventListener(JSContext* aCx, JSObject* aObj, jsid aIdval, jsval* aVp)
  {
    JS_ASSERT(JSID_IS_INT(aIdval));
    JS_ASSERT(JSID_TO_INT(aIdval) >= 0 && JSID_TO_INT(aIdval) < STRING_COUNT);

    const char* name = sEventStrings[JSID_TO_INT(aIdval)];
    WorkerPrivate* worker = GetInstancePrivate(aCx, aObj, name);
    if (!worker) {
      return !JS_IsExceptionPending(aCx);
    }

    return worker->GetEventListenerOnEventTarget(aCx, name + 2, aVp);
  }

  static JSBool
  SetEventListener(JSContext* aCx, JSObject* aObj, jsid aIdval, JSBool aStrict,
                   jsval* aVp)
  {
    JS_ASSERT(JSID_IS_INT(aIdval));
    JS_ASSERT(JSID_TO_INT(aIdval) >= 0 && JSID_TO_INT(aIdval) < STRING_COUNT);

    const char* name = sEventStrings[JSID_TO_INT(aIdval)];
    WorkerPrivate* worker = GetInstancePrivate(aCx, aObj, name);
    if (!worker) {
      return !JS_IsExceptionPending(aCx);
    }

    return worker->SetEventListenerOnEventTarget(aCx, name + 2, aVp);
  }

  static JSBool
  Construct(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    return ConstructInternal(aCx, aArgc, aVp, false);
  }

  static void
  Finalize(JSContext* aCx, JSObject* aObj)
  {
    JS_ASSERT(JS_GetClass(aObj) == &sClass);
    WorkerPrivate* worker = GetJSPrivateSafeish<WorkerPrivate>(aObj);
    if (worker) {
      worker->FinalizeInstance(aCx, true);
    }
  }

  static void
  Trace(JSTracer* aTrc, JSObject* aObj)
  {
    JS_ASSERT(JS_GetClass(aObj) == &sClass);
    WorkerPrivate* worker = GetJSPrivateSafeish<WorkerPrivate>(aObj);
    if (worker) {
      worker->TraceInstance(aTrc);
    }
  }

  static JSBool
  Terminate(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JSObject* obj = JS_THIS_OBJECT(aCx, aVp);
    if (!obj) {
      return false;
    }

    const char*& name = sFunctions[0].name;
    WorkerPrivate* worker = GetInstancePrivate(aCx, obj, name);
    if (!worker) {
      return !JS_IsExceptionPending(aCx);
    }

    return worker->Terminate(aCx);
  }

  static JSBool
  PostMessage(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    JSObject* obj = JS_THIS_OBJECT(aCx, aVp);
    if (!obj) {
      return false;
    }

    const char*& name = sFunctions[1].name;
    WorkerPrivate* worker = GetInstancePrivate(aCx, obj, name);
    if (!worker) {
      return !JS_IsExceptionPending(aCx);
    }

    jsval message;
    if (!JS_ConvertArguments(aCx, aArgc, JS_ARGV(aCx, aVp), "v", &message)) {
      return false;
    }

    return worker->PostMessage(aCx, message);
  }
};

JSClass Worker::sClass = {
  "Worker",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize, NULL, NULL, NULL,
  NULL, NULL, NULL, Trace, NULL
};

JSPropertySpec Worker::sProperties[] = {
  { sEventStrings[STRING_onerror], STRING_onerror, PROPERTY_FLAGS,
    GetEventListener, SetEventListener },
  { sEventStrings[STRING_onmessage], STRING_onmessage, PROPERTY_FLAGS,
    GetEventListener, SetEventListener },
  { 0, 0, 0, NULL, NULL }
};

JSFunctionSpec Worker::sFunctions[] = {
  JS_FN("terminate", Terminate, 0, FUNCTION_FLAGS),
  JS_FN("postMessage", PostMessage, 1, FUNCTION_FLAGS),
  JS_FS_END
};

const char* const Worker::sEventStrings[STRING_COUNT] = {
  "onerror",
  "onmessage"
};

class ChromeWorker : public Worker
{
  static JSClass sClass;

public:
  static JSClass*
  Class()
  {
    return &sClass;
  }

  static JSObject*
  InitClass(JSContext* aCx, JSObject* aObj, JSObject* aParentProto,
            bool aMainRuntime)
  {
    JSObject* proto = js::InitClassWithReserved(aCx, aObj, aParentProto, &sClass, Construct,
                                                0, NULL, NULL, NULL, NULL);
    if (!proto) {
      return NULL;
    }

    if (!aMainRuntime) {
      WorkerPrivate* parent = GetWorkerPrivateFromContext(aCx);
      parent->AssertIsOnWorkerThread();

      JSObject* constructor = JS_GetConstructor(aCx, proto);
      if (!constructor)
        return NULL;
      js::SetFunctionNativeReserved(constructor, CONSTRUCTOR_SLOT_PARENT,
                                    PRIVATE_TO_JSVAL(parent));
    }

    return proto;
  }

  static void
  ClearPrivateSlot(JSContext* aCx, JSObject* aObj, bool aSaveEventHandlers)
  {
    Worker::ClearPrivateSlot(aCx, aObj, aSaveEventHandlers);
  }

private:
  
  
  
  ChromeWorker();
  ~ChromeWorker();

  static WorkerPrivate*
  GetInstancePrivate(JSContext* aCx, JSObject* aObj, const char* aFunctionName)
  {
    if (aObj) {
      JSClass* classPtr = JS_GetClass(aObj);
      if (classPtr == &sClass) {
        return GetJSPrivateSafeish<WorkerPrivate>(aObj);
      }
    }

    return Worker::GetInstancePrivate(aCx, aObj, aFunctionName);
  }

  static JSBool
  Construct(JSContext* aCx, uintN aArgc, jsval* aVp)
  {
    return ConstructInternal(aCx, aArgc, aVp, true);
  }

  static void
  Finalize(JSContext* aCx, JSObject* aObj)
  {
    JS_ASSERT(JS_GetClass(aObj) == &sClass);
    WorkerPrivate* worker = GetJSPrivateSafeish<WorkerPrivate>(aObj);
    if (worker) {
      worker->FinalizeInstance(aCx, true);
    }
  }

  static void
  Trace(JSTracer* aTrc, JSObject* aObj)
  {
    JS_ASSERT(JS_GetClass(aObj) == &sClass);
    WorkerPrivate* worker = GetJSPrivateSafeish<WorkerPrivate>(aObj);
    if (worker) {
      worker->TraceInstance(aTrc);
    }
  }
};

JSClass ChromeWorker::sClass = {
  "ChromeWorker",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize, NULL, NULL, NULL,
  NULL, NULL, NULL, Trace, NULL
};

WorkerPrivate*
Worker::GetInstancePrivate(JSContext* aCx, JSObject* aObj,
                           const char* aFunctionName)
{
  JSClass* classPtr = JS_GetClass(aObj);
  if (classPtr == &sClass || classPtr == ChromeWorker::Class()) {
    return GetJSPrivateSafeish<WorkerPrivate>(aObj);
  }

  JS_ReportErrorNumber(aCx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                       sClass.name, aFunctionName, classPtr->name);
  return NULL;
}

} 

BEGIN_WORKERS_NAMESPACE

namespace worker {

JSObject*
InitClass(JSContext* aCx, JSObject* aGlobal, JSObject* aProto,
          bool aMainRuntime)
{
  return Worker::InitClass(aCx, aGlobal, aProto, aMainRuntime);
}

void
ClearPrivateSlot(JSContext* aCx, JSObject* aObj, bool aSaveEventHandlers)
{
  JSClass* clasp = JS_GetClass(aObj);
  JS_ASSERT(clasp == Worker::Class() || clasp == ChromeWorker::Class());

  if (clasp == ChromeWorker::Class()) {
    ChromeWorker::ClearPrivateSlot(aCx, aObj, aSaveEventHandlers);
  }
  else {
    Worker::ClearPrivateSlot(aCx, aObj, aSaveEventHandlers);
  }
}

} 

WorkerCrossThreadDispatcher*
GetWorkerCrossThreadDispatcher(JSContext* aCx, jsval aWorker)
{
  if (JSVAL_IS_PRIMITIVE(aWorker)) {
    return NULL;
  }

  WorkerPrivate* w =
      Worker::GetInstancePrivate(aCx, JSVAL_TO_OBJECT(aWorker),
                                 "GetWorkerCrossThreadDispatcher");
  if (!w) {
    return NULL;
  }
  return w->GetCrossThreadDispatcher();
}


namespace chromeworker {

bool
InitClass(JSContext* aCx, JSObject* aGlobal, JSObject* aProto,
          bool aMainRuntime)
{
  return !!ChromeWorker::InitClass(aCx, aGlobal, aProto, aMainRuntime);
}

} 

bool
ClassIsWorker(JSClass* aClass)
{
  return Worker::Class() == aClass || ChromeWorker::Class() == aClass;
}

END_WORKERS_NAMESPACE
