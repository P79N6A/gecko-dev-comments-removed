










































#include <stdio.h>
#include <stdlib.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"

#ifdef WIN32
#    include "jswin.h"
#else
#    include <signal.h>
#endif

using namespace js;

#ifdef DEBUG

JS_PUBLIC_DATA(JSUint32) OOM_maxAllocations = (JSUint32)-1;
JS_PUBLIC_DATA(JSUint32) OOM_counter = 0;
#endif





JS_STATIC_ASSERT(sizeof(void *) == sizeof(void (*)()));

static JS_NEVER_INLINE void
CrashInJS()
{
    






#if defined(WIN32)
    



    *((volatile int *) NULL) = 123;
    exit(3);
#elif defined(__APPLE__)
    



    *((volatile int *) NULL) = 123;  
    raise(SIGABRT);  
#else
    raise(SIGABRT);  
#endif
}

JS_PUBLIC_API(void) JS_Assert(const char *s, const char *file, JSIntn ln)
{
    fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
    fflush(stderr);
    CrashInJS();
}

#ifdef JS_BASIC_STATS

#include <math.h>
#include <string.h>
#include "jscompat.h"










static uint32
BinToVal(uintN logscale, uintN bin)
{
    JS_ASSERT(bin <= 10);
    if (bin <= 1 || logscale == 0)
        return bin;
    --bin;
    if (logscale == 2)
        return JS_BIT(bin);
    JS_ASSERT(logscale == 10);
    return (uint32) pow(10.0, (double) bin);
}

static uintN
ValToBin(uintN logscale, uint32 val)
{
    uintN bin;

    if (val <= 1)
        return val;
    bin = (logscale == 10)
          ? (uintN) ceil(log10((double) val))
          : (logscale == 2)
          ? (uintN) JS_CEILING_LOG2W(val)
          : val;
    return JS_MIN(bin, 10);
}

void
JS_BasicStatsAccum(JSBasicStats *bs, uint32 val)
{
    uintN oldscale, newscale, bin;
    double mean;

    ++bs->num;
    if (bs->max < val)
        bs->max = val;
    bs->sum += val;
    bs->sqsum += (double)val * val;

    oldscale = bs->logscale;
    if (oldscale != 10) {
        mean = bs->sum / bs->num;
        if (bs->max > 16 && mean > 8) {
            newscale = (bs->max > 1e6 && mean > 1000) ? 10 : 2;
            if (newscale != oldscale) {
                uint32 newhist[11], newbin;

                PodArrayZero(newhist);
                for (bin = 0; bin <= 10; bin++) {
                    newbin = ValToBin(newscale, BinToVal(oldscale, bin));
                    newhist[newbin] += bs->hist[bin];
                }
                memcpy(bs->hist, newhist, sizeof bs->hist);
                bs->logscale = newscale;
            }
        }
    }

    bin = ValToBin(bs->logscale, val);
    ++bs->hist[bin];
}

double
JS_MeanAndStdDev(uint32 num, double sum, double sqsum, double *sigma)
{
    double var;

    if (num == 0 || sum == 0) {
        *sigma = 0;
        return 0;
    }

    var = num * sqsum - sum * sum;
    if (var < 0 || num == 1)
        var = 0;
    else
        var /= (double)num * (num - 1);

    
    *sigma = (var != 0) ? sqrt(var) : 0;
    return sum / num;
}

void
JS_DumpBasicStats(JSBasicStats *bs, const char *title, FILE *fp)
{
    double mean, sigma;

    mean = JS_MeanAndStdDevBS(bs, &sigma);
    fprintf(fp, "\nmean %s %g, std. deviation %g, max %lu\n",
            title, mean, sigma, (unsigned long) bs->max);
    JS_DumpHistogram(bs, fp);
}

void
JS_DumpHistogram(JSBasicStats *bs, FILE *fp)
{
    uintN bin;
    uint32 cnt, max;
    double sum, mean;

    for (bin = 0, max = 0, sum = 0; bin <= 10; bin++) {
        cnt = bs->hist[bin];
        if (max < cnt)
            max = cnt;
        sum += cnt;
    }
    mean = sum / cnt;
    for (bin = 0; bin <= 10; bin++) {
        uintN val = BinToVal(bs->logscale, bin);
        uintN end = (bin == 10) ? 0 : BinToVal(bs->logscale, bin + 1);
        cnt = bs->hist[bin];
        if (val + 1 == end)
            fprintf(fp, "        [%6u]", val);
        else if (end != 0)
            fprintf(fp, "[%6u, %6u]", val, end - 1);
        else
            fprintf(fp, "[%6u,   +inf]", val);
        fprintf(fp, ": %8u ", cnt);
        if (cnt != 0) {
            if (max > 1e6 && mean > 1e3)
                cnt = (uint32) ceil(log10((double) cnt));
            else if (max > 16 && mean > 8)
                cnt = JS_CEILING_LOG2W(cnt);
            for (uintN i = 0; i < cnt; i++)
                putc('*', fp);
        }
        putc('\n', fp);
    }
}

#endif 
