





#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h




#include "mozilla/MemoryReporting.h"
#include "mozilla/NullPtr.h"
#include "mozilla/PodOperations.h"

#include <string.h>

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/HashTable.h"
#include "js/Utility.h"
#include "js/Vector.h"

class nsISupports;      

namespace js {








JS_FRIEND_API(size_t) MemoryReportingSundriesThreshold();




struct InefficientNonFlatteningStringHashPolicy
{
    typedef JSString *Lookup;
    static HashNumber hash(const Lookup &l);
    static bool match(const JSString *const &k, const Lookup &l);
};












#define DECL_SIZE(gc, mSize)                      size_t mSize;
#define ZERO_SIZE(gc, mSize)                      mSize(0),
#define COPY_OTHER_SIZE(gc, mSize)                mSize(other.mSize),
#define ADD_OTHER_SIZE(gc, mSize)                 mSize += other.mSize;
#define ADD_SIZE_TO_N_IF_LIVE_GC_THING(gc, mSize) n += (gc == js::IsLiveGCThing) ? mSize : 0;


enum {
    IsLiveGCThing,
    NotLiveGCThing
};

struct ZoneStatsPod
{
#define FOR_EACH_SIZE(macro) \
    macro(NotLiveGCThing, gcHeapArenaAdmin) \
    macro(NotLiveGCThing, unusedGCThings) \
    macro(IsLiveGCThing,  lazyScriptsGCHeap) \
    macro(NotLiveGCThing, lazyScriptsMallocHeap) \
    macro(IsLiveGCThing,  ionCodesGCHeap) \
    macro(IsLiveGCThing,  typeObjectsGCHeap) \
    macro(NotLiveGCThing, typeObjectsMallocHeap) \
    macro(NotLiveGCThing, typePool) \
    macro(IsLiveGCThing,  stringsShortGCHeap) \
    macro(IsLiveGCThing,  stringsNormalGCHeap) \
    macro(NotLiveGCThing, stringsNormalMallocHeap)

    ZoneStatsPod()
      : FOR_EACH_SIZE(ZERO_SIZE)
        extra()
    {}

    void add(const ZoneStatsPod &other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
        
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        
        return n;
    }

    FOR_EACH_SIZE(DECL_SIZE)
    void *extra;    

#undef FOR_EACH_SIZE
};

} 

namespace JS {


struct ObjectsExtraSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(js::NotLiveGCThing, mallocHeapSlots) \
    macro(js::NotLiveGCThing, mallocHeapElementsNonAsmJS) \
    macro(js::NotLiveGCThing, mallocHeapElementsAsmJS) \
    macro(js::NotLiveGCThing, nonHeapElementsAsmJS) \
    macro(js::NotLiveGCThing, nonHeapCodeAsmJS) \
    macro(js::NotLiveGCThing, mallocHeapAsmJSModuleData) \
    macro(js::NotLiveGCThing, mallocHeapArgumentsData) \
    macro(js::NotLiveGCThing, mallocHeapRegExpStatics) \
    macro(js::NotLiveGCThing, mallocHeapPropertyIteratorData) \
    macro(js::NotLiveGCThing, mallocHeapCtypesData)

    ObjectsExtraSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    void add(const ObjectsExtraSizes &other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        return n;
    }

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};


struct TypeInferenceSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(js::NotLiveGCThing, typeScripts) \
    macro(js::NotLiveGCThing, pendingArrays) \
    macro(js::NotLiveGCThing, allocationSiteTables) \
    macro(js::NotLiveGCThing, arrayTypeTables) \
    macro(js::NotLiveGCThing, objectTypeTables)

    TypeInferenceSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    void add(const TypeInferenceSizes &other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        return n;
    }

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};


struct CodeSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(_, ion) \
    macro(_, baseline) \
    macro(_, regexp) \
    macro(_, other) \
    macro(_, unused)

    CodeSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};




struct StringInfo
{
    StringInfo()
      : length(0), numCopies(0), shortGCHeap(0), normalGCHeap(0), normalMallocHeap(0)
    {}

    StringInfo(size_t len, size_t shorts, size_t normals, size_t chars)
      : length(len),
        numCopies(1),
        shortGCHeap(shorts),
        normalGCHeap(normals),
        normalMallocHeap(chars)
    {}

