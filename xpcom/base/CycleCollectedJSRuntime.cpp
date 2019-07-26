





#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMJSClass.h"
#include "jsfriendapi.h"
#include "jsprf.h"
#include "nsCycleCollectionNoteRootCallback.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCycleCollector.h"
#include "nsDOMJSUtils.h"
#include "nsLayoutStatics.h"
#include "xpcpublic.h"

using namespace mozilla;
using namespace mozilla::dom;

inline bool
AddToCCKind(JSGCTraceKind kind)
{
  return kind == JSTRACE_OBJECT || kind == JSTRACE_SCRIPT;
}

struct NoteWeakMapChildrenTracer : public JSTracer
{
  NoteWeakMapChildrenTracer(nsCycleCollectionNoteRootCallback& cb)
  : mCb(cb)
  {
  }
  nsCycleCollectionNoteRootCallback& mCb;
  bool mTracedAny;
  JSObject* mMap;
  void* mKey;
  void* mKeyDelegate;
};

static void
TraceWeakMappingChild(JSTracer* trc, void** thingp, JSGCTraceKind kind)
{
  MOZ_ASSERT(trc->callback == TraceWeakMappingChild);
  void* thing = *thingp;
  NoteWeakMapChildrenTracer* tracer =
    static_cast<NoteWeakMapChildrenTracer*>(trc);

  if (kind == JSTRACE_STRING) {
    return;
  }

  if (!xpc_IsGrayGCThing(thing) && !tracer->mCb.WantAllTraces()) {
    return;
  }

  if (AddToCCKind(kind)) {
    tracer->mCb.NoteWeakMapping(tracer->mMap, tracer->mKey, tracer->mKeyDelegate, thing);
    tracer->mTracedAny = true;
  } else {
    JS_TraceChildren(trc, thing, kind);
  }
}

struct NoteWeakMapsTracer : public js::WeakMapTracer
{
  NoteWeakMapsTracer(JSRuntime* rt, js::WeakMapTraceCallback cb,
                     nsCycleCollectionNoteRootCallback& cccb)
  : js::WeakMapTracer(rt, cb), mCb(cccb), mChildTracer(cccb)
  {
    JS_TracerInit(&mChildTracer, rt, TraceWeakMappingChild);
  }
  nsCycleCollectionNoteRootCallback& mCb;
  NoteWeakMapChildrenTracer mChildTracer;
};

static void
TraceWeakMapping(js::WeakMapTracer* trc, JSObject* m,
               void* k, JSGCTraceKind kkind,
               void* v, JSGCTraceKind vkind)
{
  MOZ_ASSERT(trc->callback == TraceWeakMapping);
  NoteWeakMapsTracer* tracer = static_cast<NoteWeakMapsTracer* >(trc);

  
  if ((!k || !xpc_IsGrayGCThing(k)) && MOZ_LIKELY(!tracer->mCb.WantAllTraces())) {
    if (!v || !xpc_IsGrayGCThing(v) || vkind == JSTRACE_STRING) {
      return;
    }
  }

  
  
  
  
  MOZ_ASSERT(AddToCCKind(kkind));

  
  
  
  
  if (!AddToCCKind(kkind)) {
    k = nullptr;
  }

  JSObject* kdelegate = nullptr;
  if (k && kkind == JSTRACE_OBJECT) {
    kdelegate = js::GetWeakmapKeyDelegate((JSObject*)k);
  }

  if (AddToCCKind(vkind)) {
    tracer->mCb.NoteWeakMapping(m, k, kdelegate, v);
  } else {
    tracer->mChildTracer.mTracedAny = false;
    tracer->mChildTracer.mMap = m;
    tracer->mChildTracer.mKey = k;
    tracer->mChildTracer.mKeyDelegate = kdelegate;

    if (v && vkind != JSTRACE_STRING) {
      JS_TraceChildren(&tracer->mChildTracer, v, vkind);
    }

    
    
    if (!tracer->mChildTracer.mTracedAny && k && xpc_IsGrayGCThing(k) && kdelegate) {
      tracer->mCb.NoteWeakMapping(m, k, kdelegate, nullptr);
    }
  }
}


