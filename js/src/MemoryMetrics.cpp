



































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
CompartmentStatsCallback(JSContext *cx, void *vdata, JSCompartment *compartment)
{
    
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(vdata);

    
    MOZ_ALWAYS_TRUE(rtStats->compartmentStatsVector.growBy(1));
    CompartmentStats &cStats = rtStats->compartmentStatsVector.back();
    cStats.init(rtStats->getNameCb(cx, compartment), rtStats->destroyNameCb);
    rtStats->currCompartmentStats = &cStats;

    
#ifdef JS_METHODJIT
    cStats.mjitCode = compartment->sizeOfMjitCode();
#endif
    compartment->sizeOfTypeInferenceData(cx, &cStats.typeInferenceSizes, rtStats->mallocSizeOf);
    cStats.shapesCompartmentTables = compartment->sizeOfShapeTable(rtStats->mallocSizeOf);
}

static void
ExplicitNonHeapCompartmentCallback(JSContext *cx, void *data, JSCompartment *compartment)
{
#ifdef JS_METHODJIT
    size_t *n = static_cast<size_t *>(data);
    *n += compartment->sizeOfMjitCode();
#endif
}

static void
ChunkCallback(JSContext *cx, void *vdata, gc::Chunk *chunk)
{
    
    
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(vdata);
    for (size_t i = 0; i < gc::ArenasPerChunk; i++)
        if (chunk->decommittedArenas.get(i))
            rtStats->gcHeapChunkDirtyDecommitted += gc::ArenaSize;
}

static void
ArenaCallback(JSContext *cx, void *vdata, gc::Arena *arena,
              JSGCTraceKind traceKind, size_t thingSize)
{
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(vdata);

    rtStats->currCompartmentStats->gcHeapArenaHeaders += sizeof(gc::ArenaHeader);
    size_t allocationSpace = arena->thingsSpan(thingSize);
    rtStats->currCompartmentStats->gcHeapArenaPadding +=
        gc::ArenaSize - allocationSpace - sizeof(gc::ArenaHeader);
    
    
    
    
    rtStats->currCompartmentStats->gcHeapArenaUnused += allocationSpace;
}

static void
CellCallback(JSContext *cx, void *vdata, void *thing, JSGCTraceKind traceKind,
             size_t thingSize)
{
    RuntimeStats *rtStats = static_cast<RuntimeStats *>(vdata);
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
        size_t slotsSize, elementsSize;
        obj->sizeOfExcludingThis(rtStats->mallocSizeOf, &slotsSize, &elementsSize);
        cStats->objectSlots += slotsSize;
        cStats->objectElements += elementsSize;
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
    JSContext *cx = JS_NewContext(rt, 0);
    if (!cx)
        return false;

    {
        JSAutoRequest ar(cx);

        if (!rtStats->compartmentStatsVector.reserve(rt->compartments.length()))
            return false;

        rtStats->gcHeapChunkCleanDecommitted =
            rt->gcChunkPool.countCleanDecommittedArenas(rt) * gc::ArenaSize;
        rtStats->gcHeapChunkCleanUnused =
            size_t(JS_GetGCParameter(rt, JSGC_UNUSED_CHUNKS)) * gc::ChunkSize -
            rtStats->gcHeapChunkCleanDecommitted;
        rtStats->gcHeapChunkTotal =
            size_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * gc::ChunkSize;

        IterateCompartmentsArenasCells(cx, rtStats, CompartmentStatsCallback,
                                       ArenaCallback, CellCallback);
        IterateChunks(cx, rtStats, ChunkCallback);

        rtStats->runtimeObject = rtStats->mallocSizeOf(rt);

        rt->sizeOfExcludingThis(rtStats->mallocSizeOf,
                                &rtStats->runtimeNormal,
                                &rtStats->runtimeTemporary,
                                &rtStats->runtimeRegexpCode,
                                &rtStats->runtimeStackCommitted);

        
        
        rtStats->runtimeAtomsTable =
            rt->atomState.atoms.sizeOfExcludingThis(rtStats->mallocSizeOf);

        JSContext *acx, *iter = NULL;
        while ((acx = JS_ContextIteratorUnlocked(rt, &iter)) != NULL)
            rtStats->runtimeContexts += acx->sizeOfIncludingThis(rtStats->mallocSizeOf);
    }

    JS_DestroyContextNoGC(cx);

    
    
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
                                 cStats.objectElements;
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

JS_PUBLIC_API(bool)
GetExplicitNonHeapForRuntime(JSRuntime *rt, int64_t *amount,
                             JSMallocSizeOfFun mallocSizeOf)
{
    JSContext *cx = JS_NewContext(rt, 0);
    if (!cx)
        return false;

    
    *amount = int64_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) *
              gc::ChunkSize;

    {
        JSAutoRequest ar(cx);

        
        size_t n = 0;
        IterateCompartments(cx, &n, ExplicitNonHeapCompartmentCallback);
        *amount += n;

        
        
        size_t regexpCode, stackCommitted;
        rt->sizeOfExcludingThis(mallocSizeOf,
                                NULL,
                                NULL,
                                &regexpCode,
                                &stackCommitted);

        *amount += regexpCode;
        *amount += stackCommitted;
    }

    JS_DestroyContextNoGC(cx);

    return true;
}

} 

#endif 