    void add(size_t shorts, size_t normals, size_t chars) {
        shortGCHeap += shorts;
        normalGCHeap += normals;
        normalMallocHeap += chars;
        numCopies++;
    }

    void add(const StringInfo& info) {
        MOZ_ASSERT(length == info.length);

        shortGCHeap += info.shortGCHeap;
        normalGCHeap += info.normalGCHeap;
        normalMallocHeap += info.normalMallocHeap;
        numCopies += info.numCopies;
    }

    size_t totalSizeOf() const {
        return shortGCHeap + normalGCHeap + normalMallocHeap;
    }

    size_t totalGCHeapSizeOf() const {
        return shortGCHeap + normalGCHeap;
    }

    
    size_t length;

    
    size_t numCopies;

    
    size_t shortGCHeap;
    size_t normalGCHeap;
    size_t normalMallocHeap;
};







struct NotableStringInfo : public StringInfo
{
    NotableStringInfo();
    NotableStringInfo(JSString *str, const StringInfo &info);
    NotableStringInfo(const NotableStringInfo& info);
    NotableStringInfo(mozilla::MoveRef<NotableStringInfo> info);
    NotableStringInfo &operator=(mozilla::MoveRef<NotableStringInfo> info);

    ~NotableStringInfo() {
        js_free(buffer);
    }

    
    
    static size_t notableSize() {
        return js::MemoryReportingSundriesThreshold();
    }

    
    
    size_t bufferSize;
    char *buffer;
};



struct RuntimeSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(_, object) \
    macro(_, atomsTable) \
    macro(_, contexts) \
    macro(_, dtoa) \
    macro(_, temporary) \
    macro(_, regexpData) \
    macro(_, interpreterStack) \
    macro(_, gcMarker) \
    macro(_, mathCache) \
    macro(_, scriptData) \
    macro(_, scriptSources)

    RuntimeSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        code()
    {}

    FOR_EACH_SIZE(DECL_SIZE)
    CodeSizes code;

#undef FOR_EACH_SIZE
};

struct ZoneStats : js::ZoneStatsPod
{
    ZoneStats() {
        strings.init();
    }

    ZoneStats(mozilla::MoveRef<ZoneStats> other)
        : ZoneStatsPod(other),
          strings(mozilla::OldMove(other->strings)),
          notableStrings(mozilla::OldMove(other->notableStrings))
    {}

    
    
    
    
    
    void add(const ZoneStats &other) {
        ZoneStatsPod::add(other);

        MOZ_ASSERT(notableStrings.empty());
        MOZ_ASSERT(other.notableStrings.empty());

        for (StringsHashMap::Range r = other.strings.all(); !r.empty(); r.popFront()) {
            StringsHashMap::AddPtr p = strings.lookupForAdd(r.front().key);
            if (p) {
                
                p->value.add(r.front().value);
            } else {
                
                strings.add(p, r.front().key, r.front().value);
            }
        }
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = ZoneStatsPod::sizeOfLiveGCThings();
        for (size_t i = 0; i < notableStrings.length(); i++) {
            const JS::NotableStringInfo& info = notableStrings[i];
            n += info.totalGCHeapSizeOf();
        }
        return n;
    }

    typedef js::HashMap<JSString*,
                        StringInfo,
                        js::InefficientNonFlatteningStringHashPolicy,
                        js::SystemAllocPolicy> StringsHashMap;

    StringsHashMap strings;
    js::Vector<NotableStringInfo, 0, js::SystemAllocPolicy> notableStrings;
};