struct FixWeakMappingGrayBitsTracer : public js::WeakMapTracer
{
  FixWeakMappingGrayBitsTracer(JSRuntime* rt)
    : js::WeakMapTracer(rt, FixWeakMappingGrayBits)
  {}

  void
  FixAll()
  {
    do {
      mAnyMarked = false;
      js::TraceWeakMaps(this);
    } while (mAnyMarked);
  }

private:

  static void
  FixWeakMappingGrayBits(js::WeakMapTracer* trc, JSObject* m,
                         void* k, JSGCTraceKind kkind,
                         void* v, JSGCTraceKind vkind)
  {
    MOZ_ASSERT(!JS::IsIncrementalGCInProgress(trc->runtime),
               "Don't call FixWeakMappingGrayBits during a GC.");

    FixWeakMappingGrayBitsTracer* tracer = static_cast<FixWeakMappingGrayBitsTracer*>(trc);

    
    bool delegateMightNeedMarking = k && xpc_IsGrayGCThing(k);
    bool valueMightNeedMarking = v && xpc_IsGrayGCThing(v) && vkind != JSTRACE_STRING;
    if (!delegateMightNeedMarking && !valueMightNeedMarking) {
      return;
    }

    if (!AddToCCKind(kkind)) {
      k = nullptr;
    }

    if (delegateMightNeedMarking && kkind == JSTRACE_OBJECT) {
      JSObject* kdelegate = js::GetWeakmapKeyDelegate((JSObject*)k);
      if (kdelegate && !xpc_IsGrayGCThing(kdelegate)) {
        JS::UnmarkGrayGCThingRecursively(k, JSTRACE_OBJECT);
        tracer->mAnyMarked = true;
      }
    }

    if (v && xpc_IsGrayGCThing(v) &&
        (!k || !xpc_IsGrayGCThing(k)) &&
        (!m || !xpc_IsGrayGCThing(m)) &&
        vkind != JSTRACE_SHAPE) {
      JS::UnmarkGrayGCThingRecursively(v, vkind);
      tracer->mAnyMarked = true;
    }
  }

  bool mAnyMarked;
};

class JSContextParticipant : public nsCycleCollectionParticipant
{
public:
  static NS_METHOD RootImpl(void *n)
  {
    return NS_OK;
  }
  static NS_METHOD UnlinkImpl(void *n)
  {
    return NS_OK;
  }
  static NS_METHOD UnrootImpl(void *n)
  {
    return NS_OK;
  }
  static NS_METHOD_(void) UnmarkIfPurpleImpl(void *n)
  {
  }
  static NS_METHOD TraverseImpl(JSContextParticipant *that, void *n,
                                nsCycleCollectionTraversalCallback &cb)
  {
    JSContext *cx = static_cast<JSContext*>(n);

    
    
    
    
    
    unsigned refCount = js::ContextHasOutstandingRequests(cx) ? 2 : 1;

    cb.DescribeRefCountedNode(refCount, "JSContext");
    if (JSObject *global = js::GetDefaultGlobalForContext(cx)) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[global object]");
      cb.NoteJSChild(global);
    }

    return NS_OK;
  }
};

static const CCParticipantVTable<JSContextParticipant>::Type
JSContext_cycleCollectorGlobal =
{
  NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(JSContextParticipant)
};

struct Closure
{
  bool cycleCollectionEnabled;
  nsCycleCollectionNoteRootCallback *cb;
};

static void
CheckParticipatesInCycleCollection(void *aThing, const char *name, void *aClosure)
{
  Closure *closure = static_cast<Closure*>(aClosure);

  if (closure->cycleCollectionEnabled) {
    return;
  }

  if (AddToCCKind(js::GCThingTraceKind(aThing)) &&
      xpc_IsGrayGCThing(aThing))
  {
    closure->cycleCollectionEnabled = true;
  }
}

