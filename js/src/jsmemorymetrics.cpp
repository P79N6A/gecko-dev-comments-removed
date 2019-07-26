






#include "js/MemoryMetrics.h"

#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"

#include "jsapi.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"
#include "jsobj.h"
#include "jsscript.h"

#include "ion/Ion.h"
#include "ion/IonCode.h"
#include "vm/Shape.h"

#include "jsobjinlines.h"

using mozilla::DebugOnly;

using namespace js;

using JS::RuntimeStats;
using JS::ObjectPrivateVisitor;
using JS::ZoneStats;
using JS::CompartmentStats;

JS_FRIEND_API(size_t)
js::MemoryReportingSundriesThreshold()
{
    return 8 * 1024;
}

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
ZoneStats::GCHeapThingsSize()
{
    
    size_t n = 0;
    n += gcHeapStringsNormal;
    n += gcHeapStringsShort;
    n += gcHeapTypeObjects;
    n += gcHeapIonCodes;

    return n;
}

size_t
CompartmentStats::GCHeapThingsSize()
{
    
    size_t n = 0;
    n += gcHeapObjectsOrdinary;
    n += gcHeapObjectsFunction;
    n += gcHeapObjectsDenseArray;
    n += gcHeapObjectsSlowArray;
    n += gcHeapObjectsCrossCompartmentWrapper;
    n += gcHeapShapesTreeGlobalParented;
    n += gcHeapShapesTreeNonGlobalParented;
    n += gcHeapShapesDict;
    n += gcHeapShapesBase;
    n += gcHeapScripts;

    return n;
}

static void
DecommittedArenasChunkCallback(JSRuntime *rt, void *data, gc::Chunk *chunk)
{
    
    if (chunk->decommittedArenas.isAllClear())
        return;

    size_t n = 0;
    for (size_t i = 0; i < gc::ArenasPerChunk; i++) {
        if (chunk->decommittedArenas.get(i))
            n += gc::ArenaSize;
    }
    JS_ASSERT(n > 0);
    *static_cast<size_t *>(data) += n;
}

static void
StatsCompartmentCallback(JSRuntime *rt, void *data, JSCompartment *compartment)
{
    
    RuntimeStats *rtStats = static_cast<IteratorClosure *>(data)->rtStats;

    
    MOZ_ALWAYS_TRUE(rtStats->compartmentStatsVector.growBy(1));
    CompartmentStats &cStats = rtStats->compartmentStatsVector.back();
    rtStats->initExtraCompartmentStats(compartment, &cStats);

    compartment->compartmentStats = &cStats;

    
    compartment->sizeOfIncludingThis(rtStats->mallocSizeOf_,
                                     &cStats.compartmentObject,
                                     &cStats.typeInference,
                                     &cStats.shapesCompartmentTables,
                                     &cStats.crossCompartmentWrappersTable,
                                     &cStats.regexpCompartment,
                                     &cStats.debuggeesSet);
}

static void
StatsZoneCallback(JSRuntime *rt, void *data, Zone *zone)
{
    
    RuntimeStats *rtStats = static_cast<IteratorClosure *>(data)->rtStats;

    
    MOZ_ALWAYS_TRUE(rtStats->zoneStatsVector.growBy(1));
    ZoneStats &zStats = rtStats->zoneStatsVector.back();
    rtStats->initExtraZoneStats(zone, &zStats);
    rtStats->currZoneStats = &zStats;

    zone->sizeOfIncludingThis(rtStats->mallocSizeOf_,
                              &zStats.typePool);
}

static void
StatsArenaCallback(JSRuntime *rt, void *data, gc::Arena *arena,
                   JSGCTraceKind traceKind, size_t thingSize)
{
    RuntimeStats *rtStats = static_cast<IteratorClosure *>(data)->rtStats;

    
    
    size_t allocationSpace = arena->thingsSpan(thingSize);
    rtStats->currZoneStats->gcHeapArenaAdmin += gc::ArenaSize - allocationSpace;

    
    
    
    
    rtStats->currZoneStats->gcHeapUnusedGcThings += allocationSpace;
}

static CompartmentStats *
GetCompartmentStats(JSCompartment *comp)
{
    return static_cast<CompartmentStats *>(comp->compartmentStats);
}

