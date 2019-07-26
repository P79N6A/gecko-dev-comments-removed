





#include "mozilla/ArrayUtils.h"

#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#endif

#include "jscntxt.h"
#include "jsgc.h"
#include "jsonparser.h"
#include "jsprf.h"
#include "jstypes.h"
#include "jswatchpoint.h"

#include "builtin/MapObject.h"
#include "frontend/BytecodeCompiler.h"
#include "gc/GCInternals.h"
#include "gc/Marking.h"
#ifdef JS_ION
# include "jit/IonMacroAssembler.h"
#endif
#include "js/HashTable.h"
#include "vm/Debugger.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace js::gc;

using mozilla::ArrayEnd;

typedef RootedValueMap::Range RootRange;
typedef RootedValueMap::Entry RootEntry;
typedef RootedValueMap::Enum RootEnum;

#ifdef JSGC_USE_EXACT_ROOTING
static inline void
MarkExactStackRoot(JSTracer *trc, Rooted<void*> *rooter, ThingRootKind kind)
{
    void **addr = (void **)rooter->address();
    if (IsNullTaggedPointer(*addr))
        return;

    if (kind == THING_ROOT_OBJECT && *addr == TaggedProto::LazyProto)
        return;

    switch (kind) {
      case THING_ROOT_OBJECT:      MarkObjectRoot(trc, (JSObject **)addr, "exact-object"); break;
      case THING_ROOT_STRING:      MarkStringRoot(trc, (JSString **)addr, "exact-string"); break;
      case THING_ROOT_SCRIPT:      MarkScriptRoot(trc, (JSScript **)addr, "exact-script"); break;
      case THING_ROOT_LAZY_SCRIPT: MarkLazyScriptRoot(trc, (LazyScript **)addr, "exact-lazy-script"); break;
      case THING_ROOT_SHAPE:       MarkShapeRoot(trc, (Shape **)addr, "exact-shape"); break;
      case THING_ROOT_BASE_SHAPE:  MarkBaseShapeRoot(trc, (BaseShape **)addr, "exact-baseshape"); break;
      case THING_ROOT_TYPE:        MarkTypeRoot(trc, (types::Type *)addr, "exact-type"); break;
      case THING_ROOT_TYPE_OBJECT: MarkTypeObjectRoot(trc, (types::TypeObject **)addr, "exact-typeobject"); break;
      case THING_ROOT_JIT_CODE:    MarkJitCodeRoot(trc, (jit::JitCode **)addr, "exact-jitcode"); break;
      case THING_ROOT_VALUE:       MarkValueRoot(trc, (Value *)addr, "exact-value"); break;
      case THING_ROOT_ID:          MarkIdRoot(trc, (jsid *)addr, "exact-id"); break;
      case THING_ROOT_PROPERTY_ID: MarkIdRoot(trc, &((js::PropertyId *)addr)->asId(), "exact-propertyid"); break;
      case THING_ROOT_BINDINGS:    ((Bindings *)addr)->trace(trc); break;
      case THING_ROOT_PROPERTY_DESCRIPTOR: ((JSPropertyDescriptor *)addr)->trace(trc); break;
      case THING_ROOT_CUSTOM: {
          
          
          const size_t rooterOffset = offsetof(RootedGeneric<void*>, rooter);
          reinterpret_cast< RootedGeneric<void*>* >(uintptr_t(rooter) - rooterOffset)->trace(trc);
          break;
      }
      default: MOZ_ASSUME_UNREACHABLE("Invalid THING_ROOT kind"); break;
    }
}

static inline void
MarkExactStackRootList(JSTracer *trc, Rooted<void*> *rooter, ThingRootKind kind)
{
    while (rooter) {
        MarkExactStackRoot(trc, rooter, kind);
        rooter = rooter->previous();
    }
}

static void
MarkExactStackRoots(JSTracer *trc)
{
    for (unsigned i = 0; i < THING_ROOT_LIMIT; i++) {
        for (ContextIter cx(trc->runtime); !cx.done(); cx.next())
            MarkExactStackRootList(trc, cx->thingGCRooters[i], ThingRootKind(i));

        MarkExactStackRootList(trc, trc->runtime->mainThread.thingGCRooters[i], ThingRootKind(i));
    }
}
#endif 

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

    if (skipUncollectedCompartments && !aheader->zone->isCollecting())
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
void *
js::gc::GetAddressableGCThing(JSRuntime *rt, uintptr_t w)
{
    void *thing;
    ArenaHeader *aheader;
    AllocKind thingKind;
    ConservativeGCTest status =
        IsAddressableGCThing(rt, w, false, &thingKind, &aheader, &thing);
    if (status != CGCT_VALID)
        return nullptr;
    return thing;
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

    




    if (InFreeList(aheader, thing))
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

#if JS_STACK_GROWTH_DIRECTION < 0 && defined(JS_ION) && !defined(JS_ARM_SIMULATOR)
    
    
    
    
    
    
    
    for (jit::JitActivationIterator iter(rt); !iter.done(); ++iter) {
        uintptr_t *jitMin, *jitEnd;
        iter.jitStackRange(jitMin, jitEnd);

        MarkRangeConservatively(trc, i, jitMin);
        i = jitEnd;
    }
#endif

    
    MarkRangeConservatively(trc, i, end);
}

