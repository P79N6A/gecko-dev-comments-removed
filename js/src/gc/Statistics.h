





#ifndef gc_Statistics_h
#define gc_Statistics_h

#include "mozilla/DebugOnly.h"
#include "mozilla/PodOperations.h"
#include "mozilla/UniquePtr.h"

#include "jsalloc.h"
#include "jsgc.h"
#include "jspubtd.h"

#include "js/GCAPI.h"
#include "js/Vector.h"

struct JSCompartment;

namespace js {

class GCParallelTask;

namespace gcstats {

enum Phase {
    PHASE_GC_BEGIN,
    PHASE_WAIT_BACKGROUND_THREAD,
    PHASE_MARK_DISCARD_CODE,
    PHASE_PURGE,
    PHASE_MARK,
    PHASE_MARK_ROOTS,
    PHASE_MARK_DELAYED,
    PHASE_SWEEP,
    PHASE_SWEEP_MARK,
    PHASE_SWEEP_MARK_TYPES,
    PHASE_SWEEP_MARK_INCOMING_BLACK,
    PHASE_SWEEP_MARK_WEAK,
    PHASE_SWEEP_MARK_INCOMING_GRAY,
    PHASE_SWEEP_MARK_GRAY,
    PHASE_SWEEP_MARK_GRAY_WEAK,
    PHASE_FINALIZE_START,
    PHASE_SWEEP_ATOMS,
    PHASE_SWEEP_SYMBOL_REGISTRY,
    PHASE_SWEEP_COMPARTMENTS,
    PHASE_SWEEP_DISCARD_CODE,
    PHASE_SWEEP_INNER_VIEWS,
    PHASE_SWEEP_CC_WRAPPER,
    PHASE_SWEEP_BASE_SHAPE,
    PHASE_SWEEP_INITIAL_SHAPE,
    PHASE_SWEEP_TYPE_OBJECT,
    PHASE_SWEEP_BREAKPOINT,
    PHASE_SWEEP_REGEXP,
    PHASE_SWEEP_MISC,
    PHASE_SWEEP_TYPES,
    PHASE_SWEEP_TYPES_BEGIN,
    PHASE_SWEEP_TYPES_END,
    PHASE_SWEEP_OBJECT,
    PHASE_SWEEP_STRING,
    PHASE_SWEEP_SCRIPT,
    PHASE_SWEEP_SHAPE,
    PHASE_SWEEP_JITCODE,
    PHASE_FINALIZE_END,
    PHASE_DESTROY,
    PHASE_COMPACT,
    PHASE_COMPACT_MOVE,
    PHASE_COMPACT_UPDATE,
    PHASE_COMPACT_UPDATE_GRAY,
    PHASE_GC_END,

    PHASE_LIMIT
};

enum Stat {
    STAT_NEW_CHUNK,
    STAT_DESTROY_CHUNK,
    STAT_MINOR_GC,

    STAT_LIMIT
};

class StatisticsSerializer;

struct ZoneGCStats
{
    
    int collectedZoneCount;

    
    int zoneCount;

    
    int collectedCompartmentCount;

    
    int compartmentCount;

    bool isCollectingAllZones() const { return collectedZoneCount == zoneCount; }

    ZoneGCStats()
      : collectedZoneCount(0), zoneCount(0), collectedCompartmentCount(0), compartmentCount(0)
    {}
};

struct Statistics
{
    explicit Statistics(JSRuntime *rt);
    ~Statistics();

    void beginPhase(Phase phase);
    void endPhase(Phase phase);
    void endParallelPhase(Phase phase, const GCParallelTask *task);

    void beginSlice(const ZoneGCStats &zoneStats, JSGCInvocationKind gckind,
                    JS::gcreason::Reason reason);
    void endSlice();

    void reset(const char *reason) { slices.back().resetReason = reason; }
    void nonincremental(const char *reason) { nonincrementalReason = reason; }

    void count(Stat s) {
        MOZ_ASSERT(s < STAT_LIMIT);
        counts[s]++;
    }

    int64_t beginSCC();
    void endSCC(unsigned scc, int64_t start);