static void
StatsCellCallback(JSRuntime *rt, void *data, void *thing, JSGCTraceKind traceKind,
                  size_t thingSize)
{
    IteratorClosure *closure = static_cast<IteratorClosure *>(data);
    RuntimeStats *rtStats = closure->rtStats;
    ZoneStats *zStats = rtStats->currZoneStats;
    switch (traceKind) {
      case JSTRACE_OBJECT: {
        JSObject *obj = static_cast<JSObject *>(thing);
        CompartmentStats *cStats = GetCompartmentStats(obj->compartment());
        if (obj->isFunction())
            cStats->gcHeapObjectsFunction += thingSize;
        else if (obj->isArray())
            cStats->gcHeapObjectsDenseArray += thingSize;
        else if (obj->isCrossCompartmentWrapper())
            cStats->gcHeapObjectsCrossCompartmentWrapper += thingSize;
        else
            cStats->gcHeapObjectsOrdinary += thingSize;

        JS::ObjectsExtraSizes objectsExtra;
        obj->sizeOfExcludingThis(rtStats->mallocSizeOf_, &objectsExtra);
        cStats->objectsExtra.add(objectsExtra);

        
        
        if (ObjectPrivateVisitor *opv = closure->opv) {
            nsISupports *iface;
            if (opv->getISupports_(obj, &iface) && iface) {
                cStats->objectsExtra.private_ += opv->sizeOfIncludingThis(iface);
            }
        }
        break;
      }

      case JSTRACE_STRING: {
        JSString *str = static_cast<JSString *>(thing);

        size_t strSize = str->sizeOfExcludingThis(rtStats->mallocSizeOf_);

        
        
        if (strSize >= JS::HugeStringInfo::MinSize() && zStats->hugeStrings.growBy(1)) {
            zStats->gcHeapStringsNormal += thingSize;
            JS::HugeStringInfo &info = zStats->hugeStrings.back();
            info.length = str->length();
            info.size = strSize;
            PutEscapedString(info.buffer, sizeof(info.buffer), &str->asLinear(), 0);
        } else if (str->isShort()) {
            MOZ_ASSERT(strSize == 0);
            zStats->gcHeapStringsShort += thingSize;
        } else {
            zStats->gcHeapStringsNormal += thingSize;
            zStats->stringCharsNonHuge += strSize;
        }
        break;
      }

      case JSTRACE_SHAPE: {
        RawShape shape = static_cast<RawShape>(thing);
        CompartmentStats *cStats = GetCompartmentStats(shape->compartment());
        size_t propTableSize, kidsSize;
        shape->sizeOfExcludingThis(rtStats->mallocSizeOf_, &propTableSize, &kidsSize);
        if (shape->inDictionary()) {
            cStats->gcHeapShapesDict += thingSize;
            cStats->shapesExtraDictTables += propTableSize;
            JS_ASSERT(kidsSize == 0);
        } else {
            if (shape->base()->getObjectParent() == shape->compartment()->maybeGlobal()) {
                cStats->gcHeapShapesTreeGlobalParented += thingSize;
            } else {
                cStats->gcHeapShapesTreeNonGlobalParented += thingSize;
            }
            cStats->shapesExtraTreeTables += propTableSize;
            cStats->shapesExtraTreeShapeKids += kidsSize;
        }
        break;
      }

      case JSTRACE_BASE_SHAPE: {
        RawBaseShape base = static_cast<RawBaseShape>(thing);
        CompartmentStats *cStats = GetCompartmentStats(base->compartment());
        cStats->gcHeapShapesBase += thingSize;
        break;
      }

      case JSTRACE_SCRIPT: {
        JSScript *script = static_cast<JSScript *>(thing);
        CompartmentStats *cStats = GetCompartmentStats(script->compartment());
        cStats->gcHeapScripts += thingSize;
        cStats->scriptData += script->sizeOfData(rtStats->mallocSizeOf_);
#ifdef JS_METHODJIT
        cStats->jaegerData += script->sizeOfJitScripts(rtStats->mallocSizeOf_);
# ifdef JS_ION
        cStats->ionData += ion::MemoryUsed(script, rtStats->mallocSizeOf_);
# endif
#endif

        ScriptSource *ss = script->scriptSource();
        SourceSet::AddPtr entry = closure->seenSources.lookupForAdd(ss);
        if (!entry) {
            closure->seenSources.add(entry, ss); 
            rtStats->runtime.scriptSources += ss->sizeOfIncludingThis(rtStats->mallocSizeOf_);
        }
        break;
      }

      case JSTRACE_IONCODE: {
#ifdef JS_METHODJIT
# ifdef JS_ION
        zStats->gcHeapIonCodes += thingSize;
        
# endif
#endif
        break;
      }

      case JSTRACE_TYPE_OBJECT: {
        types::TypeObject *obj = static_cast<types::TypeObject *>(thing);
        zStats->gcHeapTypeObjects += thingSize;
        zStats->typeObjects += obj->sizeOfExcludingThis(rtStats->mallocSizeOf_);
        break;
      }

    }

    
    zStats->gcHeapUnusedGcThings -= thingSize;
}

