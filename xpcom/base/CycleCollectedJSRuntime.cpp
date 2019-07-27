























































#include "mozilla/CycleCollectedJSRuntime.h"
#include <algorithm>
#include "mozilla/ArrayUtils.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMJSClass.h"
#include "mozilla/dom/ScriptSettings.h"
#include "jsprf.h"
#include "nsCycleCollectionNoteRootCallback.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCycleCollector.h"
#include "nsDOMJSUtils.h"
#include "nsJSUtils.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#include "nsIException.h"
#include "nsThreadUtils.h"
#include "xpcpublic.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {

struct DeferredFinalizeFunctionHolder
{
  DeferredFinalizeFunction run;
  void* data;
};

class IncrementalFinalizeRunnable : public nsRunnable
{
  typedef nsAutoTArray<DeferredFinalizeFunctionHolder, 16> DeferredFinalizeArray;
  typedef CycleCollectedJSRuntime::DeferredFinalizerTable DeferredFinalizerTable;

  CycleCollectedJSRuntime* mRuntime;
  nsTArray<nsISupports*> mSupports;
  DeferredFinalizeArray mDeferredFinalizeFunctions;
  uint32_t mFinalizeFunctionToRun;
  bool mReleasing;

  static const PRTime SliceMillis = 10; 

  static PLDHashOperator
  DeferredFinalizerEnumerator(DeferredFinalizeFunction& aFunction,
                              void*& aData,
                              void* aClosure);

public:
  IncrementalFinalizeRunnable(CycleCollectedJSRuntime* aRt,
                              nsTArray<nsISupports*>& aMSupports,
                              DeferredFinalizerTable& aFinalizerTable);
  virtual ~IncrementalFinalizeRunnable();

  void ReleaseNow(bool aLimited);

  NS_DECL_NSIRUNNABLE
};

} 

static void
TraceWeakMappingChild(JSTracer* aTrc, void** aThingp, JSGCTraceKind aKind);

struct NoteWeakMapChildrenTracer : public JSTracer
{
  NoteWeakMapChildrenTracer(JSRuntime* aRt,
                            nsCycleCollectionNoteRootCallback& aCb)
    : JSTracer(aRt, TraceWeakMappingChild), mCb(aCb), mTracedAny(false),
      mMap(nullptr), mKey(JS::GCCellPtr::NullPtr()), mKeyDelegate(nullptr)
  {
  }
  nsCycleCollectionNoteRootCallback& mCb;
  bool mTracedAny;
  JSObject* mMap;
  JS::GCCellPtr mKey;
  JSObject* mKeyDelegate;
};

static void
TraceWeakMappingChild(JSTracer* aTrc, void** aThingp, JSGCTraceKind aKind)
{
  MOZ_ASSERT(aTrc->callback == TraceWeakMappingChild);
  NoteWeakMapChildrenTracer* tracer =
    static_cast<NoteWeakMapChildrenTracer*>(aTrc);
  JS::GCCellPtr thing(*aThingp, aKind);

  if (thing.isString()) {
    return;
  }

  if (!JS::GCThingIsMarkedGray(thing) && !tracer->mCb.WantAllTraces()) {
    return;
  }

  if (AddToCCKind(thing.kind())) {
    tracer->mCb.NoteWeakMapping(tracer->mMap, tracer->mKey,
                                tracer->mKeyDelegate, thing);
    tracer->mTracedAny = true;
  } else {
    JS_TraceChildren(aTrc, thing.asCell(), thing.kind());
  }
}

struct NoteWeakMapsTracer : public js::WeakMapTracer
{
  NoteWeakMapsTracer(JSRuntime* aRt, js::WeakMapTraceCallback aCb,
                     nsCycleCollectionNoteRootCallback& aCccb)
    : js::WeakMapTracer(aRt, aCb), mCb(aCccb), mChildTracer(aRt, aCccb)
  {
  }
  nsCycleCollectionNoteRootCallback& mCb;
  NoteWeakMapChildrenTracer mChildTracer;
};

static void
TraceWeakMapping(js::WeakMapTracer* aTrc, JSObject* aMap,
                 JS::GCCellPtr aKey, JS::GCCellPtr aValue)
{
  MOZ_ASSERT(aTrc->callback == TraceWeakMapping);
  NoteWeakMapsTracer* tracer = static_cast<NoteWeakMapsTracer*>(aTrc);

  
  if ((!aKey || !JS::GCThingIsMarkedGray(aKey)) &&
      MOZ_LIKELY(!tracer->mCb.WantAllTraces())) {
    if (!aValue || !JS::GCThingIsMarkedGray(aValue) || aValue.isString()) {
      return;
    }
  }

  
  
  
  
  MOZ_ASSERT(AddToCCKind(aKey.kind()));

  
  
  
  
  if (!AddToCCKind(aKey.kind())) {
    aKey = JS::GCCellPtr::NullPtr();
  }

  JSObject* kdelegate = nullptr;
  if (aKey.isObject()) {
    kdelegate = js::GetWeakmapKeyDelegate(aKey.toObject());
  }

  if (AddToCCKind(aValue.kind())) {
    tracer->mCb.NoteWeakMapping(aMap, aKey, kdelegate, aValue);
  } else {
    tracer->mChildTracer.mTracedAny = false;
    tracer->mChildTracer.mMap = aMap;
    tracer->mChildTracer.mKey = aKey;
    tracer->mChildTracer.mKeyDelegate = kdelegate;

    if (aValue.isString()) {
      JS_TraceChildren(&tracer->mChildTracer, aValue.asCell(), aValue.kind());
    }

    
    
    if (!tracer->mChildTracer.mTracedAny &&
        aKey && JS::GCThingIsMarkedGray(aKey) && kdelegate) {
      tracer->mCb.NoteWeakMapping(aMap, aKey, kdelegate,
                                  JS::GCCellPtr::NullPtr());
    }
  }
}