    char16_t *formatMessage();
    char16_t *formatJSON(uint64_t timestamp);
    UniqueChars formatDetailedMessage();

    JS::GCSliceCallback setSliceCallback(JS::GCSliceCallback callback);

    int64_t clearMaxGCPauseAccumulator();
    int64_t getMaxGCPauseSinceClear();

  private:
    JSRuntime *runtime;

    int64_t startupTime;

    FILE *fp;
    bool fullFormat;

    



    int gcDepth;

    ZoneGCStats zoneStats;

    JSGCInvocationKind gckind;

    const char *nonincrementalReason;

    struct SliceData {
        SliceData(JS::gcreason::Reason reason, int64_t start, size_t startFaults)
          : reason(reason), resetReason(nullptr), start(start), startFaults(startFaults)
        {
            mozilla::PodArrayZero(phaseTimes);
        }

        JS::gcreason::Reason reason;
        const char *resetReason;
        int64_t start, end;
        size_t startFaults, endFaults;
        int64_t phaseTimes[PHASE_LIMIT];

        int64_t duration() const { return end - start; }
    };

    Vector<SliceData, 8, SystemAllocPolicy> slices;

    
    int64_t phaseStartTimes[PHASE_LIMIT];

    
    int64_t phaseTimes[PHASE_LIMIT];

    
    int64_t phaseTotals[PHASE_LIMIT];

    
    unsigned int counts[STAT_LIMIT];

    
    size_t preBytes;

    
    int64_t maxPauseInInterval;

#ifdef DEBUG
    
    static const size_t MAX_NESTING = 8;
    Phase phaseNesting[MAX_NESTING];
#endif
    mozilla::DebugOnly<size_t> phaseNestingDepth;

    
    Vector<int64_t, 0, SystemAllocPolicy> sccTimes;

    JS::GCSliceCallback sliceCallback;

    void beginGC(JSGCInvocationKind kind);
    void endGC();

    void gcDuration(int64_t *total, int64_t *maxPause);
    void sccDurations(int64_t *total, int64_t *maxPause);
    void printStats();
    bool formatData(StatisticsSerializer &ss, uint64_t timestamp);

    UniqueChars formatDescription();
    UniqueChars formatSliceDescription(unsigned i, const SliceData &slice);
    UniqueChars formatTotals();
    UniqueChars formatPhaseTimes(int64_t *phaseTimes);

    double computeMMU(int64_t resolution);
};

struct AutoGCSlice
{
    AutoGCSlice(Statistics &stats, const ZoneGCStats &zoneStats, JSGCInvocationKind gckind,
                JS::gcreason::Reason reason
                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        stats.beginSlice(zoneStats, gckind, reason);
    }
    ~AutoGCSlice() { stats.endSlice(); }

    Statistics &stats;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

struct AutoPhase
{
    AutoPhase(Statistics &stats, Phase phase
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats), task(nullptr), phase(phase), enabled(true)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        stats.beginPhase(phase);
    }

    AutoPhase(Statistics &stats, bool condition, Phase phase
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats), task(nullptr), phase(phase), enabled(condition)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (enabled)
            stats.beginPhase(phase);
    }

    AutoPhase(Statistics &stats, const GCParallelTask &task, Phase phase
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats), task(&task), phase(phase), enabled(true)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (enabled)
            stats.beginPhase(phase);
    }

    ~AutoPhase() {
        if (enabled) {
            if (task)
                stats.endParallelPhase(phase, task);
            else
                stats.endPhase(phase);
        }
    }

    Statistics &stats;
    const GCParallelTask *task;
    Phase phase;
    bool enabled;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

struct AutoSCC
{
    AutoSCC(Statistics &stats, unsigned scc
            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats), scc(scc)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        start = stats.beginSCC();
    }
    ~AutoSCC() {
        stats.endSCC(scc, start);
    }

    Statistics &stats;
    unsigned scc;
    int64_t start;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

const char *ExplainInvocationKind(JSGCInvocationKind gckind);
const char *ExplainReason(JS::gcreason::Reason reason);

} 
} 

#endif 
