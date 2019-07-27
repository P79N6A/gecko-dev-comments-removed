





#ifndef js_MemoryMetrics_h
#define js_MemoryMetrics_h




#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"

#include <string.h>

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/HashTable.h"
#include "js/TracingAPI.h"
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



struct ServoSizes
{
    enum Kind {
        GCHeapUsed,
        GCHeapUnused,
        GCHeapAdmin,
        GCHeapDecommitted,
        MallocHeap,
        NonHeap,
        Ignore
    };

    ServoSizes() { mozilla::PodZero(this); }

    void add(Kind kind, size_t n) {
        switch (kind) {
            case GCHeapUsed:        gcHeapUsed        += n; break;
            case GCHeapUnused:      gcHeapUnused      += n; break;
            case GCHeapAdmin:       gcHeapAdmin       += n; break;
            case GCHeapDecommitted: gcHeapDecommitted += n; break;
            case MallocHeap:        mallocHeap        += n; break;
            case NonHeap:           nonHeap           += n; break;
            case Ignore:                    break;
            default:                MOZ_CRASH("bad ServoSizes kind");
        }
    }

    size_t gcHeapUsed;
    size_t gcHeapUnused;
    size_t gcHeapAdmin;
    size_t gcHeapDecommitted;
    size_t mallocHeap;
    size_t nonHeap;
};

} 

namespace js {








JS_FRIEND_API(size_t) MemoryReportingSundriesThreshold();




struct InefficientNonFlatteningStringHashPolicy
{
    typedef JSString* Lookup;
    static HashNumber hash(const Lookup& l);
    static bool match(const JSString* const& k, const Lookup& l);
};

struct CStringHashPolicy
{
    typedef const char* Lookup;
    static HashNumber hash(const Lookup& l);
    static bool match(const char* const& k, const Lookup& l);
};

















#define DECL_SIZE(tabKind, servoKind, mSize)        size_t mSize;
#define ZERO_SIZE(tabKind, servoKind, mSize)        mSize(0),
#define COPY_OTHER_SIZE(tabKind, servoKind, mSize)  mSize(other.mSize),
#define ADD_OTHER_SIZE(tabKind, servoKind, mSize)   mSize += other.mSize;
#define SUB_OTHER_SIZE(tabKind, servoKind, mSize)   MOZ_ASSERT(mSize >= other.mSize); \
                                                    mSize -= other.mSize;
#define ADD_SIZE_TO_N(tabKind, servoKind, mSize)                  n += mSize;
#define ADD_SIZE_TO_N_IF_LIVE_GC_THING(tabKind, servoKind, mSize) n += (ServoSizes::servoKind == ServoSizes::GCHeapUsed) ? mSize : 0;
#define ADD_TO_TAB_SIZES(tabKind, servoKind, mSize)               sizes->add(JS::TabSizes::tabKind, mSize);
#define ADD_TO_SERVO_SIZES(tabKind, servoKind, mSize)             sizes->add(JS::ServoSizes::servoKind, mSize);

} 

namespace JS {

struct ClassInfo
{
#define FOR_EACH_SIZE(macro) \
    macro(Objects, GCHeapUsed, objectsGCHeap) \
    macro(Objects, MallocHeap, objectsMallocHeapSlots) \
    macro(Objects, MallocHeap, objectsMallocHeapElementsNonAsmJS) \
    macro(Objects, MallocHeap, objectsMallocHeapElementsAsmJS) \
    macro(Objects, NonHeap,    objectsNonHeapElementsAsmJS) \
    macro(Objects, NonHeap,    objectsNonHeapElementsMapped) \
    macro(Objects, NonHeap,    objectsNonHeapCodeAsmJS) \
    macro(Objects, MallocHeap, objectsMallocHeapMisc) \
    \
    macro(Other,   GCHeapUsed, shapesGCHeapTree) \
    macro(Other,   GCHeapUsed, shapesGCHeapDict) \
    macro(Other,   GCHeapUsed, shapesGCHeapBase) \
    macro(Other,   MallocHeap, shapesMallocHeapTreeTables) \
    macro(Other,   MallocHeap, shapesMallocHeapDictTables) \
    macro(Other,   MallocHeap, shapesMallocHeapTreeKids)