static PLDHashOperator
NoteJSHolder(void *holder, nsScriptObjectTracer *&tracer, void *arg)
{
  Closure *closure = static_cast<Closure*>(arg);

  closure->cycleCollectionEnabled = false;
  tracer->Trace(holder, TraceCallbackFunc(CheckParticipatesInCycleCollection), closure);
  if (closure->cycleCollectionEnabled) {
    closure->cb->NoteNativeRoot(holder, tracer);
  }

  return PL_DHASH_NEXT;
}

NS_METHOD
JSGCThingParticipant::TraverseImpl(JSGCThingParticipant* that, void* p,
                                   nsCycleCollectionTraversalCallback& cb)
{
  CycleCollectedJSRuntime* runtime = reinterpret_cast<CycleCollectedJSRuntime*>
    (reinterpret_cast<char*>(that) -
     offsetof(CycleCollectedJSRuntime, mGCThingCycleCollectorGlobal));

  runtime->TraverseGCThing(CycleCollectedJSRuntime::TRAVERSE_FULL,
                           p, js::GCThingTraceKind(p), cb);
  return NS_OK;
}



static const CCParticipantVTable<JSGCThingParticipant>::Type
sGCThingCycleCollectorGlobal =
{
  NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(JSGCThingParticipant)
};

NS_METHOD
JSZoneParticipant::TraverseImpl(JSZoneParticipant* that, void* p,
                                nsCycleCollectionTraversalCallback& cb)
{
  CycleCollectedJSRuntime* runtime = reinterpret_cast<CycleCollectedJSRuntime*>
    (reinterpret_cast<char*>(that) -
     offsetof(CycleCollectedJSRuntime, mJSZoneCycleCollectorGlobal));

  MOZ_ASSERT(!cb.WantAllTraces());
  JS::Zone* zone = static_cast<JS::Zone*>(p);

  runtime->TraverseZone(zone, cb);
  return NS_OK;
}

struct TraversalTracer : public JSTracer
{
  TraversalTracer(nsCycleCollectionTraversalCallback& aCb) : mCb(aCb)
  {
  }
  nsCycleCollectionTraversalCallback& mCb;
};

static void
NoteJSChild(JSTracer* aTrc, void* aThing, JSGCTraceKind aTraceKind)
{
  TraversalTracer* tracer = static_cast<TraversalTracer*>(aTrc);

  
  if (!xpc_IsGrayGCThing(aThing) && !tracer->mCb.WantAllTraces()) {
    return;
  }

  







if (AddToCCKind(aTraceKind)) {
    if (MOZ_UNLIKELY(tracer->mCb.WantDebugInfo())) {
      
      if (tracer->debugPrinter) {
        char buffer[200];
        tracer->debugPrinter(aTrc, buffer, sizeof(buffer));
        tracer->mCb.NoteNextEdgeName(buffer);
      } else if (tracer->debugPrintIndex != (size_t)-1) {
        char buffer[200];
        JS_snprintf(buffer, sizeof(buffer), "%s[%lu]",
                    static_cast<const char *>(tracer->debugPrintArg),
                    tracer->debugPrintIndex);
        tracer->mCb.NoteNextEdgeName(buffer);
      } else {
        tracer->mCb.NoteNextEdgeName(static_cast<const char*>(tracer->debugPrintArg));
      }
    }
    tracer->mCb.NoteJSChild(aThing);
  } else if (aTraceKind == JSTRACE_SHAPE) {
    JS_TraceShapeCycleCollectorChildren(aTrc, aThing);
  } else if (aTraceKind != JSTRACE_STRING) {
    JS_TraceChildren(aTrc, aThing, aTraceKind);
  }
}

static void
NoteJSChildTracerShim(JSTracer* aTrc, void** aThingp, JSGCTraceKind aTraceKind)
{
  NoteJSChild(aTrc, *aThingp, aTraceKind);
}

static void
NoteJSChildGrayWrapperShim(void* aData, void* aThing)
{
  TraversalTracer* trc = static_cast<TraversalTracer*>(aData);
  NoteJSChild(trc, aThing, js::GCThingTraceKind(aThing));
}
























