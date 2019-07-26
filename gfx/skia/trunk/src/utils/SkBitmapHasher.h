







#ifndef SkBitmapHasher_DEFINED
#define SkBitmapHasher_DEFINED

#include "SkBitmap.h"




class SkBitmapHasher {
public:
    









    static bool ComputeDigest(const SkBitmap& bitmap, uint64_t *result);

private:
    static bool ComputeDigestInternal(const SkBitmap& bitmap, uint64_t *result);
};

#endif