    ClassInfo()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    void add(const ClassInfo& other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
    }

    void subtract(const ClassInfo& other) {
        FOR_EACH_SIZE(SUB_OTHER_SIZE)
    }

    size_t sizeOfAllThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N)
        return n;
    }

    bool isNotable() const {
        static const size_t NotabilityThreshold = 16 * 1024;
        return sizeOfAllThings() >= NotabilityThreshold;
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        return n;
    }

    void addToTabSizes(TabSizes* sizes) const {
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
    }

    void addToServoSizes(ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
    }

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};







struct NotableClassInfo : public ClassInfo
{
    NotableClassInfo();
    NotableClassInfo(const char* className, const ClassInfo& info);
    NotableClassInfo(NotableClassInfo&& info);
    NotableClassInfo& operator=(NotableClassInfo&& info);

    ~NotableClassInfo() {
        js_free(className_);
    }

    char* className_;

  private:
    NotableClassInfo(const NotableClassInfo& info) = delete;
};


struct CodeSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(_, NonHeap, ion) \
    macro(_, NonHeap, baseline) \
    macro(_, NonHeap, regexp) \
    macro(_, NonHeap, other) \
    macro(_, NonHeap, unused)

    CodeSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    void addToServoSizes(ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
    }

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};


struct GCSizes
{
    
    
#define FOR_EACH_SIZE(macro) \
    macro(_, MallocHeap, marker) \
    macro(_, NonHeap,    nurseryCommitted) \
    macro(_, NonHeap,    nurseryDecommitted) \
    macro(_, MallocHeap, nurseryMallocedBuffers) \
    macro(_, MallocHeap, storeBufferVals) \
    macro(_, MallocHeap, storeBufferCells) \
    macro(_, MallocHeap, storeBufferSlots) \
    macro(_, MallocHeap, storeBufferWholeCells) \
    macro(_, MallocHeap, storeBufferRelocVals) \
    macro(_, MallocHeap, storeBufferRelocCells) \
    macro(_, MallocHeap, storeBufferGenerics)

    GCSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    void addToServoSizes(ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
    }

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};






struct StringInfo
{
#define FOR_EACH_SIZE(macro) \
    macro(Strings, GCHeapUsed, gcHeapLatin1) \
    macro(Strings, GCHeapUsed, gcHeapTwoByte) \
    macro(Strings, MallocHeap, mallocHeapLatin1) \
    macro(Strings, MallocHeap, mallocHeapTwoByte)

    StringInfo()
      : FOR_EACH_SIZE(ZERO_SIZE)
        numCopies(0)
    {}

    void add(const StringInfo& other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE);
        numCopies++;
    }

    void subtract(const StringInfo& other) {
        FOR_EACH_SIZE(SUB_OTHER_SIZE);
        numCopies--;
    }

    bool isNotable() const {
        static const size_t NotabilityThreshold = 16 * 1024;
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N)
        return n >= NotabilityThreshold;
    }

    size_t sizeOfLiveGCThings() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        return n;
    }

    void addToTabSizes(TabSizes* sizes) const {
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
    }

    void addToServoSizes(ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
    }

    FOR_EACH_SIZE(DECL_SIZE)
    uint32_t numCopies;     

#undef FOR_EACH_SIZE
};






struct NotableStringInfo : public StringInfo
{
    static const size_t MAX_SAVED_CHARS = 1024;

    NotableStringInfo();
    NotableStringInfo(JSString* str, const StringInfo& info);
    NotableStringInfo(NotableStringInfo&& info);
    NotableStringInfo& operator=(NotableStringInfo&& info);

    ~NotableStringInfo() {
        js_free(buffer);
    }