static const CCParticipantVTable<JSZoneParticipant>::Type
sJSZoneCycleCollectorGlobal = {
  NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(JSZoneParticipant)
};

CycleCollectedJSRuntime::CycleCollectedJSRuntime(uint32_t aMaxbytes,
                                                 JSUseHelperThreads aUseHelperThreads,
                                                 bool aExpectUnrootedGlobals)
  : mGCThingCycleCollectorGlobal(sGCThingCycleCollectorGlobal),
    mJSZoneCycleCollectorGlobal(sJSZoneCycleCollectorGlobal),
    mJSRuntime(nullptr)
#ifdef DEBUG
  , mObjectToUnlink(nullptr)
  , mExpectUnrootedGlobals(aExpectUnrootedGlobals)
#endif
{
  mJSRuntime = JS_NewRuntime(aMaxbytes, aUseHelperThreads);
  if (!mJSRuntime) {
    MOZ_CRASH();
  }

  JS_SetGrayGCRootsTracer(mJSRuntime, TraceGrayJS, this);

  mJSHolders.Init(512);

  nsCycleCollector_registerJSRuntime(this);
}

CycleCollectedJSRuntime::~CycleCollectedJSRuntime()
{
  nsCycleCollector_forgetJSRuntime();

  JS_DestroyRuntime(mJSRuntime);
  mJSRuntime = nullptr;
}

void
CycleCollectedJSRuntime::MaybeTraceGlobals(JSTracer* aTracer) const
{
  JSContext* iter = nullptr;
  while (JSContext* acx = JS_ContextIterator(Runtime(), &iter)) {
    MOZ_ASSERT(js::HasUnrootedGlobal(acx) == mExpectUnrootedGlobals);
    if (!js::HasUnrootedGlobal(acx)) {
      continue;
    }

    if (JSObject* global = js::GetDefaultGlobalForContext(acx)) {
      JS_CallObjectTracer(aTracer, &global, "Global Object");
    }
  }
}

void
CycleCollectedJSRuntime::DescribeGCThing(bool aIsMarked, void* aThing,
                                         JSGCTraceKind aTraceKind,
                                         nsCycleCollectionTraversalCallback& aCb) const
{
  if (!aCb.WantDebugInfo()) {
    aCb.DescribeGCedNode(aIsMarked, "JS Object");
    return;
  }

  char name[72];
  if (aTraceKind == JSTRACE_OBJECT) {
    JSObject* obj = static_cast<JSObject*>(aThing);
    js::Class* clasp = js::GetObjectClass(obj);

    
    if (DescribeCustomObjects(obj, clasp, name)) {
      
    } else if (clasp == &js::FunctionClass) {
      JSFunction* fun = JS_GetObjectFunction(obj);
      JSString* str = JS_GetFunctionDisplayId(fun);
      if (str) {
        NS_ConvertUTF16toUTF8 fname(JS_GetInternedStringChars(str));
        JS_snprintf(name, sizeof(name),
                    "JS Object (Function - %s)", fname.get());
      } else {
        JS_snprintf(name, sizeof(name), "JS Object (Function)");
      }
    } else {
      JS_snprintf(name, sizeof(name), "JS Object (%s)",
                  clasp->name);
    }
  } else {
    static const char trace_types[][11] = {
      "Object",
      "String",
      "Script",
      "LazyScript",
      "IonCode",
      "Shape",
      "BaseShape",
      "TypeObject",
    };
    JS_STATIC_ASSERT(NS_ARRAY_LENGTH(trace_types) == JSTRACE_LAST + 1);
    JS_snprintf(name, sizeof(name), "JS %s", trace_types[aTraceKind]);
  }

  
  aCb.DescribeGCedNode(aIsMarked, name);
}

