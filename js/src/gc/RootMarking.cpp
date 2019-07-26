






#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Util.h"

#include "jsapi.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsprf.h"
#include "jswatchpoint.h"

#include "frontend/Parser.h"
#include "gc/GCInternals.h"
#include "gc/Marking.h"
#ifdef JS_ION
# include "ion/IonMacroAssembler.h"
# include "ion/IonFrameIterator.h"
#endif
#include "js/HashTable.h"
#include "vm/Debugger.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"

#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#endif

using namespace js;
using namespace js::gc;

using mozilla::ArrayEnd;

typedef RootedValueMap::Range RootRange;
typedef RootedValueMap::Entry RootEntry;
typedef RootedValueMap::Enum RootEnum;

#ifdef JSGC_USE_EXACT_ROOTING
static inline void
MarkExactStackRooter(JSTracer *trc, Rooted<void*> rooter, ThingRootKind kind)
{
    void **addr = (void **)rooter->address();
    if (!*addr)
        return;

    switch (kind) {
      case THING_ROOT_OBJECT:      MarkObjectRoot(trc, (JSObject **)addr, "exact-object"); break;
      case THING_ROOT_STRING:      MarkStringRoot(trc, (JSSTring **)addr, "exact-string"); break;
      case THING_ROOT_SCRIPT:      MarkScriptRoot(trc, (JSScript **)addr, "exact-script"); break;
      case THING_ROOT_SHAPE:       MarkShapeRoot(trc, (Shape **)addr, "exact-shape"); break;
      case THING_ROOT_BASE_SHAPE:  MarkBaseShapeRoot(trc, (BaseShape **)addr, "exact-baseshape"); break;
      case THING_ROOT_TYPE:        MarkTypeRoot(trc, (types::Type *)addr, "exact-type"); break;
      case THING_ROOT_TYPE_OBJECT: MarkTypeObjectRoot(trc, (types::TypeObject **)addr, "exact-typeobject"); break;
      case THING_ROOT_VALUE:       MarkValueRoot(trc, (Value *)addr, "exact-value"); break;
      case THING_ROOT_ID:          MarkIdRoot(trc, (jsid *)addr, "exact-id"); break;
      case THING_ROOT_PROPERTY_ID: MarkIdRoot(trc, &((js::PropertyId *)addr)->asId(), "exact-propertyid"); break;
      case THING_ROOT_BINDINGS:    ((Bindings *)addr)->trace(trc); break;
      default: JS_NOT_REACHED("Invalid THING_ROOT kind"); break;
    }
}

static inline void
MarkExactStackRooters(JSTracer *trc, Rooted<void*> rooter, ThingRootKind kind)
{
    Rooted<void*> *rooter = cx->thingGCRooters[i];
    while (rooter) {
        MarkExactStackRoot(trc, rooter, ThingRootKind(i));
        rooter = rooter->previous();
    }
}

static void
MarkExactStackRoots(JSTracer *trc)
{
    for (unsigned i = 0; i < THING_ROOT_LIMIT; i++) {
        for (ContextIter cx(trc->runtime); !cx.done(); cx.next()) {
            MarkExactStackRooters(trc, cx->thingGCRooters[i], ThingRootKind(i));
        }
        MarkExactStackRooters(trc, rt->mainThread.thingGCRooters[i], ThingRootKind(i));
    }
}
#endif 

static inline bool
InFreeList(ArenaHeader *aheader, uintptr_t addr)
{
    if (!aheader->hasFreeThings())
        return false;

    FreeSpan firstSpan(aheader->getFirstFreeSpan());

    for (const FreeSpan *span = &firstSpan;;) {
        
        if (addr < span->first)
            return false;

        




        if (addr <= span->last)
            return true;

        



        span = span->nextSpan();
    }
}

enum ConservativeGCTest
{
    CGCT_VALID,
    CGCT_LOWBITSET, 
    CGCT_NOTARENA,  
    CGCT_OTHERCOMPARTMENT,  
    CGCT_NOTCHUNK,  
    CGCT_FREEARENA, 
    CGCT_NOTLIVE,   
    CGCT_END
};





