






#include "SkBitmap.h"
#include "SkBitmapHasher.h"
#include "SkEndian.h"
#include "SkImageEncoder.h"

#include "SkMD5.h"




static void write_int32_to_buffer(uint32_t val, SkWStream* out) {
    val = SkEndian_SwapLE32(val);
    for (size_t byte = 0; byte < 4; ++byte) {
        out->write8((uint8_t)(val & 0xff));
        val = val >> 8;
    }
}




static inline uint64_t first_8_bytes_as_uint64(const uint8_t *bytearray) {
    return SkEndian_SwapLE64(*(reinterpret_cast<const uint64_t *>(bytearray)));
}

 bool SkBitmapHasher::ComputeDigestInternal(const SkBitmap& bitmap, uint64_t *result) {
    SkMD5 out;

    
    write_int32_to_buffer(SkToU32(bitmap.width()), &out);
    write_int32_to_buffer(SkToU32(bitmap.height()), &out);

    
    SkAutoTDelete<SkImageEncoder> enc(CreateARGBImageEncoder());
    if (!enc->encodeStream(&out, bitmap, SkImageEncoder::kDefaultQuality)) {
        return false;
    }

    SkMD5::Digest digest;
    out.finish(digest);
    *result = first_8_bytes_as_uint64(digest.data);
    return true;
}

 bool SkBitmapHasher::ComputeDigest(const SkBitmap& bitmap, uint64_t *result) {
    if (ComputeDigestInternal(bitmap, result)) {
        return true;
    }

    
    
    SkBitmap copyBitmap;
    if (!bitmap.copyTo(&copyBitmap, kN32_SkColorType)) {
        return false;
    }
    return ComputeDigestInternal(copyBitmap, result);
}