    char* buffer;
    size_t length;

  private:
    NotableStringInfo(const NotableStringInfo& info) = delete;
};



struct ScriptSourceInfo
{
#define FOR_EACH_SIZE(macro) \
    macro(_, MallocHeap, compressed) \
    macro(_, MallocHeap, uncompressed) \
    macro(_, MallocHeap, misc)

    ScriptSourceInfo()
      : FOR_EACH_SIZE(ZERO_SIZE)
        numScripts(0)
    {}

    void add(const ScriptSourceInfo& other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
        numScripts++;
    }

    void subtract(const ScriptSourceInfo& other) {
        FOR_EACH_SIZE(SUB_OTHER_SIZE)
        numScripts--;
    }

    void addToServoSizes(ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
    }

    bool isNotable() const {
        static const size_t NotabilityThreshold = 16 * 1024;
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N)
        return n >= NotabilityThreshold;
    }

    FOR_EACH_SIZE(DECL_SIZE)
    uint32_t numScripts;    
                            
                            
#undef FOR_EACH_SIZE
};







struct NotableScriptSourceInfo : public ScriptSourceInfo
{
    NotableScriptSourceInfo();
    NotableScriptSourceInfo(const char* filename, const ScriptSourceInfo& info);
    NotableScriptSourceInfo(NotableScriptSourceInfo&& info);
    NotableScriptSourceInfo& operator=(NotableScriptSourceInfo&& info);

    ~NotableScriptSourceInfo() {
        js_free(filename_);
    }

    char* filename_;

  private:
    NotableScriptSourceInfo(const NotableScriptSourceInfo& info) = delete;
};



struct RuntimeSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(_, MallocHeap, object) \
    macro(_, MallocHeap, atomsTable) \
    macro(_, MallocHeap, contexts) \
    macro(_, MallocHeap, dtoa) \
    macro(_, MallocHeap, temporary) \
    macro(_, MallocHeap, interpreterStack) \
    macro(_, MallocHeap, mathCache) \
    macro(_, MallocHeap, uncompressedSourceCache) \
    macro(_, MallocHeap, compressedSourceSet) \
    macro(_, MallocHeap, scriptData)

    RuntimeSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        scriptSourceInfo(),
        code(),
        gc(),
        notableScriptSources()
    {
        allScriptSources = js_new<ScriptSourcesHashMap>();
        if (!allScriptSources || !allScriptSources->init())
            MOZ_CRASH("oom");
    }

    ~RuntimeSizes() {
        
        
        
        js_delete(allScriptSources);
    }

    void addToServoSizes(ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
        scriptSourceInfo.addToServoSizes(sizes);
        code.addToServoSizes(sizes);
        gc.addToServoSizes(sizes);
    }

    
    
    
    
    FOR_EACH_SIZE(DECL_SIZE)
    ScriptSourceInfo scriptSourceInfo;
    CodeSizes code;
    GCSizes gc;

    typedef js::HashMap<const char*, ScriptSourceInfo,
                        js::CStringHashPolicy,
                        js::SystemAllocPolicy> ScriptSourcesHashMap;

    
    
    
    
    ScriptSourcesHashMap* allScriptSources;
    js::Vector<NotableScriptSourceInfo, 0, js::SystemAllocPolicy> notableScriptSources;

#undef FOR_EACH_SIZE
};

struct UnusedGCThingSizes
{
#define FOR_EACH_SIZE(macro) \
    macro(Other, GCHeapUnused, object) \
    macro(Other, GCHeapUnused, script) \
    macro(Other, GCHeapUnused, lazyScript) \
    macro(Other, GCHeapUnused, shape) \
    macro(Other, GCHeapUnused, baseShape) \
    macro(Other, GCHeapUnused, objectGroup) \
    macro(Other, GCHeapUnused, string) \
    macro(Other, GCHeapUnused, symbol) \
    macro(Other, GCHeapUnused, jitcode) \

