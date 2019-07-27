






#ifndef GrRectanizer_DEFINED
#define GrRectanizer_DEFINED

#include "GrTypes.h"

struct SkIPoint16;

class GrRectanizer {
public:
    GrRectanizer(int width, int height) : fWidth(width), fHeight(height) {
        SkASSERT(width >= 0);
        SkASSERT(height >= 0);
    }

    virtual ~GrRectanizer() {}

    virtual void reset() = 0;

    int width() const { return fWidth; }
    int height() const { return fHeight; }

    
    
    virtual bool addRect(int width, int height, SkIPoint16* loc) = 0;
    virtual float percentFull() const = 0;

    


    static GrRectanizer* Factory(int width, int height);

private:
    int fWidth;
    int fHeight;
};

#endif