void
CycleCollectedJSRuntime::NoteGCThingJSChildren(void* aThing,
                                               JSGCTraceKind aTraceKind,
                                               nsCycleCollectionTraversalCallback& aCb) const
{
  MOZ_ASSERT(mJSRuntime);
  TraversalTracer trc(aCb);
  JS_TracerInit(&trc, mJSRuntime, NoteJSChildTracerShim);
  trc.eagerlyTraceWeakMaps = DoNotTraceWeakMaps;
  JS_TraceChildren(&trc, aThing, aTraceKind);
}

void
CycleCollectedJSRuntime::NoteGCThingXPCOMChildren(js::Class* aClasp, JSObject* aObj,
                                                  nsCycleCollectionTraversalCallback& aCb) const
{
  MOZ_ASSERT(aClasp);
  MOZ_ASSERT(aClasp == js::GetObjectClass(aObj));

  if (NoteCustomGCThingXPCOMChildren(aClasp, aObj, aCb)) {
    
    return;
  }
  
  
  else if (aClasp->flags & JSCLASS_HAS_PRIVATE &&
           aClasp->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(aCb, "js::GetObjectPrivate(obj)");
    aCb.NoteXPCOMChild(static_cast<nsISupports*>(js::GetObjectPrivate(aObj)));
  } else {
    const DOMClass* domClass = GetDOMClass(aObj);
    if (domClass) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(aCb, "UnwrapDOMObject(obj)");
      if (domClass->mDOMObjectIsISupports) {
        aCb.NoteXPCOMChild(UnwrapDOMObject<nsISupports>(aObj));
      } else if (domClass->mParticipant) {
        aCb.NoteNativeChild(UnwrapDOMObject<void>(aObj),
                            domClass->mParticipant);
      }
    }
  }
}

void
CycleCollectedJSRuntime::TraverseGCThing(TraverseSelect aTs, void* aThing,
                                         JSGCTraceKind aTraceKind,
                                         nsCycleCollectionTraversalCallback& aCb)
{
  MOZ_ASSERT(aTraceKind == js::GCThingTraceKind(aThing));
  bool isMarkedGray = xpc_IsGrayGCThing(aThing);

  if (aTs == TRAVERSE_FULL) {
    DescribeGCThing(!isMarkedGray, aThing, aTraceKind, aCb);
  }

  
  
  
  
  if (!isMarkedGray && !aCb.WantAllTraces()) {
    return;
  }

  if (aTs == TRAVERSE_FULL) {
    NoteGCThingJSChildren(aThing, aTraceKind, aCb);
  }

  if (aTraceKind == JSTRACE_OBJECT) {
    JSObject* obj = static_cast<JSObject*>(aThing);
    NoteGCThingXPCOMChildren(js::GetObjectClass(obj), obj, aCb);
  }
}

struct TraverseObjectShimClosure {
  nsCycleCollectionTraversalCallback& cb;
  CycleCollectedJSRuntime* self;
};

void
CycleCollectedJSRuntime::TraverseZone(JS::Zone* aZone,
                                      nsCycleCollectionTraversalCallback& aCb)
{
  









  aCb.DescribeGCedNode(false, "JS Zone");

  







  TraversalTracer trc(aCb);
  JS_TracerInit(&trc, mJSRuntime, NoteJSChildTracerShim);
  trc.eagerlyTraceWeakMaps = DoNotTraceWeakMaps;
  js::VisitGrayWrapperTargets(aZone, NoteJSChildGrayWrapperShim, &trc);

  



  TraverseObjectShimClosure closure = { aCb, this };
  js::IterateGrayObjects(aZone, TraverseObjectShim, &closure);
}

 void
CycleCollectedJSRuntime::TraverseObjectShim(void* aData, void* aThing)
{
  TraverseObjectShimClosure* closure =
      static_cast<TraverseObjectShimClosure*>(aData);

  MOZ_ASSERT(js::GCThingTraceKind(aThing) == JSTRACE_OBJECT);
  closure->self->TraverseGCThing(CycleCollectedJSRuntime::TRAVERSE_CPP, aThing,
                                 JSTRACE_OBJECT, closure->cb);
}