    UnusedGCThingSizes()
      : FOR_EACH_SIZE(ZERO_SIZE)
        dummy()
    {}

    UnusedGCThingSizes(UnusedGCThingSizes&& other)
      : FOR_EACH_SIZE(COPY_OTHER_SIZE)
        dummy()
    {}

    void addToKind(JS::TraceKind kind, intptr_t n) {
        switch (kind) {
          case JS::TraceKind::Object:       object += n;      break;
          case JS::TraceKind::String:       string += n;      break;
          case JS::TraceKind::Symbol:       symbol += n;      break;
          case JS::TraceKind::Script:       script += n;      break;
          case JS::TraceKind::Shape:        shape += n;       break;
          case JS::TraceKind::BaseShape:    baseShape += n;   break;
          case JS::TraceKind::JitCode:      jitcode += n;     break;
          case JS::TraceKind::LazyScript:   lazyScript += n;  break;
          case JS::TraceKind::ObjectGroup:  objectGroup += n; break;
          default:
            MOZ_CRASH("Bad trace kind for UnusedGCThingSizes");
        }
    }

    void addSizes(const UnusedGCThingSizes& other) {
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
    }

    size_t totalSize() const {
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N)
        return n;
    }

    void addToTabSizes(JS::TabSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
    }

    void addToServoSizes(JS::ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
    }

    FOR_EACH_SIZE(DECL_SIZE)
    int dummy;  

#undef FOR_EACH_SIZE
};

struct ZoneStats
{
#define FOR_EACH_SIZE(macro) \
    macro(Other,   GCHeapUsed,  symbolsGCHeap) \
    macro(Other,   GCHeapAdmin, gcHeapArenaAdmin) \
    macro(Other,   GCHeapUsed,  lazyScriptsGCHeap) \
    macro(Other,   MallocHeap,  lazyScriptsMallocHeap) \
    macro(Other,   GCHeapUsed,  jitCodesGCHeap) \
    macro(Other,   GCHeapUsed,  objectGroupsGCHeap) \
    macro(Other,   MallocHeap,  objectGroupsMallocHeap) \
    macro(Other,   MallocHeap,  typePool) \
    macro(Other,   MallocHeap,  baselineStubsOptimized)

    ZoneStats()
      : FOR_EACH_SIZE(ZERO_SIZE)
        unusedGCThings(),
        stringInfo(),
        extra(),
        allStrings(nullptr),
        notableStrings(),
        isTotals(true)
    {}

    ZoneStats(ZoneStats&& other)
      : FOR_EACH_SIZE(COPY_OTHER_SIZE)
        unusedGCThings(mozilla::Move(other.unusedGCThings)),
        stringInfo(mozilla::Move(other.stringInfo)),
        extra(other.extra),
        allStrings(other.allStrings),
        notableStrings(mozilla::Move(other.notableStrings)),
        isTotals(other.isTotals)
    {
        other.allStrings = nullptr;
        MOZ_ASSERT(!other.isTotals);
    }

    ~ZoneStats() {
        
        
        
        js_delete(allStrings);
    }

    bool initStrings(JSRuntime* rt);

    void addSizes(const ZoneStats& other) {
        MOZ_ASSERT(isTotals);
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
        unusedGCThings.addSizes(other.unusedGCThings);
        stringInfo.add(other.stringInfo);
    }

    size_t sizeOfLiveGCThings() const {
        MOZ_ASSERT(isTotals);
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        n += stringInfo.sizeOfLiveGCThings();
        return n;
    }

    void addToTabSizes(JS::TabSizes* sizes) const {
        MOZ_ASSERT(isTotals);
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
        unusedGCThings.addToTabSizes(sizes);
        stringInfo.addToTabSizes(sizes);
    }

    void addToServoSizes(JS::ServoSizes *sizes) const {
        MOZ_ASSERT(isTotals);
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
        unusedGCThings.addToServoSizes(sizes);
        stringInfo.addToServoSizes(sizes);
    }

    
    
    
    
