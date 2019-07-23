



































#ifndef GFX_PATH_H
#define GFX_PATH_H

#include "gfxTypes.h"

class gfxContext;
struct gfxPoint;
typedef struct cairo_path cairo_path_t;





class THEBES_API gfxPath {
    THEBES_INLINE_DECL_REFCOUNTING(gfxPath)

    friend class gfxContext;

protected:
    gfxPath(cairo_path_t* aPath);

public:
    virtual ~gfxPath();

protected:
    cairo_path_t* mPath;
};





class THEBES_API gfxFlattenedPath : public gfxPath {
    friend class gfxContext;

protected:
    gfxFlattenedPath(cairo_path_t* aPath);

public:
    virtual ~gfxFlattenedPath();

    


    gfxFloat GetLength();

    






    gfxPoint FindPoint(gfxPoint aOffset,
                       gfxFloat* aAngle = nsnull);
};

#endif
