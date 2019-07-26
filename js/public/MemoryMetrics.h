






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

    void add(TypeInferenceSizes &sizes) {
        this->scripts   += sizes.scripts;
        this->objects   += sizes.objects;
        this->tables    += sizes.tables;
        this->temporary += sizes.temporary;
    }
};



struct RuntimeSizes
{
    RuntimeSizes()
      : object(0)
      , atomsTable(0)
      , contexts(0)
      , dtoa(0)
      , temporary(0)
      , jaegerCode(0)
      , ionCode(0)
      , regexpCode(0)
      , unusedCode(0)
      , stackCommitted(0)
      , gcMarker(0)
      , mathCache(0)
      , scriptFilenames(0)
      , scriptSources(0)
    {}

    size_t object;
    size_t atomsTable;
    size_t contexts;
    size_t dtoa;
    size_t temporary;
    size_t jaegerCode;
    size_t ionCode;
    size_t regexpCode;
    size_t unusedCode;
    size_t stackCommitted;
    size_t gcMarker;
    size_t mathCache;
    size_t scriptFilenames;
    size_t scriptSources;
};

struct CompartmentStats
{
    CompartmentStats() {
        memset(this, 0, sizeof(*this));
    }

    
    void   *extra1;
    void   *extra2;

    
    
    size_t gcHeapArenaAdmin;
    size_t gcHeapUnusedGcThings;

    size_t gcHeapObjectsNonFunction;
    size_t gcHeapObjectsFunction;
    size_t gcHeapStrings;
    size_t gcHeapShapesTree;
    size_t gcHeapShapesDict;
    size_t gcHeapShapesBase;
    size_t gcHeapScripts;
    size_t gcHeapTypeObjects;
    size_t gcHeapIonCodes;
#if JS_HAS_XML_SUPPORT
    size_t gcHeapXML;
#endif

    size_t objectSlots;
    size_t objectElements;
    size_t objectMisc;
    size_t objectPrivate;
    size_t stringChars;
    size_t shapesExtraTreeTables;
    size_t shapesExtraDictTables;
    size_t shapesExtraTreeShapeKids;
    size_t shapesCompartmentTables;
    size_t scriptData;
    size_t jaegerData;
    size_t ionData;
    size_t compartmentObject;
    size_t crossCompartmentWrappers;
    size_t regexpCompartment;
    size_t debuggeesSet;

    TypeInferenceSizes typeInferenceSizes;

    
    void add(CompartmentStats &cStats) {
        #define ADD(x)  this->x += cStats.x

        ADD(gcHeapArenaAdmin);
        ADD(gcHeapUnusedGcThings);

        ADD(gcHeapObjectsNonFunction);
        ADD(gcHeapObjectsFunction);
        ADD(gcHeapStrings);
        ADD(gcHeapShapesTree);
        ADD(gcHeapShapesDict);
        ADD(gcHeapShapesBase);
        ADD(gcHeapScripts);
        ADD(gcHeapTypeObjects);
        ADD(gcHeapIonCodes);
    #if JS_HAS_XML_SUPPORT
        ADD(gcHeapXML);
    #endif

        ADD(objectSlots);
        ADD(objectElements);
        ADD(objectMisc);
        ADD(objectPrivate);
        ADD(stringChars);
        ADD(shapesExtraTreeTables);
        ADD(shapesExtraDictTables);
        ADD(shapesExtraTreeShapeKids);
        ADD(shapesCompartmentTables);
        ADD(scriptData);
        ADD(jaegerData);
        ADD(ionData);
        ADD(compartmentObject);
        ADD(crossCompartmentWrappers);
        ADD(regexpCompartment);
        ADD(debuggeesSet);

        #undef ADD

        typeInferenceSizes.add(cStats.typeInferenceSizes);
    }

    
    size_t gcHeapThingsSize();
};

struct RuntimeStats
{
    RuntimeStats(JSMallocSizeOfFun mallocSizeOf)
      : runtime()
      , gcHeapChunkTotal(0)
      , gcHeapDecommittedArenas(0)
      , gcHeapUnusedChunks(0)
      , gcHeapUnusedArenas(0)
      , gcHeapUnusedGcThings(0)
      , gcHeapChunkAdmin(0)
      , gcHeapGcThings(0)
      , totals()
      , compartmentStatsVector()
      , currCompartmentStats(NULL)
      , mallocSizeOf(mallocSizeOf)
    {}

    RuntimeSizes runtime;

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    size_t gcHeapChunkTotal;
    size_t gcHeapDecommittedArenas;
    size_t gcHeapUnusedChunks;
    size_t gcHeapUnusedArenas;
    size_t gcHeapUnusedGcThings;
    size_t gcHeapChunkAdmin;
    size_t gcHeapGcThings;

    
    CompartmentStats totals;
 
    js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> compartmentStatsVector;
    CompartmentStats *currCompartmentStats;

    JSMallocSizeOfFun mallocSizeOf;

    virtual void initExtraCompartmentStats(JSCompartment *c, CompartmentStats *cstats) = 0;
};

#ifdef JS_THREADSAFE

class ObjectPrivateVisitor
{
public:
    
    
    virtual size_t sizeOfIncludingThis(void *aSupports) = 0;
};

extern JS_PUBLIC_API(bool)
CollectRuntimeStats(JSRuntime *rt, RuntimeStats *rtStats, ObjectPrivateVisitor *opv);

extern JS_PUBLIC_API(int64_t)
GetExplicitNonHeapForRuntime(JSRuntime *rt, JSMallocSizeOfFun mallocSizeOf);

#endif 

extern JS_PUBLIC_API(size_t)
SystemCompartmentCount(const JSRuntime *rt);

extern JS_PUBLIC_API(size_t)
UserCompartmentCount(const JSRuntime *rt);

} 

#endif 
