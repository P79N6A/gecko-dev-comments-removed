





































#include "jsstddef.h"
#include "jsbit.h"
#include "jsutil.h"




JS_PUBLIC_API(JSIntn) JS_CeilingLog2(JSUint32 n)
{
    JSIntn log2;

    JS_CEILING_LOG2(log2, n);
    return log2;
}





JS_PUBLIC_API(JSIntn) JS_FloorLog2(JSUint32 n)
{
    JSIntn log2;

    JS_FLOOR_LOG2(log2, n);
    return log2;
}




#if !defined(JS_HAS_BUILTIN_BITSCAN64) && JS_BYTES_PER_WORD == 8

JSUword
js_FloorLog2wImpl(JSUword n)
{
    JSUword log2, m;

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
