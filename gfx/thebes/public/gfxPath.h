



































#ifndef GFX_PATH_H
#define GFX_PATH_H

#include "gfxTypes.h"

struct gfxPoint;
typedef struct cairo_path cairo_path_t;

class THEBES_API gfxFlattenedPath {
    THEBES_INLINE_DECL_REFCOUNTING(gfxPath)

public:
    gfxFlattenedPath(cairo_path_t *aPath);
    ~gfxFlattenedPath();

    


    gfxFloat GetLength();

    






    gfxPoint FindPoint(gfxPoint aOffset,
                       gfxFloat *aAngle = nsnull);

protected:
    cairo_path_t *mPath;
};

#endif
