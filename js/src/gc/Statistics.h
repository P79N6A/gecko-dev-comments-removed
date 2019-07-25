






































#ifndef jsgc_statistics_h___
#define jsgc_statistics_h___

#include <string.h>

#include "jspubtd.h"
#include "jsutil.h"

struct JSCompartment;

namespace js {
namespace gcstats {

enum Reason {
    PUBLIC_API,
    MAYBEGC,
    LASTCONTEXT,
    DESTROYCONTEXT,
    LASTDITCH,
    TOOMUCHMALLOC,
    ALLOCTRIGGER,
    CHUNK,
    SHAPE,
    REFILL
};
static const int NUM_REASONS = REFILL + 1;

static inline const char *
ExplainReason(Reason r)
{
    static const char *strs[] = {"  API", "Maybe", "LastC", "DestC", "LastD",
                                 "Mallc", "Alloc", "Chunk", "Shape", "Refil"};

    JS_ASSERT(strcmp(strs[SHAPE], "Shape") == 0 &&
              sizeof(strs) / sizeof(strs[0]) == NUM_REASONS);

    return strs[r];
}

enum Phase {
    PHASE_GC,
    PHASE_MARK,
    PHASE_SWEEP,
    PHASE_SWEEP_OBJECT,
    PHASE_SWEEP_STRING,
    PHASE_SWEEP_SCRIPT,
    PHASE_SWEEP_SHAPE,
    PHASE_DISCARD_CODE,
    PHASE_DISCARD_ANALYSIS,
    PHASE_XPCONNECT,
    PHASE_DESTROY,

    PHASE_LIMIT
};

enum Stat {
    STAT_NEW_CHUNK,
    STAT_DESTROY_CHUNK,

    STAT_LIMIT
};

struct Statistics {
    Statistics(JSRuntime *rt);
    ~Statistics();

    void beginGC(JSCompartment *comp, Reason reason);
    void endGC();

    void beginPhase(Phase phase);
    void endPhase(Phase phase);

    void count(Stat s) {
        JS_ASSERT(s < STAT_LIMIT);
        counts[s]++;
    }

  private:
    JSRuntime *runtime;

    uint64 startupTime;

    FILE *fp;
    bool fullFormat;

    Reason triggerReason;
    JSCompartment *compartment;

    uint64 phaseStarts[PHASE_LIMIT];
    uint64 phaseEnds[PHASE_LIMIT];
    uint64 phaseTimes[PHASE_LIMIT];
    uint64 totals[PHASE_LIMIT];
    unsigned int counts[STAT_LIMIT];

    double t(Phase phase);
    double total(Phase phase);
    double beginDelay(Phase phase1, Phase phase2);
    double endDelay(Phase phase1, Phase phase2);
    void printStats();
    void statsToString(char *buffer, size_t size);

    struct ColumnInfo {
        const char *title;
        char str[12];
        char totalStr[12];
        int width;

        ColumnInfo() {}
        ColumnInfo(const char *title, double t, double total);
        ColumnInfo(const char *title, double t);
        ColumnInfo(const char *title, unsigned int data);
        ColumnInfo(const char *title, const char *data);
    };

    void makeTable(ColumnInfo *cols);
};

struct AutoGC {
    AutoGC(Statistics &stats, JSCompartment *comp, Reason reason JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : stats(stats) { JS_GUARD_OBJECT_NOTIFIER_INIT; stats.beginGC(comp, reason); }
    ~AutoGC() { stats.endGC(); }

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
