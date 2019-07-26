







#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/PodOperations.h"

#include <stdio.h>
#include <stdlib.h>
#include "jstypes.h"
#include "jsutil.h"

#ifdef WIN32
#    include "jswin.h"
#else
#    include <signal.h>
#endif

#include "js/TemplateLib.h"
#include "js/Utility.h"

#if USE_ZLIB
#include "zlib.h"
#endif

using namespace js;

using mozilla::PodArrayZero;

#if USE_ZLIB
static void *
zlib_alloc(void *cx, uInt items, uInt size)
{
    return js_malloc(items * size);
}

static void
zlib_free(void *cx, void *addr)
{
    js_free(addr);
}

Compressor::Compressor(const unsigned char *inp, size_t inplen)
    : inp(inp),
      inplen(inplen),
      outbytes(0)
{
    JS_ASSERT(inplen > 0);
    zs.opaque = NULL;
    zs.next_in = (Bytef *)inp;
    zs.avail_in = 0;
    zs.next_out = NULL;
    zs.avail_out = 0;
    zs.zalloc = zlib_alloc;
    zs.zfree = zlib_free;
}


Compressor::~Compressor()
{
    int ret = deflateEnd(&zs);
    if (ret != Z_OK) {
        
        JS_ASSERT(ret == Z_DATA_ERROR);
        JS_ASSERT(uInt(zs.next_in - inp) < inplen || !zs.avail_out);
    }
}

bool
Compressor::init()
{
    if (inplen >= UINT32_MAX)
        return false;
    
    
    
    int ret = deflateInit(&zs, Z_BEST_SPEED);
    if (ret != Z_OK) {
        JS_ASSERT(ret == Z_MEM_ERROR);
        return false;
    }
    return true;
}

void
Compressor::setOutput(unsigned char *out, size_t outlen)
{
    JS_ASSERT(outlen > outbytes);
    zs.next_out = out + outbytes;
    zs.avail_out = outlen - outbytes;
}

Compressor::Status
Compressor::compressMore()
{
    JS_ASSERT(zs.next_out);
    uInt left = inplen - (zs.next_in - inp);
    bool done = left <= CHUNKSIZE;
    if (done)
        zs.avail_in = left;
    else if (zs.avail_in == 0)
        zs.avail_in = CHUNKSIZE;
    Bytef *oldout = zs.next_out;
    int ret = deflate(&zs, done ? Z_FINISH : Z_NO_FLUSH);
    outbytes += zs.next_out - oldout;
    if (ret == Z_MEM_ERROR) {
        zs.avail_out = 0;
        return OOM;
    }
    if (ret == Z_BUF_ERROR || (done && ret == Z_OK)) {
        JS_ASSERT(zs.avail_out == 0);
        return MOREOUTPUT;
    }
    JS_ASSERT_IF(!done, ret == Z_OK);
    JS_ASSERT_IF(done, ret == Z_STREAM_END);
    return done ? DONE : CONTINUE;
}

bool
js::DecompressString(const unsigned char *inp, size_t inplen, unsigned char *out, size_t outlen)
{
    JS_ASSERT(inplen <= UINT32_MAX);
    z_stream zs;
    zs.zalloc = zlib_alloc;
    zs.zfree = zlib_free;
    zs.opaque = NULL;
    zs.next_in = (Bytef *)inp;
    zs.avail_in = inplen;
    zs.next_out = out;
    JS_ASSERT(outlen);
    zs.avail_out = outlen;
    int ret = inflateInit(&zs);
    if (ret != Z_OK) {
        JS_ASSERT(ret == Z_MEM_ERROR);
        return false;
    }
    ret = inflate(&zs, Z_FINISH);
    JS_ASSERT(ret == Z_STREAM_END);
    ret = inflateEnd(&zs);
    JS_ASSERT(ret == Z_OK);
    return true;
}
#endif

#ifdef DEBUG

JS_PUBLIC_DATA(uint32_t) OOM_maxAllocations = UINT32_MAX;
JS_PUBLIC_DATA(uint32_t) OOM_counter = 0;
#endif





JS_STATIC_ASSERT(sizeof(void *) == sizeof(void (*)()));

JS_PUBLIC_API(void)
JS_Assert(const char *s, const char *file, int ln)
{
    MOZ_ReportAssertionFailure(s, file, ln);
    MOZ_CRASH();
}

#ifdef JS_BASIC_STATS

#include <math.h>
#include <string.h>










static uint32_t
BinToVal(unsigned logscale, unsigned bin)
{
    JS_ASSERT(bin <= 10);
    if (bin <= 1 || logscale == 0)
        return bin;
    --bin;
    if (logscale == 2)
        return JS_BIT(bin);
    JS_ASSERT(logscale == 10);
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
        ? (unsigned) JS_CEILING_LOG2W(val)
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
                cnt = JS_CEILING_LOG2W(cnt);
            for (unsigned i = 0; i < cnt; i++)
                putc('*', fp);
        }
        putc('\n', fp);
    }
}

#endif 
