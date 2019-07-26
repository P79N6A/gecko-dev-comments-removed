






































#ifndef jsgc_statistics_h___
#define jsgc_statistics_h___

#include <string.h>

#include "jsfriendapi.h"
#include "jspubtd.h"
#include "jsutil.h"

struct JSCompartment;

namespace js {
namespace gcstats {

enum Phase {
    PHASE_GC_BEGIN,
    PHASE_WAIT_BACKGROUND_THREAD,
    PHASE_PURGE,
    PHASE_MARK,
    PHASE_MARK_ROOTS,
    PHASE_MARK_DELAYED,
    PHASE_MARK_OTHER,
    PHASE_FINALIZE_START,
    PHASE_SWEEP,
    PHASE_SWEEP_COMPARTMENTS,
    PHASE_SWEEP_OBJECT,
    PHASE_SWEEP_STRING,
    PHASE_SWEEP_SCRIPT,
    PHASE_SWEEP_SHAPE,
    PHASE_SWEEP_IONCODE,
    PHASE_DISCARD_CODE,
    PHASE_DISCARD_ANALYSIS,
    PHASE_DISCARD_TI,
    PHASE_SWEEP_TYPES,
    PHASE_CLEAR_SCRIPT_ANALYSIS,
    PHASE_FINALIZE_END,
    PHASE_DESTROY,
    PHASE_GC_END,

    PHASE_LIMIT
};

enum Stat {
    STAT_NEW_CHUNK,
    STAT_DESTROY_CHUNK,

    STAT_LIMIT
};

class StatisticsSerializer;

struct Statistics {
    Statistics(JSRuntime *rt);
    ~Statistics();

    void beginPhase(Phase phase);
    void endPhase(Phase phase);

    void beginSlice(int collectedCount, int compartmentCount, gcreason::Reason reason);
    void endSlice();

    void reset(const char *reason) { slices.back().resetReason = reason; }
    void nonincremental(const char *reason) { nonincrementalReason = reason; }

    void count(Stat s) {
        JS_ASSERT(s < STAT_LIMIT);
        counts[s]++;
    }

    jschar *formatMessage();
    jschar *formatJSON(uint64_t timestamp);

  private:
    JSRuntime *runtime;

    int64_t startupTime;

    FILE *fp;
    bool fullFormat;

    



    int gcDepth;

    int collectedCount;
    int compartmentCount;
    const char *nonincrementalReason;

    struct SliceData {
        SliceData(gcreason::Reason reason, int64_t start)
          : reason(reason), resetReason(NULL), start(start)
        {
            PodArrayZero(phaseTimes);
        }

        gcreason::Reason reason;
        const char *resetReason;
        int64_t start, end;
        int64_t phaseTimes[PHASE_LIMIT];

        int64_t duration() const { return end - start; }
    };

    Vector<SliceData, 8, SystemAllocPolicy> slices;

    
    int64_t phaseStarts[PHASE_LIMIT];

    
    int64_t phaseTimes[PHASE_LIMIT];

    
    int64_t phaseTotals[PHASE_LIMIT];

    
    unsigned int counts[STAT_LIMIT];

    
    size_t preBytes;

    void beginGC();
    void endGC();

    int64_t gcDuration();
    void printStats();
    bool formatData(StatisticsSerializer &ss, uint64_t timestamp);

    double computeMMU(int64_t resolution);
};

struct AutoGCSlice {
    AutoGCSlice(Statistics &stats, int collectedCount, int compartmentCount, gcreason::Reason reason
                JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        stats.beginSlice(collectedCount, compartmentCount, reason);
    }
    ~AutoGCSlice() { stats.endSlice(); }

    Statistics &stats;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

struct AutoPhase {
    AutoPhase(Statistics &stats, Phase phase JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats), phase(phase) { JS_GUARD_OBJECT_NOTIFIER_INIT; stats.beginPhase(phase); }
    ~AutoPhase() { stats.endPhase(phase); }

    Statistics &stats;
    Phase phase;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 
} 

#endif 