    FOR_EACH_SIZE(DECL_SIZE)
    UnusedGCThingSizes unusedGCThings;
    StringInfo stringInfo;
    void* extra;    

    typedef js::HashMap<JSString*, StringInfo,
                        js::InefficientNonFlatteningStringHashPolicy,
                        js::SystemAllocPolicy> StringsHashMap;

    
    
    
    
    StringsHashMap* allStrings;
    js::Vector<NotableStringInfo, 0, js::SystemAllocPolicy> notableStrings;
    bool isTotals;

#undef FOR_EACH_SIZE
};

struct CompartmentStats
{
    
    
    
    
#define FOR_EACH_SIZE(macro) \
    macro(Private, MallocHeap, objectsPrivate) \
    macro(Other,   GCHeapUsed, scriptsGCHeap) \
    macro(Other,   MallocHeap, scriptsMallocHeapData) \
    macro(Other,   MallocHeap, baselineData) \
    macro(Other,   MallocHeap, baselineStubsFallback) \
    macro(Other,   MallocHeap, ionData) \
    macro(Other,   MallocHeap, typeInferenceTypeScripts) \
    macro(Other,   MallocHeap, typeInferenceAllocationSiteTables) \
    macro(Other,   MallocHeap, typeInferenceArrayTypeTables) \
    macro(Other,   MallocHeap, typeInferenceObjectTypeTables) \
    macro(Other,   MallocHeap, compartmentObject) \
    macro(Other,   MallocHeap, compartmentTables) \
    macro(Other,   MallocHeap, innerViewsTable) \
    macro(Other,   MallocHeap, lazyArrayBuffersTable) \
    macro(Other,   MallocHeap, objectMetadataTable) \
    macro(Other,   MallocHeap, crossCompartmentWrappersTable) \
    macro(Other,   MallocHeap, regexpCompartment) \
    macro(Other,   MallocHeap, savedStacksSet)

    CompartmentStats()
      : FOR_EACH_SIZE(ZERO_SIZE)
        classInfo(),
        extra(),
        allClasses(nullptr),
        notableClasses(),
        isTotals(true)
    {}

    CompartmentStats(CompartmentStats&& other)
      : FOR_EACH_SIZE(COPY_OTHER_SIZE)
        classInfo(mozilla::Move(other.classInfo)),
        extra(other.extra),
        allClasses(other.allClasses),
        notableClasses(mozilla::Move(other.notableClasses)),
        isTotals(other.isTotals)
    {
        other.allClasses = nullptr;
        MOZ_ASSERT(!other.isTotals);
    }

    ~CompartmentStats() {
        
        
        
        js_delete(allClasses);
    }

    bool initClasses(JSRuntime* rt);

    void addSizes(const CompartmentStats& other) {
        MOZ_ASSERT(isTotals);
        FOR_EACH_SIZE(ADD_OTHER_SIZE)
        classInfo.add(other.classInfo);
    }

    size_t sizeOfLiveGCThings() const {
        MOZ_ASSERT(isTotals);
        size_t n = 0;
        FOR_EACH_SIZE(ADD_SIZE_TO_N_IF_LIVE_GC_THING)
        n += classInfo.sizeOfLiveGCThings();
        return n;
    }

    void addToTabSizes(TabSizes* sizes) const {
        MOZ_ASSERT(isTotals);
        FOR_EACH_SIZE(ADD_TO_TAB_SIZES);
        classInfo.addToTabSizes(sizes);
    }

    void addToServoSizes(ServoSizes *sizes) const {
        MOZ_ASSERT(isTotals);
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES);
        classInfo.addToServoSizes(sizes);
    }

    
    
    
    FOR_EACH_SIZE(DECL_SIZE)
    ClassInfo classInfo;
    void* extra;            

    typedef js::HashMap<const char*, ClassInfo,
                        js::CStringHashPolicy,
                        js::SystemAllocPolicy> ClassesHashMap;

    
    ClassesHashMap* allClasses;
    js::Vector<NotableClassInfo, 0, js::SystemAllocPolicy> notableClasses;
    bool isTotals;

