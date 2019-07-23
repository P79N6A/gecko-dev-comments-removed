










































#include <stdio.h>
#include <stdlib.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"

#ifdef WIN32
#    include <windows.h>
#endif





JS_STATIC_ASSERT(sizeof(void *) == sizeof(void (*)()));

JS_PUBLIC_API(void) JS_Assert(const char *s, const char *file, JSIntn ln)
{
    fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
#if defined(WIN32)
    DebugBreak();
    exit(3);
#elif defined(XP_OS2) || (defined(__GNUC__) && (defined(__i386) || defined(__x86_64__)))
    asm("int $3");
#endif
    abort();
}

#ifdef JS_BASIC_STATS

#include <math.h>
#include <string.h>
#include "jscompat.h"
#include "jsbit.h"










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
          ? (uintN) JS_CeilingLog2(val)
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

                memset(newhist, 0, sizeof newhist);
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
    uint32 cnt, max, prev, val, i;
    double sum, mean;

    for (bin = 0, max = 0, sum = 0; bin <= 10; bin++) {
        cnt = bs->hist[bin];
        if (max < cnt)
            max = cnt;
        sum += cnt;
    }
    mean = sum / cnt;
    for (bin = 0, prev = 0; bin <= 10; bin++, prev = val) {
        val = BinToVal(bs->logscale, bin);
        cnt = bs->hist[bin];
        if (prev + 1 >= val)
            fprintf(fp, "        [%6u]", val);
        else
            fprintf(fp, "[%6u, %6u]", prev + 1, val);
        fprintf(fp, "%s %8u ", (bin == 10) ? "+" : ":", cnt);
        if (cnt != 0) {
            if (max > 1e6 && mean > 1e3)
                cnt = (uint32) ceil(log10((double) cnt));
            else if (max > 16 && mean > 8)
                cnt = JS_CeilingLog2(cnt);
            for (i = 0; i < cnt; i++)
                putc('*', fp);
        }
        putc('\n', fp);
    }
}

#endif 

#if defined(DEBUG_notme) && defined(XP_UNIX)

#define __USE_GNU 1
#include <dlfcn.h>
#include <string.h>
#include "jshash.h"
#include "jsprf.h"

JSCallsite js_calltree_root = {0, NULL, NULL, 0, NULL, NULL, NULL, NULL};

static JSCallsite *
CallTree(void **bp)
{
    void **bpup, **bpdown, *pc;
    JSCallsite *parent, *site, **csp;
    Dl_info info;
    int ok, offset;
    const char *symbol;
    char *method;

    
    bpup = NULL;
    for (;;) {
        bpdown = (void**) bp[0];
        bp[0] = (void*) bpup;
        if ((void**) bpdown[0] < bpdown)
            break;
        bpup = bp;
        bp = bpdown;
    }

    
    parent = &js_calltree_root;
    do {
        bpup = (void**) bp[0];
        bp[0] = (void*) bpdown;
        pc = bp[1];

        csp = &parent->kids;
        while ((site = *csp) != NULL) {
            if (site->pc == (uint32)pc) {
                
                *csp = site->siblings;
                site->siblings = parent->kids;
                parent->kids = site;

                
                goto upward;
            }
            csp = &site->siblings;
        }

        
        for (site = parent; site; site = site->parent) {
            if (site->pc == (uint32)pc)
                goto upward;
        }

        



        info.dli_fname = info.dli_sname = NULL;
        ok = dladdr(pc, &info);
        if (ok < 0) {
            fprintf(stderr, "dladdr failed!\n");
            return NULL;
        }


        symbol = info.dli_sname;
        offset = (char*)pc - (char*)info.dli_fbase;
        method = symbol
                 ? strdup(symbol)
                 : JS_smprintf("%s+%X",
                               info.dli_fname ? info.dli_fname : "main",
                               offset);
        if (!method)
            return NULL;

        
        site = (JSCallsite *) js_malloc(sizeof(JSCallsite));
        if (!site)
            return NULL;

        
        site->pc = (uint32)pc;
        site->name = method;
        site->library = info.dli_fname;
        site->offset = offset;
        site->parent = parent;
        site->siblings = parent->kids;
        parent->kids = site;
        site->kids = NULL;

      upward:
        parent = site;
        bpdown = bp;
        bp = bpup;
    } while (bp);

    return site;
}

JS_FRIEND_API(JSCallsite *)
JS_Backtrace(int skip)
{
    void **bp, **bpdown;

    
#if defined(__i386)
    __asm__( "movl %%ebp, %0" : "=g"(bp));
#elif defined(__x86_64__)
    __asm__( "movq %%rbp, %0" : "=g"(bp));
#else
    




    bp = (void**) __builtin_frame_address(0);
#endif
    while (--skip >= 0) {
        bpdown = (void**) *bp++;
        if (bpdown < bp)
            break;
        bp = bpdown;
    }

    return CallTree(bp);
}

JS_FRIEND_API(void)
JS_DumpBacktrace(JSCallsite *trace)
{
    while (trace) {
        fprintf(stdout, "%s [%s +0x%X]\n", trace->name, trace->library,
                trace->offset);
        trace = trace->parent;
    }
}

#endif 
