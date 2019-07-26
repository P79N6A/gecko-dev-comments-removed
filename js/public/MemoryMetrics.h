





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

namespace JS {

struct TabSizes
{
    enum Kind {
        Objects,
        Strings,
        Private,
        Other
    };

    TabSizes() { mozilla::PodZero(this); }

    void add(Kind kind, size_t n) {
        switch (kind) {
            case Objects: objects  += n; break;
            case Strings: strings  += n; break;
            case Private: private_ += n; break;
            case Other:   other    += n; break;
            default:      MOZ_CRASH("bad TabSizes kind");
        }
    }

    size_t objects;
    size_t strings;
    size_t private_;
    size_t other;
};

} 

namespace js {








JS_FRIEND_API(size_t) MemoryReportingSundriesThreshold();




struct InefficientNonFlatteningStringHashPolicy
{
    typedef JSString *Lookup;
    static HashNumber hash(const Lookup &l);
    static bool match(const JSString *const &k, const Lookup &l);
};












#define DECL_SIZE(kind, gc, mSize)                      size_t mSize;
#define ZERO_SIZE(kind, gc, mSize)                      mSize(0),
#define COPY_OTHER_SIZE(kind, gc, mSize)                mSize(other.mSize),
#define ADD_OTHER_SIZE(kind, gc, mSize)                 mSize += other.mSize;
#define ADD_SIZE_TO_N_IF_LIVE_GC_THING(kind, gc, mSize) n += (js::gc) ? mSize : 0;
#define ADD_TO_TAB_SIZES(kind, gc, mSize)               sizes->add(JS::TabSizes::kind, mSize);


enum {
    NotLiveGCThing = false,
    IsLiveGCThing = true
};

struct ZoneStatsPod
{
#define FOR_EACH_SIZE(macro) \
    macro(Other,   NotLiveGCThing, gcHeapArenaAdmin) \
    macro(Other,   NotLiveGCThing, unusedGCThings) \
    macro(Other,   IsLiveGCThing,  lazyScriptsGCHeap) \
    macro(Other,   NotLiveGCThing, lazyScriptsMallocHeap) \
    macro(Other,   IsLiveGCThing,  jitCodesGCHeap) \
    macro(Other,   IsLiveGCThing,  typeObjectsGCHeap) \
    macro(Other,   NotLiveGCThing, typeObjectsMallocHeap) \
    macro(Other,   NotLiveGCThing, typePool) \
    macro(Strings, IsLiveGCThing,  stringsGCHeap) \
    macro(Strings, NotLiveGCThing, stringsMallocHeap)

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

    void addToTabSizes(JS::TabSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
        
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
    macro(Objects, NotLiveGCThing, mallocHeapSlots) \
    macro(Objects, NotLiveGCThing, mallocHeapElementsNonAsmJS) \
    macro(Objects, NotLiveGCThing, mallocHeapElementsAsmJS) \
    macro(Objects, NotLiveGCThing, nonHeapElementsAsmJS) \
    macro(Objects, NotLiveGCThing, nonHeapCodeAsmJS) \
    macro(Objects, NotLiveGCThing, mallocHeapAsmJSModuleData) \
    macro(Objects, NotLiveGCThing, mallocHeapArgumentsData) \
    macro(Objects, NotLiveGCThing, mallocHeapRegExpStatics) \
    macro(Objects, NotLiveGCThing, mallocHeapPropertyIteratorData) \
    macro(Objects, NotLiveGCThing, mallocHeapCtypesData)

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

    void addToTabSizes(TabSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
    }

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};


struct CodeSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(_, _, ion) \
    macro(_, _, baseline) \
    macro(_, _, regexp) \
    macro(_, _, other) \
    macro(_, _, unused)

    CodeSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};


struct GCSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(_, _, marker) \
    macro(_, _, nursery) \
    macro(_, _, storeBufferVals) \
    macro(_, _, storeBufferCells) \
    macro(_, _, storeBufferSlots) \
    macro(_, _, storeBufferWholeCells) \
    macro(_, _, storeBufferRelocVals) \
    macro(_, _, storeBufferRelocCells) \
    macro(_, _, storeBufferGenerics)

