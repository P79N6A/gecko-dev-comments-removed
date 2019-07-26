





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
#define ADD_SIZE_TO_N_IF_LIVE_GC_THING(kind, gc, mSize) n += (js::gc == js::IsLiveGCThing) ? mSize : 0;
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
    macro(Other,   IsLiveGCThing,  ionCodesGCHeap) \
    macro(Other,   IsLiveGCThing,  typeObjectsGCHeap) \
    macro(Other,   NotLiveGCThing, typeObjectsMallocHeap) \
    macro(Other,   NotLiveGCThing, typePool) \
    macro(Strings, IsLiveGCThing,  stringsShortGCHeap) \
    macro(Strings, IsLiveGCThing,  stringsNormalGCHeap) \
    macro(Strings, NotLiveGCThing, stringsNormalMallocHeap)

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
    macro(_, _, object) \
    macro(_, _, atomsTable) \
    macro(_, _, contexts) \
    macro(_, _, dtoa) \
    macro(_, _, temporary) \
    macro(_, _, regexpData) \
    macro(_, _, interpreterStack) \
    macro(_, _, gcMarker) \
    macro(_, _, mathCache) \
    macro(_, _, scriptData) \
    macro(_, _, scriptSources)

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
    macro(Other,   NotLiveGCThing, typeInferencePendingArrays) \
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

extern JS_PUBLIC_API(bool)
AddSizeOfTab(JSRuntime *rt, JSObject *obj, mozilla::MallocSizeOf mallocSizeOf,
             ObjectPrivateVisitor *opv, TabSizes *sizes);

} 

#undef DECL_SIZE
#undef ZERO_SIZE
#undef COPY_OTHER_SIZE
#undef ADD_OTHER_SIZE
#undef ADD_SIZE_TO_N_IF_LIVE_GC_THING
#undef ADD_TO_TAB_SIZES

#endif 
