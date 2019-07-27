





#include "vm/Compression.h"

#include "mozilla/DebugOnly.h"

#include <zlib.h>

#include "js/Utility.h"

using mozilla::DebugOnly;
using namespace js;

static void *
zlib_alloc(void *cx, uInt items, uInt size)
{
    return js_calloc(items, size);
}

static void
zlib_free(void *cx, void *addr)
{
    js_free(addr);
}

Compressor::~Compressor()
{
    if (initialized) {
        int ret = deflateEnd(zs);
        if (ret != Z_OK) {
            
            MOZ_ASSERT(ret == Z_DATA_ERROR);
            MOZ_ASSERT(uInt(zs->next_in - inp) < inplen || !zs->avail_out);
        }
    }
    js_free(zs);
}

bool
Compressor::prepare(const unsigned char *inp_, size_t inplen_)
{
    MOZ_ASSERT(inplen_ > 0);

    if (inplen_ >= UINT32_MAX)
        return false;

    if (!zs) {
        zs = js_pod_malloc<z_stream>();
        if (!zs)
            return false;
        zs->zalloc = zlib_alloc;
        zs->zfree = zlib_free;
    }

    inp = inp_;
    inplen = inplen_;
    outbytes = 0;

    zs->opaque = nullptr;
    zs->next_in = (Bytef *)inp;
    zs->avail_in = 0;
    zs->next_out = nullptr;
    zs->avail_out = 0;

    if (initialized) {
        DebugOnly<int> ret = deflateReset(zs);
        MOZ_ASSERT(ret == Z_OK);
    } else {
        
        
        int ret = deflateInit(zs, Z_BEST_SPEED);
        if (ret != Z_OK) {
            MOZ_ASSERT(ret == Z_MEM_ERROR);
            return false;
        }
        initialized = true;
    }

    return true;
}

void
Compressor::setOutput(unsigned char *out, size_t outlen)
{
    MOZ_ASSERT(initialized);
    MOZ_ASSERT(outlen > outbytes);
    zs->next_out = out + outbytes;
    zs->avail_out = outlen - outbytes;
}

Compressor::Status
Compressor::compressMore()
{
    MOZ_ASSERT(initialized);
    MOZ_ASSERT(zs->next_out);
    uInt left = inplen - (zs->next_in - inp);
    bool done = left <= CHUNKSIZE;
    if (done)
        zs->avail_in = left;
    else if (zs->avail_in == 0)
        zs->avail_in = CHUNKSIZE;
    Bytef *oldout = zs->next_out;
    int ret = deflate(zs, done ? Z_FINISH : Z_NO_FLUSH);
    outbytes += zs->next_out - oldout;
    if (ret == Z_MEM_ERROR) {
        zs->avail_out = 0;
        return OOM;
    }
    if (ret == Z_BUF_ERROR || (done && ret == Z_OK)) {
        MOZ_ASSERT(zs->avail_out == 0);
        return MOREOUTPUT;
    }
    MOZ_ASSERT_IF(!done, ret == Z_OK);
    MOZ_ASSERT_IF(done, ret == Z_STREAM_END);
    return done ? DONE : CONTINUE;
}

bool
js::DecompressString(const unsigned char *inp, size_t inplen, unsigned char *out, size_t outlen)
{
    MOZ_ASSERT(inplen <= UINT32_MAX);

    
#ifdef MOZ_MSAN
    __msan_unpoison(out, outlen);
#endif

    z_stream zs;
    zs.zalloc = zlib_alloc;
    zs.zfree = zlib_free;
    zs.opaque = nullptr;
    zs.next_in = (Bytef *)inp;
    zs.avail_in = inplen;
    zs.next_out = out;
    MOZ_ASSERT(outlen);
    zs.avail_out = outlen;
    int ret = inflateInit(&zs);
    if (ret != Z_OK) {
        MOZ_ASSERT(ret == Z_MEM_ERROR);
        return false;
    }
    ret = inflate(&zs, Z_FINISH);
    MOZ_ASSERT(ret == Z_STREAM_END);
    ret = inflateEnd(&zs);
    MOZ_ASSERT(ret == Z_OK);
    return true;
}