    GCSizes()
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
      : numCopies(0),
        gcHeap(0),
        mallocHeap(0)
    {}

    StringInfo(size_t gcSize, size_t mallocSize)
      : numCopies(1),
        gcHeap(gcSize),
        mallocHeap(mallocSize)
    {}

    void add(size_t gcSize, size_t mallocSize) {
        numCopies++;
        gcHeap += gcSize;
        mallocHeap += mallocSize;
    }

    void add(const StringInfo& info) {
        numCopies += info.numCopies;
        gcHeap += info.gcHeap;
        mallocHeap += info.mallocHeap;
    }

    uint32_t numCopies;     

    
    size_t gcHeap;
    size_t mallocHeap;
};







struct NotableStringInfo : public StringInfo
{
    NotableStringInfo();
    NotableStringInfo(JSString *str, const StringInfo &info);
    NotableStringInfo(NotableStringInfo &&info);
    NotableStringInfo &operator=(NotableStringInfo &&info);

    ~NotableStringInfo() {
        js_free(buffer);
    }

    
    
    static size_t notableSize() {
        return js::MemoryReportingSundriesThreshold();
    }

    char *buffer;
    size_t length;

  private:
    NotableStringInfo(const NotableStringInfo& info) MOZ_DELETE;
};



struct RuntimeSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(_, _, object) \
    macro(_, _, atomsTable) \
    macro(_, _, contexts) \
    macro(_, _, dtoa) \
    macro(_, _, temporary) \
    macro(_, _, regexpData) \
    macro(_, _, interpreterStack) \
    macro(_, _, mathCache) \
    macro(_, _, sourceDataCache) \
    macro(_, _, scriptData) \
    macro(_, _, scriptSources)

    RuntimeSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        code(),
        gc()
    {}

    FOR_EACH_SIZE(DECL_SIZE)
    CodeSizes code;
    GCSizes   gc;

#undef FOR_EACH_SIZE
};

struct ZoneStats : js::ZoneStatsPod
{
    ZoneStats()
      : strings(nullptr)
    {}

    ZoneStats(ZoneStats &&other)
      : ZoneStatsPod(mozilla::Move(other)),
        strings(other.strings),
        notableStrings(mozilla::Move(other.notableStrings))
    {
        other.strings = nullptr;
    }

    bool initStrings(JSRuntime *rt);

    
    
    void addIgnoringStrings(const ZoneStats &other) {
        ZoneStatsPod::add(other);
    }

    
    
    void addStrings(const ZoneStats &other) {
        for (StringsHashMap::Range r = other.strings->all(); !r.empty(); r.popFront()) {
            StringsHashMap::AddPtr p = strings->lookupForAdd(r.front().key());
            if (p) {
                
                p->value().add(r.front().value());
            } else {
                
                strings->add(p, r.front().key(), r.front().value());
            }
        }
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = ZoneStatsPod::sizeOfLiveGCThings();
        for (size_t i = 0; i < notableStrings.length(); i++) {
            const JS::NotableStringInfo& info = notableStrings[i];
            n += info.gcHeap;
        }
        return n;
    }

    typedef js::HashMap<JSString*,
                        StringInfo,
                        js::InefficientNonFlatteningStringHashPolicy,
                        js::SystemAllocPolicy> StringsHashMap;

    
    
    
    
    StringsHashMap *strings;
    js::Vector<NotableStringInfo, 0, js::SystemAllocPolicy> notableStrings;
};

