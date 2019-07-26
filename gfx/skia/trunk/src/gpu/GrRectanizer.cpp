









#include "GrRectanizer.h"
#include "GrTBSearch.h"

#define MIN_HEIGHT_POW2     2

class GrRectanizerPow2 : public GrRectanizer {
public:
    GrRectanizerPow2(int w, int h) : GrRectanizer(w, h) {
        fNextStripY = 0;
        fAreaSoFar = 0;
        sk_bzero(fRows, sizeof(fRows));
    }

    virtual ~GrRectanizerPow2() {
    }

    virtual void reset() {
        fNextStripY = 0;
        fAreaSoFar = 0;
        sk_bzero(fRows, sizeof(fRows));
    }

    virtual bool addRect(int w, int h, GrIPoint16* loc);

    virtual float percentFull() const {
        return fAreaSoFar / ((float)this->width() * this->height());
    }

    virtual int stripToPurge(int height) const { return -1; }
    virtual void purgeStripAtY(int yCoord) { }

    

    struct Row {
        GrIPoint16  fLoc;
        int         fRowHeight;

        bool canAddWidth(int width, int containerWidth) const {
            return fLoc.fX + width <= containerWidth;
        }
    };

    Row fRows[16];

    static int HeightToRowIndex(int height) {
        SkASSERT(height >= MIN_HEIGHT_POW2);
        return 32 - SkCLZ(height - 1);
    }

    int fNextStripY;
    int32_t fAreaSoFar;

    bool canAddStrip(int height) const {
        return fNextStripY + height <= this->height();
    }

    void initRow(Row* row, int rowHeight) {
        row->fLoc.set(0, fNextStripY);
        row->fRowHeight = rowHeight;
        fNextStripY += rowHeight;
    }
};

bool GrRectanizerPow2::addRect(int width, int height, GrIPoint16* loc) {
    if ((unsigned)width > (unsigned)this->width() ||
        (unsigned)height > (unsigned)this->height()) {
        return false;
    }

    int32_t area = width * height;

    




    height = GrNextPow2(height);
    if (height < MIN_HEIGHT_POW2) {
        height = MIN_HEIGHT_POW2;
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







