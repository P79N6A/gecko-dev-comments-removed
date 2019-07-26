






#include "js/MemoryMetrics.h"

#include "mozilla/Assertions.h"

#include "jsapi.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsscript.h"

#include "jsobjinlines.h"

#include "ion/IonCode.h"

namespace js {

size_t MemoryReportingSundriesThreshold()
{
    return 8 * 1024;
}

} 

#ifdef JS_THREADSAFE

namespace JS {

using namespace js;

typedef HashSet<ScriptSource *, DefaultHasher<ScriptSource *>, SystemAllocPolicy> SourceSet;

struct IteratorClosure
{
  RuntimeStats *rtStats;
  ObjectPrivateVisitor *opv;
  SourceSet seenSources;
  IteratorClosure(RuntimeStats *rt, ObjectPrivateVisitor *v) : rtStats(rt), opv(v) {}
  bool init() {
      return seenSources.init();
  }
};

size_t
CompartmentStats::gcHeapThingsSize()
{
    
    size_t n = 0;
    n += gcHeapObjectsOrdinary;
    n += gcHeapObjectsFunction;
    n += gcHeapObjectsDenseArray;
    n += gcHeapObjectsSlowArray;
    n += gcHeapObjectsCrossCompartmentWrapper;
    n += gcHeapStrings;
    n += gcHeapShapesTree;
    n += gcHeapShapesDict;
    n += gcHeapShapesBase;
    n += gcHeapScripts;
    n += gcHeapTypeObjects;
    n += gcHeapIonCodes;
#if JS_HAS_XML_SUPPORT
    n += gcHeapXML;
#endif

#ifdef DEBUG
    size_t n2 = n;
    n2 += gcHeapArenaAdmin;
    n2 += gcHeapUnusedGcThings;
    
    JS_ASSERT(n2 % gc::ArenaSize == 0);
#endif

    return n;
}

static void
StatsCompartmentCallback(JSRuntime *rt, void *data, JSCompartment *compartment)
{
    
    RuntimeStats *rtStats = static_cast<IteratorClosure *>(data)->rtStats;

    
    MOZ_ALWAYS_TRUE(rtStats->compartmentStatsVector.growBy(1));
    CompartmentStats &cStats = rtStats->compartmentStatsVector.back();
    rtStats->initExtraCompartmentStats(compartment, &cStats);
    rtStats->currCompartmentStats = &cStats;

    
    compartment->sizeOfIncludingThis(rtStats->mallocSizeOf,
                                     &cStats.compartmentObject,
                                     &cStats.typeInferenceSizes,
                                     &cStats.shapesCompartmentTables,
                                     &cStats.crossCompartmentWrappersTable,
                                     &cStats.regexpCompartment,
                                     &cStats.debuggeesSet);
}

static void
StatsChunkCallback(JSRuntime *rt, void *data, gc::Chunk *chunk)
{
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(data);
    for (size_t i = 0; i < gc::ArenasPerChunk; i++)
        if (chunk->decommittedArenas.get(i))
            rtStats->gcHeapDecommittedArenas += gc::ArenaSize;
}

static void
StatsArenaCallback(JSRuntime *rt, void *data, gc::Arena *arena,
                   JSGCTraceKind traceKind, size_t thingSize)
{
    RuntimeStats *rtStats = static_cast<IteratorClosure *>(data)->rtStats;

    
    
    size_t allocationSpace = arena->thingsSpan(thingSize);
    rtStats->currCompartmentStats->gcHeapArenaAdmin +=
        gc::ArenaSize - allocationSpace;

    
    
    
    
    rtStats->currCompartmentStats->gcHeapUnusedGcThings += allocationSpace;
}

static void
StatsCellCallback(JSRuntime *rt, void *data, void *thing, JSGCTraceKind traceKind,
                  size_t thingSize)
{
    IteratorClosure *closure = static_cast<IteratorClosure *>(data);
    RuntimeStats *rtStats = closure->rtStats;
    CompartmentStats *cStats = rtStats->currCompartmentStats;
    switch (traceKind) {
    case JSTRACE_OBJECT:
    {
        JSObject *obj = static_cast<JSObject *>(thing);
        if (obj->isFunction()) {
            cStats->gcHeapObjectsFunction += thingSize;
        } else if (obj->isDenseArray()) {
            cStats->gcHeapObjectsDenseArray += thingSize;
        } else if (obj->isSlowArray()) {
            cStats->gcHeapObjectsSlowArray += thingSize;
        } else if (obj->isCrossCompartmentWrapper()) {
            cStats->gcHeapObjectsCrossCompartmentWrapper += thingSize;
        } else {
            cStats->gcHeapObjectsOrdinary += thingSize;
        }
        size_t slotsSize, elementsSize, miscSize;
        obj->sizeOfExcludingThis(rtStats->mallocSizeOf, &slotsSize,
                                 &elementsSize, &miscSize);
        cStats->objectSlots += slotsSize;
        cStats->objectElements += elementsSize;
        cStats->objectMisc += miscSize;

        if (ObjectPrivateVisitor *opv = closure->opv) {
            js::Class *clazz = js::GetObjectClass(obj);
            if (clazz->flags & JSCLASS_HAS_PRIVATE &&
                clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS)
            {
                cStats->objectPrivate += opv->sizeOfIncludingThis(GetObjectPrivate(obj));
            }
        }
        break;
    }
    case JSTRACE_STRING:
    {
        JSString *str = static_cast<JSString *>(thing);
        cStats->gcHeapStrings += thingSize;

        size_t strSize = str->sizeOfExcludingThis(rtStats->mallocSizeOf);

        
        
        if (strSize >= HugeStringInfo::MinSize() && cStats->hugeStrings.growBy(1)) {
            HugeStringInfo &info = cStats->hugeStrings.back();
            info.length = str->length();
            info.size = str->sizeOfExcludingThis(rtStats->mallocSizeOf);
            PutEscapedString(info.buffer, sizeof(info.buffer), &str->asLinear(), 0);
        } else {
          cStats->nonHugeStringChars += strSize;
        }
        break;
    }
    case JSTRACE_SHAPE:
    {
        Shape *shape = static_cast<Shape*>(thing);
        size_t propTableSize, kidsSize;
        shape->sizeOfExcludingThis(rtStats->mallocSizeOf, &propTableSize, &kidsSize);
        if (shape->inDictionary()) {
            cStats->gcHeapShapesDict += thingSize;
            cStats->shapesExtraDictTables += propTableSize;
            JS_ASSERT(kidsSize == 0);
        } else {
            cStats->gcHeapShapesTree += thingSize;
            cStats->shapesExtraTreeTables += propTableSize;
            cStats->shapesExtraTreeShapeKids += kidsSize;
        }
        break;
    }
    case JSTRACE_BASE_SHAPE:
    {
        cStats->gcHeapShapesBase += thingSize;
        break;
    }
    case JSTRACE_SCRIPT:
    {
        JSScript *script = static_cast<JSScript *>(thing);
        cStats->gcHeapScripts += thingSize;
        cStats->scriptData += script->sizeOfData(rtStats->mallocSizeOf);
#ifdef JS_METHODJIT
        cStats->jaegerData += script->sizeOfJitScripts(rtStats->mallocSizeOf);
# ifdef JS_ION
        if (script->hasIonScript())
            cStats->ionData += script->ion->sizeOfIncludingThis(rtStats->mallocSizeOf);
# endif
#endif

        ScriptSource *ss = script->scriptSource();
        SourceSet::AddPtr entry = closure->seenSources.lookupForAdd(ss);
        if (!entry) {
            closure->seenSources.add(entry, ss); 
            rtStats->runtime.scriptSources += ss->sizeOfIncludingThis(rtStats->mallocSizeOf);
        }
        break;
    }
    case JSTRACE_IONCODE:
    {
#ifdef JS_METHODJIT
# ifdef JS_ION
        cStats->gcHeapIonCodes += thingSize;
        
# endif
#endif
        break;
    }
    case JSTRACE_TYPE_OBJECT:
    {
        types::TypeObject *obj = static_cast<types::TypeObject *>(thing);
        cStats->gcHeapTypeObjects += thingSize;
        obj->sizeOfExcludingThis(&cStats->typeInferenceSizes, rtStats->mallocSizeOf);
        break;
    }
#if JS_HAS_XML_SUPPORT
    case JSTRACE_XML:
    {
        cStats->gcHeapXML += thingSize;
        break;
    }
#endif
    }
    
    cStats->gcHeapUnusedGcThings -= thingSize;
}

JS_PUBLIC_API(bool)
CollectRuntimeStats(JSRuntime *rt, RuntimeStats *rtStats, ObjectPrivateVisitor *opv)
{
    if (!rtStats->compartmentStatsVector.reserve(rt->compartments.length()))
        return false;

    rtStats->gcHeapChunkTotal =
        size_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * gc::ChunkSize;

    rtStats->gcHeapUnusedChunks =
        size_t(JS_GetGCParameter(rt, JSGC_UNUSED_CHUNKS)) * gc::ChunkSize;

    
    IterateChunks(rt, rtStats, StatsChunkCallback);

    
    IteratorClosure closure(rtStats, opv);
    if (!closure.init())
        return false;
    rtStats->runtime.scriptSources = 0;
    IterateCompartmentsArenasCells(rt, &closure, StatsCompartmentCallback,
                                   StatsArenaCallback, StatsCellCallback);

    
    rt->sizeOfIncludingThis(rtStats->mallocSizeOf, &rtStats->runtime);

    rtStats->gcHeapGcThings = 0;
    for (size_t i = 0; i < rtStats->compartmentStatsVector.length(); i++) {
        CompartmentStats &cStats = rtStats->compartmentStatsVector[i];

        rtStats->totals.add(cStats);
        rtStats->gcHeapGcThings += cStats.gcHeapThingsSize();
    }

    size_t numDirtyChunks =
        (rtStats->gcHeapChunkTotal - rtStats->gcHeapUnusedChunks) / gc::ChunkSize;
    size_t perChunkAdmin =
        sizeof(gc::Chunk) - (sizeof(gc::Arena) * gc::ArenasPerChunk);
    rtStats->gcHeapChunkAdmin = numDirtyChunks * perChunkAdmin;
    rtStats->gcHeapUnusedArenas -= rtStats->gcHeapChunkAdmin;

    
    
    rtStats->gcHeapUnusedArenas = rtStats->gcHeapChunkTotal -
                                  rtStats->gcHeapDecommittedArenas -
                                  rtStats->gcHeapUnusedChunks -
                                  rtStats->totals.gcHeapUnusedGcThings -
                                  rtStats->gcHeapChunkAdmin -
                                  rtStats->totals.gcHeapArenaAdmin -
                                  rtStats->gcHeapGcThings;
    return true;
}

JS_PUBLIC_API(int64_t)
GetExplicitNonHeapForRuntime(JSRuntime *rt, JSMallocSizeOfFun mallocSizeOf)
{
    
    size_t n = size_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * gc::ChunkSize;

    
    
    
    
    n += rt->sizeOfExplicitNonHeap();

    return int64_t(n);
}

JS_PUBLIC_API(size_t)
SystemCompartmentCount(const JSRuntime *rt)
{
    size_t n = 0;
    for (size_t i = 0; i < rt->compartments.length(); i++) {
        if (rt->compartments[i]->isSystemCompartment)
            ++n;
    }
    return n;
}

JS_PUBLIC_API(size_t)
UserCompartmentCount(const JSRuntime *rt)
{
    size_t n = 0;
    for (size_t i = 0; i < rt->compartments.length(); i++) {
        if (!rt->compartments[i]->isSystemCompartment)
            ++n;
    }
    return n;
}

} 

#endif 