struct CompartmentStats
{
#define FOR_EACH_SIZE(macro) \
    macro(js::IsLiveGCThing,  objectsGCHeapOrdinary) \
    macro(js::IsLiveGCThing,  objectsGCHeapFunction) \
    macro(js::IsLiveGCThing,  objectsGCHeapDenseArray) \
    macro(js::IsLiveGCThing,  objectsGCHeapSlowArray) \
    macro(js::IsLiveGCThing,  objectsGCHeapCrossCompartmentWrapper) \
    macro(js::NotLiveGCThing, objectsPrivate) \
    macro(js::IsLiveGCThing,  shapesGCHeapTreeGlobalParented) \
    macro(js::IsLiveGCThing,  shapesGCHeapTreeNonGlobalParented) \
    macro(js::IsLiveGCThing,  shapesGCHeapDict) \
    macro(js::IsLiveGCThing,  shapesGCHeapBase) \
    macro(js::NotLiveGCThing, shapesMallocHeapTreeTables) \
    macro(js::NotLiveGCThing, shapesMallocHeapDictTables) \
    macro(js::NotLiveGCThing, shapesMallocHeapTreeShapeKids) \
    macro(js::NotLiveGCThing, shapesMallocHeapCompartmentTables) \
    macro(js::IsLiveGCThing,  scriptsGCHeap) \
    macro(js::NotLiveGCThing, scriptsMallocHeapData) \
    macro(js::NotLiveGCThing, baselineData) \
    macro(js::NotLiveGCThing, baselineStubsFallback) \
    macro(js::NotLiveGCThing, baselineStubsOptimized) \
    macro(js::NotLiveGCThing, ionData) \
    macro(js::NotLiveGCThing, compartmentObject) \
    macro(js::NotLiveGCThing, crossCompartmentWrappersTable) \
    macro(js::NotLiveGCThing, regexpCompartment) \
    macro(js::NotLiveGCThing, debuggeesSet) \

    CompartmentStats()
      : FOR_EACH_SIZE(ZERO_SIZE)
        objectsExtra(),
        typeInference(),
        extra()
    {}

    CompartmentStats(const CompartmentStats &other)
      : FOR_EACH_SIZE(COPY_OTHER_SIZE)
        objectsExtra(other.objectsExtra),
        typeInference(other.typeInference),
        extra(other.extra)
    {}

    void add(const CompartmentStats &other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
        objectsExtra.add(other.objectsExtra);
        typeInference.add(other.typeInference);
        
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        n += objectsExtra.sizeOfLiveGCThings();
        n += typeInference.sizeOfLiveGCThings();
        
        return n;
    }

    FOR_EACH_SIZE(DECL_SIZE)
    ObjectsExtraSizes  objectsExtra;
    TypeInferenceSizes typeInference;
    void               *extra;  

#undef FOR_EACH_SIZE
};

struct RuntimeStats
{
#define FOR_EACH_SIZE(macro) \
    macro(_, gcHeapChunkTotal) \
    macro(_, gcHeapDecommittedArenas) \
    macro(_, gcHeapUnusedChunks) \
    macro(_, gcHeapUnusedArenas) \
    macro(_, gcHeapChunkAdmin) \
    macro(_, gcHeapGCThings) \

    RuntimeStats(mozilla::MallocSizeOf mallocSizeOf)
      : FOR_EACH_SIZE(ZERO_SIZE)
        runtime(),
        cTotals(),
        zTotals(),
        compartmentStatsVector(),
        zoneStatsVector(),
        currZoneStats(nullptr),
        mallocSizeOf_(mallocSizeOf)
    {}

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    FOR_EACH_SIZE(DECL_SIZE)

    RuntimeSizes runtime;

    CompartmentStats cTotals;   
    ZoneStats zTotals;          

    js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> compartmentStatsVector;
    js::Vector<ZoneStats, 0, js::SystemAllocPolicy> zoneStatsVector;

    ZoneStats *currZoneStats;

    mozilla::MallocSizeOf mallocSizeOf_;

    virtual void initExtraCompartmentStats(JSCompartment *c, CompartmentStats *cstats) = 0;
    virtual void initExtraZoneStats(JS::Zone *zone, ZoneStats *zstats) = 0;

#undef FOR_EACH_SIZE
};

class ObjectPrivateVisitor
{
  public:
    
    
    virtual size_t sizeOfIncludingThis(nsISupports *aSupports) = 0;

    
    
    typedef bool(*GetISupportsFun)(JSObject *obj, nsISupports **iface);
    GetISupportsFun getISupports_;

    ObjectPrivateVisitor(GetISupportsFun getISupports)
      : getISupports_(getISupports)
    {}
};

extern JS_PUBLIC_API(bool)
CollectRuntimeStats(JSRuntime *rt, RuntimeStats *rtStats, ObjectPrivateVisitor *opv);

extern JS_PUBLIC_API(size_t)
SystemCompartmentCount(JSRuntime *rt);

extern JS_PUBLIC_API(size_t)
UserCompartmentCount(JSRuntime *rt);

extern JS_PUBLIC_API(size_t)
PeakSizeOfTemporary(const JSRuntime *rt);

} 

#endif 
