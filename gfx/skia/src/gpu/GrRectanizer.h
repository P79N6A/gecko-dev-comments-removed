









#ifndef GrRectanizer_DEFINED
#define GrRectanizer_DEFINED

#include "GrRect.h"
#include "GrTDArray.h"

class GrRectanizerPurgeListener {
public:
    virtual ~GrRectanizerPurgeListener() {}

    virtual void notifyPurgeStrip(void*, int yCoord) = 0;
};

class GrRectanizer {
public:
    GrRectanizer(int width, int height) : fWidth(width), fHeight(height) {
        GrAssert(width >= 0);
        GrAssert(height >= 0);
    }

    virtual ~GrRectanizer() {}

    int width() const { return fWidth; }
    int height() const { return fHeight; }

    virtual bool addRect(int width, int height, GrIPoint16* loc) = 0;
    virtual float percentFull() const = 0;

    
    
    
    virtual int stripToPurge(int height) const = 0;
    virtual void purgeStripAtY(int yCoord) = 0;

    


    static GrRectanizer* Factory(int width, int height);

private:
    int fWidth;
    int fHeight;
};

#endif


