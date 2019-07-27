







#include "GrRectanizer_pow2.h"

bool GrRectanizerPow2::addRect(int width, int height, SkIPoint16* loc) {
    if ((unsigned)width > (unsigned)this->width() ||
        (unsigned)height > (unsigned)this->height()) {
        return false;
    }

    int32_t area = width * height; 

    height = GrNextPow2(height);
    if (height < kMIN_HEIGHT_POW2) {
        height = kMIN_HEIGHT_POW2;
    }

    Row* row = &fRows[HeightToRowIndex(height)];
    SkASSERT(row->fRowHeight == 0 || row->fRowHeight == height);

    if (0 == row->fRowHeight) {
        if (!this->canAddStrip(height)) {
            return false;
        }
        this->initRow(row, height);
    } else {
        if (!row->canAddWidth(width, this->width())) {
            if (!this->canAddStrip(height)) {
                return false;
            }
            
            
            this->initRow(row, height);
        }
    }

    SkASSERT(row->fRowHeight == height);
    SkASSERT(row->canAddWidth(width, this->width()));
    *loc = row->fLoc;
    row->fLoc.fX += width;

    SkASSERT(row->fLoc.fX <= this->width());
    SkASSERT(row->fLoc.fY <= this->height());
    SkASSERT(fNextStripY <= this->height());
    fAreaSoFar += area;
    return true;
}







