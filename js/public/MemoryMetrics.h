



































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

typedef void* (* GetNameCallback)(JSContext *cx, JSCompartment *c);
typedef void (* DestroyNameCallback)(void *string);

struct CompartmentStats
{
    CompartmentStats()
    {
        memset(this, 0, sizeof(*this));
    }

    void init(void *name_, DestroyNameCallback destroyName)
    {
        name = name_;
        destroyNameCb = destroyName;
    }

    ~CompartmentStats()
    {
        destroyNameCb(name);
    }

    
    void *name;
    DestroyNameCallback destroyNameCb;

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

#ifdef JS_METHODJIT
    size_t mjitCode;
    size_t mjitData;
#endif
    TypeInferenceSizes typeInferenceSizes;
};

struct RuntimeStats
{
    RuntimeStats(JSMallocSizeOfFun mallocSizeOf, GetNameCallback getNameCb,
                 DestroyNameCallback destroyNameCb)
      : runtimeObject(0)
      , runtimeAtomsTable(0)
      , runtimeContexts(0)
      , runtimeNormal(0)
      , runtimeTemporary(0)
      , runtimeRegexpCode(0)
      , runtimeStackCommitted(0)
      , gcHeapChunkTotal(0)
      , gcHeapChunkCleanUnused(0)
      , gcHeapChunkDirtyUnused(0)
      , gcHeapChunkCleanDecommitted(0)
      , gcHeapChunkDirtyDecommitted(0)
      , gcHeapArenaUnused(0)
      , gcHeapChunkAdmin(0)
      , gcHeapUnusedPercentage(0)
      , totalObjects(0)
      , totalShapes(0)
      , totalScripts(0)
      , totalStrings(0)
#ifdef JS_METHODJIT
      , totalMjit(0)
#endif
      , totalTypeInference(0)
      , totalAnalysisTemp(0)
      , compartmentStatsVector()
      , currCompartmentStats(NULL)
      , mallocSizeOf(mallocSizeOf)
      , getNameCb(getNameCb)
      , destroyNameCb(destroyNameCb)
    {}

    size_t runtimeObject;
    size_t runtimeAtomsTable;
    size_t runtimeContexts;
    size_t runtimeNormal;
    size_t runtimeTemporary;
    size_t runtimeRegexpCode;
    size_t runtimeStackCommitted;
    size_t gcHeapChunkTotal;
    size_t gcHeapChunkCleanUnused;
    size_t gcHeapChunkDirtyUnused;
    size_t gcHeapChunkCleanDecommitted;
    size_t gcHeapChunkDirtyDecommitted;
    size_t gcHeapArenaUnused;
    size_t gcHeapChunkAdmin;
    size_t gcHeapUnusedPercentage;
    size_t totalObjects;
    size_t totalShapes;
    size_t totalScripts;
    size_t totalStrings;
#ifdef JS_METHODJIT
    size_t totalMjit;
#endif
    size_t totalTypeInference;
    size_t totalAnalysisTemp;

    js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> compartmentStatsVector;
    CompartmentStats *currCompartmentStats;

    JSMallocSizeOfFun mallocSizeOf;
    GetNameCallback getNameCb;
    DestroyNameCallback destroyNameCb;
};

#ifdef JS_THREADSAFE

extern JS_PUBLIC_API(bool)
CollectRuntimeStats(JSRuntime *rt, RuntimeStats *rtStats);

extern JS_PUBLIC_API(bool)
GetExplicitNonHeapForRuntime(JSRuntime *rt, int64_t *amount,
                             JSMallocSizeOfFun mallocSizeOf);

#endif 

extern JS_PUBLIC_API(size_t)
SystemCompartmentCount(const JSRuntime *rt);

extern JS_PUBLIC_API(size_t)
UserCompartmentCount(const JSRuntime *rt);

} 

#endif 
