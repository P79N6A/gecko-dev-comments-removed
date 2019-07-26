









#ifndef GrClip_DEFINED
#define GrClip_DEFINED

#include "GrRect.h"
#include "SkClipStack.h"

class GrSurface;








class GrClipData : public SkNoncopyable {
public:
    const SkClipStack*  fClipStack;
    SkIPoint            fOrigin;

    GrClipData()
        : fClipStack(NULL) {
        fOrigin.setZero();
    }

    bool operator==(const GrClipData& other) const {
        if (fOrigin != other.fOrigin) {
            return false;
        }

        if (NULL != fClipStack && NULL != other.fClipStack) {
            return *fClipStack == *other.fClipStack;
        }

        return fClipStack == other.fClipStack;
    }

    bool operator!=(const GrClipData& other) const {
        return !(*this == other);
    }

    void getConservativeBounds(const GrSurface* surface,
                               GrIRect* devResult,
                               bool* isIntersectionOfRects = NULL) const;
};

#endif

