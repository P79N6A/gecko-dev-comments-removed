




#ifndef GFX_PATH_H
#define GFX_PATH_H

#include "gfxTypes.h"
#include "nsISupportsImpl.h"

class gfxContext;
struct gfxPoint;
typedef struct cairo_path cairo_path_t;





class gfxPath {
    NS_INLINE_DECL_REFCOUNTING(gfxPath)

    friend class gfxContext;

protected:
    gfxPath(cairo_path_t* aPath);

    void EnsureFlattenedPath();

public:
    virtual ~gfxPath();
    
    


    gfxFloat GetLength();

    






    gfxPoint FindPoint(gfxPoint aOffset,
                       gfxFloat* aAngle = nullptr);

protected:
    cairo_path_t* mPath;
    cairo_path_t* mFlattenedPath;
};

#endif