static MOZ_NEVER_INLINE void
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

MOZ_NEVER_INLINE void
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
JS::AutoIdArray::trace(JSTracer *trc)
{
    JS_ASSERT(tag_ == IDARRAY);
    gc::MarkIdRange(trc, idArray->length, idArray->vector, "JSAutoIdArray.idArray");
}

inline void
AutoGCRooter::trace(JSTracer *trc)
{
    switch (tag_) {
      case PARSER:
        frontend::MarkParser(trc, this);
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

      case ID:
        MarkIdRoot(trc, &static_cast<AutoIdRooter *>(this)->id_, "JS::AutoIdRooter.id_");
        return;

      case VALVECTOR: {
        AutoValueVector::VectorImpl &vector = static_cast<AutoValueVector *>(this)->vector;
        MarkValueRootRange(trc, vector.length(), vector.begin(), "js::AutoValueVector.vector");
        return;
      }

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

      case FUNVECTOR: {
        AutoFunctionVector::VectorImpl &vector = static_cast<AutoFunctionVector *>(this)->vector;
        MarkObjectRootRange(trc, vector.length(), vector.begin(), "js::AutoFunctionVector.vector");
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
        



        AutoValueArray<1> *array = static_cast<AutoValueArray<1> *>(this);
        MarkValueRootRange(trc, array->length(), array->begin(), "js::AutoValueArray");
        return;
      }

      case SCRIPTVECTOR: {
        AutoScriptVector::VectorImpl &vector = static_cast<AutoScriptVector *>(this)->vector;
        MarkScriptRootRange(trc, vector.length(), vector.begin(), "js::AutoScriptVector.vector");
        return;
      }

      case OBJOBJHASHMAP: {
        AutoObjectObjectHashMap::HashMapImpl &map = static_cast<AutoObjectObjectHashMap *>(this)->map;
        for (AutoObjectObjectHashMap::Enum e(map); !e.empty(); e.popFront()) {
            MarkObjectRoot(trc, &e.front().value(), "AutoObjectObjectHashMap value");
            JS_SET_TRACING_LOCATION(trc, (void *)&e.front().key());
            JSObject *key = e.front().key();
            MarkObjectRoot(trc, &key, "AutoObjectObjectHashMap key");
            if (key != e.front().key())
                e.rekeyFront(key);
        }
        return;
      }

      case OBJU32HASHMAP: {
        AutoObjectUnsigned32HashMap *self = static_cast<AutoObjectUnsigned32HashMap *>(this);
        AutoObjectUnsigned32HashMap::HashMapImpl &map = self->map;
        for (AutoObjectUnsigned32HashMap::Enum e(map); !e.empty(); e.popFront()) {
            JSObject *key = e.front().key();
            MarkObjectRoot(trc, &key, "AutoObjectUnsignedHashMap key");
            if (key != e.front().key())
                e.rekeyFront(key);
        }
        return;
      }

      case OBJHASHSET: {
        AutoObjectHashSet *self = static_cast<AutoObjectHashSet *>(this);
        AutoObjectHashSet::HashSetImpl &set = self->set;
        for (AutoObjectHashSet::Enum e(set); !e.empty(); e.popFront()) {
            JSObject *obj = e.front();
            MarkObjectRoot(trc, &obj, "AutoObjectHashSet value");
            if (obj != e.front())
                e.rekeyFront(obj);
        }
        return;
      }

      case HASHABLEVALUE: {
        AutoHashableValueRooter *rooter = static_cast<AutoHashableValueRooter *>(this);
        rooter->trace(trc);
        return;
      }

      case IONMASM: {
#ifdef JS_ION
        static_cast<js::jit::MacroAssembler::AutoRooter *>(this)->masm()->trace(trc);
#endif
        return;
      }

      case IONALLOC: {
#ifdef JS_ION
        static_cast<js::jit::AutoTempAllocatorRooter *>(this)->trace(trc);
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

      case JSONPARSER:
        static_cast<js::JSONParser *>(this)->trace(trc);
        return;

      case CUSTOM:
        static_cast<JS::CustomAutoRooter *>(this)->trace(trc);
        return;
    }

    JS_ASSERT(tag_ >= 0);
    if (Value *vp = static_cast<AutoArrayRooter *>(this)->array)
        MarkValueRootRange(trc, tag_, vp, "JS::AutoArrayRooter.array");
}

 void
AutoGCRooter::traceAll(JSTracer *trc)
{
    for (ContextIter cx(trc->runtime); !cx.done(); cx.next()) {
        for (js::AutoGCRooter *gcr = cx->autoGCRooters; gcr; gcr = gcr->down)
            gcr->trace(trc);
    }
}

 void
AutoGCRooter::traceAllWrappers(JSTracer *trc)
{
    for (ContextIter cx(trc->runtime); !cx.done(); cx.next()) {
        for (js::AutoGCRooter *gcr = cx->autoGCRooters; gcr; gcr = gcr->down) {
            if (gcr->tag_ == WRAPVECTOR || gcr->tag_ == WRAPPER)
                gcr->trace(trc);
        }
    }
}

void
AutoHashableValueRooter::trace(JSTracer *trc)
{
    MarkValueRoot(trc, reinterpret_cast<Value*>(&value), "AutoHashableValueRooter");
}

void
StackShape::trace(JSTracer *trc)
{
    if (base)
        MarkBaseShapeRoot(trc, (BaseShape**) &base, "StackShape base");
    MarkIdRoot(trc, (jsid*) &propid, "StackShape id");
}

void
JSPropertyDescriptor::trace(JSTracer *trc)
{
    if (obj)
        MarkObjectRoot(trc, &obj, "Descriptor::obj");
    MarkValueRoot(trc, &value, "Descriptor::value");
    if ((attrs & JSPROP_GETTER) && getter) {
        JSObject *tmp = JS_FUNC_TO_DATA_PTR(JSObject *, getter);
        MarkObjectRoot(trc, &tmp, "Descriptor::get");
        getter = JS_DATA_TO_FUNC_PTR(JSPropertyOp, tmp);
    }
    if ((attrs & JSPROP_SETTER) && setter) {
        JSObject *tmp = JS_FUNC_TO_DATA_PTR(JSObject *, setter);
        MarkObjectRoot(trc, &tmp, "Descriptor::set");
        setter = JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, tmp);
    }
}

