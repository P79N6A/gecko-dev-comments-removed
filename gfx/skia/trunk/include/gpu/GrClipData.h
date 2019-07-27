






#ifndef GrClip_DEFINED
#define GrClip_DEFINED

#include "SkClipStack.h"
#include "GrSurface.h"

struct SkIRect;








class GrClipData : SkNoncopyable {
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
                               SkIRect* devResult,
                               bool* isIntersectionOfRects = NULL) const {
        this->getConservativeBounds(surface->width(), surface->height(),
                                    devResult, isIntersectionOfRects);
    }

    void getConservativeBounds(int width, int height,
                               SkIRect* devResult,
                               bool* isIntersectionOfRects = NULL) const;
};

#endif
