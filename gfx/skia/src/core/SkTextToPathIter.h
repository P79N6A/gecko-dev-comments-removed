






#ifndef SkTextToPathIter_DEFINED
#define SkTextToPathIter_DEFINED

#include "SkAutoKern.h"
#include "SkPaint.h"

class SkGlyphCache;

class SkTextToPathIter {
public:
    SkTextToPathIter(const char text[], size_t length, const SkPaint& paint,
                     bool applyStrokeAndPathEffects);
    ~SkTextToPathIter();

    const SkPaint&  getPaint() const { return fPaint; }
    SkScalar        getPathScale() const { return fScale; }

    struct Rec {
        const SkPath*   fPath;  
        SkScalar        fXPos;
    };

    


    bool next(const SkPath** path, SkScalar* xpos);

private:
    SkGlyphCache*   fCache;
    SkPaint         fPaint;
    SkScalar        fScale;
    SkFixed         fPrevAdvance;
    const char*     fText;
    const char*     fStop;
    SkMeasureCacheProc fGlyphCacheProc;

    const SkPath*   fPath;      
    SkScalar        fXPos;      
    SkAutoKern      fAutoKern;
    int             fXYIndex;   
};

#endif