namespace js {
namespace gc {

template<typename T>
struct PersistentRootedMarker
{
    typedef PersistentRooted<T> Element;
    typedef mozilla::LinkedList<Element> List;
    typedef void (*MarkFunc)(JSTracer *trc, T *ref, const char *name);

    template <MarkFunc Mark>
    static void
    markChainIfNotNull(JSTracer *trc, List &list, const char *name)
    {
        for (Element *r = list.getFirst(); r; r = r->getNext()) {
            if (r->get())
                Mark(trc, r->address(), name);
        }
    }

    template <MarkFunc Mark>
    static void
    markChain(JSTracer *trc, List &list, const char *name)
    {
        for (Element *r = list.getFirst(); r; r = r->getNext())
            Mark(trc, r->address(), name);
    }
};
}
}

void
js::gc::MarkPersistentRootedChains(JSTracer *trc)
{
    JSRuntime *rt = trc->runtime;

    
    PersistentRootedMarker<JSFunction*>::markChainIfNotNull<MarkObjectRoot>(
        trc, rt->functionPersistentRooteds, "PersistentRooted<JSFunction *>");
    PersistentRootedMarker<JSObject*>::markChainIfNotNull<MarkObjectRoot>(
        trc, rt->objectPersistentRooteds, "PersistentRooted<JSObject *>");
    PersistentRootedMarker<JSScript*>::markChainIfNotNull<MarkScriptRoot>(
        trc, rt->scriptPersistentRooteds, "PersistentRooted<JSScript *>");
    PersistentRootedMarker<JSString*>::markChainIfNotNull<MarkStringRoot>(
        trc, rt->stringPersistentRooteds, "PersistentRooted<JSString *>");

    
    PersistentRootedMarker<jsid>::markChain<MarkIdRoot>(trc, rt->idPersistentRooteds,
                                                        "PersistentRooted<jsid>");
    PersistentRootedMarker<Value>::markChain<MarkValueRoot>(trc, rt->valuePersistentRooteds,
                                                            "PersistentRooted<Value>");
}

