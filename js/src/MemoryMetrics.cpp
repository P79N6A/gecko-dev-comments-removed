



































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
CompartmentMemoryCallback(JSContext *cx, void *vdata, JSCompartment *compartment)
{
    
    IterateData *data = static_cast<IterateData *>(vdata);

    
    MOZ_ALWAYS_TRUE(data->compartmentStatsVector.growBy(1));
    CompartmentStats &curr = data->compartmentStatsVector.back();
    curr.init(data->getNameCb(cx, compartment), data->destroyNameCb);
    data->currCompartmentStats = &curr;

    
#ifdef JS_METHODJIT
    curr.mjitCode = compartment->sizeOfMjitCode();
#endif
    SizeOfCompartmentTypeInferenceData(cx, compartment,
                                       &curr.typeInferenceMemory,
                                       data->mallocSizeOf);
    curr.shapesCompartmentTables =
        SizeOfCompartmentShapeTable(compartment, data->mallocSizeOf);
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
    
    
    IterateData *data = static_cast<IterateData *>(vdata);
    for (size_t i = 0; i < gc::ArenasPerChunk; i++)
        if (chunk->decommittedArenas.get(i))
            data->gcHeapChunkDirtyDecommitted += gc::ArenaSize;
}

static void
ArenaCallback(JSContext *cx, void *vdata, gc::Arena *arena,
              JSGCTraceKind traceKind, size_t thingSize)
{
    IterateData *data = static_cast<IterateData *>(vdata);

    data->currCompartmentStats->gcHeapArenaHeaders +=
        sizeof(gc::ArenaHeader);
    size_t allocationSpace = arena->thingsSpan(thingSize);
    data->currCompartmentStats->gcHeapArenaPadding +=
        gc::ArenaSize - allocationSpace - sizeof(gc::ArenaHeader);
    
    
    
    
    data->currCompartmentStats->gcHeapArenaUnused += allocationSpace;
}

static void
CellCallback(JSContext *cx, void *vdata, void *thing, JSGCTraceKind traceKind,
             size_t thingSize)
{
    IterateData *data = static_cast<IterateData *>(vdata);
    CompartmentStats *curr = data->currCompartmentStats;
    switch (traceKind) {
    case JSTRACE_OBJECT:
    {
        JSObject *obj = static_cast<JSObject *>(thing);
        if (obj->isFunction()) {
            curr->gcHeapObjectsFunction += thingSize;
        } else {
            curr->gcHeapObjectsNonFunction += thingSize;
        }
        size_t slotsSize, elementsSize;
        obj->sizeOfExcludingThis(data->mallocSizeOf, &slotsSize, &elementsSize);
        curr->objectSlots += slotsSize;
        curr->objectElements += elementsSize;
        break;
    }
    case JSTRACE_STRING:
    {
        JSString *str = static_cast<JSString *>(thing);
        curr->gcHeapStrings += thingSize;
        curr->stringChars += str->sizeOfExcludingThis(data->mallocSizeOf);
        break;
    }
    case JSTRACE_SHAPE:
    {
        Shape *shape = static_cast<Shape*>(thing);
        size_t propTableSize, kidsSize;
        shape->sizeOfExcludingThis(data->mallocSizeOf, &propTableSize, &kidsSize);
        if (shape->inDictionary()) {
            curr->gcHeapShapesDict += thingSize;
            curr->shapesExtraDictTables += propTableSize;
            JS_ASSERT(kidsSize == 0);
        } else {
            curr->gcHeapShapesTree += thingSize;
            curr->shapesExtraTreeTables += propTableSize;
            curr->shapesExtraTreeShapeKids += kidsSize;
        }
        break;
    }
    case JSTRACE_BASE_SHAPE:
    {
        curr->gcHeapShapesBase += thingSize;
        break;
    }
    case JSTRACE_SCRIPT:
    {
        JSScript *script = static_cast<JSScript *>(thing);
        curr->gcHeapScripts += thingSize;
        curr->scriptData += script->sizeOfData(data->mallocSizeOf);
#ifdef JS_METHODJIT
        curr->mjitData += script->sizeOfJitScripts(data->mallocSizeOf);
# ifdef JS_ION
        if (script->hasIonScript())
            curr->mjitData += script->ion->size();
# endif
#endif
        break;
    }
    case JSTRACE_IONCODE:
    {
        ion::IonCode *code = static_cast<ion::IonCode *>(thing);
	curr->mjitCode += code->bufferSize();
        break;
    }
    case JSTRACE_TYPE_OBJECT:
    {
        types::TypeObject *obj = static_cast<types::TypeObject *>(thing);
        curr->gcHeapTypeObjects += thingSize;
        SizeOfTypeObjectExcludingThis(obj, &curr->typeInferenceMemory,
                                      data->mallocSizeOf);
        break;
    }
    case JSTRACE_XML:
    {
        curr->gcHeapXML += thingSize;
        break;
    }
    }
    
    curr->gcHeapArenaUnused -= thingSize;
}

