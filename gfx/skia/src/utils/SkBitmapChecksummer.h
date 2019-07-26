







#ifndef SkBitmapChecksummer_DEFINED
#define SkBitmapChecksummer_DEFINED

#include "SkBitmap.h"
#include "SkBitmapTransformer.h"




class SkBitmapChecksummer {
public:
    









    static uint64_t Compute64(const SkBitmap& bitmap);

private:
    static uint64_t Compute64Internal(const SkBitmap& bitmap,
                                      const SkBitmapTransformer& transformer);
};

#endif
