





































#include "jsstdint.h"
#include "jsutil.h"






#ifdef JS_HAS_BUILTIN_BITSCAN32
JS_STATIC_ASSERT(sizeof(unsigned int) == sizeof(JSUint32));
JS_STATIC_ASSERT_IF(JS_BYTES_PER_WORD == 4,
                    sizeof(unsigned int) == sizeof(JSUword));
#endif
#ifdef JS_HAS_BUILTIN_BITSCAN64
JS_STATIC_ASSERT_IF(JS_BYTES_PER_WORD == 8,
                    sizeof(unsigned long long) == sizeof(JSUword));
#endif

#if !defined(JS_HAS_BUILTIN_BITSCAN32) && JS_BYTES_PER_WORD == 4

size_t
js_FloorLog2wImpl(size_t n)
{
    size_t log2;

    JS_FLOOR_LOG2(log2, n);
    return log2;
}
#endif



#if !defined(JS_HAS_BUILTIN_BITSCAN64) && JS_BYTES_PER_WORD == 8

size_t
js_FloorLog2wImpl(size_t n)
{
    size_t log2, m;

    JS_ASSERT(n != 0);

    log2 = 0;
    m = n >> 32;
    if (m != 0) { n = m; log2 = 32; }
    m = n >> 16;
    if (m != 0) { n = m; log2 |= 16; }
    m = n >> 8;
    if (m != 0) { n = m; log2 |= 8; }
    m = n >> 4;
    if (m != 0) { n = m; log2 |= 4; }
    m = n >> 2;
    if (m != 0) { n = m; log2 |= 2; }
    log2 |= (n >> 1);

    return log2;
}

#endif
