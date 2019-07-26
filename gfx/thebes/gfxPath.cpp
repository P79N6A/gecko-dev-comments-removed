




#include "gfxPath.h"
#include "mozilla/gfx/2D.h"

#include "cairo.h"

using namespace mozilla::gfx;

gfxPath::gfxPath(cairo_path_t* aPath)
  : mPath(aPath)
{
}

gfxPath::gfxPath(Path* aPath)
  : mPath(nullptr)
  , mMoz2DPath(aPath)
{
}

gfxPath::~gfxPath()
{
    cairo_path_destroy(mPath);
}
