



































#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h






#include <string.h>

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/Utility.h"
#include "js/Vector.h"

namespace JS {


struct TypeInferenceMemoryStats
{
    int64_t scripts;
    int64_t objects;
    int64_t tables;
    int64_t temporary;
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

    int64_t gcHeapArenaHeaders;
    int64_t gcHeapArenaPadding;
    int64_t gcHeapArenaUnused;

    int64_t gcHeapObjectsNonFunction;
    int64_t gcHeapObjectsFunction;
    int64_t gcHeapStrings;
    int64_t gcHeapShapesTree;
    int64_t gcHeapShapesDict;
    int64_t gcHeapShapesBase;
    int64_t gcHeapScripts;
    int64_t gcHeapTypeObjects;
    int64_t gcHeapXML;

    int64_t objectSlots;
    int64_t stringChars;
    int64_t shapesExtraTreeTables;
    int64_t shapesExtraDictTables;
    int64_t shapesExtraTreeShapeKids;
    int64_t shapesCompartmentTables;
    int64_t scriptData;

#ifdef JS_METHODJIT
    int64_t mjitCode;
    int64_t mjitData;
#endif
    TypeInferenceMemoryStats typeInferenceMemory;
};

struct IterateData
{
    IterateData(JSMallocSizeOfFun mallocSizeOf, GetNameCallback getNameCb,
                DestroyNameCallback destroyNameCb)
      : runtimeObject(0)
      , runtimeAtomsTable(0)
      , runtimeContexts(0)
      , runtimeThreadsNormal(0)
      , runtimeThreadsTemporary(0)
      , runtimeThreadsRegexpCode(0)
      , runtimeThreadsStackCommitted(0)
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

    int64_t runtimeObject;
    int64_t runtimeAtomsTable;
    int64_t runtimeContexts;
    int64_t runtimeThreadsNormal;
    int64_t runtimeThreadsTemporary;
    int64_t runtimeThreadsRegexpCode;
    int64_t runtimeThreadsStackCommitted;
    int64_t gcHeapChunkTotal;
    int64_t gcHeapChunkCleanUnused;
    int64_t gcHeapChunkDirtyUnused;
    int64_t gcHeapChunkCleanDecommitted;
    int64_t gcHeapChunkDirtyDecommitted;
    int64_t gcHeapArenaUnused;
    int64_t gcHeapChunkAdmin;
    int64_t gcHeapUnusedPercentage;
    int64_t totalObjects;
    int64_t totalShapes;
    int64_t totalScripts;
    int64_t totalStrings;
#ifdef JS_METHODJIT
    int64_t totalMjit;
#endif
    int64_t totalTypeInference;
    int64_t totalAnalysisTemp;

    js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> compartmentStatsVector;
    CompartmentStats *currCompartmentStats;

    JSMallocSizeOfFun mallocSizeOf;
    GetNameCallback getNameCb;
    DestroyNameCallback destroyNameCb;
};

#ifdef JS_THREADSAFE

extern JS_PUBLIC_API(bool)
CollectCompartmentStatsForRuntime(JSRuntime *rt, IterateData *data);

extern JS_PUBLIC_API(bool)
GetExplicitNonHeapForRuntime(JSRuntime *rt, int64_t *amount,
                             JSMallocSizeOfFun mallocSizeOf);

#endif 

extern JS_PUBLIC_API(void)
SizeOfCompartmentTypeInferenceData(JSContext *cx, JSCompartment *compartment,
                                   TypeInferenceMemoryStats *stats,
                                   JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(void)
SizeOfObjectTypeInferenceData( void *object,
                              TypeInferenceMemoryStats *stats,
                              JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(size_t)
SizeOfCompartmentShapeTable(JSCompartment *c, JSMallocSizeOfFun mallocSizeOf);

extern JS_PUBLIC_API(size_t)
SystemCompartmentCount(const JSRuntime *rt);

extern JS_PUBLIC_API(size_t)
UserCompartmentCount(const JSRuntime *rt);

} 

#endif 