JS_PUBLIC_API(bool)
JS::CollectRuntimeStats(JSRuntime *rt, RuntimeStats *rtStats, ObjectPrivateVisitor *opv)
{
    if (!rtStats->compartmentStatsVector.reserve(rt->numCompartments))
        return false;

    if (!rtStats->zoneStatsVector.reserve(rt->zones.length()))
        return false;

    rtStats->gcHeapChunkTotal =
        size_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * gc::ChunkSize;

    rtStats->gcHeapUnusedChunks =
        size_t(JS_GetGCParameter(rt, JSGC_UNUSED_CHUNKS)) * gc::ChunkSize;

    IterateChunks(rt, &rtStats->gcHeapDecommittedArenas,
                  DecommittedArenasChunkCallback);

    
    IteratorClosure closure(rtStats, opv);
    if (!closure.init())
        return false;
    rtStats->runtime.scriptSources = 0;
    IterateZonesCompartmentsArenasCells(rt, &closure, StatsZoneCallback, StatsCompartmentCallback,
                                        StatsArenaCallback, StatsCellCallback);

    
    rt->sizeOfIncludingThis(rtStats->mallocSizeOf_, &rtStats->runtime);

    DebugOnly<size_t> totalArenaSize = 0;

    rtStats->gcHeapGcThings = 0;
    for (size_t i = 0; i < rtStats->zoneStatsVector.length(); i++) {
        ZoneStats &zStats = rtStats->zoneStatsVector[i];

        rtStats->zTotals.add(zStats);
        rtStats->gcHeapGcThings += zStats.GCHeapThingsSize();
#ifdef DEBUG
        totalArenaSize += zStats.gcHeapArenaAdmin + zStats.gcHeapUnusedGcThings;
#endif
    }

    for (size_t i = 0; i < rtStats->compartmentStatsVector.length(); i++) {
        CompartmentStats &cStats = rtStats->compartmentStatsVector[i];

        rtStats->cTotals.add(cStats);
        rtStats->gcHeapGcThings += cStats.GCHeapThingsSize();
    }

#ifdef DEBUG
    totalArenaSize += rtStats->gcHeapGcThings;
    JS_ASSERT(totalArenaSize % gc::ArenaSize == 0);
#endif

    for (CompartmentsIter comp(rt); !comp.done(); comp.next())
        comp->compartmentStats = NULL;

    size_t numDirtyChunks =
        (rtStats->gcHeapChunkTotal - rtStats->gcHeapUnusedChunks) / gc::ChunkSize;
    size_t perChunkAdmin =
        sizeof(gc::Chunk) - (sizeof(gc::Arena) * gc::ArenasPerChunk);
    rtStats->gcHeapChunkAdmin = numDirtyChunks * perChunkAdmin;
    rtStats->gcHeapUnusedArenas -= rtStats->gcHeapChunkAdmin;

    
    
    rtStats->gcHeapUnusedArenas = rtStats->gcHeapChunkTotal -
                                  rtStats->gcHeapDecommittedArenas -
                                  rtStats->gcHeapUnusedChunks -
                                  rtStats->zTotals.gcHeapUnusedGcThings -
                                  rtStats->gcHeapChunkAdmin -
                                  rtStats->zTotals.gcHeapArenaAdmin -
                                  rtStats->gcHeapGcThings;
    return true;
}

JS_PUBLIC_API(int64_t)
JS::GetExplicitNonHeapForRuntime(JSRuntime *rt, JSMallocSizeOfFun mallocSizeOf)
{
    
    size_t n = size_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * gc::ChunkSize;

    
    size_t decommittedArenas = 0;
    IterateChunks(rt, &decommittedArenas, DecommittedArenasChunkCallback);
    n -= decommittedArenas;

    
    n += rt->sizeOfNonHeapAsmJSArrays_;

    
    
    
    
    n += rt->sizeOfExplicitNonHeap();

    return int64_t(n);
}

JS_PUBLIC_API(size_t)
JS::SystemCompartmentCount(JSRuntime *rt)
{
    size_t n = 0;
    for (CompartmentsIter comp(rt); !comp.done(); comp.next()) {
        if (comp->isSystem)
            ++n;
    }
    return n;
}

JS_PUBLIC_API(size_t)
JS::UserCompartmentCount(JSRuntime *rt)
{
    size_t n = 0;
    for (CompartmentsIter comp(rt); !comp.done(); comp.next()) {
        if (!comp->isSystem)
            ++n;
    }
    return n;
}

JS_PUBLIC_API(size_t)
JS::PeakSizeOfTemporary(const JSRuntime *rt)
{
    return rt->tempLifoAlloc.peakSizeOfExcludingThis();
}

