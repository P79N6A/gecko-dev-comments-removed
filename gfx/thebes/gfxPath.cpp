




#include "gfxPath.h"
#include "gfxPoint.h"
#include "gfxPlatform.h"
#include "gfxASurface.h"
#include "mozilla/gfx/2D.h"

#include "cairo.h"

using namespace mozilla::gfx;

gfxPath::gfxPath(cairo_path_t* aPath)
  : mPath(aPath)
  , mFlattenedPath(nullptr)
{
}

gfxPath::gfxPath(Path* aPath)
  : mPath(nullptr)
  , mFlattenedPath(nullptr)
  , mMoz2DPath(aPath)
{
}

gfxPath::~gfxPath()
{
    cairo_path_destroy(mPath);
    cairo_path_destroy(mFlattenedPath);
}

void
gfxPath::EnsureFlattenedPath()
{
    if (mFlattenedPath) {
        return;
    }

    gfxASurface* surf = gfxPlatform::GetPlatform()->ScreenReferenceSurface();
    cairo_t* cr = cairo_create(surf->CairoSurface());
    cairo_append_path(cr, mPath);
    mFlattenedPath = cairo_copy_path_flat(cr);
    cairo_destroy(cr);
}

static gfxFloat
CalcSubLengthAndAdvance(cairo_path_data_t *aData,
                        gfxPoint &aPathStart, gfxPoint &aCurrent)
{
    float sublength = 0;

    switch (aData->header.type) {
        case CAIRO_PATH_MOVE_TO:
        {
            aCurrent = aPathStart = gfxPoint(aData[1].point.x,
                                             aData[1].point.y);
            break;
        }
        case CAIRO_PATH_LINE_TO:
        {
            gfxPoint diff =
                gfxPoint(aData[1].point.x, aData[1].point.y) - aCurrent;
            sublength = sqrt(diff.x * diff.x + diff.y * diff.y);
            aCurrent = gfxPoint(aData[1].point.x, aData[1].point.y);
            break;
        }
        case CAIRO_PATH_CURVE_TO:
            
            NS_WARNING("curve_to in flattened path");
            break;
        case CAIRO_PATH_CLOSE_PATH:
        {
            gfxPoint diff = aPathStart - aCurrent;
            sublength = sqrt(diff.x * diff.x + diff.y * diff.y);
            aCurrent = aPathStart;
            break;
        }
    }
    return sublength;
}

gfxFloat
gfxPath::GetLength()
{
    if (mMoz2DPath) {
        return mMoz2DPath->ComputeLength();
    }

    EnsureFlattenedPath();

    gfxPoint start(0, 0);     
    gfxPoint current(0, 0);   
    gfxFloat length = 0;      

    for (int32_t i = 0;
         i < mFlattenedPath->num_data;
         i += mFlattenedPath->data[i].header.length) {
        length += CalcSubLengthAndAdvance(&mFlattenedPath->data[i], start, current);
    }
    return length;
}

gfxPoint
gfxPath::FindPoint(gfxPoint aOffset, gfxFloat *aAngle)
{
    if (mMoz2DPath) {
        Point tangent; 
        Point result = mMoz2DPath->ComputePointAtLength(aOffset.x, &tangent);

        
        Point normal(-tangent.y, tangent.x);
        result += normal * aOffset.y;

        if (aAngle)
            *aAngle = atan2(tangent.y, tangent.x);

        return gfxPoint(result.x, result.y);
    }

    EnsureFlattenedPath();

    gfxPoint start(0, 0);     
    gfxPoint current(0, 0);   
    gfxFloat length = 0;      

    for (int32_t i = 0;
         i < mFlattenedPath->num_data;
         i += mFlattenedPath->data[i].header.length) {
        gfxPoint prev = current;

        gfxFloat sublength = CalcSubLengthAndAdvance(&mFlattenedPath->data[i],
                                                     start, current);

        gfxPoint diff = current - prev;
        if (aAngle)
            *aAngle = atan2(diff.y, diff.x);

        if (sublength != 0 && length + sublength >= aOffset.x) {
            gfxFloat ratio = (aOffset.x - length) / sublength;
            gfxFloat normalization =
                1.0 / sqrt(diff.x * diff.x + diff.y * diff.y);

            return prev * (1.0f - ratio) + current * ratio +
                gfxPoint(-diff.y, diff.x) * aOffset.y * normalization;
        }
        length += sublength;
    }

    
    return current;
}