static inline ConservativeGCTest
IsAddressableGCThing(JSRuntime *rt, uintptr_t w,
                     bool skipUncollectedCompartments,
                     gc::AllocKind *thingKindPtr,
                     ArenaHeader **arenaHeader,
                     void **thing)
{
    






    JS_STATIC_ASSERT(JSID_TYPE_STRING == 0 && JSID_TYPE_OBJECT == 4);
    if (w & 0x3)
        return CGCT_LOWBITSET;

    



    const uintptr_t JSID_PAYLOAD_MASK = ~uintptr_t(JSID_TYPE_MASK);
#if JS_BITS_PER_WORD == 32
    uintptr_t addr = w & JSID_PAYLOAD_MASK;
#elif JS_BITS_PER_WORD == 64
    uintptr_t addr = w & JSID_PAYLOAD_MASK & JSVAL_PAYLOAD_MASK;
#endif

    Chunk *chunk = Chunk::fromAddress(addr);

    if (!rt->gcChunkSet.has(chunk))
        return CGCT_NOTCHUNK;

    




    if (!Chunk::withinArenasRange(addr))
        return CGCT_NOTARENA;

    
    size_t arenaOffset = Chunk::arenaIndex(addr);
    if (chunk->decommittedArenas.get(arenaOffset))
        return CGCT_FREEARENA;

    ArenaHeader *aheader = &chunk->arenas[arenaOffset].aheader;

    if (!aheader->allocated())
        return CGCT_FREEARENA;

    if (skipUncollectedCompartments && !aheader->compartment->isCollecting())
        return CGCT_OTHERCOMPARTMENT;

    AllocKind thingKind = aheader->getAllocKind();
    uintptr_t offset = addr & ArenaMask;
    uintptr_t minOffset = Arena::firstThingOffset(thingKind);
    if (offset < minOffset)
        return CGCT_NOTARENA;

    
    uintptr_t shift = (offset - minOffset) % Arena::thingSize(thingKind);
    addr -= shift;

    if (thing)
        *thing = reinterpret_cast<void *>(addr);
    if (arenaHeader)
        *arenaHeader = aheader;
    if (thingKindPtr)
        *thingKindPtr = thingKind;
    return CGCT_VALID;
}

#ifdef JSGC_ROOT_ANALYSIS
bool
js::gc::IsAddressableGCThing(JSRuntime *rt, uintptr_t w)
{
    void *thing;
    ArenaHeader *aheader;
    AllocKind thingKind;
    ConservativeGCTest status =
        IsAddressableGCThing(rt, w, false, &thingKind, &aheader, &thing);
    return status == CGCT_VALID;
}
#endif





static inline ConservativeGCTest
MarkIfGCThingWord(JSTracer *trc, uintptr_t w)
{
    void *thing;
    ArenaHeader *aheader;
    AllocKind thingKind;
    ConservativeGCTest status =
        IsAddressableGCThing(trc->runtime, w, IS_GC_MARKING_TRACER(trc),
                             &thingKind, &aheader, &thing);
    if (status != CGCT_VALID)
        return status;

    




    if (InFreeList(aheader, uintptr_t(thing)))
        return CGCT_NOTLIVE;

    JSGCTraceKind traceKind = MapAllocToTraceKind(thingKind);
#ifdef DEBUG
    const char pattern[] = "machine_stack %p";
    char nameBuf[sizeof(pattern) - 2 + sizeof(thing) * 2];
    JS_snprintf(nameBuf, sizeof(nameBuf), pattern, thing);
    JS_SET_TRACING_NAME(trc, nameBuf);
#endif
    JS_SET_TRACING_LOCATION(trc, (void *)w);
    void *tmp = thing;
    MarkKind(trc, &tmp, traceKind);
    JS_ASSERT(tmp == thing);

#ifdef DEBUG
    if (trc->runtime->gcIncrementalState == MARK_ROOTS)
        trc->runtime->mainThread.gcSavedRoots.append(
            PerThreadData::SavedGCRoot(thing, traceKind));
#endif

    return CGCT_VALID;
}

