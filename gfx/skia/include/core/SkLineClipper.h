






#ifndef SkLineClipper_DEFINED
#define SkLineClipper_DEFINED

#include "SkRect.h"
#include "SkPoint.h"

class SkLineClipper {
public:
    enum {
        kMaxPoints = 4
    };

    










    static int ClipLine(const SkPoint pts[2], const SkRect& clip,
                        SkPoint lines[kMaxPoints]);

    







    static bool IntersectLine(const SkPoint src[2], const SkRect& clip,
                              SkPoint dst[2]);
};

#endif

