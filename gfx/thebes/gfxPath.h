




#ifndef GFX_PATH_H
#define GFX_PATH_H

#include "gfxTypes.h"
#include "nsISupportsImpl.h"
#include "mozilla/RefPtr.h"

class gfxContext;
typedef struct cairo_path cairo_path_t;

namespace mozilla {
namespace gfx {
class Path;
}
}





class gfxPath {
    NS_INLINE_DECL_REFCOUNTING(gfxPath)

    friend class gfxContext;

    gfxPath(cairo_path_t* aPath);

public:
    gfxPath(mozilla::gfx::Path* aPath);
    virtual ~gfxPath();

private:
    cairo_path_t* mPath;
    mozilla::RefPtr<mozilla::gfx::Path> mMoz2DPath;
};

#endif
