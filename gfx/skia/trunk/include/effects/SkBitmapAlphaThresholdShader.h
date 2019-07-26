






#include "SkShader.h"
#include "SkBitmap.h"
#include "SkRegion.h"
#include "SkString.h"

class SK_API SkBitmapAlphaThresholdShader : public SkShader {
public:
    







    static SkShader* Create(const SkBitmap& bitmap, const SkRegion& region, U8CPU threshold);
};
