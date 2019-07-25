



































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

#ifdef JS_THREADSAFE

namespace JS {

using namespace js;

static void
StatsCompartmentCallback(JSRuntime *rt, void *data, JSCompartment *compartment)
{
    
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(data);

    
    MOZ_ALWAYS_TRUE(rtStats->compartmentStatsVector.growBy(1));
    CompartmentStats &cStats = rtStats->compartmentStatsVector.back();
    cStats.init(rtStats->getNameCb(rt, compartment), rtStats->destroyNameCb);
    rtStats->currCompartmentStats = &cStats;

    
#ifdef JS_METHODJIT
    cStats.mjitCode = compartment->sizeOfMjitCode();
#endif
    compartment->sizeOfTypeInferenceData(&cStats.typeInferenceSizes, rtStats->mallocSizeOf);
    cStats.shapesCompartmentTables = compartment->sizeOfShapeTable(rtStats->mallocSizeOf);
}

static void
StatsChunkCallback(JSRuntime *rt, void *data, gc::Chunk *chunk)
{
    
    
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(data);
    for (size_t i = 0; i < gc::ArenasPerChunk; i++)
        if (chunk->decommittedArenas.get(i))
            rtStats->gcHeapChunkDirtyDecommitted += gc::ArenaSize;
}

static void
StatsArenaCallback(JSRuntime *rt, void *data, gc::Arena *arena,
                   JSGCTraceKind traceKind, size_t thingSize)
{
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(data);

    rtStats->currCompartmentStats->gcHeapArenaHeaders += sizeof(gc::ArenaHeader);
    size_t allocationSpace = arena->thingsSpan(thingSize);
    rtStats->currCompartmentStats->gcHeapArenaPadding +=
        gc::ArenaSize - allocationSpace - sizeof(gc::ArenaHeader);
    
    
    
    
    rtStats->currCompartmentStats->gcHeapArenaUnused += allocationSpace;
}

static void
StatsCellCallback(JSRuntime *rt, void *data, void *thing, JSGCTraceKind traceKind,
                  size_t thingSize)
{
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(data);
    CompartmentStats *cStats = rtStats->currCompartmentStats;
    switch (traceKind) {
    case JSTRACE_OBJECT:
    {
        JSObject *obj = static_cast<JSObject *>(thing);
        if (obj->isFunction()) {
            cStats->gcHeapObjectsFunction += thingSize;
        } else {
            cStats->gcHeapObjectsNonFunction += thingSize;
        }
        size_t slotsSize, elementsSize, miscSize;
        obj->sizeOfExcludingThis(rtStats->mallocSizeOf, &slotsSize,
                                 &elementsSize, &miscSize);
        cStats->objectSlots += slotsSize;
        cStats->objectElements += elementsSize;
        cStats->objectMisc += miscSize;
        break;
    }
    case JSTRACE_STRING:
    {
        JSString *str = static_cast<JSString *>(thing);
        cStats->gcHeapStrings += thingSize;
        cStats->stringChars += str->sizeOfExcludingThis(rtStats->mallocSizeOf);
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
        cStats->mjitData += script->sizeOfJitScripts(rtStats->mallocSizeOf);
# ifdef JS_ION
        if (script->hasIonScript())
            cStats->mjitData += script->ion->size();
# endif
#endif
        break;
    }
    case JSTRACE_IONCODE:
    {
#ifdef JS_ION
        ion::IonCode *code = static_cast<ion::IonCode *>(thing);
	    cStats->mjitCode += code->bufferSize();
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
    case JSTRACE_XML:
    {
        cStats->gcHeapXML += thingSize;
        break;
    }
    }
    
    cStats->gcHeapArenaUnused -= thingSize;
}

JS_PUBLIC_API(bool)
CollectRuntimeStats(JSRuntime *rt, RuntimeStats *rtStats)
{
    if (!rtStats->compartmentStatsVector.reserve(rt->compartments.length()))
        return false;
    
    rtStats->gcHeapChunkCleanDecommitted =
        rt->gcChunkPool.countCleanDecommittedArenas(rt) * gc::ArenaSize;
    rtStats->gcHeapChunkCleanUnused =
        size_t(JS_GetGCParameter(rt, JSGC_UNUSED_CHUNKS)) * gc::ChunkSize -
        rtStats->gcHeapChunkCleanDecommitted;
    rtStats->gcHeapChunkTotal =
        size_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * gc::ChunkSize;
    
    IterateCompartmentsArenasCells(rt, rtStats, StatsCompartmentCallback,
                                   StatsArenaCallback, StatsCellCallback);
    IterateChunks(rt, rtStats, StatsChunkCallback);
    
    rtStats->runtimeObject = rtStats->mallocSizeOf(rt);
    
    rt->sizeOfExcludingThis(rtStats->mallocSizeOf,
                            &rtStats->runtimeNormal,
                            &rtStats->runtimeTemporary,
                            &rtStats->runtimeRegexpCode,
                            &rtStats->runtimeStackCommitted,
                            &rtStats->runtimeGCMarker);
    
    rtStats->runtimeAtomsTable =
        rt->atomState.atoms.sizeOfExcludingThis(rtStats->mallocSizeOf);
    
    for (ContextIter acx(rt); !acx.done(); acx.next())
        rtStats->runtimeContexts += acx->sizeOfIncludingThis(rtStats->mallocSizeOf);

    
    
    rtStats->gcHeapChunkDirtyUnused = rtStats->gcHeapChunkTotal -
                                      rtStats->gcHeapChunkCleanUnused -
                                      rtStats->gcHeapChunkCleanDecommitted -
                                      rtStats->gcHeapChunkDirtyDecommitted;

    for (size_t index = 0;
         index < rtStats->compartmentStatsVector.length();
         index++) {
        CompartmentStats &cStats = rtStats->compartmentStatsVector[index];

        size_t used = cStats.gcHeapArenaHeaders +
                      cStats.gcHeapArenaPadding +
                      cStats.gcHeapArenaUnused +
                      cStats.gcHeapObjectsNonFunction +
                      cStats.gcHeapObjectsFunction +
                      cStats.gcHeapStrings +
                      cStats.gcHeapShapesTree +
                      cStats.gcHeapShapesDict +
                      cStats.gcHeapShapesBase +
                      cStats.gcHeapScripts +
                      cStats.gcHeapTypeObjects +
                      cStats.gcHeapXML;

        rtStats->gcHeapChunkDirtyUnused -= used;
        rtStats->gcHeapArenaUnused += cStats.gcHeapArenaUnused;
        rtStats->totalObjects += cStats.gcHeapObjectsNonFunction +
                                 cStats.gcHeapObjectsFunction +
                                 cStats.objectSlots +
                                 cStats.objectElements +
                                 cStats.objectMisc;
        rtStats->totalShapes  += cStats.gcHeapShapesTree +
                                 cStats.gcHeapShapesDict +
                                 cStats.gcHeapShapesBase +
                                 cStats.shapesExtraTreeTables +
                                 cStats.shapesExtraDictTables +
                                 cStats.shapesCompartmentTables;
        rtStats->totalScripts += cStats.gcHeapScripts +
                                 cStats.scriptData;
        rtStats->totalStrings += cStats.gcHeapStrings +
                                 cStats.stringChars;
#ifdef JS_METHODJIT
        rtStats->totalMjit    += cStats.mjitCode +
                                 cStats.mjitData;
#endif
        rtStats->totalTypeInference += cStats.gcHeapTypeObjects +
                                       cStats.typeInferenceSizes.objects +
                                       cStats.typeInferenceSizes.scripts +
                                       cStats.typeInferenceSizes.tables;
        rtStats->totalAnalysisTemp  += cStats.typeInferenceSizes.temporary;
    }

    size_t numDirtyChunks = (rtStats->gcHeapChunkTotal -
                             rtStats->gcHeapChunkCleanUnused) /
                            gc::ChunkSize;
    size_t perChunkAdmin =
        sizeof(gc::Chunk) - (sizeof(gc::Arena) * gc::ArenasPerChunk);
    rtStats->gcHeapChunkAdmin = numDirtyChunks * perChunkAdmin;
    rtStats->gcHeapChunkDirtyUnused -= rtStats->gcHeapChunkAdmin;

    
    
    
    rtStats->gcHeapUnusedPercentage = (rtStats->gcHeapChunkCleanUnused +
                                       rtStats->gcHeapChunkDirtyUnused +
                                       rtStats->gcHeapChunkCleanDecommitted +
                                       rtStats->gcHeapChunkDirtyDecommitted +
                                       rtStats->gcHeapArenaUnused) * 10000 /
                                       rtStats->gcHeapChunkTotal;

    return true;
}

static void
ExplicitNonHeapCompartmentCallback(JSRuntime *rt, void *data, JSCompartment *compartment)
{
#ifdef JS_METHODJIT
    size_t *n = static_cast<size_t *>(data);
    *n += compartment->sizeOfMjitCode();
#endif
}

JS_PUBLIC_API(int64_t)
GetExplicitNonHeapForRuntime(JSRuntime *rt, JSMallocSizeOfFun mallocSizeOf)
{
    
    size_t n = size_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * gc::ChunkSize;

    
    JS_IterateCompartments(rt, &n, ExplicitNonHeapCompartmentCallback);
    
    
    
    size_t regexpCode, stackCommitted;
    rt->sizeOfExcludingThis(mallocSizeOf,
                            NULL,
                            NULL,
                            &regexpCode,
                            &stackCommitted,
                            NULL);
    
    n += regexpCode;
    n += stackCommitted;

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