void
CycleCollectedJSRuntime::MaybeTraverseGlobals(nsCycleCollectionNoteRootCallback& aCb) const
{
  JSContext *iter = nullptr, *acx;
  while ((acx = JS_ContextIterator(Runtime(), &iter))) {
    
    
    JSObject* global = js::GetDefaultGlobalForContext(acx);
    if (global && xpc_IsGrayGCThing(global)) {
      aCb.NoteNativeRoot(acx, JSContextParticipant());
    }
  }
}

void
CycleCollectedJSRuntime::TraverseNativeRoots(nsCycleCollectionNoteRootCallback& aCb)
{
  MaybeTraverseGlobals(aCb);

  
  
  TraverseAdditionalNativeRoots(aCb);

  Closure closure = { true, &aCb };
  mJSHolders.Enumerate(NoteJSHolder, &closure);
}

 void
CycleCollectedJSRuntime::TraceGrayJS(JSTracer* aTracer, void* aData)
{
  CycleCollectedJSRuntime* self = static_cast<CycleCollectedJSRuntime*>(aData);

  
  self->TraceNativeRoots(aTracer);
}

struct JsGcTracer : public TraceCallbacks
{
  virtual void Trace(JS::Heap<JS::Value> *p, const char *name, void *closure) const MOZ_OVERRIDE {
    JS_CallHeapValueTracer(static_cast<JSTracer*>(closure), p, name);
  }
  virtual void Trace(JS::Heap<jsid> *p, const char *name, void *closure) const MOZ_OVERRIDE {
    JS_CallHeapIdTracer(static_cast<JSTracer*>(closure), p, name);
  }
  virtual void Trace(JS::Heap<JSObject *> *p, const char *name, void *closure) const MOZ_OVERRIDE {
    JS_CallHeapObjectTracer(static_cast<JSTracer*>(closure), p, name);
  }
  virtual void Trace(JS::Heap<JSString *> *p, const char *name, void *closure) const MOZ_OVERRIDE {
    JS_CallHeapStringTracer(static_cast<JSTracer*>(closure), p, name);
  }
  virtual void Trace(JS::Heap<JSScript *> *p, const char *name, void *closure) const MOZ_OVERRIDE {
    JS_CallHeapScriptTracer(static_cast<JSTracer*>(closure), p, name);
  }
};

static PLDHashOperator
TraceJSHolder(void* aHolder, nsScriptObjectTracer*& aTracer, void* aArg)
{
  aTracer->Trace(aHolder, JsGcTracer(), aArg);

  return PL_DHASH_NEXT;
}

void
CycleCollectedJSRuntime::TraceNativeRoots(JSTracer* aTracer)
{
  MaybeTraceGlobals(aTracer);

  
  
  TraceAdditionalNativeRoots(aTracer);

  mJSHolders.Enumerate(TraceJSHolder, aTracer);
}

void
CycleCollectedJSRuntime::AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer)
{
  MOZ_ASSERT(aTracer->Trace, "AddJSHolder needs a non-null Trace function");
  bool wasEmpty = mJSHolders.Count() == 0;
  mJSHolders.Put(aHolder, aTracer);
  if (wasEmpty && mJSHolders.Count() == 1) {
    nsLayoutStatics::AddRef();
  }
}

void
CycleCollectedJSRuntime::RemoveJSHolder(void* aHolder)
{
#ifdef DEBUG
  
  
  
  
  if (aHolder != mObjectToUnlink) {
    AssertNoObjectsToTrace(aHolder);
  }
#endif
  bool hadOne = mJSHolders.Count() == 1;
  mJSHolders.Remove(aHolder);
  if (hadOne && mJSHolders.Count() == 0) {
    nsLayoutStatics::Release();
  }
}

#ifdef DEBUG
bool
CycleCollectedJSRuntime::TestJSHolder(void* aHolder)
{
  return mJSHolders.Get(aHolder, nullptr);
}

static void
AssertNoGcThing(void* aGCThing, const char* aName, void* aClosure)
{
  MOZ_ASSERT(!aGCThing);
}