static void
MarkWordConservatively(JSTracer *trc, uintptr_t w)
{
    





#ifdef MOZ_VALGRIND
    JS_SILENCE_UNUSED_VALUE_IN_EXPR(VALGRIND_MAKE_MEM_DEFINED(&w, sizeof(w)));
#endif

    MarkIfGCThingWord(trc, w);
}

MOZ_ASAN_BLACKLIST
static void
MarkRangeConservatively(JSTracer *trc, const uintptr_t *begin, const uintptr_t *end)
{
    JS_ASSERT(begin <= end);
    for (const uintptr_t *i = begin; i < end; ++i)
        MarkWordConservatively(trc, *i);
}

#ifndef JSGC_USE_EXACT_ROOTING
static void
MarkRangeConservativelyAndSkipIon(JSTracer *trc, JSRuntime *rt, const uintptr_t *begin, const uintptr_t *end)
{
    const uintptr_t *i = begin;

#if JS_STACK_GROWTH_DIRECTION < 0 && defined(JS_ION)
    
    
    
    for (ion::IonActivationIterator ion(rt); ion.more(); ++ion) {
        uintptr_t *ionMin, *ionEnd;
        ion.ionStackRange(ionMin, ionEnd);

        MarkRangeConservatively(trc, i, ionMin);
        i = ionEnd;
    }
#endif

    
    MarkRangeConservatively(trc, i, end);
}

static JS_NEVER_INLINE void
MarkConservativeStackRoots(JSTracer *trc, bool useSavedRoots)
{
    JSRuntime *rt = trc->runtime;

#ifdef DEBUG
    if (useSavedRoots) {
        for (PerThreadData::SavedGCRoot *root = rt->mainThread.gcSavedRoots.begin();
             root != rt->mainThread.gcSavedRoots.end();
             root++)
        {
            JS_SET_TRACING_NAME(trc, "cstack");
            MarkKind(trc, &root->thing, root->kind);
        }
        return;
    }

    if (rt->gcIncrementalState == MARK_ROOTS)
        rt->mainThread.gcSavedRoots.clearAndFree();
#endif

    ConservativeGCData *cgcd = &rt->conservativeGC;
    if (!cgcd->hasStackToScan()) {
#ifdef JS_THREADSAFE
        JS_ASSERT(!rt->requestDepth);
#endif
        return;
    }

    uintptr_t *stackMin, *stackEnd;
#if JS_STACK_GROWTH_DIRECTION > 0
    stackMin = rt->nativeStackBase;
    stackEnd = cgcd->nativeStackTop;
#else
    stackMin = cgcd->nativeStackTop + 1;
    stackEnd = reinterpret_cast<uintptr_t *>(rt->nativeStackBase);
#endif

    JS_ASSERT(stackMin <= stackEnd);
    MarkRangeConservativelyAndSkipIon(trc, rt, stackMin, stackEnd);
    MarkRangeConservatively(trc, cgcd->registerSnapshot.words,
                            ArrayEnd(cgcd->registerSnapshot.words));
}

#endif 

void
js::MarkStackRangeConservatively(JSTracer *trc, Value *beginv, Value *endv)
{
    const uintptr_t *begin = beginv->payloadUIntPtr();
    const uintptr_t *end = endv->payloadUIntPtr();
#ifdef JS_NUNBOX32
    



    JS_ASSERT(begin <= end);
    for (const uintptr_t *i = begin; i < end; i += sizeof(Value) / sizeof(uintptr_t))
        MarkWordConservatively(trc, *i);
#else
    MarkRangeConservatively(trc, begin, end);
#endif
}

JS_NEVER_INLINE void
ConservativeGCData::recordStackTop()
{
    
    uintptr_t dummy;
    nativeStackTop = &dummy;

    



#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4611)
#endif
    (void) setjmp(registerSnapshot.jmpbuf);
#if defined(_MSC_VER)
# pragma warning(pop)
#endif
}

