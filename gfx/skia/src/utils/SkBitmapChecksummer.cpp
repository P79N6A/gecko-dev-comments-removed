







#include "SkBitmap.h"
#include "SkBitmapChecksummer.h"
#include "SkBitmapTransformer.h"
#include "SkCityHash.h"
#include "SkEndian.h"




static void write_int_to_buffer(int val, char* buf) {
    val = SkEndian_SwapLE32(val);
    for (int byte=0; byte<4; byte++) {
        *buf++ = (char)(val & 0xff);
        val = val >> 8;
    }
}

 uint64_t SkBitmapChecksummer::Compute64Internal(
        const SkBitmap& bitmap, const SkBitmapTransformer& transformer) {
    size_t pixelBufferSize = transformer.bytesNeededTotal();
    size_t totalBufferSize = pixelBufferSize + 8; 

    SkAutoMalloc bufferManager(totalBufferSize);
    char *bufferStart = static_cast<char *>(bufferManager.get());
    char *bufPtr = bufferStart;
    
    write_int_to_buffer(bitmap.width(), bufPtr);
    bufPtr += 4;
    write_int_to_buffer(bitmap.height(), bufPtr);
    bufPtr += 4;

    
    if (!transformer.copyBitmapToPixelBuffer(bufPtr, pixelBufferSize)) {
        return 0;
    }
    return SkCityHash::Compute64(bufferStart, totalBufferSize);
}

 uint64_t SkBitmapChecksummer::Compute64(const SkBitmap& bitmap) {
    const SkBitmapTransformer::PixelFormat kPixelFormat =
        SkBitmapTransformer::kARGB_8888_Premul_PixelFormat;

    
    const SkBitmapTransformer transformer =
        SkBitmapTransformer(bitmap, kPixelFormat);
    if (transformer.isValid(false)) {
        return Compute64Internal(bitmap, transformer);
    }

    
    
    SkBitmap copyBitmap;
    bitmap.copyTo(&copyBitmap, SkBitmap::kARGB_8888_Config);
    const SkBitmapTransformer copyTransformer =
        SkBitmapTransformer(copyBitmap, kPixelFormat);
    if (copyTransformer.isValid(true)) {
        return Compute64Internal(copyBitmap, copyTransformer);
    } else {
        return 0;
    }
}