struct CompartmentStats
{
#define FOR_EACH_SIZE(macro) \
    macro(Objects, IsLiveGCThing,  objectsGCHeapOrdinary) \
    macro(Objects, IsLiveGCThing,  objectsGCHeapFunction) \
    macro(Objects, IsLiveGCThing,  objectsGCHeapDenseArray) \
    macro(Objects, IsLiveGCThing,  objectsGCHeapSlowArray) \
    macro(Objects, IsLiveGCThing,  objectsGCHeapCrossCompartmentWrapper) \
    macro(Private, NotLiveGCThing, objectsPrivate) \
    macro(Other,   IsLiveGCThing,  shapesGCHeapTreeGlobalParented) \
    macro(Other,   IsLiveGCThing,  shapesGCHeapTreeNonGlobalParented) \
    macro(Other,   IsLiveGCThing,  shapesGCHeapDict) \
    macro(Other,   IsLiveGCThing,  shapesGCHeapBase) \
    macro(Other,   NotLiveGCThing, shapesMallocHeapTreeTables) \
    macro(Other,   NotLiveGCThing, shapesMallocHeapDictTables) \
    macro(Other,   NotLiveGCThing, shapesMallocHeapTreeShapeKids) \
    macro(Other,   NotLiveGCThing, shapesMallocHeapCompartmentTables) \
    macro(Other,   IsLiveGCThing,  scriptsGCHeap) \
    macro(Other,   NotLiveGCThing, scriptsMallocHeapData) \
    macro(Other,   NotLiveGCThing, baselineData) \
    macro(Other,   NotLiveGCThing, baselineStubsFallback) \
    macro(Other,   NotLiveGCThing, baselineStubsOptimized) \
    macro(Other,   NotLiveGCThing, ionData) \
    macro(Other,   NotLiveGCThing, typeInferenceTypeScripts) \
    macro(Other,   NotLiveGCThing, typeInferenceAllocationSiteTables) \
    macro(Other,   NotLiveGCThing, typeInferenceArrayTypeTables) \
    macro(Other,   NotLiveGCThing, typeInferenceObjectTypeTables) \
    macro(Other,   NotLiveGCThing, compartmentObject) \
    macro(Other,   NotLiveGCThing, crossCompartmentWrappersTable) \
    macro(Other,   NotLiveGCThing, regexpCompartment) \
    macro(Other,   NotLiveGCThing, debuggeesSet)

    CompartmentStats()
      : FOR_EACH_SIZE(ZERO_SIZE)
        objectsExtra(),
        extra()
    {}

    CompartmentStats(const CompartmentStats &other)
      : FOR_EACH_SIZE(COPY_OTHER_SIZE)
        objectsExtra(other.objectsExtra),
        extra(other.extra)
    {}

    void add(const CompartmentStats &other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
        objectsExtra.add(other.objectsExtra);
        
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        n += objectsExtra.sizeOfLiveGCThings();
        
        return n;
    }

    void addToTabSizes(TabSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES);
        objectsExtra.addToTabSizes(sizes);
        
    }

    FOR_EACH_SIZE(DECL_SIZE)
    ObjectsExtraSizes  objectsExtra;
    void               *extra;  

#undef FOR_EACH_SIZE
};

typedef js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> CompartmentStatsVector;
typedef js::Vector<ZoneStats, 0, js::SystemAllocPolicy> ZoneStatsVector;

struct RuntimeStats
{
#define FOR_EACH_SIZE(macro) \
    macro(_, _, gcHeapChunkTotal) \
    macro(_, _, gcHeapDecommittedArenas) \
    macro(_, _, gcHeapUnusedChunks) \
    macro(_, _, gcHeapUnusedArenas) \
    macro(_, _, gcHeapChunkAdmin) \
    macro(_, _, gcHeapGCThings) \

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

    CompartmentStatsVector compartmentStatsVector;
    ZoneStatsVector zoneStatsVector;

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

extern JS_PUBLIC_API(bool)
AddSizeOfTab(JSRuntime *rt, JS::HandleObject obj, mozilla::MallocSizeOf mallocSizeOf,
             ObjectPrivateVisitor *opv, TabSizes *sizes);

} 

#undef DECL_SIZE
#undef ZERO_SIZE
#undef COPY_OTHER_SIZE
#undef ADD_OTHER_SIZE
#undef ADD_SIZE_TO_N_IF_LIVE_GC_THING
#undef ADD_TO_TAB_SIZES

#endif 