void
AutoIdArray::trace(JSTracer *trc)
{
    JS_ASSERT(tag == IDARRAY);
    gc::MarkIdRange(trc, idArray->length, idArray->vector, "JSAutoIdArray.idArray");
}

inline void
AutoGCRooter::trace(JSTracer *trc)
{
    switch (tag) {
      case JSVAL:
        MarkValueRoot(trc, &static_cast<AutoValueRooter *>(this)->val, "JS::AutoValueRooter.val");
        return;

      case PARSER:
        static_cast<frontend::Parser *>(this)->trace(trc);
        return;

      case IDARRAY: {
        JSIdArray *ida = static_cast<AutoIdArray *>(this)->idArray;
        MarkIdRange(trc, ida->length, ida->vector, "JS::AutoIdArray.idArray");
        return;
      }

      case DESCRIPTORS: {
        PropDescArray &descriptors =
            static_cast<AutoPropDescArrayRooter *>(this)->descriptors;
        for (size_t i = 0, len = descriptors.length(); i < len; i++) {
            PropDesc &desc = descriptors[i];
            MarkValueRoot(trc, &desc.pd_, "PropDesc::pd_");
            MarkValueRoot(trc, &desc.value_, "PropDesc::value_");
            MarkValueRoot(trc, &desc.get_, "PropDesc::get_");
            MarkValueRoot(trc, &desc.set_, "PropDesc::set_");
        }
        return;
      }

      case DESCRIPTOR : {
        PropertyDescriptor &desc = *static_cast<AutoPropertyDescriptorRooter *>(this);
        if (desc.obj)
            MarkObjectRoot(trc, &desc.obj, "Descriptor::obj");
        MarkValueRoot(trc, &desc.value, "Descriptor::value");
        if ((desc.attrs & JSPROP_GETTER) && desc.getter) {
            JSObject *tmp = JS_FUNC_TO_DATA_PTR(JSObject *, desc.getter);
            MarkObjectRoot(trc, &tmp, "Descriptor::get");
            desc.getter = JS_DATA_TO_FUNC_PTR(JSPropertyOp, tmp);
        }
        if (desc.attrs & JSPROP_SETTER && desc.setter) {
            JSObject *tmp = JS_FUNC_TO_DATA_PTR(JSObject *, desc.setter);
            MarkObjectRoot(trc, &tmp, "Descriptor::set");
            desc.setter = JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, tmp);
        }
        return;
      }

#if JS_HAS_XML_SUPPORT
      case NAMESPACES: {
        JSXMLArray<JSObject> &array = static_cast<AutoNamespaceArray *>(this)->array;
        MarkObjectRange(trc, array.length, array.vector, "JSXMLArray.vector");
        js_XMLArrayCursorTrace(trc, array.cursors);
        return;
      }

      case XML:
        js_TraceXML(trc, static_cast<AutoXMLRooter *>(this)->xml);
        return;
#endif

      case OBJECT:
        if (static_cast<AutoObjectRooter *>(this)->obj)
            MarkObjectRoot(trc, &static_cast<AutoObjectRooter *>(this)->obj,
                           "JS::AutoObjectRooter.obj");
        return;

      case ID:
        MarkIdRoot(trc, &static_cast<AutoIdRooter *>(this)->id_, "JS::AutoIdRooter.id_");
        return;

      case VALVECTOR: {
        AutoValueVector::VectorImpl &vector = static_cast<AutoValueVector *>(this)->vector;
        MarkValueRootRange(trc, vector.length(), vector.begin(), "js::AutoValueVector.vector");
        return;
      }

      case STRING:
        if (static_cast<AutoStringRooter *>(this)->str)
            MarkStringRoot(trc, &static_cast<AutoStringRooter *>(this)->str,
                           "JS::AutoStringRooter.str");
        return;

      case IDVECTOR: {
        AutoIdVector::VectorImpl &vector = static_cast<AutoIdVector *>(this)->vector;
        MarkIdRootRange(trc, vector.length(), vector.begin(), "js::AutoIdVector.vector");
        return;
      }

      case SHAPEVECTOR: {
        AutoShapeVector::VectorImpl &vector = static_cast<js::AutoShapeVector *>(this)->vector;
        MarkShapeRootRange(trc, vector.length(), const_cast<Shape **>(vector.begin()),
                           "js::AutoShapeVector.vector");
        return;
      }

      case OBJVECTOR: {
        AutoObjectVector::VectorImpl &vector = static_cast<AutoObjectVector *>(this)->vector;
        MarkObjectRootRange(trc, vector.length(), vector.begin(), "js::AutoObjectVector.vector");
        return;
      }

      case STRINGVECTOR: {
        AutoStringVector::VectorImpl &vector = static_cast<AutoStringVector *>(this)->vector;
        MarkStringRootRange(trc, vector.length(), vector.begin(), "js::AutoStringVector.vector");
        return;
      }

      case NAMEVECTOR: {
        AutoNameVector::VectorImpl &vector = static_cast<AutoNameVector *>(this)->vector;
        MarkStringRootRange(trc, vector.length(), vector.begin(), "js::AutoNameVector.vector");
        return;
      }

      case VALARRAY: {
        AutoValueArray *array = static_cast<AutoValueArray *>(this);
        MarkValueRootRange(trc, array->length(), array->start(), "js::AutoValueArray");
        return;
      }

      case SCRIPTVECTOR: {
        AutoScriptVector::VectorImpl &vector = static_cast<AutoScriptVector *>(this)->vector;
        for (size_t i = 0; i < vector.length(); i++)
            MarkScriptRoot(trc, &vector[i], "AutoScriptVector element");
        return;
      }

      case OBJOBJHASHMAP: {
        AutoObjectObjectHashMap::HashMapImpl &map = static_cast<AutoObjectObjectHashMap *>(this)->map;
        for (AutoObjectObjectHashMap::Enum e(map); !e.empty(); e.popFront()) {
            mozilla::DebugOnly<RawObject> key = e.front().key;
            MarkObjectRoot(trc, (RawObject *) &e.front().key, "AutoObjectObjectHashMap key");
            JS_ASSERT(key == e.front().key);
            MarkObjectRoot(trc, &e.front().value, "AutoObjectObjectHashMap value");
        }
        return;
      }

      case PROPDESC: {
        PropDesc::AutoRooter *rooter = static_cast<PropDesc::AutoRooter *>(this);
        MarkValueRoot(trc, &rooter->pd->pd_, "PropDesc::AutoRooter pd");
        MarkValueRoot(trc, &rooter->pd->value_, "PropDesc::AutoRooter value");
        MarkValueRoot(trc, &rooter->pd->get_, "PropDesc::AutoRooter get");
        MarkValueRoot(trc, &rooter->pd->set_, "PropDesc::AutoRooter set");
        return;
      }

      case SHAPERANGE: {
        Shape::Range::AutoRooter *rooter = static_cast<Shape::Range::AutoRooter *>(this);
        rooter->trace(trc);
        return;
      }

      case STACKSHAPE: {
        StackShape::AutoRooter *rooter = static_cast<StackShape::AutoRooter *>(this);
        if (rooter->shape->base)
            MarkBaseShapeRoot(trc, (BaseShape**) &rooter->shape->base, "StackShape::AutoRooter base");
        MarkIdRoot(trc, (jsid*) &rooter->shape->propid, "StackShape::AutoRooter id");
        return;
      }

      case STACKBASESHAPE: {
        StackBaseShape::AutoRooter *rooter = static_cast<StackBaseShape::AutoRooter *>(this);
        if (rooter->base->parent)
            MarkObjectRoot(trc, (JSObject**) &rooter->base->parent, "StackBaseShape::AutoRooter parent");
        if ((rooter->base->flags & BaseShape::HAS_GETTER_OBJECT) && rooter->base->rawGetter) {
            MarkObjectRoot(trc, (JSObject**) &rooter->base->rawGetter,
                           "StackBaseShape::AutoRooter getter");
        }
        if ((rooter->base->flags & BaseShape::HAS_SETTER_OBJECT) && rooter->base->rawSetter) {
            MarkObjectRoot(trc, (JSObject**) &rooter->base->rawSetter,
                           "StackBaseShape::AutoRooter setter");
        }
        return;
      }

      case GETTERSETTER: {
        AutoRooterGetterSetter::Inner *rooter = static_cast<AutoRooterGetterSetter::Inner *>(this);
        if ((rooter->attrs & JSPROP_GETTER) && *rooter->pgetter)
            MarkObjectRoot(trc, (JSObject**) rooter->pgetter, "AutoRooterGetterSetter getter");
        if ((rooter->attrs & JSPROP_SETTER) && *rooter->psetter)
            MarkObjectRoot(trc, (JSObject**) rooter->psetter, "AutoRooterGetterSetter setter");
        return;
      }

      case REGEXPSTATICS: {
          



        return;
      }

      case HASHABLEVALUE: {
          



        return;
      }

      case IONMASM: {
#ifdef JS_ION
        static_cast<js::ion::MacroAssembler::AutoRooter *>(this)->masm()->trace(trc);
#endif
        return;
      }

      case IONALLOC: {
#ifdef JS_ION
        static_cast<js::ion::AutoTempAllocatorRooter *>(this)->trace(trc);
#endif
        return;
      }

      case WRAPPER: {
        




          MarkValueUnbarriered(trc, &static_cast<AutoWrapperRooter *>(this)->value.get(),
                               "JS::AutoWrapperRooter.value");
        return;
      }

      case WRAPVECTOR: {
        AutoWrapperVector::VectorImpl &vector = static_cast<AutoWrapperVector *>(this)->vector;
        




        for (WrapperValue *p = vector.begin(); p < vector.end(); p++)
            MarkValueUnbarriered(trc, &p->get(), "js::AutoWrapperVector.vector");
        return;
      }
    }

    JS_ASSERT(tag >= 0);
    MarkValueRootRange(trc, tag, static_cast<AutoArrayRooter *>(this)->array,
                       "JS::AutoArrayRooter.array");
}

 void
