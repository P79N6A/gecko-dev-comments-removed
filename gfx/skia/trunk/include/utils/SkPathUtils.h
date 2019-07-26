







#ifndef SkPathUtils_DEFINED
#define SkPathUtils_DEFINED

#include "SkPath.h"







class SK_API SkPathUtils {
public:
    




    static void BitsToPath_Path(SkPath* path, const char* bitmap,
                            int w, int h, int rowBytes);

    




    static void BitsToPath_Region(SkPath* path, const char* bitmap,
                                   int w, int h, int rowBytes);

};

#endif
