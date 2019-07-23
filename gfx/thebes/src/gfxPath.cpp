



































#include "gfxPath.h"
#include "gfxPoint.h"

#include "cairo.h"

gfxFlattenedPath::gfxFlattenedPath(cairo_path_t *aPath) : mPath(aPath)
{
}

gfxFlattenedPath::~gfxFlattenedPath()
{
    cairo_path_destroy(mPath);
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
gfxFlattenedPath::GetLength()
{
    gfxPoint start(0, 0);     
    gfxPoint current(0, 0);   
    gfxFloat length = 0;      

    for (PRInt32 i = 0;
         i < mPath->num_data;
         i += mPath->data[i].header.length) {
        length += CalcSubLengthAndAdvance(&mPath->data[i], start, current);
    }
    return length;
}

gfxPoint
gfxFlattenedPath::FindPoint(gfxPoint aOffset, gfxFloat *aAngle)
{
    gfxPoint start(0, 0);     
    gfxPoint current(0, 0);   
    gfxFloat length = 0;      

    for (PRInt32 i = 0;
         i < mPath->num_data;
         i += mPath->data[i].header.length) {
        gfxPoint prev = current;

        gfxFloat sublength = CalcSubLengthAndAdvance(&mPath->data[i],
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
