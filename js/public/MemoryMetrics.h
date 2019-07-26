






#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h




#include <string.h>

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/Utility.h"
#include "js/Vector.h"

namespace js {








JS_FRIEND_API(size_t) MemoryReportingSundriesThreshold();

} 

namespace JS {


struct TypeInferenceSizes
{
    TypeInferenceSizes()
      : scripts(0)
      , objects(0)
      , tables(0)
      , temporary(0)
    {}

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



struct HugeStringInfo
{
    HugeStringInfo()
      : length(0)
      , size(0)
    {
        memset(&buffer, 0, sizeof(buffer));
    }

    
    
    static size_t MinSize()
    {
      return js::MemoryReportingSundriesThreshold();
    }

    
    
    size_t length;
    size_t size;

    
    
    char buffer[32];
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
      , stack(0)
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
    size_t stack;
    size_t gcMarker;
    size_t mathCache;
    size_t scriptFilenames;
    size_t scriptSources;
};

struct CompartmentStats
{
    CompartmentStats()
      : extra1(0)
      , extra2(0)
      , gcHeapArenaAdmin(0)
      , gcHeapUnusedGcThings(0)
      , gcHeapObjectsOrdinary(0)
      , gcHeapObjectsFunction(0)
      , gcHeapObjectsDenseArray(0)
      , gcHeapObjectsSlowArray(0)
      , gcHeapObjectsCrossCompartmentWrapper(0)
      , gcHeapStringsNormal(0)
      , gcHeapStringsShort(0)
      , gcHeapShapesTree(0)
      , gcHeapShapesDict(0)
      , gcHeapShapesBase(0)
      , gcHeapScripts(0)
      , gcHeapTypeObjects(0)
      , gcHeapIonCodes(0)
#if JS_HAS_XML_SUPPORT
      , gcHeapXML(0)
#endif
      , objectSlots(0)
      , objectElements(0)
      , objectMisc(0)
      , objectPrivate(0)
      , stringCharsNonHuge(0)
      , shapesExtraTreeTables(0)
      , shapesExtraDictTables(0)
      , shapesExtraTreeShapeKids(0)
      , shapesCompartmentTables(0)
      , scriptData(0)
      , jaegerData(0)
      , ionData(0)
      , compartmentObject(0)
      , crossCompartmentWrappersTable(0)
      , regexpCompartment(0)
      , debuggeesSet(0)
    {}

    CompartmentStats(const CompartmentStats &other)
      : extra1(other.extra1)
      , extra2(other.extra2)
      , gcHeapArenaAdmin(other.gcHeapArenaAdmin)
      , gcHeapUnusedGcThings(other.gcHeapUnusedGcThings)
      , gcHeapObjectsOrdinary(other.gcHeapObjectsOrdinary)
      , gcHeapObjectsFunction(other.gcHeapObjectsFunction)
      , gcHeapObjectsDenseArray(other.gcHeapObjectsDenseArray)
      , gcHeapObjectsSlowArray(other.gcHeapObjectsSlowArray)
      , gcHeapObjectsCrossCompartmentWrapper(other.gcHeapObjectsCrossCompartmentWrapper)
      , gcHeapStringsNormal(other.gcHeapStringsNormal)
      , gcHeapStringsShort(other.gcHeapStringsShort)
      , gcHeapShapesTree(other.gcHeapShapesTree)
      , gcHeapShapesDict(other.gcHeapShapesDict)
      , gcHeapShapesBase(other.gcHeapShapesBase)
      , gcHeapScripts(other.gcHeapScripts)
      , gcHeapTypeObjects(other.gcHeapTypeObjects)
      , gcHeapIonCodes(other.gcHeapIonCodes)
#if JS_HAS_XML_SUPPORT
      , gcHeapXML(other.gcHeapXML)
#endif
      , objectSlots(other.objectSlots)
      , objectElements(other.objectElements)
      , objectMisc(other.objectMisc)
      , objectPrivate(other.objectPrivate)
      , stringCharsNonHuge(other.stringCharsNonHuge)
      , shapesExtraTreeTables(other.shapesExtraTreeTables)
      , shapesExtraDictTables(other.shapesExtraDictTables)
      , shapesExtraTreeShapeKids(other.shapesExtraTreeShapeKids)
      , shapesCompartmentTables(other.shapesCompartmentTables)
      , scriptData(other.scriptData)
      , jaegerData(other.jaegerData)
      , ionData(other.ionData)
      , compartmentObject(other.compartmentObject)
      , crossCompartmentWrappersTable(other.crossCompartmentWrappersTable)
      , regexpCompartment(other.regexpCompartment)
      , debuggeesSet(other.debuggeesSet)
      , typeInferenceSizes(other.typeInferenceSizes)
    {
      hugeStrings.append(other.hugeStrings);
    }

    
    void   *extra1;
    void   *extra2;

    
    
    size_t gcHeapArenaAdmin;
    size_t gcHeapUnusedGcThings;

    size_t gcHeapObjectsOrdinary;
    size_t gcHeapObjectsFunction;
    size_t gcHeapObjectsDenseArray;
    size_t gcHeapObjectsSlowArray;
    size_t gcHeapObjectsCrossCompartmentWrapper;
    size_t gcHeapStringsNormal;
    size_t gcHeapStringsShort;
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
    size_t stringCharsNonHuge;
    size_t shapesExtraTreeTables;
    size_t shapesExtraDictTables;
    size_t shapesExtraTreeShapeKids;
    size_t shapesCompartmentTables;
    size_t scriptData;
    size_t jaegerData;
    size_t ionData;
    size_t compartmentObject;
    size_t crossCompartmentWrappersTable;
    size_t regexpCompartment;
    size_t debuggeesSet;

    TypeInferenceSizes typeInferenceSizes;
    js::Vector<HugeStringInfo, 0, js::SystemAllocPolicy> hugeStrings;

    
    void add(CompartmentStats &cStats)
    {
        #define ADD(x)  this->x += cStats.x

        ADD(gcHeapArenaAdmin);
        ADD(gcHeapUnusedGcThings);

        ADD(gcHeapObjectsOrdinary);
        ADD(gcHeapObjectsFunction);
        ADD(gcHeapObjectsDenseArray);
        ADD(gcHeapObjectsSlowArray);
        ADD(gcHeapObjectsCrossCompartmentWrapper);
        ADD(gcHeapStringsNormal);
        ADD(gcHeapStringsShort);
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
        ADD(stringCharsNonHuge);
        ADD(shapesExtraTreeTables);
        ADD(shapesExtraDictTables);
        ADD(shapesExtraTreeShapeKids);
        ADD(shapesCompartmentTables);
        ADD(scriptData);
        ADD(jaegerData);
        ADD(ionData);
        ADD(compartmentObject);
        ADD(crossCompartmentWrappersTable);
        ADD(regexpCompartment);
        ADD(debuggeesSet);

        #undef ADD

        typeInferenceSizes.add(cStats.typeInferenceSizes);
        hugeStrings.append(cStats.hugeStrings);
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