AutoGCRooter::traceAll(JSTracer *trc)
{
    for (js::AutoGCRooter *gcr = trc->runtime->autoGCRooters; gcr; gcr = gcr->down)
        gcr->trace(trc);
}

 void
AutoGCRooter::traceAllWrappers(JSTracer *trc)
{
    for (js::AutoGCRooter *gcr = trc->runtime->autoGCRooters; gcr; gcr = gcr->down) {
        if (gcr->tag == WRAPVECTOR || gcr->tag == WRAPPER)
            gcr->trace(trc);
    }
}

void
Shape::Range::AutoRooter::trace(JSTracer *trc)
{
    if (r->cursor)
        MarkShapeRoot(trc, const_cast<Shape**>(&r->cursor), "Shape::Range::AutoRooter");
}

void
RegExpStatics::AutoRooter::trace(JSTracer *trc)
{
    if (statics->regexp)
        MarkObjectRoot(trc, reinterpret_cast<JSObject**>(&statics->regexp),
                       "RegExpStatics::AutoRooter regexp");
    if (statics->matchesInput)
        MarkStringRoot(trc, reinterpret_cast<JSString**>(&statics->matchesInput),
                       "RegExpStatics::AutoRooter matchesInput");
    if (statics->pendingInput)
        MarkStringRoot(trc, reinterpret_cast<JSString**>(&statics->pendingInput),
                       "RegExpStatics::AutoRooter pendingInput");
}

