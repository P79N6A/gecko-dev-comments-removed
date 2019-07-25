






































#include <stdio.h>

#include "jscntxt.h"
#include "jsprobes.h"
#include "jsutil.h"
#include "jscrashformat.h"
#include "jscrashreport.h"
#include "prmjtime.h"

#include "gc/Statistics.h"

namespace js {
namespace gcstats {

Statistics::Statistics(JSRuntime *rt)
  : runtime(rt)
{
    char *env = getenv("MOZ_GCTIMER");
    if (!env || strcmp(env, "none") == 0) {
        fp = NULL;
        return;
    }

    if (strcmp(env, "stdout") == 0) {
        fullFormat = false;
        fp = stdout;
    } else if (strcmp(env, "stderr") == 0) {
        fullFormat = false;
        fp = stderr;
    } else {
        fullFormat = true;

        fp = fopen(env, "a");
        JS_ASSERT(fp);

        fprintf(fp, "     AppTime,  Total,   Wait,   Mark,  Sweep, FinObj,"
                " FinStr, FinScr, FinShp, Destry,    End, +Chu, -Chu, T, Reason\n");
    }

    PodArrayZero(counts);
    PodArrayZero(totals);

    startupTime = PRMJ_Now();
}

Statistics::~Statistics()
{
    if (fp) {
        if (fullFormat)
            fprintf(fp,
                    "------>TOTAL  "
                    "%6.1f,         %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f\n",
                    total(PHASE_GC), total(PHASE_MARK), total(PHASE_SWEEP),
                    total(PHASE_SWEEP_OBJECT), total(PHASE_SWEEP_STRING),
                    total(PHASE_SWEEP_SCRIPT), total(PHASE_SWEEP_SHAPE),
                    total(PHASE_DESTROY));

        if (fp != stdout && fp != stderr)
            fclose(fp);
    }
}

struct GCCrashData
{
    int isRegen;
    int isCompartment;
};

void
Statistics::beginGC(JSCompartment *comp, Reason reason)
{
    compartment = comp;

    PodArrayZero(phaseStarts);
    PodArrayZero(phaseEnds);
    PodArrayZero(phaseTimes);

    triggerReason = reason;

    beginPhase(PHASE_GC);
    Probes::GCStart(compartment);

    GCCrashData crashData;
    crashData.isRegen = runtime->shapeGen & SHAPE_OVERFLOW_BIT;
    crashData.isCompartment = !!compartment;
    crash::SaveCrashData(crash::JS_CRASH_TAG_GC, &crashData, sizeof(crashData));
}

double
Statistics::t(Phase phase)
{
    return double(phaseTimes[phase]) / PRMJ_USEC_PER_MSEC;
}

double
Statistics::total(Phase phase)
{
    return double(totals[phase]) / PRMJ_USEC_PER_MSEC;
}

double
Statistics::beginDelay(Phase phase1, Phase phase2)
{
    return double(phaseStarts[phase1] - phaseStarts[phase2]) / PRMJ_USEC_PER_MSEC;
}

double
Statistics::endDelay(Phase phase1, Phase phase2)
{
    return double(phaseEnds[phase1] - phaseEnds[phase2]) / PRMJ_USEC_PER_MSEC;
}

void
Statistics::printStats()
{
    if (fullFormat) {
        
        fprintf(fp,
                "%12.0f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, ",
                double(phaseStarts[PHASE_GC] - startupTime) / PRMJ_USEC_PER_MSEC,
                t(PHASE_GC),
                beginDelay(PHASE_MARK, PHASE_GC),
                t(PHASE_MARK), t(PHASE_SWEEP),
                t(PHASE_SWEEP_OBJECT), t(PHASE_SWEEP_STRING),
                t(PHASE_SWEEP_SCRIPT), t(PHASE_SWEEP_SHAPE),
                t(PHASE_DESTROY),
                endDelay(PHASE_GC, PHASE_DESTROY));

        fprintf(fp, "%4d, %4d,", counts[STAT_NEW_CHUNK], counts[STAT_DESTROY_CHUNK]);
        fprintf(fp, " %s, %s\n", compartment ? "C" : "G", ExplainReason(triggerReason));
    } else {
        fprintf(fp, "%f %f %f\n",
                t(PHASE_GC), t(PHASE_MARK), t(PHASE_SWEEP));
    }
    fflush(fp);
}

void
Statistics::endGC()
{
    Probes::GCEnd(compartment);
    endPhase(PHASE_GC);
    crash::SnapshotGCStack();

    for (int i = 0; i < PHASE_LIMIT; i++)
        totals[i] += phaseTimes[i];

    if (JSAccumulateTelemetryDataCallback cb = runtime->telemetryCallback) {
        (*cb)(JS_TELEMETRY_GC_REASON, triggerReason);
        (*cb)(JS_TELEMETRY_GC_IS_COMPARTMENTAL, compartment ? 1 : 0);
        (*cb)(JS_TELEMETRY_GC_IS_SHAPE_REGEN,
              runtime->shapeGen & SHAPE_OVERFLOW_BIT ? 1 : 0);
        (*cb)(JS_TELEMETRY_GC_MS, t(PHASE_GC));
        (*cb)(JS_TELEMETRY_GC_MARK_MS, t(PHASE_MARK));
        (*cb)(JS_TELEMETRY_GC_SWEEP_MS, t(PHASE_SWEEP));
    }

    if (fp)
        printStats();

    PodArrayZero(counts);
}

void
Statistics::beginPhase(Phase phase)
{
    phaseStarts[phase] = PRMJ_Now();

    if (phase == gcstats::PHASE_SWEEP) {
        Probes::GCStartSweepPhase(NULL);
        if (!compartment) {
            for (JSCompartment **c = runtime->compartments.begin();
                 c != runtime->compartments.end(); ++c)
            {
                Probes::GCStartSweepPhase(*c);
            }
        }
    }
}

void
Statistics::endPhase(Phase phase)
{
    phaseEnds[phase] = PRMJ_Now();
    phaseTimes[phase] += phaseEnds[phase] - phaseStarts[phase];

    if (phase == gcstats::PHASE_SWEEP) {
        if (!compartment) {
            for (JSCompartment **c = runtime->compartments.begin();
                 c != runtime->compartments.end(); ++c)
            {
                Probes::GCEndSweepPhase(*c);
            }
        }
        Probes::GCEndSweepPhase(NULL);
    }
}

} 
} 