struct FixWeakMappingGrayBitsTracer : public js::WeakMapTracer
{
  explicit FixWeakMappingGrayBitsTracer(JSRuntime* aRt)
    : js::WeakMapTracer(aRt, FixWeakMappingGrayBits)
  {
  }

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
  FixWeakMappingGrayBits(js::WeakMapTracer* aTrc, JSObject* aMap,
                         JS::GCCellPtr aKey, JS::GCCellPtr aValue)
  {
    FixWeakMappingGrayBitsTracer* tracer =
      static_cast<FixWeakMappingGrayBitsTracer*>(aTrc);

    
    bool delegateMightNeedMarking = aKey && JS::GCThingIsMarkedGray(aKey);
    bool valueMightNeedMarking = aValue && JS::GCThingIsMarkedGray(aValue) &&
                                 aValue.kind() != JSTRACE_STRING;
    if (!delegateMightNeedMarking && !valueMightNeedMarking) {
      return;
    }

    if (!AddToCCKind(aKey.kind())) {
      aKey = JS::GCCellPtr::NullPtr();
    }

    if (delegateMightNeedMarking && aKey.isObject()) {
      JSObject* kdelegate = js::GetWeakmapKeyDelegate(aKey.toObject());
      if (kdelegate && !JS::ObjectIsMarkedGray(kdelegate)) {
        if (JS::UnmarkGrayGCThingRecursively(aKey)) {
          tracer->mAnyMarked = true;
        }
      }
    }

    if (aValue && JS::GCThingIsMarkedGray(aValue) &&
        (!aKey || !JS::GCThingIsMarkedGray(aKey)) &&
        (!aMap || !JS::ObjectIsMarkedGray(aMap)) &&
        aValue.kind() != JSTRACE_SHAPE) {
      if (JS::UnmarkGrayGCThingRecursively(aValue)) {
        tracer->mAnyMarked = true;
      }
    }
  }

  bool mAnyMarked;
};

struct Closure
{
  explicit Closure(nsCycleCollectionNoteRootCallback* aCb)
    : mCycleCollectionEnabled(true), mCb(aCb)
  {
  }

  bool mCycleCollectionEnabled;
  nsCycleCollectionNoteRootCallback* mCb;
};

static void
CheckParticipatesInCycleCollection(JS::GCCellPtr aThing, const char* aName,
                                   void* aClosure)
{
  Closure* closure = static_cast<Closure*>(aClosure);

  if (closure->mCycleCollectionEnabled) {
    return;
  }

  if (AddToCCKind(aThing.kind()) && JS::GCThingIsMarkedGray(aThing)) {
    closure->mCycleCollectionEnabled = true;
  }
}

