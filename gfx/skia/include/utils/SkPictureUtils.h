






#ifndef SkPictureUtils_DEFINED
#define SkPictureUtils_DEFINED

#include "SkPicture.h"

class SkData;
struct SkRect;

class SK_API SkPictureUtils {
public:
    









    static SkData* GatherPixelRefs(SkPicture* pict, const SkRect& area);
};

#endif
