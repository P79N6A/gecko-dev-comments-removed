






#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h




#include <string.h>

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/Utility.h"
#include "js/Vector.h"

class nsISupports;      

namespace js {








JS_FRIEND_API(size_t) MemoryReportingSundriesThreshold();

} 

namespace JS {


struct ObjectsExtraSizes {
    size_t slots;
    size_t elements;
    size_t argumentsData;
    size_t regExpStatics;
    size_t propertyIteratorData;
    size_t ctypesData;
    size_t private_;    
                        

    ObjectsExtraSizes() { memset(this, 0, sizeof(ObjectsExtraSizes)); }

    void add(ObjectsExtraSizes &sizes) {
        this->slots                += sizes.slots;
        this->elements             += sizes.elements;
        this->argumentsData        += sizes.argumentsData;
        this->regExpStatics        += sizes.regExpStatics;
        this->propertyIteratorData += sizes.propertyIteratorData;
        this->ctypesData           += sizes.ctypesData;
        this->private_             += sizes.private_;
    }
};


struct TypeInferenceSizes
{
    size_t typeScripts;
    size_t typeResults;
    size_t analysisPool;
    size_t typePool;
    size_t pendingArrays;
    size_t allocationSiteTables;
    size_t arrayTypeTables;
    size_t objectTypeTables;
    size_t typeObjects;

    TypeInferenceSizes() { memset(this, 0, sizeof(TypeInferenceSizes)); }

    void add(TypeInferenceSizes &sizes) {
        this->typeScripts          += sizes.typeScripts;
        this->typeResults          += sizes.typeResults;
        this->analysisPool         += sizes.analysisPool;
        this->typePool             += sizes.typePool;
        this->pendingArrays        += sizes.pendingArrays;
        this->allocationSiteTables += sizes.allocationSiteTables;
        this->arrayTypeTables      += sizes.arrayTypeTables;
        this->objectTypeTables     += sizes.objectTypeTables;
        this->typeObjects          += sizes.typeObjects;
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
    RuntimeSizes() { memset(this, 0, sizeof(RuntimeSizes)); }

    size_t object;
    size_t atomsTable;
    size_t contexts;
    size_t dtoa;
    size_t temporary;
    size_t jaegerCode;
    size_t ionCode;
    size_t regexpCode;
    size_t unusedCode;
    size_t regexpData;
    size_t stack;
    size_t gcMarker;
    size_t mathCache;
    size_t scriptData;
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
      , gcHeapShapesTreeGlobalParented(0)
      , gcHeapShapesTreeNonGlobalParented(0)
      , gcHeapShapesDict(0)
      , gcHeapShapesBase(0)
      , gcHeapScripts(0)
      , gcHeapTypeObjects(0)
      , gcHeapIonCodes(0)
      , objectsExtra()
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
      , typeInference()
      , hugeStrings()
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
      , gcHeapShapesTreeGlobalParented(other.gcHeapShapesTreeGlobalParented)
      , gcHeapShapesTreeNonGlobalParented(other.gcHeapShapesTreeNonGlobalParented)
      , gcHeapShapesDict(other.gcHeapShapesDict)
      , gcHeapShapesBase(other.gcHeapShapesBase)
      , gcHeapScripts(other.gcHeapScripts)
      , gcHeapTypeObjects(other.gcHeapTypeObjects)
      , gcHeapIonCodes(other.gcHeapIonCodes)
      , objectsExtra(other.objectsExtra)
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
      , typeInference(other.typeInference)
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
    size_t gcHeapShapesTreeGlobalParented;
    size_t gcHeapShapesTreeNonGlobalParented;
    size_t gcHeapShapesDict;
    size_t gcHeapShapesBase;
    size_t gcHeapScripts;
    size_t gcHeapTypeObjects;
    size_t gcHeapIonCodes;
    ObjectsExtraSizes objectsExtra;

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

    TypeInferenceSizes typeInference;
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
        ADD(gcHeapShapesTreeGlobalParented);
        ADD(gcHeapShapesTreeNonGlobalParented);
        ADD(gcHeapShapesDict);
        ADD(gcHeapShapesBase);
        ADD(gcHeapScripts);
        ADD(gcHeapTypeObjects);
        ADD(gcHeapIonCodes);
        objectsExtra.add(cStats.objectsExtra);

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

        typeInference.add(cStats.typeInference);
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
      , mallocSizeOf_(mallocSizeOf)
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

    JSMallocSizeOfFun mallocSizeOf_;

    virtual void initExtraCompartmentStats(JSCompartment *c, CompartmentStats *cstats) = 0;
};

#ifdef JS_THREADSAFE

class ObjectPrivateVisitor
{
public:
    
    
    virtual size_t sizeOfIncludingThis(nsISupports *aSupports) = 0;

    
    
    typedef JSBool(*GetISupportsFun)(JSObject *obj, nsISupports **iface);
    GetISupportsFun getISupports_;

    ObjectPrivateVisitor(GetISupportsFun getISupports)
      : getISupports_(getISupports)
    {}
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