JS_PUBLIC_API(bool)
CollectCompartmentStatsForRuntime(JSRuntime *rt, IterateData *data)
{
    JSContext *cx = JS_NewContext(rt, 0);
    if (!cx)
        return false;

    {
        JSAutoRequest ar(cx);

        if (!data->compartmentStatsVector.reserve(rt->compartments.length()))
            return false;

        data->gcHeapChunkCleanDecommitted =
            rt->gcChunkPool.countCleanDecommittedArenas(rt) *
            gc::ArenaSize;
        data->gcHeapChunkCleanUnused =
            int64_t(JS_GetGCParameter(rt, JSGC_UNUSED_CHUNKS)) *
            gc::ChunkSize -
            data->gcHeapChunkCleanDecommitted;
        data->gcHeapChunkTotal =
            int64_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) *
            gc::ChunkSize;

        IterateCompartmentsArenasCells(cx, data, CompartmentMemoryCallback,
                                       ArenaCallback, CellCallback);
        IterateChunks(cx, data, ChunkCallback);

        data->runtimeObject = data->mallocSizeOf(rt);

        size_t normal, temporary, regexpCode, stackCommitted;
        rt->sizeOfExcludingThis(data->mallocSizeOf,
                                &normal,
                                &temporary,
                                &regexpCode,
                                &stackCommitted);

        data->runtimeNormal = normal;
        data->runtimeTemporary = temporary;
        data->runtimeRegexpCode = regexpCode;
        data->runtimeStackCommitted = stackCommitted;

        
        
        data->runtimeAtomsTable =
            rt->atomState.atoms.sizeOfExcludingThis(data->mallocSizeOf);

        JSContext *acx, *iter = NULL;
        while ((acx = JS_ContextIteratorUnlocked(rt, &iter)) != NULL)
            data->runtimeContexts += acx->sizeOfIncludingThis(data->mallocSizeOf);
    }

    JS_DestroyContextNoGC(cx);

    
    
    data->gcHeapChunkDirtyUnused = data->gcHeapChunkTotal -
                                   data->gcHeapChunkCleanUnused -
                                   data->gcHeapChunkCleanDecommitted -
                                   data->gcHeapChunkDirtyDecommitted;

    for (size_t index = 0;
         index < data->compartmentStatsVector.length();
         index++) {
        CompartmentStats &stats = data->compartmentStatsVector[index];

        int64_t used = stats.gcHeapArenaHeaders +
                       stats.gcHeapArenaPadding +
                       stats.gcHeapArenaUnused +
                       stats.gcHeapObjectsNonFunction +
                       stats.gcHeapObjectsFunction +
                       stats.gcHeapStrings +
                       stats.gcHeapShapesTree +
                       stats.gcHeapShapesDict +
                       stats.gcHeapShapesBase +
                       stats.gcHeapScripts +
                       stats.gcHeapTypeObjects +
                       stats.gcHeapXML;

        data->gcHeapChunkDirtyUnused -= used;
        data->gcHeapArenaUnused += stats.gcHeapArenaUnused;
        data->totalObjects += stats.gcHeapObjectsNonFunction +
                              stats.gcHeapObjectsFunction +
                              stats.objectSlots +
                              stats.objectElements;
        data->totalShapes  += stats.gcHeapShapesTree +
                              stats.gcHeapShapesDict +
                              stats.gcHeapShapesBase +
                              stats.shapesExtraTreeTables +
                              stats.shapesExtraDictTables +
                              stats.shapesCompartmentTables;
        data->totalScripts += stats.gcHeapScripts +
                              stats.scriptData;
        data->totalStrings += stats.gcHeapStrings +
                              stats.stringChars;
#ifdef JS_METHODJIT
        data->totalMjit    += stats.mjitCode +
                              stats.mjitData;
#endif
        data->totalTypeInference += stats.gcHeapTypeObjects +
                                    stats.typeInferenceMemory.objects +
                                    stats.typeInferenceMemory.scripts +
                                    stats.typeInferenceMemory.tables;
        data->totalAnalysisTemp  += stats.typeInferenceMemory.temporary;
    }

    size_t numDirtyChunks = (data->gcHeapChunkTotal -
                             data->gcHeapChunkCleanUnused) /
                            gc::ChunkSize;
    int64_t perChunkAdmin =
        sizeof(gc::Chunk) - (sizeof(gc::Arena) * gc::ArenasPerChunk);
    data->gcHeapChunkAdmin = numDirtyChunks * perChunkAdmin;
    data->gcHeapChunkDirtyUnused -= data->gcHeapChunkAdmin;

    
    
    
    data->gcHeapUnusedPercentage = (data->gcHeapChunkCleanUnused +
                                    data->gcHeapChunkDirtyUnused +
                                    data->gcHeapChunkCleanDecommitted +
                                    data->gcHeapChunkDirtyDecommitted +
                                    data->gcHeapArenaUnused) * 10000 /
                                   data->gcHeapChunkTotal;

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