void
js::gc::MarkRuntime(JSTracer *trc, bool useSavedRoots)
{
    JSRuntime *rt = trc->runtime;
    JS_ASSERT(trc->callback != GCMarker::GrayCallback);

    JS_ASSERT(!rt->mainThread.suppressGC);

    if (IS_GC_MARKING_TRACER(trc)) {
        for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
            if (!c->zone()->isCollecting())
                c->markCrossCompartmentWrappers(trc);
        }
        Debugger::markCrossCompartmentDebuggerObjectReferents(trc);
    }

    AutoGCRooter::traceAll(trc);

    if (!rt->isBeingDestroyed()) {
#ifdef JSGC_USE_EXACT_ROOTING
        MarkExactStackRoots(trc);
#else
        MarkConservativeStackRoots(trc, useSavedRoots);
#endif
        rt->markSelfHostingGlobal(trc);
    }

    for (RootRange r = rt->gcRootsHash.all(); !r.empty(); r.popFront()) {
        const RootEntry &entry = r.front();
        const char *name = entry.value().name ? entry.value().name : "root";
        JSGCRootType type = entry.value().type;
        void *key = entry.key();
        if (type == JS_GC_ROOT_VALUE_PTR) {
            MarkValueRoot(trc, reinterpret_cast<Value *>(key), name);
        } else if (*reinterpret_cast<void **>(key)){
            if (type == JS_GC_ROOT_STRING_PTR)
                MarkStringRoot(trc, reinterpret_cast<JSString **>(key), name);
            else if (type == JS_GC_ROOT_OBJECT_PTR)
                MarkObjectRoot(trc, reinterpret_cast<JSObject **>(key), name);
            else if (type == JS_GC_ROOT_SCRIPT_PTR)
                MarkScriptRoot(trc, reinterpret_cast<JSScript **>(key), name);
            else
                MOZ_ASSUME_UNREACHABLE("unexpected js::RootInfo::type value");
        }
    }

    MarkPersistentRootedChains(trc);

    if (rt->scriptAndCountsVector) {
        ScriptAndCountsVector &vec = *rt->scriptAndCountsVector;
        for (size_t i = 0; i < vec.length(); i++)
            MarkScriptRoot(trc, &vec[i].script, "scriptAndCountsVector");
    }

    if (!rt->isBeingDestroyed() && !trc->runtime->isHeapMinorCollecting()) {
        if (!IS_GC_MARKING_TRACER(trc) || rt->atomsCompartment()->zone()->isCollecting()) {
            MarkPermanentAtoms(trc);
            MarkAtoms(trc);
#ifdef JS_ION
            jit::JitRuntime::Mark(trc);
#endif
        }
    }

    for (ContextIter acx(rt); !acx.done(); acx.next())
        acx->mark(trc);

    for (ZonesIter zone(rt, SkipAtoms); !zone.done(); zone.next()) {
        if (IS_GC_MARKING_TRACER(trc) && !zone->isCollecting())
            continue;

        
        if (rt->profilingScripts && !rt->isHeapMinorCollecting()) {
            for (CellIterUnderGC i(zone, FINALIZE_SCRIPT); !i.done(); i.next()) {
                JSScript *script = i.get<JSScript>();
                if (script->hasScriptCounts()) {
                    MarkScriptRoot(trc, &script, "profilingScripts");
                    JS_ASSERT(script == i.get<JSScript>());
                }
            }
        }
    }

    
    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        if (trc->runtime->isHeapMinorCollecting())
            c->globalWriteBarriered = false;

        if (IS_GC_MARKING_TRACER(trc) && !c->zone()->isCollecting())
            continue;

        
        if (!IS_GC_MARKING_TRACER(trc)) {
            if (c->watchpointMap)
                c->watchpointMap->markAll(trc);
        }

        
        if (c->debugScopes)
            c->debugScopes->mark(trc);
    }

    MarkInterpreterActivations(rt, trc);

#ifdef JS_ION
    jit::MarkJitActivations(rt, trc);
#endif

    if (!rt->isHeapMinorCollecting()) {
        




        for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next())
            c->mark(trc);

        






        for (size_t i = 0; i < rt->gcBlackRootTracers.length(); i++) {
            const JSRuntime::ExtraTracer &e = rt->gcBlackRootTracers[i];
            (*e.op)(trc, e.data);
        }

        
        if (JSTraceDataOp op = rt->gcGrayRootTracer.op) {
            if (!IS_GC_MARKING_TRACER(trc))
                (*op)(trc, rt->gcGrayRootTracer.data);
        }
    }
}

void
js::gc::BufferGrayRoots(GCMarker *gcmarker)
{
    JSRuntime *rt = gcmarker->runtime;
    gcmarker->startBufferingGrayRoots();
    if (JSTraceDataOp op = rt->gcGrayRootTracer.op)
        (*op)(gcmarker, rt->gcGrayRootTracer.data);
    gcmarker->endBufferingGrayRoots();
}