#undef FOR_EACH_SIZE
};

typedef js::Vector<CompartmentStats, 0, js::SystemAllocPolicy> CompartmentStatsVector;
typedef js::Vector<ZoneStats, 0, js::SystemAllocPolicy> ZoneStatsVector;

struct RuntimeStats
{
    
    
    
    
    
#define FOR_EACH_SIZE(macro) \
    macro(_, Ignore,            gcHeapChunkTotal) \
    macro(_, GCHeapDecommitted, gcHeapDecommittedArenas) \
    macro(_, GCHeapUnused,      gcHeapUnusedChunks) \
    macro(_, GCHeapUnused,      gcHeapUnusedArenas) \
    macro(_, GCHeapAdmin,       gcHeapChunkAdmin) \
    macro(_, Ignore,            gcHeapGCThings)

    explicit RuntimeStats(mozilla::MallocSizeOf mallocSizeOf)
      : FOR_EACH_SIZE(ZERO_SIZE)
        runtime(),
        cTotals(),
        zTotals(),
        compartmentStatsVector(),
        zoneStatsVector(),
        currZoneStats(nullptr),
        mallocSizeOf_(mallocSizeOf)
    {}

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    void addToServoSizes(ServoSizes *sizes) const {
        FOR_EACH_SIZE(ADD_TO_SERVO_SIZES)
        runtime.addToServoSizes(sizes);
    }

    FOR_EACH_SIZE(DECL_SIZE)

    RuntimeSizes runtime;

    CompartmentStats cTotals;   
    ZoneStats zTotals;          

    CompartmentStatsVector compartmentStatsVector;
    ZoneStatsVector zoneStatsVector;

    ZoneStats* currZoneStats;

    mozilla::MallocSizeOf mallocSizeOf_;

    virtual void initExtraCompartmentStats(JSCompartment* c, CompartmentStats* cstats) = 0;
    virtual void initExtraZoneStats(JS::Zone* zone, ZoneStats* zstats) = 0;

#undef FOR_EACH_SIZE
};

class ObjectPrivateVisitor
{
  public:
    
    
    virtual size_t sizeOfIncludingThis(nsISupports* aSupports) = 0;

    
    
    typedef bool(*GetISupportsFun)(JSObject* obj, nsISupports** iface);
    GetISupportsFun getISupports_;

    explicit ObjectPrivateVisitor(GetISupportsFun getISupports)
      : getISupports_(getISupports)
    {}
};

extern JS_PUBLIC_API(bool)
CollectRuntimeStats(JSRuntime* rt, RuntimeStats* rtStats, ObjectPrivateVisitor* opv, bool anonymize);

extern JS_PUBLIC_API(size_t)
SystemCompartmentCount(JSRuntime* rt);

extern JS_PUBLIC_API(size_t)
UserCompartmentCount(JSRuntime* rt);

extern JS_PUBLIC_API(size_t)
PeakSizeOfTemporary(const JSRuntime* rt);

extern JS_PUBLIC_API(bool)
AddSizeOfTab(JSRuntime* rt, JS::HandleObject obj, mozilla::MallocSizeOf mallocSizeOf,
             ObjectPrivateVisitor* opv, TabSizes* sizes);

extern JS_PUBLIC_API(bool)
AddServoSizeOf(JSRuntime *rt, mozilla::MallocSizeOf mallocSizeOf,
               ObjectPrivateVisitor *opv, ServoSizes *sizes);

} 

#undef DECL_SIZE
#undef ZERO_SIZE
#undef COPY_OTHER_SIZE
#undef ADD_OTHER_SIZE
#undef SUB_OTHER_SIZE
#undef ADD_SIZE_TO_N
#undef ADD_SIZE_TO_N_IF_LIVE_GC_THING
#undef ADD_TO_TAB_SIZES

#endif 