static PLDHashOperator
NoteJSHolder(void* aHolder, nsScriptObjectTracer*& aTracer, void* aArg)
{
  Closure* closure = static_cast<Closure*>(aArg);

  bool noteRoot;
  if (MOZ_UNLIKELY(closure->mCb->WantAllTraces())) {
    noteRoot = true;
  } else {
    closure->mCycleCollectionEnabled = false;
    aTracer->Trace(aHolder,
                   TraceCallbackFunc(CheckParticipatesInCycleCollection),
                   closure);
    noteRoot = closure->mCycleCollectionEnabled;
  }

  if (noteRoot) {
    closure->mCb->NoteNativeRoot(aHolder, aTracer);
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
JSGCThingParticipant::Traverse(void* aPtr,
                               nsCycleCollectionTraversalCallback& aCb)
{
  auto runtime = reinterpret_cast<CycleCollectedJSRuntime*>(
    reinterpret_cast<char*>(this) - offsetof(CycleCollectedJSRuntime,
                                             mGCThingCycleCollectorGlobal));

  JS::GCCellPtr cellPtr(aPtr, js::GCThingTraceKind(aPtr));
  runtime->TraverseGCThing(CycleCollectedJSRuntime::TRAVERSE_FULL, cellPtr, aCb);
  return NS_OK;
}



static JSGCThingParticipant sGCThingCycleCollectorGlobal;

NS_IMETHODIMP
JSZoneParticipant::Traverse(void* aPtr, nsCycleCollectionTraversalCallback& aCb)
{
  auto runtime = reinterpret_cast<CycleCollectedJSRuntime*>(
    reinterpret_cast<char*>(this) - offsetof(CycleCollectedJSRuntime,
                                             mJSZoneCycleCollectorGlobal));

  MOZ_ASSERT(!aCb.WantAllTraces());
  JS::Zone* zone = static_cast<JS::Zone*>(aPtr);

  runtime->TraverseZone(zone, aCb);
  return NS_OK;
}

static void
NoteJSChildTracerShim(JSTracer* aTrc, void** aThingp, JSGCTraceKind aTraceKind);

struct TraversalTracer : public JSTracer
{
  TraversalTracer(JSRuntime* aRt, nsCycleCollectionTraversalCallback& aCb)
    : JSTracer(aRt, NoteJSChildTracerShim, DoNotTraceWeakMaps), mCb(aCb)
  {
  }
  nsCycleCollectionTraversalCallback& mCb;
};

static void
NoteJSChild(JSTracer* aTrc, void* aThing, JSGCTraceKind aTraceKind)
{
  JS::GCCellPtr thing(aThing, aTraceKind);
  TraversalTracer* tracer = static_cast<TraversalTracer*>(aTrc);

  
  if (!JS::GCThingIsMarkedGray(thing) && !tracer->mCb.WantAllTraces()) {
    return;
  }

  







  if (AddToCCKind(aTraceKind)) {
    if (MOZ_UNLIKELY(tracer->mCb.WantDebugInfo())) {
      
      if (tracer->debugPrinter()) {
        char buffer[200];
        tracer->debugPrinter()(aTrc, buffer, sizeof(buffer));
        tracer->mCb.NoteNextEdgeName(buffer);
      } else if (tracer->debugPrintIndex() != (size_t)-1) {
        char buffer[200];
        JS_snprintf(buffer, sizeof(buffer), "%s[%lu]",
                    static_cast<const char*>(tracer->debugPrintArg()),
                    tracer->debugPrintIndex());
        tracer->mCb.NoteNextEdgeName(buffer);
      } else {
        tracer->mCb.NoteNextEdgeName(static_cast<const char*>(tracer->debugPrintArg()));
      }
    }
    if (thing.isObject()) {
      tracer->mCb.NoteJSObject(thing.toObject());
    } else {
      tracer->mCb.NoteJSScript(thing.toScript());
    }
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
NoteJSChildGrayWrapperShim(void* aData, JS::GCCellPtr aThing)
{
  TraversalTracer* trc = static_cast<TraversalTracer*>(aData);
  NoteJSChild(trc, aThing.asCell(), aThing.kind());
}
























static const JSZoneParticipant sJSZoneCycleCollectorGlobal;

CycleCollectedJSRuntime::CycleCollectedJSRuntime(JSRuntime* aParentRuntime,
                                                 uint32_t aMaxBytes,
                                                 uint32_t aMaxNurseryBytes)
  : mGCThingCycleCollectorGlobal(sGCThingCycleCollectorGlobal)
  , mJSZoneCycleCollectorGlobal(sJSZoneCycleCollectorGlobal)
  , mJSRuntime(nullptr)
  , mJSHolders(256)
  , mOutOfMemoryState(OOMState::OK)
  , mLargeAllocationFailureState(OOMState::OK)
{
  mozilla::dom::InitScriptSettings();

  mJSRuntime = JS_NewRuntime(aMaxBytes, aMaxNurseryBytes, aParentRuntime);
  if (!mJSRuntime) {
    MOZ_CRASH();
  }

  if (!JS_AddExtraGCRootsTracer(mJSRuntime, TraceBlackJS, this)) {
    MOZ_CRASH();
  }
  JS_SetGrayGCRootsTracer(mJSRuntime, TraceGrayJS, this);
  JS_SetGCCallback(mJSRuntime, GCCallback, this);
  JS::SetOutOfMemoryCallback(mJSRuntime, OutOfMemoryCallback, this);
  JS::SetLargeAllocationFailureCallback(mJSRuntime,
                                        LargeAllocationFailureCallback, this);
  JS_SetContextCallback(mJSRuntime, ContextCallback, this);
  JS_SetDestroyZoneCallback(mJSRuntime, XPCStringConvert::FreeZoneCache);
  JS_SetSweepZoneCallback(mJSRuntime, XPCStringConvert::ClearZoneCache);

  static js::DOMCallbacks DOMcallbacks = {
    InstanceClassHasProtoAtDepth
  };
  SetDOMCallbacks(mJSRuntime, &DOMcallbacks);

  nsCycleCollector_registerJSRuntime(this);
}

CycleCollectedJSRuntime::~CycleCollectedJSRuntime()
{
  MOZ_ASSERT(mJSRuntime);
  MOZ_ASSERT(!mDeferredFinalizerTable.Count());
  MOZ_ASSERT(!mDeferredSupports.Length());

  
  mPendingException = nullptr;

  JS_DestroyRuntime(mJSRuntime);
  mJSRuntime = nullptr;
  nsCycleCollector_forgetJSRuntime();

  mozilla::dom::DestroyScriptSettings();
}

size_t
CycleCollectedJSRuntime::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;

  
  
  n += mJSHolders.SizeOfExcludingThis(nullptr, aMallocSizeOf);

  return n;
}

static PLDHashOperator
UnmarkJSHolder(void* aHolder, nsScriptObjectTracer*& aTracer, void* aArg)
{
  aTracer->CanSkip(aHolder, true);
  return PL_DHASH_NEXT;
}

void
CycleCollectedJSRuntime::UnmarkSkippableJSHolders()
{
  mJSHolders.Enumerate(UnmarkJSHolder, nullptr);
}

void
CycleCollectedJSRuntime::DescribeGCThing(bool aIsMarked, JS::GCCellPtr aThing,
                                         nsCycleCollectionTraversalCallback& aCb) const
{
  if (!aCb.WantDebugInfo()) {
    aCb.DescribeGCedNode(aIsMarked, "JS Object");
    return;
  }

  char name[72];
  uint64_t compartmentAddress = 0;
  if (aThing.isObject()) {
    JSObject* obj = aThing.toObject();
    compartmentAddress = (uint64_t)js::GetObjectCompartment(obj);
    const js::Class* clasp = js::GetObjectClass(obj);

    
    if (DescribeCustomObjects(obj, clasp, name)) {
      
    } else if (js::IsFunctionObject(obj)) {
      JSFunction* fun = JS_GetObjectFunction(obj);
      JSString* str = JS_GetFunctionDisplayId(fun);
      if (str) {
        JSFlatString* flat = JS_ASSERT_STRING_IS_FLAT(str);
        nsAutoString chars;
        AssignJSFlatString(chars, flat);
        NS_ConvertUTF16toUTF8 fname(chars);
        JS_snprintf(name, sizeof(name),
                    "JS Object (Function - %s)", fname.get());
      } else {
        JS_snprintf(name, sizeof(name), "JS Object (Function)");
      }
    } else {
      JS_snprintf(name, sizeof(name), "JS Object (%s)", clasp->name);
    }
  } else {
    JS_snprintf(name, sizeof(name), "JS %s", JS::GCTraceKindToAscii(aThing.kind()));
  }

  
  aCb.DescribeGCedNode(aIsMarked, name, compartmentAddress);
}

void
CycleCollectedJSRuntime::NoteGCThingJSChildren(JS::GCCellPtr aThing,
                                               nsCycleCollectionTraversalCallback& aCb) const
{
  MOZ_ASSERT(mJSRuntime);
  TraversalTracer trc(mJSRuntime, aCb);
  JS_TraceChildren(&trc, aThing.asCell(), aThing.kind());
}

void
CycleCollectedJSRuntime::NoteGCThingXPCOMChildren(const js::Class* aClasp,
                                                  JSObject* aObj,
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
    const DOMJSClass* domClass = GetDOMClass(aObj);
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
CycleCollectedJSRuntime::TraverseGCThing(TraverseSelect aTs, JS::GCCellPtr aThing,
                                         nsCycleCollectionTraversalCallback& aCb)
{
  bool isMarkedGray = JS::GCThingIsMarkedGray(aThing);

  if (aTs == TRAVERSE_FULL) {
    DescribeGCThing(!isMarkedGray, aThing, aCb);
  }

  
  
  
  
  if (!isMarkedGray && !aCb.WantAllTraces()) {
    return;
  }

  if (aTs == TRAVERSE_FULL) {
    NoteGCThingJSChildren(aThing, aCb);
  }

  if (aThing.isObject()) {
    JSObject* obj = aThing.toObject();
    NoteGCThingXPCOMChildren(js::GetObjectClass(obj), obj, aCb);
  }
}

struct TraverseObjectShimClosure
{
  nsCycleCollectionTraversalCallback& cb;
  CycleCollectedJSRuntime* self;
};

void
CycleCollectedJSRuntime::TraverseZone(JS::Zone* aZone,
                                      nsCycleCollectionTraversalCallback& aCb)
{
  









  aCb.DescribeGCedNode(false, "JS Zone");

  







  TraversalTracer trc(mJSRuntime, aCb);
  js::VisitGrayWrapperTargets(aZone, NoteJSChildGrayWrapperShim, &trc);

  



  TraverseObjectShimClosure closure = { aCb, this };
  js::IterateGrayObjects(aZone, TraverseObjectShim, &closure);
}

 void
CycleCollectedJSRuntime::TraverseObjectShim(void* aData, JS::GCCellPtr aThing)
{
  TraverseObjectShimClosure* closure =
    static_cast<TraverseObjectShimClosure*>(aData);

  MOZ_ASSERT(aThing.isObject());
  closure->self->TraverseGCThing(CycleCollectedJSRuntime::TRAVERSE_CPP,
                                 aThing, closure->cb);
}

void
CycleCollectedJSRuntime::TraverseNativeRoots(nsCycleCollectionNoteRootCallback& aCb)
{
  
  
  TraverseAdditionalNativeRoots(aCb);

  Closure closure(&aCb);
  mJSHolders.Enumerate(NoteJSHolder, &closure);
}

 void
CycleCollectedJSRuntime::TraceBlackJS(JSTracer* aTracer, void* aData)
{
  CycleCollectedJSRuntime* self = static_cast<CycleCollectedJSRuntime*>(aData);

  self->TraceNativeBlackRoots(aTracer);
}

 void
CycleCollectedJSRuntime::TraceGrayJS(JSTracer* aTracer, void* aData)
{
  CycleCollectedJSRuntime* self = static_cast<CycleCollectedJSRuntime*>(aData);

  
  self->TraceNativeGrayRoots(aTracer);
}

 void
CycleCollectedJSRuntime::GCCallback(JSRuntime* aRuntime,
                                    JSGCStatus aStatus,
                                    void* aData)
{
  CycleCollectedJSRuntime* self = static_cast<CycleCollectedJSRuntime*>(aData);

  MOZ_ASSERT(aRuntime == self->Runtime());

  self->OnGC(aStatus);
}

 void
CycleCollectedJSRuntime::OutOfMemoryCallback(JSContext* aContext,
                                             void* aData)
{
  CycleCollectedJSRuntime* self = static_cast<CycleCollectedJSRuntime*>(aData);

  MOZ_ASSERT(JS_GetRuntime(aContext) == self->Runtime());

  self->OnOutOfMemory();
}

 void
CycleCollectedJSRuntime::LargeAllocationFailureCallback(void* aData)
{
  CycleCollectedJSRuntime* self = static_cast<CycleCollectedJSRuntime*>(aData);

  self->OnLargeAllocationFailure();
}

 bool
CycleCollectedJSRuntime::ContextCallback(JSContext* aContext,
                                         unsigned aOperation,
                                         void* aData)
{
  CycleCollectedJSRuntime* self = static_cast<CycleCollectedJSRuntime*>(aData);

  MOZ_ASSERT(JS_GetRuntime(aContext) == self->Runtime());

  return self->CustomContextCallback(aContext, aOperation);
}

struct JsGcTracer : public TraceCallbacks
{
  virtual void Trace(JS::Heap<JS::Value>* aPtr, const char* aName,
                     void* aClosure) const MOZ_OVERRIDE
  {
    JS_CallValueTracer(static_cast<JSTracer*>(aClosure), aPtr, aName);
  }
  virtual void Trace(JS::Heap<jsid>* aPtr, const char* aName,
                     void* aClosure) const MOZ_OVERRIDE
  {
    JS_CallIdTracer(static_cast<JSTracer*>(aClosure), aPtr, aName);
  }
  virtual void Trace(JS::Heap<JSObject*>* aPtr, const char* aName,
                     void* aClosure) const MOZ_OVERRIDE
  {
    JS_CallObjectTracer(static_cast<JSTracer*>(aClosure), aPtr, aName);
  }
  virtual void Trace(JS::TenuredHeap<JSObject*>* aPtr, const char* aName,
                     void* aClosure) const MOZ_OVERRIDE
  {
    JS_CallTenuredObjectTracer(static_cast<JSTracer*>(aClosure), aPtr, aName);
  }
  virtual void Trace(JS::Heap<JSString*>* aPtr, const char* aName,
                     void* aClosure) const MOZ_OVERRIDE
  {
    JS_CallStringTracer(static_cast<JSTracer*>(aClosure), aPtr, aName);
  }
  virtual void Trace(JS::Heap<JSScript*>* aPtr, const char* aName,
                     void* aClosure) const MOZ_OVERRIDE
  {
    JS_CallScriptTracer(static_cast<JSTracer*>(aClosure), aPtr, aName);
  }
  virtual void Trace(JS::Heap<JSFunction*>* aPtr, const char* aName,
                     void* aClosure) const MOZ_OVERRIDE
  {
    JS_CallFunctionTracer(static_cast<JSTracer*>(aClosure), aPtr, aName);
  }
};

static PLDHashOperator
TraceJSHolder(void* aHolder, nsScriptObjectTracer*& aTracer, void* aArg)
{
  aTracer->Trace(aHolder, JsGcTracer(), aArg);

  return PL_DHASH_NEXT;
}

void
mozilla::TraceScriptHolder(nsISupports* aHolder, JSTracer* aTracer)
{
  nsXPCOMCycleCollectionParticipant* participant = nullptr;
  CallQueryInterface(aHolder, &participant);
  participant->Trace(aHolder, JsGcTracer(), aTracer);
}

void
CycleCollectedJSRuntime::TraceNativeGrayRoots(JSTracer* aTracer)
{
  
  
  TraceAdditionalNativeGrayRoots(aTracer);

  mJSHolders.Enumerate(TraceJSHolder, aTracer);
}

void
CycleCollectedJSRuntime::AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer)
{
  mJSHolders.Put(aHolder, aTracer);
}

struct ClearJSHolder : TraceCallbacks
{
  virtual void Trace(JS::Heap<JS::Value>* aPtr, const char*, void*) const MOZ_OVERRIDE
  {
    *aPtr = JSVAL_VOID;
  }

  virtual void Trace(JS::Heap<jsid>* aPtr, const char*, void*) const MOZ_OVERRIDE
  {
    *aPtr = JSID_VOID;
  }

  virtual void Trace(JS::Heap<JSObject*>* aPtr, const char*, void*) const MOZ_OVERRIDE
  {
    *aPtr = nullptr;
  }

  virtual void Trace(JS::TenuredHeap<JSObject*>* aPtr, const char*, void*) const MOZ_OVERRIDE
  {
    *aPtr = nullptr;
  }

  virtual void Trace(JS::Heap<JSString*>* aPtr, const char*, void*) const MOZ_OVERRIDE
  {
    *aPtr = nullptr;
  }

  virtual void Trace(JS::Heap<JSScript*>* aPtr, const char*, void*) const MOZ_OVERRIDE
  {
    *aPtr = nullptr;
  }

  virtual void Trace(JS::Heap<JSFunction*>* aPtr, const char*, void*) const MOZ_OVERRIDE
  {
    *aPtr = nullptr;
  }
};

void
CycleCollectedJSRuntime::RemoveJSHolder(void* aHolder)
{
  nsScriptObjectTracer* tracer = mJSHolders.Get(aHolder);
  if (!tracer) {
    return;
  }
  tracer->Trace(aHolder, ClearJSHolder(), nullptr);
  mJSHolders.Remove(aHolder);
}

#ifdef DEBUG
bool
CycleCollectedJSRuntime::IsJSHolder(void* aHolder)
{
  return mJSHolders.Get(aHolder, nullptr);
}

static void
AssertNoGcThing(JS::GCCellPtr aGCThing, const char* aName, void* aClosure)
{
  MOZ_ASSERT(!aGCThing);
}

void
CycleCollectedJSRuntime::AssertNoObjectsToTrace(void* aPossibleJSHolder)
{
  nsScriptObjectTracer* tracer = mJSHolders.Get(aPossibleJSHolder);
  if (tracer) {
    tracer->Trace(aPossibleJSHolder, TraceCallbackFunc(AssertNoGcThing), nullptr);
  }
}
#endif

already_AddRefed<nsIException>
CycleCollectedJSRuntime::GetPendingException() const
{
  nsCOMPtr<nsIException> out = mPendingException;
  return out.forget();
}

void
CycleCollectedJSRuntime::SetPendingException(nsIException* aException)
{
  mPendingException = aException;
}

nsTArray<nsRefPtr<nsIRunnable>>&
CycleCollectedJSRuntime::GetPromiseMicroTaskQueue()
{
  return mPromiseMicroTaskQueue;
}

nsCycleCollectionParticipant*
CycleCollectedJSRuntime::GCThingParticipant()
{
  return &mGCThingCycleCollectorGlobal;
}

nsCycleCollectionParticipant*
CycleCollectedJSRuntime::ZoneParticipant()
{
  return &mJSZoneCycleCollectorGlobal;
}

nsresult
CycleCollectedJSRuntime::TraverseRoots(nsCycleCollectionNoteRootCallback& aCb)
{
  TraverseNativeRoots(aCb);

  NoteWeakMapsTracer trc(mJSRuntime, TraceWeakMapping, aCb);
  js::TraceWeakMaps(&trc);

  return NS_OK;
}






bool
CycleCollectedJSRuntime::UsefulToMergeZones() const
{
  if (!NS_IsMainThread()) {
    return false;
  }

  JSContext* iter = nullptr;
  JSContext* cx;
  JSAutoRequest ar(nsContentUtils::GetSafeJSContext());
  while ((cx = JS_ContextIterator(mJSRuntime, &iter))) {
    
    nsIScriptContext* scx = GetScriptContextFromJSContext(cx);
    JS::RootedObject obj(cx, scx ? scx->GetWindowProxyPreserveColor() : nullptr);
    if (!obj) {
      continue;
    }
    MOZ_ASSERT(js::IsOuterObject(obj));
    
    obj = JS_ObjectToInnerObject(cx, obj);
    MOZ_ASSERT(!js::GetObjectParent(obj));
    if (JS::ObjectIsMarkedGray(obj) &&
        !js::IsSystemCompartment(js::GetObjectCompartment(obj))) {
      return true;
    }
  }
  return false;
}

void
CycleCollectedJSRuntime::FixWeakMappingGrayBits() const
{
  MOZ_ASSERT(!JS::IsIncrementalGCInProgress(mJSRuntime),
             "Don't call FixWeakMappingGrayBits during a GC.");
  FixWeakMappingGrayBitsTracer fixer(mJSRuntime);
  fixer.FixAll();
}

bool
CycleCollectedJSRuntime::AreGCGrayBitsValid() const
{
  return js::AreGCGrayBitsValid(mJSRuntime);
}

void
CycleCollectedJSRuntime::GarbageCollect(uint32_t aReason) const
{
  MOZ_ASSERT(aReason < JS::gcreason::NUM_REASONS);
  JS::gcreason::Reason gcreason = static_cast<JS::gcreason::Reason>(aReason);

  JS::PrepareForFullGC(mJSRuntime);
  JS::GCForReason(mJSRuntime, GC_NORMAL, gcreason);
}

void
CycleCollectedJSRuntime::DeferredFinalize(DeferredFinalizeAppendFunction aAppendFunc,
                                          DeferredFinalizeFunction aFunc,
                                          void* aThing)
{
  void* thingArray = nullptr;
  bool hadThingArray = mDeferredFinalizerTable.Get(aFunc, &thingArray);

  thingArray = aAppendFunc(thingArray, aThing);
  if (!hadThingArray) {
    mDeferredFinalizerTable.Put(aFunc, thingArray);
  }
}

void
CycleCollectedJSRuntime::DeferredFinalize(nsISupports* aSupports)
{
#ifdef MOZ_CRASHREPORTER
  
  
  
  
  size_t oldLength = mDeferredSupports.Length();
  nsISupports** itemPtr = mDeferredSupports.AppendElement(aSupports);
  size_t newLength = mDeferredSupports.Length();
  nsISupports* item = mDeferredSupports.ElementAt(newLength - 1);
  if ((newLength - oldLength != 1) || !itemPtr ||
      (*itemPtr != aSupports) || (item != aSupports)) {
    nsAutoCString debugInfo;
    debugInfo.AppendPrintf("\noldLength [%u], newLength [%u], aSupports [%p], item [%p], itemPtr [%p], *itemPtr [%p]",
                           oldLength, newLength, aSupports, item, itemPtr, itemPtr ? *itemPtr : NULL);
    #define CRASH_MESSAGE "nsTArray::AppendElement() failed!"
    CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("\nBug 997908: ") +
                                               NS_LITERAL_CSTRING(CRASH_MESSAGE));
    CrashReporter::AppendAppNotesToCrashReport(debugInfo);
    MOZ_CRASH(CRASH_MESSAGE);
    #undef CRASH_MESSAGE
  }
#else
  mDeferredSupports.AppendElement(aSupports);
#endif
}

void
CycleCollectedJSRuntime::DumpJSHeap(FILE* aFile)
{
  js::DumpHeapComplete(Runtime(), aFile, js::CollectNurseryBeforeDump);
}


bool
ReleaseSliceNow(uint32_t aSlice, void* aData)
{
  MOZ_ASSERT(aSlice > 0, "nonsensical/useless call with slice == 0");
  nsTArray<nsISupports*>* items = static_cast<nsTArray<nsISupports*>*>(aData);

  uint32_t length = items->Length();
  aSlice = std::min(aSlice, length);
  for (uint32_t i = length; i > length - aSlice; --i) {
    
    uint32_t lastItemIdx = i - 1;

    nsISupports* wrapper = items->ElementAt(lastItemIdx);
    items->RemoveElementAt(lastItemIdx);
    NS_IF_RELEASE(wrapper);
  }

  return items->IsEmpty();
}

 PLDHashOperator
IncrementalFinalizeRunnable::DeferredFinalizerEnumerator(DeferredFinalizeFunction& aFunction,
                                                         void*& aData,
                                                         void* aClosure)
{
  DeferredFinalizeArray* array = static_cast<DeferredFinalizeArray*>(aClosure);

  DeferredFinalizeFunctionHolder* function = array->AppendElement();
  function->run = aFunction;
  function->data = aData;

  return PL_DHASH_REMOVE;
}

IncrementalFinalizeRunnable::IncrementalFinalizeRunnable(CycleCollectedJSRuntime* aRt,
                                                         nsTArray<nsISupports*>& aSupports,
                                                         DeferredFinalizerTable& aFinalizers)
  : mRuntime(aRt)
  , mFinalizeFunctionToRun(0)
  , mReleasing(false)
{
  this->mSupports.SwapElements(aSupports);
  DeferredFinalizeFunctionHolder* function =
    mDeferredFinalizeFunctions.AppendElement();
  function->run = ReleaseSliceNow;
  function->data = &this->mSupports;

  
  aFinalizers.Enumerate(DeferredFinalizerEnumerator, &mDeferredFinalizeFunctions);
}

IncrementalFinalizeRunnable::~IncrementalFinalizeRunnable()
{
  MOZ_ASSERT(this != mRuntime->mFinalizeRunnable);
}

void
IncrementalFinalizeRunnable::ReleaseNow(bool aLimited)
{
  if (mReleasing) {
    MOZ_ASSERT(false, "Try to avoid re-entering ReleaseNow!");
    return;
  }
  {
    mozilla::AutoRestore<bool> ar(mReleasing);
    mReleasing = true;
    MOZ_ASSERT(mDeferredFinalizeFunctions.Length() != 0,
               "We should have at least ReleaseSliceNow to run");
    MOZ_ASSERT(mFinalizeFunctionToRun < mDeferredFinalizeFunctions.Length(),
               "No more finalizers to run?");

    TimeDuration sliceTime = TimeDuration::FromMilliseconds(SliceMillis);
    TimeStamp started = TimeStamp::Now();
    bool timeout = false;
    do {
      const DeferredFinalizeFunctionHolder& function =
        mDeferredFinalizeFunctions[mFinalizeFunctionToRun];
      if (aLimited) {
        bool done = false;
        while (!timeout && !done) {
          



          done = function.run(100, function.data);
          timeout = TimeStamp::Now() - started >= sliceTime;
        }
        if (done) {
          ++mFinalizeFunctionToRun;
        }
        if (timeout) {
          break;
        }
      } else {
        function.run(UINT32_MAX, function.data);
        ++mFinalizeFunctionToRun;
      }
    } while (mFinalizeFunctionToRun < mDeferredFinalizeFunctions.Length());
  }

  if (mFinalizeFunctionToRun == mDeferredFinalizeFunctions.Length()) {
    MOZ_ASSERT(mRuntime->mFinalizeRunnable == this);
    mDeferredFinalizeFunctions.Clear();
    
    mRuntime->mFinalizeRunnable = nullptr;
  }
}

NS_IMETHODIMP
IncrementalFinalizeRunnable::Run()
{
  if (mRuntime->mFinalizeRunnable != this) {
    
    MOZ_ASSERT(!mSupports.Length());
    MOZ_ASSERT(!mDeferredFinalizeFunctions.Length());
    return NS_OK;
  }

  ReleaseNow(true);

  if (mDeferredFinalizeFunctions.Length()) {
    nsresult rv = NS_DispatchToCurrentThread(this);
    if (NS_FAILED(rv)) {
      ReleaseNow(false);
    }
  }

  return NS_OK;
}

void
CycleCollectedJSRuntime::FinalizeDeferredThings(DeferredFinalizeType aType)
{
  






  if (mFinalizeRunnable) {
    mFinalizeRunnable->ReleaseNow(false);
    if (mFinalizeRunnable) {
      
      
      return;
    }
  }
  mFinalizeRunnable = new IncrementalFinalizeRunnable(this,
                                                      mDeferredSupports,
                                                      mDeferredFinalizerTable);

  
  MOZ_ASSERT(!mDeferredSupports.Length());
  MOZ_ASSERT(!mDeferredFinalizerTable.Count());

  if (aType == FinalizeIncrementally) {
    NS_DispatchToCurrentThread(mFinalizeRunnable);
  } else {
    mFinalizeRunnable->ReleaseNow(false);
    MOZ_ASSERT(!mFinalizeRunnable);
  }
}

void
CycleCollectedJSRuntime::AnnotateAndSetOutOfMemory(OOMState* aStatePtr,
                                                   OOMState aNewState)
{
  *aStatePtr = aNewState;
#ifdef MOZ_CRASHREPORTER
  CrashReporter::AnnotateCrashReport(aStatePtr == &mOutOfMemoryState
                                     ? NS_LITERAL_CSTRING("JSOutOfMemory")
                                     : NS_LITERAL_CSTRING("JSLargeAllocationFailure"),
                                     aNewState == OOMState::Reporting
                                     ? NS_LITERAL_CSTRING("Reporting")
                                     : aNewState == OOMState::Reported
                                     ? NS_LITERAL_CSTRING("Reported")
                                     : NS_LITERAL_CSTRING("Recovered"));
#endif
}

void
CycleCollectedJSRuntime::OnGC(JSGCStatus aStatus)
{
  switch (aStatus) {
    case JSGC_BEGIN:
      nsCycleCollector_prepareForGarbageCollection();
      break;
    case JSGC_END: {
#ifdef MOZ_CRASHREPORTER
      if (mOutOfMemoryState == OOMState::Reported) {
        AnnotateAndSetOutOfMemory(&mOutOfMemoryState, OOMState::Recovered);
      }
      if (mLargeAllocationFailureState == OOMState::Reported) {
        AnnotateAndSetOutOfMemory(&mLargeAllocationFailureState, OOMState::Recovered);
      }
#endif

      
      FinalizeDeferredThings(JS::WasIncrementalGC(mJSRuntime) ? FinalizeIncrementally :
                                                                FinalizeNow);
      break;
    }
    default:
      MOZ_CRASH();
  }

  CustomGCCallback(aStatus);
}

void
CycleCollectedJSRuntime::OnOutOfMemory()
{
  AnnotateAndSetOutOfMemory(&mOutOfMemoryState, OOMState::Reporting);
  CustomOutOfMemoryCallback();
  AnnotateAndSetOutOfMemory(&mOutOfMemoryState, OOMState::Reported);
}

void
CycleCollectedJSRuntime::OnLargeAllocationFailure()
{
  AnnotateAndSetOutOfMemory(&mLargeAllocationFailureState, OOMState::Reporting);
  CustomLargeAllocationFailureCallback();
  AnnotateAndSetOutOfMemory(&mLargeAllocationFailureState, OOMState::Reported);
}