void
HashableValue::AutoRooter::trace(JSTracer *trc)
{
    MarkValueRoot(trc, reinterpret_cast<Value*>(&v->value), "HashableValue::AutoRooter");
}

void
js::gc::MarkRuntime(JSTracer *trc, bool useSavedRoots)
{
    JSRuntime *rt = trc->runtime;
    JS_ASSERT(trc->callback != GCMarker::GrayCallback);

    if (IS_GC_MARKING_TRACER(trc)) {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            if (!c->isCollecting())
                c->markCrossCompartmentWrappers(trc);
        }
        Debugger::markCrossCompartmentDebuggerObjectReferents(trc);
    }

    AutoGCRooter::traceAll(trc);

    if (rt->hasContexts()) {
#ifdef JSGC_USE_EXACT_ROOTING
        MarkExactStackRoots(trc);
#else
        MarkConservativeStackRoots(trc, useSavedRoots);
#endif
        rt->markSelfHostingGlobal(trc);
    }

    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront()) {
        const RootEntry &entry = r.front();
        const char *name = entry.value.name ? entry.value.name : "root";
        if (entry.value.type == JS_GC_ROOT_GCTHING_PTR)
            MarkGCThingRoot(trc, reinterpret_cast<void **>(entry.key), name);
        else
            MarkValueRoot(trc, reinterpret_cast<Value *>(entry.key), name);
    }

    for (GCLocks::Range r = rt->gcLocksHash.all(); !r.empty(); r.popFront()) {
        const GCLocks::Entry &entry = r.front();
        JS_ASSERT(entry.value >= 1);
        JS_SET_TRACING_LOCATION(trc, (void *)&entry.key);
        void *tmp = entry.key;
        MarkGCThingRoot(trc, &tmp, "locked object");
        JS_ASSERT(tmp == entry.key);
    }

    if (rt->scriptAndCountsVector) {
        ScriptAndCountsVector &vec = *rt->scriptAndCountsVector;
        for (size_t i = 0; i < vec.length(); i++)
            MarkScriptRoot(trc, &vec[i].script, "scriptAndCountsVector");
    }

    if (!IS_GC_MARKING_TRACER(trc) || rt->atomsCompartment->isCollecting()) {
        MarkAtoms(trc);
#ifdef JS_ION
        
        if (rt->hasContexts())
            ion::IonRuntime::Mark(trc);
#endif
    }

    rt->staticStrings.trace(trc);

    for (ContextIter acx(rt); !acx.done(); acx.next())
        acx->mark(trc);

    
    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (IS_GC_MARKING_TRACER(trc) && !c->isCollecting())
            continue;

        if (IS_GC_MARKING_TRACER(trc)) {
            if ((c->activeAnalysis || c->isPreservingCode())) {
                gcstats::AutoPhase ap(rt->gcStats, gcstats::PHASE_MARK_TYPES);
                c->markTypes(trc);
            } else {
                c->gcTypesMarked = false;
            }
        }

        
        if (!IS_GC_MARKING_TRACER(trc)) {
            if (c->watchpointMap)
                c->watchpointMap->markAll(trc);
        }

        
        if (rt->profilingScripts) {
            for (CellIterUnderGC i(c, FINALIZE_SCRIPT); !i.done(); i.next()) {
                JSScript *script = i.get<JSScript>();
                if (script->hasScriptCounts) {
                    MarkScriptRoot(trc, &script, "profilingScripts");
                    JS_ASSERT(script == i.get<JSScript>());
                }
            }
        }

        
        if (c->debugScopes)
            c->debugScopes->mark(trc);
    }

#ifdef JS_METHODJIT
    
    for (CompartmentsIter c(rt); !c.done(); c.next())
        mjit::ExpandInlineFrames(c);
#endif

    rt->stackSpace.mark(trc);

#ifdef JS_ION
    ion::MarkIonActivations(rt, trc);
#endif

    for (CompartmentsIter c(rt); !c.done(); c.next())
        c->mark(trc);

    
    if (JSTraceDataOp op = rt->gcBlackRootsTraceOp)
        (*op)(trc, rt->gcBlackRootsData);

    
    if (JSTraceDataOp op = rt->gcGrayRootsTraceOp) {
        if (!IS_GC_MARKING_TRACER(trc))
            (*op)(trc, rt->gcGrayRootsData);
    }
}

void
js::gc::BufferGrayRoots(GCMarker *gcmarker)
{
    JSRuntime *rt = gcmarker->runtime;
    if (JSTraceDataOp op = rt->gcGrayRootsTraceOp) {
        gcmarker->startBufferingGrayRoots();
        (*op)(gcmarker, rt->gcGrayRootsData);
        gcmarker->endBufferingGrayRoots();
    }
}
