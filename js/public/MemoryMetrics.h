






































#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h






#include <string.h>

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/Utility.h"
#include "js/Vector.h"

namespace JS {


struct TypeInferenceSizes
{
    size_t scripts;
    size_t objects;
    size_t tables;
    size_t temporary;
};

struct CompartmentStats
{
    CompartmentStats() {
        memset(this, 0, sizeof(*this));
    }

    void   *extra;
    size_t gcHeapArenaHeaders;
    size_t gcHeapArenaPadding;
    size_t gcHeapArenaUnused;

    size_t gcHeapObjectsNonFunction;
    size_t gcHeapObjectsFunction;
    size_t gcHeapStrings;
    size_t gcHeapShapesTree;
    size_t gcHeapShapesDict;
    size_t gcHeapShapesBase;
    size_t gcHeapScripts;
    size_t gcHeapTypeObjects;
    size_t gcHeapXML;

    size_t objectSlots;
    size_t objectElements;
    size_t objectMisc;
    size_t stringChars;
    size_t shapesExtraTreeTables;
    size_t shapesExtraDictTables;
    size_t shapesExtraTreeShapeKids;
    size_t shapesCompartmentTables;
    size_t scriptData;
    size_t mjitData;

    TypeInferenceSizes typeInferenceSizes;
};

struct RuntimeStats
{
    RuntimeStats(JSMallocSizeOfFun mallocSizeOf)
      : runtimeObject(0)
      , runtimeAtomsTable(0)
      , runtimeContexts(0)
      , runtimeNormal(0)
      , runtimeTemporary(0)
      , runtimeMjitCode(0)
      , runtimeRegexpCode(0)
      , runtimeUnusedCodeMemory(0)
      , runtimeStackCommitted(0)
      , runtimeGCMarker(0)
      , gcHeapChunkTotal(0)
      , gcHeapCommitted(0)
      , gcHeapUnused(0)
      , gcHeapChunkCleanUnused(0)
      , gcHeapChunkDirtyUnused(0)
      , gcHeapChunkCleanDecommitted(0)
      , gcHeapChunkDirtyDecommitted(0)
      , gcHeapArenaUnused(0)
      , gcHeapChunkAdmin(0)
      , totalObjects(0)
      , totalShapes(0)
      , totalScripts(0)
      , totalStrings(0)
      , totalMjit(0)
      , totalTypeInference(0)
      , totalAnalysisTemp(0)
      , compartmentStatsVector()
      , currCompartmentStats(NULL)
      , mallocSizeOf(mallocSizeOf)
    {}

    size_t runtimeObject;
    size_t runtimeAtomsTable;
    size_t runtimeContexts;
    size_t runtimeNormal;
    size_t runtimeTemporary;
    size_t runtimeMjitCode;
    size_t runtimeRegexpCode;
    size_t runtimeUnusedCodeMemory;
    size_t runtimeStackCommitted;
    size_t runtimeGCMarker;
    size_t gcHeapChunkTotal;
    size_t gcHeapCommitted;
    size_t gcHeapUnused;
    size_t gcHeapChunkCleanUnused;
    size_t gcHeapChunkDirtyUnused;
    size_t gcHeapChunkCleanDecommitted;
    size_t gcHeapChunkDirtyDecommitted;
    size_t gcHeapArenaUnused;
    size_t gcHeapChunkAdmin;
    size_t totalObjects;
    size_t totalShapes;
    size_t totalScripts;
    size_t totalStrings;
    size_t totalMjit;
    size_t totalTypeInference;
    size_t totalAnalysisTemp;

    js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> compartmentStatsVector;
    CompartmentStats *currCompartmentStats;

    JSMallocSizeOfFun mallocSizeOf;

    virtual void initExtraCompartmentStats(JSCompartment *c, CompartmentStats *cstats) = 0;
};

#ifdef JS_THREADSAFE

extern JS_PUBLIC_API(bool)
CollectRuntimeStats(JSRuntime *rt, RuntimeStats *rtStats);

extern JS_PUBLIC_API(int64_t)
GetExplicitNonHeapForRuntime(JSRuntime *rt, JSMallocSizeOfFun mallocSizeOf);

#endif 

extern JS_PUBLIC_API(size_t)
SystemCompartmentCount(const JSRuntime *rt);

extern JS_PUBLIC_API(size_t)
UserCompartmentCount(const JSRuntime *rt);

} 

#endif 
