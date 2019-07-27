







#include "jsutil.h"

#include "mozilla/Assertions.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/PodOperations.h"

#include <stdio.h>

#include "jstypes.h"

#ifdef WIN32
#    include "jswin.h"
#endif

#include "js/Utility.h"

using namespace js;

using mozilla::CeilingLog2Size;
using mozilla::PodArrayZero;

#if defined(DEBUG) || defined(JS_OOM_BREAKPOINT)

JS_PUBLIC_DATA(uint32_t) OOM_maxAllocations = UINT32_MAX;
JS_PUBLIC_DATA(uint32_t) OOM_counter = 0;
#endif

JS_PUBLIC_API(void)
JS_Assert(const char *s, const char *file, int ln)
{
    MOZ_ReportAssertionFailure(s, file, ln);
    MOZ_CRASH();
}

#ifdef __linux__

#include <malloc.h>
#include <stdlib.h>

namespace js {




extern void
AllTheNonBasicVanillaNewAllocations()
{
    
    
    
    

    intptr_t p =
        intptr_t(malloc(16)) +
        intptr_t(calloc(1, 16)) +
        intptr_t(realloc(nullptr, 16)) +
        intptr_t(new char) +
        intptr_t(new char) +
        intptr_t(new char) +
        intptr_t(new char[16]) +
        intptr_t(memalign(16, 16)) +
        
        
        intptr_t(valloc(4096)) +
        intptr_t(strdup("dummy"));

    printf("%u\n", uint32_t(p));  

    free((int*)p);      

    MOZ_CRASH();
}

} 

#endif 

#ifdef JS_BASIC_STATS

#include <math.h>










static uint32_t
BinToVal(unsigned logscale, unsigned bin)
{
    MOZ_ASSERT(bin <= 10);
    if (bin <= 1 || logscale == 0)
        return bin;
    --bin;
    if (logscale == 2)
        return JS_BIT(bin);
    MOZ_ASSERT(logscale == 10);
    return uint32_t(pow(10.0, (double) bin));
}

static unsigned
ValToBin(unsigned logscale, uint32_t val)
{
    unsigned bin;

    if (val <= 1)
        return val;
    bin = (logscale == 10)
        ? (unsigned) ceil(log10((double) val))
        : (logscale == 2)
        ? (unsigned) CeilingLog2Size(val)
        : val;
    return Min(bin, 10U);
}

void
JS_BasicStatsAccum(JSBasicStats *bs, uint32_t val)
{
    unsigned oldscale, newscale, bin;
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
                uint32_t newhist[11], newbin;

                PodArrayZero(newhist);
                for (bin = 0; bin <= 10; bin++) {
                    newbin = ValToBin(newscale, BinToVal(oldscale, bin));
                    newhist[newbin] += bs->hist[bin];
                }
                js_memcpy(bs->hist, newhist, sizeof bs->hist);
                bs->logscale = newscale;
            }
        }
    }

    bin = ValToBin(bs->logscale, val);
    ++bs->hist[bin];
}

double
JS_MeanAndStdDev(uint32_t num, double sum, double sqsum, double *sigma)
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
    unsigned bin;
    uint32_t cnt, max;
    double sum, mean;

    for (bin = 0, max = 0, sum = 0; bin <= 10; bin++) {
        cnt = bs->hist[bin];
        if (max < cnt)
            max = cnt;
        sum += cnt;
    }
    mean = sum / cnt;
    for (bin = 0; bin <= 10; bin++) {
        unsigned val = BinToVal(bs->logscale, bin);
        unsigned end = (bin == 10) ? 0 : BinToVal(bs->logscale, bin + 1);
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
                cnt = uint32_t(ceil(log10((double) cnt)));
            else if (max > 16 && mean > 8)
                cnt = CeilingLog2Size(cnt);
            for (unsigned i = 0; i < cnt; i++)
                putc('*', fp);
        }
        putc('\n', fp);
    }
}

#endif 