void
CycleCollectedJSRuntime::AssertNoObjectsToTrace(void* aPossibleJSHolder)
{
  nsScriptObjectTracer* tracer = mJSHolders.Get(aPossibleJSHolder);
  if (tracer && tracer->Trace) {
    tracer->Trace(aPossibleJSHolder, TraceCallbackFunc(AssertNoGcThing), nullptr);
  }
}
#endif


nsCycleCollectionParticipant*
CycleCollectedJSRuntime::JSContextParticipant()
{
  return JSContext_cycleCollectorGlobal.GetParticipant();
}

nsCycleCollectionParticipant*
CycleCollectedJSRuntime::GCThingParticipant() const
{
    return mGCThingCycleCollectorGlobal.GetParticipant();
}

nsCycleCollectionParticipant*
CycleCollectedJSRuntime::ZoneParticipant() const
{
    return mJSZoneCycleCollectorGlobal.GetParticipant();
}

bool
CycleCollectedJSRuntime::NotifyLeaveMainThread() const
{
  MOZ_ASSERT(NS_IsMainThread());
  if (JS_IsInRequest(mJSRuntime)) {
    return false;
  }
  JS_ClearRuntimeThread(mJSRuntime);
  return true;
}

void
CycleCollectedJSRuntime::NotifyEnterCycleCollectionThread() const
{
  MOZ_ASSERT(!NS_IsMainThread());
  JS_SetRuntimeThread(mJSRuntime);
}

void
CycleCollectedJSRuntime::NotifyLeaveCycleCollectionThread() const
{
  MOZ_ASSERT(!NS_IsMainThread());
  JS_ClearRuntimeThread(mJSRuntime);
}

void
CycleCollectedJSRuntime::NotifyEnterMainThread() const
{
  MOZ_ASSERT(NS_IsMainThread());
  JS_SetRuntimeThread(mJSRuntime);
}

nsresult
CycleCollectedJSRuntime::BeginCycleCollection(nsCycleCollectionNoteRootCallback &aCb)
{
  static bool gcHasRun = false;
  if (!gcHasRun) {
    uint32_t gcNumber = JS_GetGCParameter(mJSRuntime, JSGC_NUMBER);
    if (!gcNumber) {
      
      MOZ_CRASH();
    }
    gcHasRun = true;
  }

  TraverseNativeRoots(aCb);

  NoteWeakMapsTracer trc(mJSRuntime, TraceWeakMapping, aCb);
  js::TraceWeakMaps(&trc);

  return NS_OK;
}






bool
CycleCollectedJSRuntime::UsefulToMergeZones() const
{
  JSContext* iter = nullptr;
  JSContext* cx;
  while ((cx = JS_ContextIterator(mJSRuntime, &iter))) {
    
    
    
    nsIScriptContext* scx = GetScriptContextFromJSContext(cx);
    JS::RootedObject global(cx, scx ? scx->GetNativeGlobal() : nullptr);
    if (!global || !js::GetObjectParent(global)) {
      continue;
    }
    
    global = JS_ObjectToInnerObject(cx, global);
    MOZ_ASSERT(!js::GetObjectParent(global));
    if (JS::GCThingIsMarkedGray(global) &&
        !js::IsSystemCompartment(js::GetObjectCompartment(global))) {
      return true;
    }
  }
  return false;
}

void
CycleCollectedJSRuntime::FixWeakMappingGrayBits() const
{
  FixWeakMappingGrayBitsTracer fixer(mJSRuntime);
  fixer.FixAll();
}

bool
CycleCollectedJSRuntime::NeedCollect() const
{
  return !js::AreGCGrayBitsValid(mJSRuntime);
}

void
CycleCollectedJSRuntime::Collect(uint32_t aReason) const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  MOZ_ASSERT(aReason < JS::gcreason::NUM_REASONS);
  JS::gcreason::Reason gcreason = static_cast<JS::gcreason::Reason>(aReason);

  JS::PrepareForFullGC(mJSRuntime);
  JS::GCForReason(mJSRuntime, gcreason);
}
