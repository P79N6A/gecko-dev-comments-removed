






#ifndef SkQuadTreePicture_DEFINED
#define SkQuadTreePicture_DEFINED

#include "SkPicture.h"
#include "SkRect.h"








class SK_API SkQuadTreePicture : public SkPicture {
public:
    SkQuadTreePicture(const SkIRect& bounds) : fBounds(bounds) {}
    virtual SkBBoxHierarchy* createBBoxHierarchy() const SK_OVERRIDE;
private:
    SkIRect fBounds;
};

#endif
